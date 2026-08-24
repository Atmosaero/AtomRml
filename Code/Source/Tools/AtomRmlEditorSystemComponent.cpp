/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 Reece Hagan
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
#include <AzCore/Serialization/SerializeContext.h>
#include "AtomRmlEditorSystemComponent.h"

#include <AtomRml/AtomRmlTypeIds.h>

namespace AtomRml
{
    AZ_COMPONENT_IMPL(AtomRmlEditorSystemComponent, "AtomRmlEditorSystemComponent",
        AtomRmlEditorSystemComponentTypeId, BaseSystemComponent);

    void AtomRmlEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AtomRmlEditorSystemComponent, AtomRmlSystemComponent>()
                ->Version(0);
        }
    }

    AtomRmlEditorSystemComponent::AtomRmlEditorSystemComponent() = default;

    AtomRmlEditorSystemComponent::~AtomRmlEditorSystemComponent() = default;

    void AtomRmlEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("AtomRmlSystemEditorService"));
    }

    void AtomRmlEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("AtomRmlSystemEditorService"));
    }

    void AtomRmlEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void AtomRmlEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void AtomRmlEditorSystemComponent::Activate()
    {
        AtomRmlSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void AtomRmlEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AtomRmlSystemComponent::Deactivate();
    }

} // namespace AtomRml
