/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

namespace AtomRml
{
    class AtomRmlDocumentAssetHandler final
        : public AZ::Data::AssetHandler
        , private AZ::AssetTypeInfoBus::Handler
    {
    public:
        AZ_RTTI(AtomRmlDocumentAssetHandler, "{736EC474-094D-4481-88F3-9506F80D5C7D}", AZ::Data::AssetHandler);
        AZ_CLASS_ALLOCATOR(AtomRmlDocumentAssetHandler, AZ::SystemAllocator);

        ~AtomRmlDocumentAssetHandler() override;

        void Register();
        void Unregister();

        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
        LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        void DestroyAsset(AZ::Data::AssetPtr asset) override;
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;

    private:
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;
        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;
        AZ::Uuid GetComponentTypeId() const override;
        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;

        bool m_registered = false;
    };
} // namespace AtomRml
