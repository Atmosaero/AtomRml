/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AtomRmlModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <AtomRml/AtomRmlTypeIds.h>

#include <Clients/AtomRmlSystemComponent.h>

namespace AtomRml
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(AtomRmlModuleInterface,
        "AtomRmlModuleInterface", AtomRmlModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(AtomRmlModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(AtomRmlModuleInterface, AZ::SystemAllocator);

    AtomRmlModuleInterface::AtomRmlModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            AtomRmlSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList AtomRmlModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AtomRmlSystemComponent>(),
        };
    }
} // namespace AtomRml
