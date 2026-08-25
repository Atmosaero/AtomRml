/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 Reece Hagan
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
#include "AtomRmlFeatureProcessor.h"
#include "../Console/AtomRmlConsoleDocument.h"

#include <AzCore/Console/ILogger.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/Bootstrap/BootstrapNotificationBus.h>

#include <RmlUi/Core/Core.h>
#include <RmlUi/Debugger/Debugger.h>

namespace AtomRml
{
    void AtomRmlFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->Class<AtomRmlFeatureProcessor, FeatureProcessor>();
        }
    }

    void AtomRmlFeatureProcessor::Activate()
    {
        EnableSceneNotification();
        AZ::Render::Bootstrap::NotificationBus::Handler::BusConnect();

        auto name = GetParentScene()->GetName().GetCStr();
        m_context = Rml::CreateContext(name, {800,600});
    }

    void AtomRmlFeatureProcessor::Deactivate()
    {
        if (m_context)
        {
            const bool ownsDebugger = Rml::GetNumContexts() > 0 && Rml::GetContext(0) == m_context;
            const Rml::String contextName = m_context->GetName();

            m_consoleDocument.reset();
            if (ownsDebugger)
            {
                Rml::Debugger::Shutdown();
            }

            UnregisterContext(m_context);
            Rml::RemoveContext(contextName);
            m_context = nullptr;
        }

        AZ::Render::Bootstrap::NotificationBus::Handler::BusDisconnect();
        DisableSceneNotification();
    }

    void AtomRmlFeatureProcessor::ResizeDisplayCtxs()
    {
        AZ::RPI::Scene* scene = GetParentScene();
        if (!scene)
            return;

        auto viewContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (!viewContextManager)
            return;

        auto viewportContext = viewContextManager->GetDefaultViewportContext();
        if (!viewportContext)
            return;

        auto windowContext = viewportContext->GetWindowContext();
        if (!windowContext)
            return;

        const AZ::RHI::Viewport& viewport = windowContext->GetViewport();
        int32_t width = aznumeric_cast<int32_t>(viewport.m_maxX - viewport.m_minX);
        int32_t height = aznumeric_cast<int32_t>(viewport.m_maxY - viewport.m_minY);
        AZ::PackedVector2i currentScreenSize(width, height);

        for (auto& [context, renderData] : m_contextRenderData)
        {
            if (renderData.m_displayToScreen)
            {
                // For display to screen mode, resize context directly to screen size
                Rml::Vector2i currentContextSize = context->GetDimensions();
                if (currentContextSize.x != currentScreenSize.GetX() || currentContextSize.y != currentScreenSize.
                    GetY())
                {
                    context->SetDimensions(Rml::Vector2i(currentScreenSize.GetX(), currentScreenSize.GetY()));
                    AZ_Info("AtomRmlFeatureProcessor", "Updated context %p size to screen size: %dx%d",
                            context, currentScreenSize.GetX(), currentScreenSize.GetY());
                }
            }
        }
    }

    void AtomRmlFeatureProcessor::UpdateContextOutput()
    {
        if (m_renderTargetsDirty)
        {
            for (auto& [context, renderData] : m_contextRenderData)
            {
                if (renderData.m_displayToScreen)
                {
                    // For screen display mode, remove any existing render target
                    if (renderData.m_renderTarget)
                    {
                        renderData.m_renderTarget.reset();
                        renderData.m_needsRenderTarget = false;
                        AZ_Info("AtomRmlFeatureProcessor",
                                "Removed render target for context %p (switching to direct pipeline mode)", context);
                    }
                }
                else if (renderData.m_needsRenderTarget && !renderData.m_renderTarget)
                {
                    CreateRenderTarget(context);
                }
            }

            if (m_parentPass)
            {
                for (auto& [context, renderData] : m_contextRenderData)
                {
                    if (!renderData.m_isActive)
                        continue;

                    if (renderData.m_displayToScreen)
                    {
                        // Set context to direct pipeline mode (no specific render target)
                        m_parentPass->SetDirectPipelineMode(context);
                        AZ_Info("AtomRmlFeatureProcessor", "Set context %p to direct pipeline mode", context);
                    }
                    else if (renderData.m_renderTarget)
                    {
                        m_parentPass->UpdateRenderTarget(context, renderData.m_renderTarget);
                        AZ_Info("AtomRmlFeatureProcessor", "Updated render target for context %p to AtomRmlParentPass",
                                context);
                    }
                }

                m_renderTargetsDirty = false;
            }
        }
    }

    void AtomRmlFeatureProcessor::Simulate([[maybe_unused]] const FeatureProcessor::SimulatePacket& packet)
    {
        ResizeDisplayCtxs();
        UpdateContextOutput();

        //TODO: Set and forget this instead of doing it on every simulate call.
        if (m_parentPass)
        {
            for (auto& [context, renderData] : m_contextRenderData)
            {
                if (renderData.m_isActive)
                {
                    if (auto childPass = m_parentPass->GetChildPass(context))
                    {
                        childPass->SetRmlContext(context);
                    }
                }
            }
        }
    }

    void AtomRmlFeatureProcessor::Render(const RenderPacket& packet)
    {
        AZ_UNUSED(packet);
    }

    void AtomRmlFeatureProcessor::AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline)
    {
        //Add parent pass.
        const AZ::Name passName = AZ::Name("AtomRmlPass");
        const AZ::Name uiPassName = AZ::Name("UIPass");

        // Check if UIPass exists
        AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(uiPassName, renderPipeline);
        AZ::RPI::Pass* existingUIPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
        if (!existingUIPass)
        {
            AZ_Printf("AtomRmlFeatureProcessor",
                      "Cannot add AtomRmlPass because the pipeline doesn't have a pass named 'UIPass'");
            return;
        }

        // Check if AtomRmlPass already exists
        AZ::RPI::PassFilter atomRmlPassFilter = AZ::RPI::PassFilter::CreateWithPassName(passName, renderPipeline);
        AZ::RPI::Pass* existingAtomRmlPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(atomRmlPassFilter);
        if (existingAtomRmlPass)
        {
            AZ_Printf("AtomRmlFeatureProcessor", "The pass 'AtomRmlPass' already exists.");
            return;
        }

        static constexpr const char* PassRequestAssetPath = "Passes/AtomRml/AtomRmlPassRequest.azasset";
        const AZ::Data::Asset<AZ::RPI::AnyAsset> passRequestAsset =
            AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
                PassRequestAssetPath, AZ::RPI::AssetUtils::TraceLevel::Warning);
        if (!passRequestAsset.Get() || !passRequestAsset->IsReady())
        {
            AZ_Warning(
                "AtomRmlFeatureProcessor", false,
                "Cannot add AtomRmlPass because pass request asset '%s' is not available. "
                "The Asset Processor may still be processing AtomRml assets.",
                PassRequestAssetPath);
            return;
        }

        static constexpr bool AddBefore = true;
        AddPassRequestToRenderPipeline(renderPipeline, PassRequestAssetPath, uiPassName.GetCStr(), AddBefore);

        AZ::RPI::PassFilter createdPassFilter = AZ::RPI::PassFilter::CreateWithPassName(passName, renderPipeline);
        AZ::RPI::Pass* createdPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(createdPassFilter);
        m_parentPass = azrtti_cast<AtomRmlParentPass*>(createdPass);

        if (m_parentPass)
        {
            AZ_Info("AtomRmlFeatureProcessor",
                    "Successfully added 'AtomRmlPass' parent pass to pipeline '%s' using PassRequest.",
                    renderPipeline->GetDescriptor().m_name.c_str());


            RegisterContext(m_context, false);
            AddDebugToPrimeContext();
        }
        else
        {
            AZ_Error("AtomRmlFeatureProcessor", false,
                     "Failed to find or cast AtomRmlParentPass after adding to pipeline '%s'.",
                     renderPipeline->GetDescriptor().m_name.c_str());
        }
    }

    Rml::Context* AtomRmlFeatureProcessor::GetContext()
    {
        return m_context;
    }

    void AtomRmlFeatureProcessor::RegisterContext(Rml::Context* context, bool renderTargetMode)
    {
        if (!context)
        {
            AZ_Error("AtomRmlFeatureProcessor", false, "Cannot register null RmlUi context");
            return;
        }

        UICanvasRenderData renderData;
        renderData.m_renderTargetSize = {};
        renderData.m_needsRenderTarget = true;
        renderData.m_isActive = true;

        m_contextRenderData[context] = renderData;
        m_renderTargetsDirty = true;

        if (!renderTargetMode)
        {
            SetContextDisplayToScreen(context);
        }
    }

    void AtomRmlFeatureProcessor::UnregisterContext(Rml::Context* context)
    {
        if (context)
        {
            if (m_parentPass)
            {
                m_parentPass->RemoveChildPass(context);
            }
            m_contextRenderData.erase(context);
        }
    }

    void AtomRmlFeatureProcessor::SetContextDisplayToScreen(Rml::Context* context)
    {
        auto it = m_contextRenderData.find(context);
        if (it != m_contextRenderData.end())
        {
            if (it->second.m_displayToScreen)
            {
                return;
            }

            it->second.m_displayToScreen = true;
            m_renderTargetsDirty = true; // Trigger update to switch between render target and direct pipeline mode
        }
    }

    void AtomRmlFeatureProcessor::CreateRenderTarget(Rml::Context* context)
    {
        auto it = m_contextRenderData.find(context);
        if (it == m_contextRenderData.end() || !it->second.m_needsRenderTarget)
        {
            return;
        }

        const auto dia = context->GetDimensions();

        UICanvasRenderData& renderData = it->second;

        AZ::RHI::ImageDescriptor imageDesc = AZ::RHI::ImageDescriptor::Create2D(
            AZ::RHI::ImageBindFlags::Color | AZ::RHI::ImageBindFlags::ShaderRead,
            static_cast<uint32_t>(dia.x),
            static_cast<uint32_t>(dia.y),
            AZ::RHI::Format::R8G8B8A8_UNORM
        );

        renderData.m_renderTargetSize = AZ::PackedVector2i(dia.x, dia.y);

        AZ::RHI::ClearValue clearValue = AZ::RHI::ClearValue::CreateVector4Float(0.0f, 0.0f, 0.0f, 0.0f);

        AZ::RPI::CreateAttachmentImageRequest createRequest;
        createRequest.m_imageName = AZ::Name(AZStd::string::format("AtomRmlContextRT_%p", context));
        createRequest.m_isUniqueName = false;
        createRequest.m_imageDescriptor = imageDesc;
        createRequest.m_optimizedClearValue = &clearValue;
        createRequest.m_imagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool().get();

        renderData.m_renderTarget = AZ::RPI::AttachmentImage::Create(createRequest);
        if (!renderData.m_renderTarget)
        {
            AZ_Error("AtomRmlFeatureProcessor", false, "Failed to create UI render target for context %p", context);
        }
        else
        {
            renderData.m_needsRenderTarget = false;
        }
    }

    void AtomRmlFeatureProcessor::OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene)
    {
        AZ_UNUSED(bootstrapScene);
    }

    void AtomRmlFeatureProcessor::GetChildPasses(AZStd::function<void(class AtomRmlChildPass*)> fn)
    {
        auto children = m_parentPass->GetChildren();
        for (auto it = children.begin(); it != children.end(); ++it)
        {
            AtomRmlChildPass* child = azdynamic_cast<AtomRmlChildPass*>(it->get());
            if (child)
            {
                fn(child);
            }
        }
    }

    void AtomRmlFeatureProcessor::AddDebugToPrimeContext()
    {
        Rml::Debugger::Initialise(m_context);
        Rml::Debugger::SetVisible(false);

        if (m_consoleDocument == nullptr)
        {
            m_consoleDocument = AZStd::make_unique<AtomRmlConsoleDocument>();
            m_consoleDocument->Initialize(m_context, "console/console-float.rml");
        }
    }
}
