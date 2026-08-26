/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AtomRmlParentPass.h"
#include <AzCore/Console/ILogger.h>
#include <AzCore/Name/Name.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Scene.h>

namespace AtomRml
{
    AZ::RPI::Ptr<AtomRmlParentPass> AtomRmlParentPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        return aznew AtomRmlParentPass(descriptor);
    }


    AtomRmlParentPass::AtomRmlParentPass(const AZ::RPI::PassDescriptor& descriptor)
        : ParentPass(descriptor)
    {
    }

    AtomRmlParentPass::~AtomRmlParentPass()
    {
        AtomRmlPassRequestBus::Handler::BusDisconnect();
    }

    void AtomRmlParentPass::ResetInternal()
    {
        AtomRmlPassRequestBus::Handler::BusDisconnect();
        ParentPass::ResetInternal();
    }

    void AtomRmlParentPass::SetRenderPipeline(AZ::RPI::RenderPipeline* pipeline)
    {
        if (pipeline == nullptr)
        {
            // A replacement pass may be built immediately while the old pipeline is being torn down.
            AtomRmlPassRequestBus::Handler::BusDisconnect();
        }

        ParentPass::SetRenderPipeline(pipeline);
    }

    void AtomRmlParentPass::UpdateRenderTarget(Rml::Context* context,
                                             AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage)
    {
        if (!context || !attachmentImage)
        {
            return;
        }

        //Do we have it?
        bool bExists = m_contextPasses.find(context) != m_contextPasses.end();
        if (!bExists)
        {
            m_contextPasses[context] = {nullptr, attachmentImage, false}; // render target mode
            QueueForBuildAndInitialization();
            return;
        }

        auto& contextData = m_contextPasses[context];

        // Check if we need to switch from direct pipeline mode to render target mode
        if (contextData.m_isDirectPipelineMode)
        {
            SwitchContextMode(context, false, attachmentImage);
            return;
        }

        //Do we need to update it?
        if (contextData.m_renderTarget != attachmentImage)
        {
            contextData.m_renderTarget = attachmentImage;
            if (contextData.m_childPass)
            {
                contextData.m_childPass->UpdateRenderTarget(attachmentImage);
            }
            return;
        }
    }

    void AtomRmlParentPass::SetDirectPipelineMode(Rml::Context* context)
    {
        if (!context)
        {
            return;
        }

        //Do we have it?
        bool bExists = m_contextPasses.find(context) != m_contextPasses.end();
        if (!bExists)
        {
            m_contextPasses[context] = {nullptr, nullptr, true}; // direct pipeline mode, no render target
            QueueForBuildAndInitialization();
            return;
        }

        auto& contextData = m_contextPasses[context];

        // Check if we need to switch from render target mode to direct pipeline mode
        if (!contextData.m_isDirectPipelineMode)
        {
            SwitchContextMode(context, true, nullptr);
            return;
        }
    }

    void AtomRmlParentPass::BuildInternal()
    {
        if (AZ::RPI::Scene* scene = GetScene(); scene && !AtomRmlPassRequestBus::Handler::BusIsConnected())
        {
            AtomRmlPassRequestBus::Handler::BusConnect(scene->GetId());
        }

        for (auto& [context, data] : m_contextPasses)
        {
            if (data.m_childPass == nullptr)
            {
                if (data.m_isDirectPipelineMode)
                {
                    AddDirectPipelineChildPassForContext(context);
                }
                else
                {
                    AddChildPassForContext(context, data.m_renderTarget);
                }
            }
        }

        ParentPass::BuildInternal();
    }

    void AtomRmlParentPass::CreateChildPassesInternal()
    {
        // Child passes are created in BuildInternal
    }

    void AtomRmlParentPass::AddChildPassForContext(Rml::Context* context,
                                                 AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage)
    {
        if (!context || !attachmentImage)
        {
            return;
        }

        auto contextName = context->GetName().c_str();

        // Create a unique name for this child pass
        AZStd::string passName = AZStd::string::format("AtomRmlChildPass_%s", contextName);

        // Create the child pass from template
        AZ::RPI::PassSystemInterface* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ::RPI::Ptr<AtomRmlChildPass> childPass = azrtti_cast<AtomRmlChildPass*>(
            passSystem->CreatePassFromTemplate(AZ::Name("AtomRmlChildPassTemplate"), AZ::Name(passName)).get()
        );

        if (childPass)
        {
            // Set the attachment image for this child pass
            childPass->UpdateRenderTarget(attachmentImage);

            // Render-to-texture contexts do not participate in the direct MSAA composite chain.
            AddChild(childPass);
            m_contextPasses[context].m_childPass = childPass;
            m_contextPasses[context].m_renderTarget = attachmentImage;
            m_contextPasses[context].m_isDirectPipelineMode = false;

            AZLOG(AtomRml, "Created render target child pass '%s' for context %s", passName.c_str(),
                    contextName);
        }
        else
        {
            AZLOG_ERROR("Failed to create AtomRmlChildPass from template");
        }
    }

    void AtomRmlParentPass::AddDirectPipelineChildPassForContext(Rml::Context* context)
    {
        if (!context)
        {
            return;
        }

        auto contextName = context->GetName().c_str();

        AZStd::string passName = AZStd::string::format("AtomRmlDirectPipelineChildPass_%s", contextName);

        AZ::RPI::PassSystemInterface* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ::RPI::Ptr<AtomRmlChildPass> childPass = azrtti_cast<AtomRmlChildPass*>(
            passSystem->CreatePassFromTemplate(AZ::Name("AtomRmlChildPassDirectTemplate"), AZ::Name(passName)).get()
        );

        if (childPass)
        {
            childPass->SetDirectPipelineMode();

            // The parent template owns the resolve and composite passes. Keep the UI raster pass
            // between them so attachment dependencies produce UI -> resolve -> composite.
            InsertChild(childPass, 1);
            m_contextPasses[context].m_childPass = childPass;
            m_contextPasses[context].m_renderTarget = nullptr;
            m_contextPasses[context].m_isDirectPipelineMode = true;

            AZLOG(AtomRml, "Created direct pipeline child pass '%s' for context %s", passName.c_str(),
                    contextName);
        }
        else
        {
            AZLOG_ERROR(
                     "Failed to create AtomRmlChildPass from template for direct pipeline mode");
        }
    }

    void AtomRmlParentPass::RemoveChildPass(Rml::Context* context)
    {
        if (!context)
        {
            return;
        }

        auto it = m_contextPasses.find(context);
        if (it != m_contextPasses.end())
        {
            if (it->second.m_childPass)
            {
                it->second.m_childPass->QueueForRemoval();
            }
            m_contextPasses.erase(it);
            return;
        }

        AZLOG_WARN("Failed to find child pass for context %p", context);
    }

    AZ::RPI::Ptr<AtomRmlChildPass> AtomRmlParentPass::GetChildPass(Rml::Context* context) const
    {
        if (!context)
        {
            return nullptr;
        }

        const auto it = m_contextPasses.find(context);
        return it != m_contextPasses.end() ? it->second.m_childPass : nullptr;
    }

    void AtomRmlParentPass::SwitchContextMode(Rml::Context* context, bool isDirectPipeline,
                                            AZ::Data::Instance<AZ::RPI::AttachmentImage> renderTarget)
    {
        auto& contextData = m_contextPasses[context];

        // Remove existing child pass
        if (contextData.m_childPass)
        {
            RemoveChild(contextData.m_childPass);
            contextData.m_childPass = nullptr;
        }

        // Update mode and render target
        contextData.m_isDirectPipelineMode = isDirectPipeline;
        contextData.m_renderTarget = renderTarget;

        QueueForBuildAndInitialization();
    }

    AtomRmlParentPass* AtomRmlParentPass::GetParentPass()
    {
        return this;
    }
}
