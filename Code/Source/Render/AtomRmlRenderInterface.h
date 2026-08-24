/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 Reece Hagan
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/PackedVector2.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/std/parallel/mutex.h>
#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>

#include <RmlUi/Core/RenderInterface.h>

#include <AtomRml/Allocators.h>

#include <ImGuiBus.h>

namespace AtomRml
{
    class AtomRmlChildPass;

    struct ReusableBuffer
    {
        AZ_CLASS_ALLOCATOR(ReusableBuffer, AtomRmlRenderAllocator);
        AZ::Data::Instance<AZ::RPI::Buffer> buffer = {};
        size_t elementSize = 0;
        bool inUse = false;
    };

    //! Stored geometry data for compiled RmlUi geometry
    struct AtomRmlStoredGeometry
    {
        AZ_CLASS_ALLOCATOR(AtomRmlStoredGeometry, AtomRmlRenderAllocator);
        size_t indexCount = 0;

        AZStd::vector<Rml::Vertex> vertices;
        AZStd::vector<int> indices;

        enum class StorageType
        {
            Undecided, // Waiting until End() to figure it otu
            Transient, // Lives in shared dynamic buffer.
            Persistent,// Has dedicated buffers
        };
        StorageType storageType = StorageType::Undecided;
        AtomRmlChildPass* creatorPass = nullptr;

        size_t vertexOffsetInShared = 0;
        size_t indexOffsetInShared = 0;

        // Persistent buffer assets and instances
        ReusableBuffer* vertexBuffer = nullptr;
        ReusableBuffer* indexBuffer = nullptr;

        // Pre-created buffer views for rendering
        AZ::RHI::StreamBufferView vertexBufferView = {};
        AZ::RHI::IndexBufferView indexBufferView = {};

        static void ReleaseGeometry(Rml::CompiledGeometryHandle geometry);
    };

    //! Stored texture data for RmlUi textures
    struct AtomRmlStoredTexture
    {
        AZ_CLASS_ALLOCATOR(AtomRmlStoredTexture, AtomRmlRenderAllocator);
        AZ::Data::Instance<AZ::RPI::StreamingImage> streamingImage = {};
        AZ::PackedVector2i dimensions = AZ::PackedVector2i();

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> textureAsset = {};
    };

    //! Collected draw command from RmlUi rendering
    struct AtomRmlDrawCommand
    {
        Rml::CompiledGeometryHandle geometryHandle = {};
        AZ::Vector2 translation = {};
        Rml::TextureHandle texture = 0;

        AZ::Matrix4x4 transform = AZ::Matrix4x4::CreateIdentity();

        Rml::Rectanglei scissorRegion = {};
        bool clipmaskEnabled = false;
        uint8_t stencilRef = 0;

        enum class DrawType
        {
            Normal,
            Clipmask,

            //No arguments needed, just clear clipmask
            ClearClipmask,
        };

        DrawType drawType = DrawType::Normal;
        Rml::ClipMaskOperation clipmask_op = {};
    };

    class AtomRmlRenderInterface
        : public Rml::RenderInterface
        , public ImGui::ImGuiUpdateListenerBus::Handler
    {
    public:
        AtomRmlRenderInterface();
        ~AtomRmlRenderInterface() override;

        void Begin(Rml::Context* ctx, AtomRmlChildPass* pass);
        void End();

        void OnFinishedFrame(AtomRmlChildPass* pass, AZ::u8 idx);

        static AtomRmlStoredGeometry* GetStoredGeometry(Rml::CompiledGeometryHandle handle) ;
        static const AtomRmlStoredTexture* GetStoredTexture(Rml::TextureHandle handle) ;

#pragma region Rml::RenderInterface
        //begin Rml::RenderInterface
        // Required functions for basic rendering
        Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
        void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
                            Rml::TextureHandle texture) override;
        void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

        Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
        Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
        void ReleaseTexture(Rml::TextureHandle texture) override;

        void EnableScissorRegion(bool enable) override;
        void SetScissorRegion(Rml::Rectanglei region) override;

        //Semi-advanced rendering
        void SetTransform(const Rml::Matrix4f* transform) override;

        void EnableClipMask(bool enable) override;
        void RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle geometry,
                              Rml::Vector2f translation) override;
#pragma endregion
    private:
        friend class AtomRmlChildPass;

        [[nodiscard]] AZStd::vector<struct AtomRmlChildPassDrawCommand>& GetDrawCommands() const;

        // Allocate GPU buffers for all geometry (called after End(), before rendering)
        void AllocateGPUBuffers();

        ReusableBuffer* RequestBuffer(size_t capacity, size_t elementSize);

        // Persistent pooled buffers
        AZStd::vector<AZStd::unique_ptr<ReusableBuffer>> m_buffers;
        //Once rml tells us to destroy geo's well shove them in here and wait until a pass tells us its done with
        //it to actually destroy it
        AZStd::unordered_set<Rml::CompiledGeometryHandle> m_destroyedGeometries;

        AZStd::atomic_uint64_t m_textureCreationCount = 0;

        //Per frame:
        // Tracking set for geometry created this frame (to detect transients)
        AZStd::unordered_set<Rml::CompiledGeometryHandle> m_createdThisFrame;

        AtomRmlChildPass* m_pass = nullptr;
        AZ::Matrix4x4 m_transform;
        AZ::Matrix4x4 m_contextTransform;
        Rml::Rectanglei m_scissorRegion;
        Rml::ClipMaskOperation m_clipmaskOperation;
        uint8_t m_stencilRef = 0;
        bool m_scissorEnabled = false;
        bool m_draw_to_clipmask = false;
        bool m_testClipMask = false;

        //ImGui
        void OnImGuiUpdate() override;
    };
}
