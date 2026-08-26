/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>

namespace AtomRml
{
    class AtomRmlDocumentAssetHandler;

    //! Runtime representation of a processed RmlUi document (.rml).
    class AtomRmlDocumentAsset final : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(AtomRmlDocumentAsset, "{4C3FAD7A-BED7-49AD-BBDE-CAECC663962C}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(AtomRmlDocumentAsset, AZ::SystemAllocator);

        explicit AtomRmlDocumentAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AssetStatus status = AssetStatus::NotLoaded);

        static void Reflect(AZ::ReflectContext* context);

        const AZStd::string& GetContents() const;

    private:
        friend class AtomRmlDocumentAssetHandler;

        AZStd::string m_contents;
    };
} // namespace AtomRml
