/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 Reece Hagan
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
#include "AtomRmlParentPass.h"
#include <AzCore/Name/Name.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

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

            // Add as child
            AddChild(childPass);
            m_contextPasses[context].m_childPass = childPass;
            m_contextPasses[context].m_renderTarget = attachmentImage;
            m_contextPasses[context].m_isDirectPipelineMode = false;

            AZ_Info("AtomRmlParentPass", "Created render target child pass '%s' for context %s", passName.c_str(),
                    contextName);
        }
        else
        {
            AZ_Error("AtomRmlParentPass", false, "Failed to create AtomRmlChildPass from template");
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

            AddChild(childPass);
            m_contextPasses[context].m_childPass = childPass;
            m_contextPasses[context].m_renderTarget = nullptr;
            m_contextPasses[context].m_isDirectPipelineMode = true;

            AZ_Info("AtomRmlParentPass", "Created direct pipeline child pass '%s' for context %s", passName.c_str(),
                    contextName);
        }
        else
        {
            AZ_Error("AtomRmlParentPass", false,
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
            it->second.m_childPass->QueueForRemoval();
            m_contextPasses.erase(it);
            return;
        }

        AZ_Warning("AtomRmlParentPass", false, "Failed to find child pass for context %p", context);
    }

    AZ::RPI::Ptr<AtomRmlChildPass> AtomRmlParentPass::GetChildPass(Rml::Context* context) const
    {
        if (!context)
        {
            return nullptr;
        }

        return m_contextPasses.find(context)->second.m_childPass;
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
}
