/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentBus.h>

namespace AtomRml
{
    //! Controls the Rml Document Asset Ref component on an entity.
    class AtomRmlDocumentAssetRefRequests
        : public AZ::ComponentBus
    {
    public:
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = AZ::EntityId;

        //! Assigns a typed AtomRml document product by asset id.
        virtual void SetPath(const AZ::Data::AssetId& assetId) = 0;
        //! Closes the current document and cancels its pending load without clearing the assigned asset.
        virtual void Remove() = 0;
        //! Shows the current document, loading the assigned asset first when necessary.
        virtual void Show() = 0;
    };

    using AtomRmlDocumentAssetRefBus = AZ::EBus<AtomRmlDocumentAssetRefRequests>;
} // namespace AtomRml
