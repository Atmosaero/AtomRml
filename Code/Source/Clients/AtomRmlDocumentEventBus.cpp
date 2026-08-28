/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomRml/AtomRmlDocumentEventBus.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>

namespace AtomRml
{
    void AtomRmlDocumentEvent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AtomRmlDocumentEvent>("AtomRmlDocumentEvent")
                ->Attribute(AZ::Script::Attributes::Category, "AtomRml")
                ->Attribute(AZ::Script::Attributes::Module, "atomrml")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constructor<>()
                ->Property("EventType", BehaviorValueProperty(&AtomRmlDocumentEvent::m_eventType))
                ->Property("TargetElementId", BehaviorValueProperty(&AtomRmlDocumentEvent::m_targetElementId))
                ->Method("SetStringParameter", &AtomRmlDocumentEvent::SetStringParameter)
                ->Method("SetNumberParameter", &AtomRmlDocumentEvent::SetNumberParameter)
                ->Method("SetBooleanParameter", &AtomRmlDocumentEvent::SetBooleanParameter)
                ->Method("ClearParameters", &AtomRmlDocumentEvent::ClearParameters);
        }
    }

    void AtomRmlDocumentEvent::SetStringParameter(AZStd::string name, AZStd::string value)
    {
        EraseParameter(name);
        m_stringParameters.emplace(AZStd::move(name), AZStd::move(value));
    }

    void AtomRmlDocumentEvent::SetNumberParameter(AZStd::string name, float value)
    {
        EraseParameter(name);
        m_numberParameters.emplace(AZStd::move(name), value);
    }

    void AtomRmlDocumentEvent::SetBooleanParameter(AZStd::string name, bool value)
    {
        EraseParameter(name);
        m_booleanParameters.emplace(AZStd::move(name), value);
    }

    void AtomRmlDocumentEvent::ClearParameters()
    {
        m_stringParameters.clear();
        m_numberParameters.clear();
        m_booleanParameters.clear();
    }

    const AZStd::unordered_map<AZStd::string, AZStd::string>& AtomRmlDocumentEvent::GetStringParameters() const
    {
        return m_stringParameters;
    }

    const AZStd::unordered_map<AZStd::string, float>& AtomRmlDocumentEvent::GetNumberParameters() const
    {
        return m_numberParameters;
    }

    const AZStd::unordered_map<AZStd::string, bool>& AtomRmlDocumentEvent::GetBooleanParameters() const
    {
        return m_booleanParameters;
    }

    void AtomRmlDocumentEvent::EraseParameter(const AZStd::string& name)
    {
        m_stringParameters.erase(name);
        m_numberParameters.erase(name);
        m_booleanParameters.erase(name);
    }
} // namespace AtomRml
