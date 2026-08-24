/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 Reece Hagan
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
#include <AtomRml/AtomRmlTypeIds.h>
#include <AtomRmlModuleInterface.h>
#include "AtomRmlEditorSystemComponent.h"

namespace AtomRml
{
    class AtomRmlEditorModule
        : public AtomRmlModuleInterface
    {
    public:
        AZ_RTTI(AtomRmlEditorModule, AtomRmlEditorModuleTypeId, AtomRmlModuleInterface);
        AZ_CLASS_ALLOCATOR(AtomRmlEditorModule, AZ::SystemAllocator);

        AtomRmlEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                AtomRmlEditorSystemComponent::CreateDescriptor()
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<AtomRmlEditorSystemComponent>(),
            };
        }
    };
}// namespace AtomRml

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), AtomRml::AtomRmlEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AtomRml_Editor, AtomRml::AtomRmlEditorModule)
#endif
