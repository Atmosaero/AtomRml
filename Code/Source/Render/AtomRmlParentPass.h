/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AtomRmlChildPass.h"
#include "AtomRmlPassBus.h"
#include <RmlUi/Core/Context.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>

namespace AtomRml
{
    struct ContextPassData
    {
        AZ::RPI::Ptr<AtomRmlChildPass> m_childPass = nullptr;
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_renderTarget = nullptr;
        bool m_isDirectPipelineMode = false; // Track which mode this pass is in
    };

    //! Parent pass that manages child passes for each RmlUi context
    class AtomRmlParentPass final
        : public AZ::RPI::ParentPass
        , protected AtomRmlPassRequestBus::Handler
    {
        AZ_RPI_PASS(AtomRmlParentPass);

    public:
        AZ_CLASS_ALLOCATOR(AtomRmlParentPass, AZ::SystemAllocator);
        AZ_RTTI(AtomRmlParentPass, "{891D2D02-F9BC-4E90-8C7C-0F54D9017D58}", AZ::RPI::ParentPass);

        ~AtomRmlParentPass() override;
        static AZ::RPI::Ptr<AtomRmlParentPass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void UpdateRenderTarget(Rml::Context* context, AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage);

        //! Set context to direct pipeline mode (render directly to main pipeline)
        void SetDirectPipelineMode(Rml::Context* context);

        //! Removes the child pass for the given context.
        void RemoveChildPass(Rml::Context* context);

        AZ::RPI::Ptr<AtomRmlChildPass> GetChildPass(Rml::Context* context) const;

    protected:
        // Pass behavior overrides
        void ResetInternal() override;
        void BuildInternal() override;
        void CreateChildPassesInternal() override;
        void SetRenderPipeline(AZ::RPI::RenderPipeline* pipeline) override;

    private:
        AtomRmlParentPass() = delete;
        explicit AtomRmlParentPass(const AZ::RPI::PassDescriptor& descriptor);

        void AddChildPassForContext(Rml::Context* context,
                                    AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage);
        void AddDirectPipelineChildPassForContext(Rml::Context* context);

        //! Helper method to switch between direct pipeline and render target modes
        void SwitchContextMode(Rml::Context* context, bool isDirectPipeline,
                               AZ::Data::Instance<AZ::RPI::AttachmentImage> renderTarget);

        // AtomRmlPassRequestBus overrides
        AtomRmlParentPass* GetParentPass() override;

        AZStd::unordered_map<Rml::Context*, ContextPassData> m_contextPasses = {};
    };
}
