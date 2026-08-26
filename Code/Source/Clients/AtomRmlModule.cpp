/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/RTTI/RTTI.h>
#include <AtomRml/AtomRmlTypeIds.h>
#include <AtomRml/AtomRmlDocumentComponent.h>
#include <AtomRmlModuleInterface.h>
#include "AtomRmlSystemComponent.h"

namespace AtomRml
{
    class AtomRmlModule
        : public AtomRmlModuleInterface
    {
    public:
        AZ_RTTI(AtomRmlModule, AtomRmlModuleTypeId, AtomRmlModuleInterface);
        AZ_CLASS_ALLOCATOR(AtomRmlModule, AZ::SystemAllocator);

        AtomRmlModule()
        {
            m_descriptors.insert(m_descriptors.end(),
                {
                    AtomRmlSystemComponent::CreateDescriptor(),
                    AtomRmlDocumentComponent::CreateDescriptor()
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const
        {
            return AZ::ComponentTypeList{ azrtti_typeid<AtomRmlSystemComponent>() };
        }
    };
}// namespace AtomRml

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AtomRml::AtomRmlModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AtomRml, AtomRml::AtomRmlModule)
#endif
