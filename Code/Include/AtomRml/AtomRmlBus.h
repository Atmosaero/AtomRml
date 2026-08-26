/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomRml/AtomRmlTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace AtomRml
{
    class AtomRmlRenderInterface;

    class AtomRmlRequests
    {
    public:
        AZ_RTTI(AtomRmlRequests, AtomRmlRequestsTypeId);
        virtual ~AtomRmlRequests() = default;
        
        //! Get the global render interface instance
        virtual AtomRmlRenderInterface* GetRenderInterface() = 0;
    };

    class AtomRmlBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using AtomRmlRequestBus = AZ::EBus<AtomRmlRequests, AtomRmlBusTraits>;
    using AtomRmlInterface = AZ::Interface<AtomRmlRequests>;

} // namespace AtomRml
