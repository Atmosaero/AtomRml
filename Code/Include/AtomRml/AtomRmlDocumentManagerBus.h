/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AtomRml
{
    //! Finds or loads RmlUi documents independently of a level-authored asset reference.
    class AtomRmlDocumentManagerRequests : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Returns the entity owning a loaded document at pathname. When requested, creates a runtime entity to load it.
        virtual AZ::EntityId FindLoadedDocumentByPathName(
            const AZStd::string& documentPathname, bool loadIfNotFound = false) = 0;

        //! Returns the entity owning a loaded document asset. When requested, creates a runtime entity to load it.
        virtual AZ::EntityId FindLoadedDocument(
            const AZ::Data::AssetId& documentAssetId, bool loadIfNotFound = false) = 0;
    };

    using AtomRmlDocumentManagerBus = AZ::EBus<AtomRmlDocumentManagerRequests>;
} // namespace AtomRml
