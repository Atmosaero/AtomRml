/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 Atmosaero
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
#pragma once

#include <AtomRml/AtomRmlDocumentAsset.h>

#include <Atom/Bootstrap/BootstrapNotificationBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>

namespace Rml
{
    class Context;
    class ElementDocument;
} // namespace Rml

namespace AtomRml
{
    //! Loads and optionally shows an RmlUi document selected through the O3DE asset picker.
    class AtomRmlDocumentComponent final
        : public AZ::Component
        , private AZ::Data::AssetBus::Handler
        , private AZ::Render::Bootstrap::NotificationBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(AtomRmlDocumentComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        void Activate() override;
        void Deactivate() override;

        void Show();
        void Hide();
        bool IsVisible() const;

    private:
        void OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene) override;

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        void TryLoadDocument();
        void CloseDocument();

        AZ::Data::Asset<AtomRmlDocumentAsset> m_documentAsset;
        bool m_autoShow = true;

        Rml::Context* m_context = nullptr;
        Rml::ElementDocument* m_document = nullptr;
    };
} // namespace AtomRml
