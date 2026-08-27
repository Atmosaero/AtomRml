/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AtomRmlChildPass.h"
#include "AtomRmlRenderInterface.h"

#include <AzCore/Name/Name.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Jobs/Algorithms.h>

#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/GeometryView.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>

#include <RmlUi/Core.h>
#include <AtomRml/AtomRmlBus.h>
#include "../AtomRmlBudget.h"

namespace AtomRml
{
    AtomRmlChildPass::~AtomRmlChildPass()
    {
        AtomRmlRenderInterface* renderInterface = nullptr;
        AtomRmlRequestBus::BroadcastResult(renderInterface, &AtomRmlRequestBus::Events::GetRenderInterface);
        if (renderInterface)
        {
            renderInterface->OnPassDestroyed(this);
        }
    }

    SrgRecycler::SrgRecycler(const AZ::Data::Instance<AZ::RPI::Shader>& shader)
        : m_shader(shader)
    {
    }

    SrgRecycler::Srg* SrgRecycler::GetSrg()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        for (auto& srg : m_srgs)
        {
            if (!srg->inUse)
            {
                srg->inUse = true;
                return srg.get();
            }
        }

        auto srg = AZ::RPI::ShaderResourceGroup::Create(
            m_shader->GetAsset(), m_shader->GetSupervariantIndex(), AZ::Name("DrawSrg"));
        if (srg == nullptr)
        {
            AZLOG_ERROR("Failed to create SRG resource");
            return nullptr;
        }

        m_srgs.push_back(AZStd::make_unique<Srg>(srg, true));
        return m_srgs.back().get();
    }

    void SrgRecycler::FreeSrg(Srg* srg)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        srg->inUse = false;
    }

    void FrameInfo::EnsureTransientBufferCapacity(size_t vertexCount, size_t indexCount)
    {
        const size_t vertexBytes = vertexCount * sizeof(Rml::Vertex);
        const size_t indexBytes = indexCount * sizeof(int);

        // Create or resize vertex buffer if needed
        if (!m_sharedVertexBuffer || m_sharedVertexCapacity < vertexBytes)
        {
            // Grow by 1.5x to reduce frequent reallocations
            const size_t newCapacity = AZStd::max(vertexBytes, m_sharedVertexCapacity * 3 / 2);

            AZ::RPI::CommonBufferDescriptor desc;
            desc.m_poolType = AZ::RPI::CommonBufferPoolType::DynamicInputAssembly;
            desc.m_bufferName = "AtomRml Shared Transient Vertex Buffer";
            desc.m_byteCount = newCapacity;
            desc.m_elementSize = sizeof(Rml::Vertex);
            desc.m_bufferData = nullptr;

            m_sharedVertexBuffer = AZ::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
            m_sharedVertexCapacity = newCapacity;

            AZLOG(AtomRml, "Allocated shared vertex buffer: %zu bytes (%zu vertices)",
                    newCapacity, newCapacity / sizeof(Rml::Vertex));
        }

        // Create or resize index buffer if needed
        if (!m_sharedIndexBuffer || m_sharedIndexCapacity < indexBytes)
        {
            const size_t newCapacity = AZStd::max(indexBytes, m_sharedIndexCapacity * 3 / 2);

            AZ::RPI::CommonBufferDescriptor desc;
            desc.m_poolType = AZ::RPI::CommonBufferPoolType::DynamicInputAssembly;
            desc.m_bufferName = "AtomRml Shared Transient Index Buffer";
            desc.m_byteCount = newCapacity;
            desc.m_elementSize = sizeof(int);
            desc.m_bufferData = nullptr;

            m_sharedIndexBuffer = AZ::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
            m_sharedIndexCapacity = newCapacity;

            AZLOG(AtomRml, "Allocated shared index buffer: %zu bytes (%zu indices)",
                    newCapacity, newCapacity / sizeof(int));
        }
    }

    AZ::RPI::Ptr<AtomRmlChildPass> AtomRmlChildPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        return aznew AtomRmlChildPass(descriptor);
    }

    void AtomRmlChildPass::UpdateRenderTarget(AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage)
    {
        m_attachmentImage = attachmentImage;
        QueueForBuildAndInitialization();
    }

    AtomRmlChildPass::AtomRmlChildPass(const AZ::RPI::PassDescriptor& descriptor)
        : RasterPass(descriptor)
    {
        m_drawCommands.m_drawCommands[0] = {};
        m_drawCommands.m_drawCommands[1] = {};
    }

    void AtomRmlChildPass::BuildInternal()
    {
        // Two modes: render to specific target or render to main pipeline
        if (m_attachmentImage)
        {
            AttachImageToSlot(AZ::Name("ColorOutput"), m_attachmentImage);

            auto imageSize = m_attachmentImage->GetDescriptor().m_size;
            m_scissorState = AZ::RHI::Scissor(0, 0, imageSize.m_width, imageSize.m_height);
            m_viewportState = AZ::RHI::Viewport(0.f, aznumeric_cast<float>(imageSize.m_width), 0.f,
                                                aznumeric_cast<float>(imageSize.m_height));
            m_overrideScissorSate = true;
            m_overrideViewportState = true;
        }
        else
        {
            m_overrideScissorSate = false;
            m_overrideViewportState = false;
        }

        // The direct pass can switch between single-sampled render targets and the multisampled
        // pipeline attachment. Recreate output-dependent pipeline states whenever the pass rebuilds.
        m_standard = {};
        m_clearStencilPipelineState = nullptr;

        RasterPass::BuildInternal();
    }

    void AtomRmlChildPass::SetRmlContext(Rml::Context* context)
    {
        m_rmlContext = context;
    }

    void AtomRmlChildPass::SetDirectPipelineMode()
    {
        if (m_attachmentImage == nullptr)
        {
            //Already in direct pipeline mode.
            return;
        }
        m_attachmentImage = nullptr;
        QueueForBuildAndInitialization();
    }

    void AtomRmlChildPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        AZ_PROFILE_FUNCTION(AtomRmlBudget);
        RasterPass::SetupFrameGraphDependencies(frameGraph);

        if (m_rmlContext == nullptr)
            return;

        AtomRmlRenderInterface* renderInterface = nullptr;
        AtomRmlRequestBus::BroadcastResult(renderInterface, &AtomRmlRequestBus::Events::GetRenderInterface);
        if (renderInterface == nullptr)
            return;

        m_drawCommands.NextBuffer();

        renderInterface->Begin(m_rmlContext, this);
        {
            AZ_PROFILE_SCOPE(AtomRmlBudget, "Rml::Context::Render");
            m_rmlContext->Render();
        }
        renderInterface->End();

        auto drawCount = static_cast<uint32_t>(m_drawCommands.Get().drawCmds.size());
        frameGraph.SetEstimatedItemCount(drawCount);
    }

    void AtomRmlChildPass::StandardPipelineStateInit(AZ::RPI::Ptr<AZ::RPI::PipelineStateForDraw>& ps,
        const AZ::Data::Instance<AZ::RPI::Shader>& shader)
    {
        ps = aznew AZ::RPI::PipelineStateForDraw;
        ps->Init(shader);
        AZ::RHI::InputStreamLayoutBuilder layoutBuilder;
        layoutBuilder.AddBuffer()
                     ->Channel("POSITION", AZ::RHI::Format::R32G32_FLOAT)
                     ->Channel("COLOR", AZ::RHI::Format::R8G8B8A8_UNORM)
                     ->Channel("TEXCOORD0", AZ::RHI::Format::R32G32_FLOAT);
        ps->InputStreamLayout() = layoutBuilder.End();
    }

    void AtomRmlChildPass::StandardPipelineStateFinish(AZ::RPI::Ptr<AZ::RPI::PipelineStateForDraw>& ps)
    {
        ps->SetOutputFromPass(this);
        ps->Finalize();
    }

    void AtomRmlChildPass::CreatePipelineStates(PipelineStates& states, AZ::Data::Instance<AZ::RPI::Shader> shader)
    {
        if (!shader)
        {
            return;
        }
        AZ_PROFILE_FUNCTION(AtomRmlBudget);

        if (states.standard == nullptr)
        {
            StandardPipelineStateInit(states.standard, shader);

            AZ::RHI::RenderStates& renderStates = states.standard->RenderStatesOverlay();
            renderStates.m_depthStencilState.m_depth.m_enable = false;
            auto& stencilState = renderStates.m_depthStencilState.m_stencil;
            stencilState.m_enable = false;
            stencilState.m_frontFace.m_failOp = AZ::RHI::StencilOp::Keep;
            stencilState.m_frontFace.m_passOp = AZ::RHI::StencilOp::Keep;
            stencilState.m_frontFace.m_depthFailOp = AZ::RHI::StencilOp::Keep;
            stencilState.m_frontFace.m_func = AZ::RHI::ComparisonFunc::Equal;
            stencilState.m_writeMask = 0xFF;
            stencilState.m_readMask = 0xFF;

            stencilState.m_backFace = stencilState.m_frontFace;

            StandardPipelineStateFinish(states.standard);

            states.standard->GetRHIPipelineState()->GetDevicePipelineState(0)->SetName(
                AZ::Name("AtomRml Standard Standard"));
        }

        if (states.standardStencilTest == nullptr)
        {
            StandardPipelineStateInit(states.standardStencilTest, shader);

            AZ::RHI::RenderStates& renderStates = states.standardStencilTest->RenderStatesOverlay();
            renderStates.m_depthStencilState.m_depth.m_enable = false;
            auto& stencilState = renderStates.m_depthStencilState.m_stencil;
            stencilState.m_enable = true;
            stencilState.m_frontFace.m_failOp = AZ::RHI::StencilOp::Keep;
            stencilState.m_frontFace.m_passOp = AZ::RHI::StencilOp::Keep;
            stencilState.m_frontFace.m_depthFailOp = AZ::RHI::StencilOp::Keep;
            stencilState.m_frontFace.m_func = AZ::RHI::ComparisonFunc::Equal;
            stencilState.m_writeMask = 0xFF;
            stencilState.m_readMask = 0xFF;

            stencilState.m_backFace = stencilState.m_frontFace;

            StandardPipelineStateFinish(states.standardStencilTest);

            states.standardStencilTest->GetRHIPipelineState()->GetDevicePipelineState(0)->SetName(
                AZ::Name("AtomRml Standard StandardStencilTest"));
        }

        if (states.CMO_Set == nullptr)
        {
            StandardPipelineStateInit(states.CMO_Set, shader);

            AZ::RHI::RenderStates& renderStates = states.CMO_Set->RenderStatesOverlay();
            renderStates.m_depthStencilState.m_depth.m_enable = false;
            auto& stencilState = renderStates.m_depthStencilState.m_stencil;
            stencilState.m_enable = true;
            stencilState.m_frontFace.m_failOp = AZ::RHI::StencilOp::Keep;
            stencilState.m_frontFace.m_passOp = AZ::RHI::StencilOp::Replace;
            stencilState.m_frontFace.m_depthFailOp = AZ::RHI::StencilOp::Keep;
            stencilState.m_frontFace.m_func = AZ::RHI::ComparisonFunc::Always;
            stencilState.m_writeMask = 0xFF;
            stencilState.m_readMask = 0xFF;

            stencilState.m_backFace = stencilState.m_frontFace;

            AZ::RHI::TargetBlendState& blendState = renderStates.m_blendState.m_targets[0];
            blendState.m_writeMask = 0; //No Color output

            StandardPipelineStateFinish(states.CMO_Set);

            states.CMO_Set->GetRHIPipelineState()->GetDevicePipelineState(0)->SetName(
                AZ::Name("AtomRml Standard CMO_Set"));
        }

        if (states.CMO_Intersect == nullptr)
        {
            StandardPipelineStateInit(states.CMO_Intersect, shader);

            AZ::RHI::RenderStates& renderStates = states.CMO_Intersect->RenderStatesOverlay();
            renderStates.m_depthStencilState.m_depth.m_enable = false;
            auto& stencilState = renderStates.m_depthStencilState.m_stencil;
            stencilState.m_enable = true;
            stencilState.m_frontFace.m_failOp = AZ::RHI::StencilOp::Keep;
            stencilState.m_frontFace.m_passOp = AZ::RHI::StencilOp::IncrementSaturate;
            stencilState.m_frontFace.m_depthFailOp = AZ::RHI::StencilOp::Keep;
            stencilState.m_frontFace.m_func = AZ::RHI::ComparisonFunc::Always;
            stencilState.m_writeMask = 0xFF;
            stencilState.m_readMask = 0xFF;

            stencilState.m_backFace = stencilState.m_frontFace;

            AZ::RHI::TargetBlendState& blendState = renderStates.m_blendState.m_targets[0];
            blendState.m_writeMask = 0; //No Color output

            StandardPipelineStateFinish(states.CMO_Intersect);

            states.CMO_Intersect->GetRHIPipelineState()->GetDevicePipelineState(0)->SetName(
                AZ::Name("AtomRml Standard CMO_Intersect"));
        }
    }

    void AtomRmlChildPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        AZ_PROFILE_FUNCTION(AtomRmlBudget);
        RasterPass::CompileResources(context);

        // Load the UIElement shader if we haven't already
        if (!m_shader)
        {
            const char* shaderFilePath = "Shaders/AtomRml/UIElement.azshader";
            m_shader = AZ::RPI::LoadCriticalShader(shaderFilePath);
            if (!m_shader)
            {
                AZLOG_ERROR("Failed to load UIElement shader: %s", shaderFilePath);
                return;
            }
            m_srgRecycler = AZStd::make_unique<SrgRecycler>(m_shader);
            AZLOG(AtomRml, "Successfully loaded UIElement shader");
        }

        //Ensure our standard shader set exists.
        CreatePipelineStates(m_standard, m_shader);

        if (!m_creationShader)
        {
            const char* shaderFilePath = "Shaders/AtomRml/Creation.azshader";
            m_creationShader = AZ::RPI::LoadCriticalShader(shaderFilePath);
            if (m_creationShader)
            {
                m_creationSrgRecycler = AZStd::make_unique<SrgRecycler>(m_creationShader);
                AZLOG(AtomRml, "Successfully loaded Creation shader");
            }
            else
            {
                AZLOG_ERROR("Failed to load Creation shader: %s", shaderFilePath);
            }
        }

        CreatePipelineStates(m_creation, m_creationShader);

        if (!m_clearShader)
        {
            const char* clearShaderPath = "Shaders/AtomRml/ClearStencil.azshader";
            m_clearShader = AZ::RPI::LoadCriticalShader(clearShaderPath);
            if (!m_clearShader)
            {
                AZLOG_ERROR("Failed to load clear stencil shader: %s", clearShaderPath);
            }
            else
            {
                AZLOG(AtomRml, "Successfully loaded clear stencil shader");
            }
        }

        if (m_clearShader && !m_clearStencilPipelineState)
        {
            m_clearStencilPipelineState = aznew AZ::RPI::PipelineStateForDraw;
            m_clearStencilPipelineState->Init(m_clearShader);

            AZ::RHI::InputStreamLayoutBuilder layoutBuilder;
            // Fullscreen triangle generated in vertex shader
            m_clearStencilPipelineState->InputStreamLayout() = layoutBuilder.End();

            AZ::RHI::RenderStates& renderStates = m_clearStencilPipelineState->RenderStatesOverlay();

            renderStates.m_depthStencilState.m_depth.m_enable = false;

            auto& stencilState = renderStates.m_depthStencilState.m_stencil;
            stencilState.m_enable = true;
            stencilState.m_frontFace.m_failOp = AZ::RHI::StencilOp::Replace;
            stencilState.m_frontFace.m_passOp = AZ::RHI::StencilOp::Replace;
            stencilState.m_frontFace.m_depthFailOp = AZ::RHI::StencilOp::Replace;
            stencilState.m_frontFace.m_func = AZ::RHI::ComparisonFunc::Always;
            stencilState.m_writeMask = 0xFF;
            stencilState.m_readMask = 0xFF;
            stencilState.m_backFace = stencilState.m_frontFace;

            AZ::RHI::TargetBlendState& blendState = renderStates.m_blendState.m_targets[0];
            blendState.m_writeMask = 0;

            m_clearStencilPipelineState->SetOutputFromPass(this);
            m_clearStencilPipelineState->Finalize();

            AZLOG(AtomRml, "Created clear stencil pipeline state");
        }

        // Compile SRGs for all draw commands that don't have them yet
        if (!m_shader || m_rmlContext == nullptr)
            return;

        AtomRmlRenderInterface* renderInterface = nullptr;
        AtomRmlRequestBus::BroadcastResult(renderInterface, &AtomRmlRequestBus::Events::GetRenderInterface);
        if (renderInterface == nullptr)
            return;

        {
            AZ_PROFILE_SCOPE(AtomRmlBudget, "Process DrawCommands");
            auto& drawCommands = m_drawCommands.Get().drawCmds;
            for (auto& childPassCmd : drawCommands)
            {
                const bool needSRG = childPassCmd.drawCommand.drawType != AtomRmlDrawCommand::DrawType::ClearClipmask;

                if (needSRG && !childPassCmd.srgReady && !childPassCmd.drawSrg)
                {
                    childPassCmd.drawSrgRecycler =
                        childPassCmd.drawCommand.shaderType == AtomRmlDrawCommand::ShaderType::Creation
                        ? m_creationSrgRecycler.get()
                        : m_srgRecycler.get();
                    if (!childPassCmd.drawSrgRecycler)
                    {
                        continue;
                    }
                    childPassCmd.drawSrg = childPassCmd.drawSrgRecycler->GetSrg();

                    if (childPassCmd.drawSrg)
                    {
                        // Find shader input indices
                        auto transformIndex = childPassCmd.drawSrg->m_srg->FindShaderInputConstantIndex(
                            AZ::Name("m_transform"));
                        auto translateIndex = childPassCmd.drawSrg->m_srg->FindShaderInputConstantIndex(
                            AZ::Name("m_translate"));
                        auto hasTextureIndex = childPassCmd.drawSrg->m_srg->FindShaderInputConstantIndex(
                            AZ::Name("m_hasTexture"));
                        auto textureIndex = childPassCmd.drawSrg->m_srg->FindShaderInputImageIndex(
                            AZ::Name("m_texture"));
                        auto timeIndex = childPassCmd.drawSrg->m_srg->FindShaderInputConstantIndex(AZ::Name("m_time"));
                        auto dimensionsIndex = childPassCmd.drawSrg->m_srg->FindShaderInputConstantIndex(
                            AZ::Name("m_dimensions"));

                        if (transformIndex.IsValid())
                        {
                            childPassCmd.drawSrg->m_srg->
                                         SetConstant(transformIndex, childPassCmd.drawCommand.transform);
                        }

                        if (translateIndex.IsValid())
                        {
                            childPassCmd.drawSrg->m_srg->SetConstant(translateIndex,
                                                                     childPassCmd.drawCommand.translation);
                        }

                        bool hasTexture = childPassCmd.drawCommand.texture != 0;
                        if (hasTextureIndex.IsValid())
                        {
                            childPassCmd.drawSrg->m_srg->SetConstant(hasTextureIndex, hasTexture);
                        }

                        if (hasTexture && textureIndex.IsValid())
                        {
                            if (auto storedTex = renderInterface->
                                GetStoredTexture(childPassCmd.drawCommand.texture))
                            {
                                if (storedTex->streamingImage)
                                {
                                    childPassCmd.drawSrg->m_srg->SetImage(textureIndex, storedTex->streamingImage);
                                }
                            }
                        }
                        if (timeIndex.IsValid())
                        {
                            childPassCmd.drawSrg->m_srg->SetConstant(timeIndex, childPassCmd.drawCommand.shaderTime);
                        }
                        if (dimensionsIndex.IsValid())
                        {
                            childPassCmd.drawSrg->m_srg->SetConstant(
                                dimensionsIndex, childPassCmd.drawCommand.shaderDimensions);
                        }
                        childPassCmd.drawSrg->m_srg->Compile();
                        childPassCmd.srgReady = true;
                    }
                }
            }
        }
    }

    void AtomRmlChildPass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
    {
        AZ_PROFILE_FUNCTION(AtomRmlBudget);
        RasterPass::BuildCommandListInternal(context);
        auto atomRmlInterface = AtomRmlInterface::Get();
        auto& drawCommands = m_drawCommands.Get().drawCmds;
        m_submittedIdx =  m_drawCommands.m_currentIndex;

        if (atomRmlInterface == nullptr || !m_shader || !m_shader->GetAsset() || drawCommands.empty())
        {
            return;
        }

        auto renderInterface = atomRmlInterface->GetRenderInterface();
        if (renderInterface == nullptr)
        {
            return;
        }

        auto* commandList = context.GetCommandList();
        for (size_t drawIndex = context.GetSubmitRange().m_startIndex; drawIndex < context.GetSubmitRange().m_endIndex;
             ++drawIndex)
        {
            auto& drawCmd = drawCommands[drawIndex];
            if (drawCmd.drawCommand.drawType == AtomRmlDrawCommand::DrawType::ClearClipmask)
            {
                // Clear stencil buffer using fullscreen triangle
                if (m_clearStencilPipelineState)
                {
                    AZ::RHI::DeviceDrawItem clearItem;
                    clearItem.m_drawInstanceArgs = AZ::RHI::DrawInstanceArguments(1, 0);

                    // Create empty geometry view for fullscreen triangle (generated in vertex shader)
                    AZ::RHI::GeometryView geometryView{AZ::RHI::MultiDevice::AllDevices};
                    geometryView.SetDrawArguments(AZ::RHI::DrawLinear(3, 0)); // 3 vertices for fullscreen triangle
                    clearItem.m_geometryView = geometryView.GetDeviceGeometryView(context.GetDeviceIndex());
                    clearItem.m_streamIndices = geometryView.GetFullStreamBufferIndices();

                    clearItem.m_pipelineState = m_clearStencilPipelineState->GetRHIPipelineState()->
                                                                             GetDevicePipelineState(
                                                                                 context.GetDeviceIndex()).get();
                    clearItem.m_stencilRef = 0; // Clear stencil to 0
                    clearItem.m_scissorsCount = 0;
                    clearItem.m_scissors = nullptr;

                    commandList->Submit(clearItem, static_cast<uint32_t>(drawIndex));
                }
                continue;
            }

            if (!drawCmd.srgReady || !drawCmd.drawSrg)
            {
                continue;
            }

            // Get the stored geometry
            if (auto storedGeo = renderInterface->GetStoredGeometry(drawCmd.drawCommand.geometryHandle))
            {
                AZ::RHI::DeviceDrawItem drawItem;
                drawItem.m_drawInstanceArgs = AZ::RHI::DrawInstanceArguments(1, 0);

                AZ::RHI::GeometryView geometryView{AZ::RHI::MultiDevice::AllDevices};
                geometryView.SetDrawArguments(
                    AZ::RHI::DrawIndexed(0, static_cast<uint32_t>(storedGeo->indexCount), 0));
                geometryView.SetIndexBufferView(storedGeo->indexBufferView);
                geometryView.AddStreamBufferView(storedGeo->vertexBufferView);

                drawItem.m_geometryView = geometryView.GetDeviceGeometryView(context.GetDeviceIndex());
                drawItem.m_streamIndices = geometryView.GetFullStreamBufferIndices();

                const PipelineStates& pipelineStates =
                    drawCmd.drawCommand.shaderType == AtomRmlDrawCommand::ShaderType::Creation ? m_creation : m_standard;

                if (drawCmd.drawCommand.drawType == AtomRmlDrawCommand::DrawType::Normal)
                {
                    if (drawCmd.drawCommand.clipmaskEnabled)
                    {
                        drawItem.m_pipelineState = pipelineStates.standardStencilTest->GetRHIPipelineState()->
                                                              GetDevicePipelineState(context.GetDeviceIndex()).
                                                              get();
                    }
                    else
                    {
                        drawItem.m_pipelineState = pipelineStates.standard->GetRHIPipelineState()->
                                                              GetDevicePipelineState(context.GetDeviceIndex()).
                                                              get();
                    }
                }
                else
                {
                    //clipmask
                    drawItem.m_pipelineState =
                        pipelineStates.GetPipelineStateForClipMaskOp(drawCmd.drawCommand.clipmask_op)
                                  ->GetRHIPipelineState()->GetDevicePipelineState(context.GetDeviceIndex()).get();
                }

                drawItem.m_scissorsCount = 0;
                drawItem.m_scissors = nullptr;
                drawItem.m_stencilRef = drawCmd.drawCommand.stencilRef;

                AZ::RHI::Scissor scissor;

                if (drawCmd.drawCommand.scissorRegion != Rml::Rectanglei())
                {
                    const auto scissorRegion = drawCmd.drawCommand.scissorRegion;
                    scissor = AZ::RHI::Scissor(
                        scissorRegion.p0.x,
                        scissorRegion.p0.y,
                        scissorRegion.p1.x,
                        scissorRegion.p1.y);

                    drawItem.m_scissorsCount = 1;
                    drawItem.m_scissors = &scissor;
                }

                commandList->SetShaderResourceGroupForDraw(
                    *drawCmd.drawSrg->m_srg->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(
                        context.GetDeviceIndex()));

                commandList->Submit(drawItem, static_cast<uint32_t>(drawIndex));
            }
        }
    }

    void AtomRmlChildPass::FrameEndInternal()
    {
        RasterPass::FrameEndInternal();
        auto& commands = m_drawCommands.Get(m_submittedIdx);
        for (auto& command : commands.drawCmds)
        {
            if (!command.drawSrg)
            {
                continue;
            }

            command.drawSrgRecycler->FreeSrg(command.drawSrg);
        }

        AtomRmlRenderInterface* renderInterface = nullptr;
        AtomRmlRequestBus::BroadcastResult(renderInterface, &AtomRmlRequestBus::Events::GetRenderInterface);
        if (renderInterface == nullptr)
            return;

        renderInterface->OnFinishedFrame(this, m_submittedIdx);
    }
}
