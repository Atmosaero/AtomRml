/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <Atom/RPI.Public/Base.h>

namespace AtomRml
{
    class AtomRmlParentPass;

    //! Provides access to the AtomRml pass owned by the active render pipeline for a scene.
    class AtomRmlPassRequests
        : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::RPI::SceneId;

        virtual AtomRmlParentPass* GetParentPass() = 0;
    };

    using AtomRmlPassRequestBus = AZ::EBus<AtomRmlPassRequests>;
} // namespace AtomRml
