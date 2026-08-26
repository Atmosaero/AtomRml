/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomRml/AtomRmlActionBus.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>

namespace AtomRml
{
    void AtomRmlActionEvent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AtomRmlActionEvent>("AtomRmlActionEvent")
                ->Attribute(AZ::Script::Attributes::Category, "AtomRml")
                ->Attribute(AZ::Script::Attributes::Module, "atomrml")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("ActionName", BehaviorValueProperty(&AtomRmlActionEvent::m_actionName))
                ->Property("EventType", BehaviorValueProperty(&AtomRmlActionEvent::m_eventType))
                ->Property("DocumentPath", BehaviorValueProperty(&AtomRmlActionEvent::m_documentPath))
                ->Property("ElementId", BehaviorValueProperty(&AtomRmlActionEvent::m_elementId))
                ->Property("ElementTag", BehaviorValueProperty(&AtomRmlActionEvent::m_elementTag))
                ->Property("ElementClasses", BehaviorValueProperty(&AtomRmlActionEvent::m_elementClasses))
                ->Property("TargetElementId", BehaviorValueProperty(&AtomRmlActionEvent::m_targetElementId))
                ->Property("TargetElementTag", BehaviorValueProperty(&AtomRmlActionEvent::m_targetElementTag))
                ->Property("Value", BehaviorValueProperty(&AtomRmlActionEvent::m_value))
                ->Property("HasValue", BehaviorValueProperty(&AtomRmlActionEvent::m_hasValue))
                ->Property("Checked", BehaviorValueProperty(&AtomRmlActionEvent::m_checked))
                ->Property("HasChecked", BehaviorValueProperty(&AtomRmlActionEvent::m_hasChecked))
                ->Property("PointerPosition", BehaviorValueProperty(&AtomRmlActionEvent::m_pointerPosition))
                ->Property("HasPointerPosition", BehaviorValueProperty(&AtomRmlActionEvent::m_hasPointerPosition))
                ->Property("MouseButton", BehaviorValueProperty(&AtomRmlActionEvent::m_mouseButton))
                ->Property("HasMouseButton", BehaviorValueProperty(&AtomRmlActionEvent::m_hasMouseButton))
                ->Property("KeyIdentifier", BehaviorValueProperty(&AtomRmlActionEvent::m_keyIdentifier))
                ->Property("HasKeyIdentifier", BehaviorValueProperty(&AtomRmlActionEvent::m_hasKeyIdentifier))
                ->Property("KeyModifiers", BehaviorValueProperty(&AtomRmlActionEvent::m_keyModifiers))
                ->Method("HasArgument", &AtomRmlActionEvent::HasArgument)
                ->Method("GetArgument", &AtomRmlActionEvent::GetArgument);
        }
    }

    bool AtomRmlActionEvent::HasArgument(const AZStd::string& name) const
    {
        return m_arguments.contains(name);
    }

    AZStd::string AtomRmlActionEvent::GetArgument(const AZStd::string& name) const
    {
        const auto argumentIterator = m_arguments.find(name);
        return argumentIterator != m_arguments.end() ? argumentIterator->second : AZStd::string{};
    }

    void AtomRmlActionEvent::SetArgument(AZStd::string name, AZStd::string value)
    {
        m_arguments.insert_or_assign(AZStd::move(name), AZStd::move(value));
    }

    const AZStd::unordered_map<AZStd::string, AZStd::string>& AtomRmlActionEvent::GetArguments() const
    {
        return m_arguments;
    }
} // namespace AtomRml
