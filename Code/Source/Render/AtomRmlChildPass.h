/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/PipelineState.h>

#include "AtomRmlRenderInterface.h"

namespace Rml
{
    class Context;
}

namespace AtomRml
{
    class SrgRecycler
    {
    public:
        SrgRecycler(const AZ::Data::Instance<AZ::RPI::Shader>& shader);
        struct Srg
        {
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_srg;
            bool inUse = false;
        };

        Srg* GetSrg();
        void FreeSrg(Srg* srg);

    private:
        AZ::Data::Instance<AZ::RPI::Shader> m_shader = {};
        AZStd::vector<AZStd::unique_ptr<Srg>> m_srgs = {};
        AZStd::mutex m_mutex;
    };

    struct AtomRmlChildPassDrawCommand
    {
        AtomRmlDrawCommand drawCommand = {};
        SrgRecycler::Srg* drawSrg = nullptr;
        SrgRecycler* drawSrgRecycler = nullptr;
        bool srgReady = false;
    };

    struct FrameInfo
    {
        AZStd::vector<AtomRmlChildPassDrawCommand> drawCmds;

        // Shared dynamic buffers for transient geometry
        AZ::Data::Instance<AZ::RPI::Buffer> m_sharedVertexBuffer;
        AZ::Data::Instance<AZ::RPI::Buffer> m_sharedIndexBuffer;
        size_t m_sharedVertexCapacity = 0;
        size_t m_sharedIndexCapacity = 0;

        void EnsureTransientBufferCapacity(size_t vertexCount, size_t indexCount);
    };

    struct BufferedAtomRmlDrawCommands
    {
        static constexpr AZ::u32 DrawCommandBuffering = 2;
        AZStd::array<FrameInfo, DrawCommandBuffering> m_drawCommands;
        AZ::u8 m_currentIndex = 0;

        void NextBuffer() { m_currentIndex = (m_currentIndex + 1) % DrawCommandBuffering; }

        FrameInfo& Get() { return m_drawCommands[m_currentIndex]; }
        FrameInfo& Get(AZ::u8 idx) { return m_drawCommands[idx]; }
    };

    struct PipelineStates
    {
        AZ::RPI::Ptr<AZ::RPI::PipelineStateForDraw> standard;
        AZ::RPI::Ptr<AZ::RPI::PipelineStateForDraw> standardStencilTest;
        //The following pipeline states are for Rml::ClipMaskOperation
        AZ::RPI::Ptr<AZ::RPI::PipelineStateForDraw> CMO_Set;
        //For SetInverse use Set
        AZ::RPI::Ptr<AZ::RPI::PipelineStateForDraw> CMO_Intersect;

        AZ::RPI::Ptr<AZ::RPI::PipelineStateForDraw> GetPipelineStateForClipMaskOp(
            const Rml::ClipMaskOperation operation) const
        {
            switch (operation)
            {
            case Rml::ClipMaskOperation::SetInverse:
            case Rml::ClipMaskOperation::Set:
                return CMO_Set;
            case Rml::ClipMaskOperation::Intersect:
                return CMO_Intersect;
            default:
                return standard;
            }
        }
    };

    //! Child pass that can render RmlUi either to a specific render target or directly to the main pipeline
    class AtomRmlChildPass final
        : public AZ::RPI::RasterPass
    {
        AZ_RPI_PASS(AtomRmlChildPass);

    public:
        AZ_CLASS_ALLOCATOR(AtomRmlChildPass, AZ::SystemAllocator);
        AZ_RTTI(AtomRmlChildPass, "{5D989E88-0CEE-47F0-BB8D-15593FA1C390}", AZ::RPI::RasterPass);

        ~AtomRmlChildPass() override;
        static AZ::RPI::Ptr<AtomRmlChildPass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void UpdateRenderTarget(AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage);
        void SetRmlContext(Rml::Context* context);

        //! Set the pass to render directly to the main pipeline (no specific render target
        void SetDirectPipelineMode();

        AZ::Data::Instance<AZ::RPI::AttachmentImage> GetAttachmentImage() const
        {
            return m_attachmentImage;
        }

        Rml::Context* GetRmlContext() const
        {
            return m_rmlContext;
        }

    protected:
        void BuildInternal() override;
        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
        void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;
        void FrameEndInternal() override;

        void StandardPipelineStateInit(AZ::RPI::Ptr<AZ::RPI::PipelineStateForDraw>& ps,
            const AZ::Data::Instance<AZ::RPI::Shader>& shader);
        void StandardPipelineStateFinish(AZ::RPI::Ptr<AZ::RPI::PipelineStateForDraw>& ps);

    private:
        friend class AtomRmlRenderInterface;

        AtomRmlChildPass() = delete;
        explicit AtomRmlChildPass(const AZ::RPI::PassDescriptor& descriptor);

        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_attachmentImage;
        Rml::Context* m_rmlContext = nullptr;

        BufferedAtomRmlDrawCommands m_drawCommands = {};

        AZStd::unique_ptr<SrgRecycler> m_srgRecycler;
        AZ::Data::Instance<AZ::RPI::Shader> m_shader;
        AZStd::unique_ptr<SrgRecycler> m_creationSrgRecycler;
        AZ::Data::Instance<AZ::RPI::Shader> m_creationShader;

        //! Shader for clearing stencil buffer (fullscreen triangle)
        AZ::Data::Instance<AZ::RPI::Shader> m_clearShader;
        AZ::RPI::Ptr<AZ::RPI::PipelineStateForDraw> m_clearStencilPipelineState;

        void CreatePipelineStates(PipelineStates& states, AZ::Data::Instance<AZ::RPI::Shader> shader);

        PipelineStates m_standard;
        PipelineStates m_creation;
        AZ::u8 m_submittedIdx = 0;
    };
}
