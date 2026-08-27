/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRmlDocumentManager.h"

#include <AtomRml/AtomRmlDocumentAsset.h>
#include <AtomRml/AtomRmlDocumentComponent.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/algorithm.h>

namespace AtomRml
{
    AtomRmlDocumentManager::~AtomRmlDocumentManager()
    {
        Deactivate();
    }

    void AtomRmlDocumentManager::Reflect(AZ::ReflectContext* context)
    {
        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AtomRmlDocumentManagerBus>("AtomRmlDocumentManagerBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "AtomRml")
                ->Attribute(AZ::Script::Attributes::Module, "atomrml")
                ->Event(
                    "FindLoadedDocumentByPathName",
                    &AtomRmlDocumentManagerBus::Events::FindLoadedDocumentByPathName)
                ->Event("FindLoadedDocument", &AtomRmlDocumentManagerBus::Events::FindLoadedDocument);
        }
    }

    void AtomRmlDocumentManager::Activate()
    {
        if (m_isActive)
        {
            return;
        }

        AZ_Assert(
            AZ::Interface<AtomRmlDocumentManagerInterface>::Get() == nullptr,
            "AtomRmlDocumentManagerInterface is already registered");
        AZ::Interface<AtomRmlDocumentManagerInterface>::Register(this);
        AtomRmlDocumentManagerBus::Handler::BusConnect();
        m_isActive = true;
    }

    void AtomRmlDocumentManager::Deactivate()
    {
        if (!m_isActive)
        {
            return;
        }

        AtomRmlDocumentManagerBus::Handler::BusDisconnect();

        for (const AZStd::unique_ptr<AZ::Entity>& entity : m_ownedDocumentEntities)
        {
            if (entity && entity->GetState() == AZ::Entity::State::Active)
            {
                entity->Deactivate();
            }
        }
        m_ownedDocumentEntities.clear();
        m_components.clear();

        if (AZ::Interface<AtomRmlDocumentManagerInterface>::Get() == this)
        {
            AZ::Interface<AtomRmlDocumentManagerInterface>::Unregister(this);
        }
        m_isActive = false;
    }

    AZ::EntityId AtomRmlDocumentManager::FindLoadedDocumentByPathName(
        const AZStd::string& documentPathname, bool loadIfNotFound)
    {
        const AZ::Data::AssetId assetId = ResolveDocumentAssetId(documentPathname);
        if (!assetId.IsValid())
        {
            AZLOG_WARN("Unable to find RML document asset '%s'", documentPathname.c_str());
            return AZ::EntityId();
        }

        return FindLoadedDocument(assetId, loadIfNotFound);
    }

    AZ::EntityId AtomRmlDocumentManager::FindLoadedDocument(
        const AZ::Data::AssetId& documentAssetId, bool loadIfNotFound)
    {
        if (!documentAssetId.IsValid())
        {
            AZLOG_WARN("Unable to find RML document because the supplied AssetId is invalid");
            return AZ::EntityId();
        }

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            assetInfo,
            &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById,
            documentAssetId);
        if (assetInfo.m_assetType != azrtti_typeid<AtomRmlDocumentAsset>())
        {
            AZLOG_WARN(
                "Asset '%s' is not an AtomRml document asset",
                documentAssetId.ToString<AZStd::string>().c_str());
            return AZ::EntityId();
        }

        AtomRmlDocumentComponent* assignedComponent = nullptr;
        for (const auto& [entityId, component] : m_components)
        {
            if (!component || component->m_documentAsset.GetId() != documentAssetId)
            {
                continue;
            }

            if (component->m_document)
            {
                return entityId;
            }
            assignedComponent = component;
        }

        if (!loadIfNotFound)
        {
            return AZ::EntityId();
        }

        if (assignedComponent)
        {
            assignedComponent->Show();
            return assignedComponent->GetEntityId();
        }

        auto entity = AZStd::make_unique<AZ::Entity>("AtomRml runtime document");
        AtomRmlDocumentComponent* component = entity->CreateComponent<AtomRmlDocumentComponent>();
        if (!component)
        {
            AZLOG_ERROR("Unable to create a runtime Rml Document Asset Ref component");
            return AZ::EntityId();
        }

        component->SetPath(documentAssetId);
        entity->Init();
        entity->Activate();

        const AZ::EntityId entityId = entity->GetId();
        m_ownedDocumentEntities.push_back(AZStd::move(entity));
        return entityId;
    }

    void AtomRmlDocumentManager::RegisterComponent(AtomRmlDocumentComponent* component)
    {
        if (component && component->GetEntityId().IsValid())
        {
            m_components.insert_or_assign(component->GetEntityId(), component);
        }
    }

    void AtomRmlDocumentManager::UnregisterComponent(AtomRmlDocumentComponent* component)
    {
        if (component)
        {
            m_components.erase(component->GetEntityId());
        }
    }

    AZ::Data::AssetId AtomRmlDocumentManager::ResolveDocumentAssetId(const AZStd::string& documentPathname) const
    {
        AZStd::string assetPath = documentPathname;
        AZStd::replace(assetPath.begin(), assetPath.end(), '\\', '/');
        while (AZ::StringFunc::StartsWith(assetPath, "./"))
        {
            assetPath.erase(0, 2);
        }
        if (AZ::StringFunc::StartsWith(assetPath, "Assets/", false))
        {
            assetPath.erase(0, 7);
        }

        AZ::Data::AssetId assetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            assetId,
            &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
            assetPath.c_str(),
            azrtti_typeid<AtomRmlDocumentAsset>(),
            false);
        return assetId;
    }
} // namespace AtomRml
