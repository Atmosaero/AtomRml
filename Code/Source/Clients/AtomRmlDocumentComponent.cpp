/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 Atmosaero
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */

#include <AtomRml/AtomRmlDocumentComponent.h>

#include <AtomRml/AtomRmlFeatureProcessorInterface.h>
#include <AtomRml/AtomRmlTypeIds.h>

#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

namespace AtomRml
{
    AZ_COMPONENT_IMPL(AtomRmlDocumentComponent, "AtomRmlDocumentComponent", AtomRmlDocumentComponentTypeId);

    void AtomRmlDocumentComponent::Reflect(AZ::ReflectContext* context)
    {
        AtomRmlDocumentAsset::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AtomRmlDocumentComponent, AZ::Component>()
                ->Version(1)
                ->Field("DocumentAsset", &AtomRmlDocumentComponent::m_documentAsset)
                ->Field("AutoShow", &AtomRmlDocumentComponent::m_autoShow);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<AtomRmlDocumentComponent>("AtomRml Document", "Loads an RmlUi document asset into the primary AtomRml context")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "AtomRml")
                    ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, azrtti_typeid<AtomRmlDocumentAsset>())
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &AtomRmlDocumentComponent::m_documentAsset,
                        "RML document",
                        "Processed .rml document asset")
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox,
                        &AtomRmlDocumentComponent::m_autoShow,
                        "Auto show",
                        "Show the document immediately after it is loaded");
            }
        }
    }

    void AtomRmlDocumentComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomRmlDocumentService"));
    }

    void AtomRmlDocumentComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    void AtomRmlDocumentComponent::GetRequiredServices(
        [[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void AtomRmlDocumentComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void AtomRmlDocumentComponent::Activate()
    {
        AZ::Render::Bootstrap::NotificationBus::Handler::BusConnect();

        if (m_documentAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::Handler::BusConnect(m_documentAsset.GetId());
            m_documentAsset.QueueLoad();
        }
        else
        {
            AZ_Warning("AtomRml", false, "AtomRml Document component has no RML document asset assigned");
        }
    }

    void AtomRmlDocumentComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        AZ::Render::Bootstrap::NotificationBus::Handler::BusDisconnect();
        CloseDocument();
        m_context = nullptr;
    }

    void AtomRmlDocumentComponent::Show()
    {
        if (m_document)
        {
            m_document->Show();
        }
    }

    void AtomRmlDocumentComponent::Hide()
    {
        if (m_document)
        {
            m_document->Hide();
        }
    }

    bool AtomRmlDocumentComponent::IsVisible() const
    {
        return m_document && m_document->IsVisible();
    }

    void AtomRmlDocumentComponent::OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene)
    {
        auto* featureProcessor = bootstrapScene ? bootstrapScene->GetFeatureProcessor<AtomRmlFeatureProcessorInterface>() : nullptr;
        m_context = featureProcessor ? featureProcessor->GetContext() : nullptr;

        if (!m_context)
        {
            AZ_Error("AtomRml", false, "AtomRml Document component could not find the primary RmlUi context");
            return;
        }

        TryLoadDocument();
    }

    void AtomRmlDocumentComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_documentAsset = asset;
        TryLoadDocument();
    }

    void AtomRmlDocumentComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_documentAsset = asset;
        CloseDocument();
        TryLoadDocument();
    }

    void AtomRmlDocumentComponent::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_Error("AtomRml", false, "Failed to load RML document asset: %s", asset.GetHint().c_str());
    }

    void AtomRmlDocumentComponent::TryLoadDocument()
    {
        if (m_document || !m_context || !m_documentAsset.IsReady())
        {
            return;
        }

        const AZStd::string& contents = m_documentAsset->GetContents();
        m_document = m_context->LoadDocumentFromMemory(contents.c_str(), m_documentAsset.GetHint().c_str());
        if (!m_document)
        {
            AZ_Error("AtomRml", false, "RmlUi failed to parse document asset: %s", m_documentAsset.GetHint().c_str());
            return;
        }

        if (m_autoShow)
        {
            m_document->Show();
        }
    }

    void AtomRmlDocumentComponent::CloseDocument()
    {
        if (m_document)
        {
            m_document->Close();
            m_document = nullptr;
        }
    }
} // namespace AtomRml
