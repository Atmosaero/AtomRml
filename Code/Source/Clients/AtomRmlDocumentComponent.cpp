/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomRml/AtomRmlDocumentComponent.h>

#include <AtomRml/AtomRmlActionBus.h>
#include <AtomRml/AtomRmlFeatureProcessorInterface.h>
#include <AtomRml/AtomRmlTypeIds.h>

#include "AtomRmlActionRouter.h"
#include "AtomRmlDocumentManager.h"

#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

namespace AtomRml
{
    namespace
    {
        class AtomRmlActionNotificationBusBehaviorHandler final
            : public AtomRmlActionNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(
                AtomRmlActionNotificationBusBehaviorHandler,
                "{83FF3EE9-C6EA-4C01-84B7-8C4BAD42F032}",
                AZ::SystemAllocator,
                OnAction);

            void OnAction(const AtomRmlActionEvent& actionEvent) override
            {
                Call(FN_OnAction, actionEvent);
            }
        };

        bool AtomRmlDocumentComponentVersionConverter(
            AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 2)
            {
                bool autoLoad = true;
                classElement.GetChildData(AZ_CRC_CE("AutoShow"), autoLoad);
                classElement.RemoveElementByName(AZ_CRC_CE("AutoShow"));
                classElement.AddElementWithData(context, "AutoLoad", autoLoad);
            }

            return true;
        }
    } // namespace

    AZ_COMPONENT_IMPL(AtomRmlDocumentComponent, "AtomRmlDocumentComponent", AtomRmlDocumentComponentTypeId);

    void AtomRmlDocumentComponent::Reflect(AZ::ReflectContext* context)
    {
        AtomRmlDocumentAsset::Reflect(context);
        AtomRmlActionEvent::Reflect(context);
        AtomRmlDocumentEvent::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AtomRmlDocumentComponent, AZ::Component>()
                ->Version(2, &AtomRmlDocumentComponentVersionConverter)
                ->Field("DocumentAsset", &AtomRmlDocumentComponent::m_documentAsset)
                ->Field("AutoLoad", &AtomRmlDocumentComponent::m_autoLoad);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<AtomRmlDocumentComponent>(
                        "Rml Document Asset Ref", "Associates an RmlUi document asset with an entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "UI")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/RmlDocumentAssetRef.svg")
                    ->Attribute(
                        AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/RmlDocumentAssetRef.svg")
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://github.com/Atmosaero/AtomRml")
                    ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, azrtti_typeid<AtomRmlDocumentAsset>())
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &AtomRmlDocumentComponent::m_documentAsset,
                        "Document pathname",
                        "The pathname of the Rml document.")
                    ->Attribute("BrowseIcon", ":/stylesheet/img/UI20/browse-edit-select-files.svg")
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox,
                        &AtomRmlDocumentComponent::m_autoLoad,
                        "Auto Load",
                        "When checked, the document is loaded when this component is activated.");
            }
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AtomRmlDocumentAssetRefBus>("AtomRmlDocumentAssetRefBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "AtomRml")
                ->Attribute(AZ::Script::Attributes::Module, "atomrml")
                ->Event("SetPath", &AtomRmlDocumentAssetRefBus::Events::SetPath)
                ->Event("Remove", &AtomRmlDocumentAssetRefBus::Events::Remove)
                ->Event("Show", &AtomRmlDocumentAssetRefBus::Events::Show);

            behaviorContext->EBus<AtomRmlActionNotificationBus>("AtomRmlActionNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "AtomRml")
                ->Attribute(AZ::Script::Attributes::Module, "atomrml")
                ->Handler<AtomRmlActionNotificationBusBehaviorHandler>();

            behaviorContext->EBus<AtomRmlDocumentEventBus>("AtomRmlDocumentEventBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "AtomRml")
                ->Attribute(AZ::Script::Attributes::Module, "atomrml")
                ->Event("DispatchEvent", &AtomRmlDocumentEventBus::Events::DispatchEvent);
        }
    }

    void AtomRmlDocumentComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomRmlDocumentService"));
    }

    void AtomRmlDocumentComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomRmlDocumentService"));
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
        m_isActive = true;
        AtomRmlDocumentAssetRefBus::Handler::BusConnect(GetEntityId());
        AtomRmlDocumentEventBus::Handler::BusConnect(GetEntityId());
        AZ::Render::Bootstrap::NotificationBus::Handler::BusConnect();
        if (auto* documentManager = AZ::Interface<AtomRmlDocumentManagerInterface>::Get())
        {
            documentManager->RegisterComponent(this);
        }

        if (m_autoLoad)
        {
            LoadDocument();
        }
    }

    void AtomRmlDocumentComponent::Deactivate()
    {
        if (auto* documentManager = AZ::Interface<AtomRmlDocumentManagerInterface>::Get())
        {
            documentManager->UnregisterComponent(this);
        }
        AtomRmlDocumentEventBus::Handler::BusDisconnect();
        AtomRmlDocumentAssetRefBus::Handler::BusDisconnect();
        m_isActive = false;
        UnloadDocument();
        AZ::Render::Bootstrap::NotificationBus::Handler::BusDisconnect();
        m_context = nullptr;
    }

    void AtomRmlDocumentComponent::SetPath(const AZ::Data::AssetId& assetId)
    {
        if (m_documentAsset.GetId() == assetId)
        {
            return;
        }

        const bool shouldLoad = m_loadRequested || (m_isActive && m_autoLoad);
        UnloadDocument();

        if (assetId.IsValid())
        {
            m_documentAsset = AZ::Data::Asset<AtomRmlDocumentAsset>(
                assetId, azrtti_typeid<AtomRmlDocumentAsset>());
            if (shouldLoad)
            {
                LoadDocument();
            }
        }
        else
        {
            m_documentAsset.Reset();
        }
    }

    void AtomRmlDocumentComponent::Remove()
    {
        UnloadDocument();
    }

    void AtomRmlDocumentComponent::LoadDocument()
    {
        if (!m_documentAsset.GetId().IsValid())
        {
            AZLOG_WARN("Rml Document Asset Ref component has no Rml document asset assigned");
            return;
        }

        m_loadRequested = true;
        if (!AZ::Data::AssetBus::Handler::BusIsConnected())
        {
            AZ::Data::AssetBus::Handler::BusConnect(m_documentAsset.GetId());
        }

        if (m_documentAsset.IsReady())
        {
            TryLoadDocument();
        }
        else
        {
            m_documentAsset.QueueLoad();
        }
    }

    void AtomRmlDocumentComponent::UnloadDocument()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        CloseDocument();
        m_loadRequested = false;
    }

    void AtomRmlDocumentComponent::Show()
    {
        if (m_document)
        {
            m_document->Show();
        }
        else
        {
            LoadDocument();
        }
    }

    bool AtomRmlDocumentComponent::DispatchEvent(const AtomRmlDocumentEvent& documentEvent)
    {
        if (!m_document)
        {
            AZLOG_WARN("Cannot dispatch Rml event '%s' because the entity has no loaded document",
                documentEvent.m_eventType.c_str());
            return false;
        }

        if (documentEvent.m_eventType.empty())
        {
            AZLOG_WARN("Cannot dispatch an Rml event with an empty type");
            return false;
        }

        Rml::Element* target = m_document;
        if (!documentEvent.m_targetElementId.empty())
        {
            target = m_document->GetElementById(documentEvent.m_targetElementId.c_str());
            if (!target)
            {
                AZLOG_WARN("Cannot dispatch Rml event '%s': element '%s' was not found",
                    documentEvent.m_eventType.c_str(), documentEvent.m_targetElementId.c_str());
                return false;
            }
        }

        Rml::Dictionary parameters;
        for (const auto& [name, value] : documentEvent.GetStringParameters())
        {
            parameters[name.c_str()] = value.c_str();
        }
        for (const auto& [name, value] : documentEvent.GetNumberParameters())
        {
            parameters[name.c_str()] = value;
        }
        for (const auto& [name, value] : documentEvent.GetBooleanParameters())
        {
            parameters[name.c_str()] = value;
        }

        const bool propagated = target->DispatchEvent(documentEvent.m_eventType.c_str(), parameters);
        AZLOG(AtomRml, "Dispatched Rml event '%s' to entity '%s' element '%s'",
            documentEvent.m_eventType.c_str(), GetEntityId().ToString().c_str(),
            documentEvent.m_targetElementId.empty() ? "<document>" : documentEvent.m_targetElementId.c_str());
        return propagated;
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
            AZLOG_ERROR("Rml Document Asset Ref component could not find the primary RmlUi context");
            return;
        }

        if (m_loadRequested)
        {
            TryLoadDocument();
        }
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
        AZLOG_ERROR("Failed to load RML document asset: %s", asset.GetHint().c_str());
    }

    void AtomRmlDocumentComponent::TryLoadDocument()
    {
        if (!m_loadRequested || m_document || !m_context || !m_documentAsset.IsReady())
        {
            return;
        }

        const AZStd::string& contents = m_documentAsset->GetContents();
        m_document = m_context->LoadDocumentFromMemory(contents.c_str(), m_documentAsset.GetHint().c_str());
        if (!m_document)
        {
            AZLOG_ERROR("RmlUi failed to parse document asset: %s", m_documentAsset.GetHint().c_str());
            return;
        }

        if (auto* actionRouter = AZ::Interface<AtomRmlActionRouterInterface>::Get())
        {
            actionRouter->RegisterDocument(m_document, GetEntityId());
        }
        else
        {
            AZLOG_WARN("Rml document actions cannot be routed because the action router is unavailable");
        }

        m_document->Show();
    }

    void AtomRmlDocumentComponent::CloseDocument()
    {
        if (m_document)
        {
            if (auto* actionRouter = AZ::Interface<AtomRmlActionRouterInterface>::Get())
            {
                actionRouter->UnregisterDocument(m_document);
            }
            m_document->Close();
            m_document = nullptr;
        }
    }
} // namespace AtomRml
