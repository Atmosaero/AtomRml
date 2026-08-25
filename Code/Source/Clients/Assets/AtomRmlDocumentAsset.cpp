/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 Atmosaero
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */

#include <AtomRml/AtomRmlDocumentAsset.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AtomRml
{
    AtomRmlDocumentAsset::AtomRmlDocumentAsset(const AZ::Data::AssetId& assetId, AssetStatus status)
        : AZ::Data::AssetData(assetId, status)
    {
    }

    void AtomRmlDocumentAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AtomRmlDocumentAsset, AZ::Data::AssetData>()->Version(1);
        }
    }

    const AZStd::string& AtomRmlDocumentAsset::GetContents() const
    {
        return m_contents;
    }
} // namespace AtomRml
