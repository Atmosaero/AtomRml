/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRmlDocumentAssetHandler.h"

#include <AzCore/Console/ILogger.h>

#include <AtomRml/AtomRmlDocumentAsset.h>
#include <AtomRml/AtomRmlDocumentComponent.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Debug/Trace.h>

namespace AtomRml
{
    AtomRmlDocumentAssetHandler::~AtomRmlDocumentAssetHandler()
    {
        Unregister();
    }

    void AtomRmlDocumentAssetHandler::Register()
    {
        if (m_registered)
        {
            return;
        }

        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager is not ready");

        const AZ::Data::AssetType assetType = GetAssetType();
        AZ::Data::AssetManager::Instance().RegisterHandler(this, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, "rml");
        AZ::AssetTypeInfoBus::Handler::BusConnect(assetType);
        m_registered = true;
    }

    void AtomRmlDocumentAssetHandler::Unregister()
    {
        if (!m_registered)
        {
            return;
        }

        AZ::AssetTypeInfoBus::Handler::BusDisconnect();
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
        m_registered = false;
    }

    AZ::Data::AssetPtr AtomRmlDocumentAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        AZ_Assert(type == GetAssetType(), "AtomRmlDocumentAssetHandler received an unsupported asset type");
        return aznew AtomRmlDocumentAsset(id);
    }

    AZ::Data::AssetHandler::LoadResult AtomRmlDocumentAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        auto* documentAsset = asset.GetAs<AtomRmlDocumentAsset>();
        if (!documentAsset || !stream)
        {
            return LoadResult::Error;
        }

        const size_t byteCount = stream->GetLength();
        documentAsset->m_contents.resize(byteCount);
        if (byteCount != 0 && stream->Read(byteCount, documentAsset->m_contents.data()) != byteCount)
        {
            AZLOG_ERROR("Failed to read RML document asset: %s", asset.GetHint().c_str());
            documentAsset->m_contents.clear();
            return LoadResult::Error;
        }

        return LoadResult::LoadComplete;
    }

    void AtomRmlDocumentAssetHandler::DestroyAsset(AZ::Data::AssetPtr asset)
    {
        delete asset;
    }

    void AtomRmlDocumentAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(GetAssetType());
    }

    AZ::Data::AssetType AtomRmlDocumentAssetHandler::GetAssetType() const
    {
        return azrtti_typeid<AtomRmlDocumentAsset>();
    }

    const char* AtomRmlDocumentAssetHandler::GetAssetTypeDisplayName() const
    {
        return "RmlUi Document";
    }

    const char* AtomRmlDocumentAssetHandler::GetGroup() const
    {
        return "UI";
    }

    const char* AtomRmlDocumentAssetHandler::GetBrowserIcon() const
    {
        return "Icons/AssetBrowser/Default_16.svg";
    }

    AZ::Uuid AtomRmlDocumentAssetHandler::GetComponentTypeId() const
    {
        return azrtti_typeid<AtomRmlDocumentComponent>();
    }

    void AtomRmlDocumentAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        extensions.emplace_back("rml");
    }
} // namespace AtomRml
