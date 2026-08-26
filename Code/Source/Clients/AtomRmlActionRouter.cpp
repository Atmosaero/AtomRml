/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRmlActionRouter.h"

#include <AzCore/Console/ILogger.h>

#include <unordered_set>

#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>

namespace AtomRml
{
    namespace
    {
        constexpr AZ::u32 CtrlModifier = 1u << 0;
        constexpr AZ::u32 ShiftModifier = 1u << 1;
        constexpr AZ::u32 AltModifier = 1u << 2;
        constexpr AZ::u32 MetaModifier = 1u << 3;
        constexpr AZ::u32 CapsLockModifier = 1u << 4;
        constexpr AZ::u32 NumLockModifier = 1u << 5;
        constexpr AZ::u32 ScrollLockModifier = 1u << 6;

        const std::unordered_set<Rml::String> StandardParameterNames = {
            "mouse_x",         "mouse_y", "button",   "key_identifier", "ctrl_key",
            "shift_key",       "alt_key", "meta_key", "caps_lock_key",  "num_lock_key",
            "scroll_lock_key", "value",   "checked",  "drag_element",   "data-binding-override-value"
        };

        const Rml::Variant* FindParameter(const Rml::Dictionary& parameters, const Rml::String& name)
        {
            const auto parameterIterator = parameters.find(name);
            return parameterIterator != parameters.end() ? &parameterIterator->second : nullptr;
        }

        bool VariantToString(const Rml::Variant& value, AZStd::string& result)
        {
            Rml::String rmlValue;
            if (!value.GetInto(rmlValue))
            {
                return false;
            }

            result.assign(rmlValue.begin(), rmlValue.end());
            return true;
        }

        const Rml::Element* GetActionElement(const Rml::Event& event)
        {
            return event.GetCurrentElement() ? event.GetCurrentElement() : event.GetTargetElement();
        }

        const Rml::ElementDocument* GetOwnerDocument(const Rml::Element* element)
        {
            if (!element)
            {
                return nullptr;
            }

            if (const auto* document = rmlui_dynamic_cast<const Rml::ElementDocument*>(element))
            {
                return document;
            }

            return element->GetOwnerDocument();
        }
    } // namespace

    AtomRmlActionEvent BuildActionEvent(const Rml::String& actionName, Rml::Event& event)
    {
        AtomRmlActionEvent actionEvent;
        actionEvent.m_actionName.assign(actionName.begin(), actionName.end());
        actionEvent.m_eventType.assign(event.GetType().begin(), event.GetType().end());

        const Rml::Element* actionElement = GetActionElement(event);
        const Rml::Element* targetElement = event.GetTargetElement();
        const Rml::ElementDocument* document = GetOwnerDocument(actionElement);

        if (document)
        {
            actionEvent.m_documentPath.assign(document->GetSourceURL().begin(), document->GetSourceURL().end());
        }

        if (actionElement)
        {
            actionEvent.m_elementId.assign(actionElement->GetId().begin(), actionElement->GetId().end());
            actionEvent.m_elementTag.assign(actionElement->GetTagName().begin(), actionElement->GetTagName().end());
            const Rml::String classes = actionElement->GetClassNames();
            actionEvent.m_elementClasses.assign(classes.begin(), classes.end());

            for (const auto& [attributeName, attributeValue] : actionElement->GetAttributes())
            {
                if (attributeName.size() <= 5 || attributeName.compare(0, 5, "data-") != 0)
                {
                    continue;
                }

                AZStd::string value;
                if (VariantToString(attributeValue, value))
                {
                    actionEvent.SetArgument(AZStd::string(attributeName.begin() + 5, attributeName.end()), AZStd::move(value));
                }
            }
        }

        if (targetElement)
        {
            actionEvent.m_targetElementId.assign(targetElement->GetId().begin(), targetElement->GetId().end());
            actionEvent.m_targetElementTag.assign(targetElement->GetTagName().begin(), targetElement->GetTagName().end());
        }

        const Rml::Dictionary& parameters = event.GetParameters();
        const Rml::Variant* mouseX = FindParameter(parameters, "mouse_x");
        const Rml::Variant* mouseY = FindParameter(parameters, "mouse_y");
        if (mouseX && mouseY)
        {
            actionEvent.m_pointerPosition.Set(mouseX->Get<float>(0.0f), mouseY->Get<float>(0.0f));
            actionEvent.m_hasPointerPosition = true;
        }

        if (const Rml::Variant* button = FindParameter(parameters, "button"))
        {
            actionEvent.m_mouseButton = button->Get<int>(-1);
            actionEvent.m_hasMouseButton = true;
        }

        if (const Rml::Variant* keyIdentifier = FindParameter(parameters, "key_identifier"))
        {
            actionEvent.m_keyIdentifier = keyIdentifier->Get<int>(0);
            actionEvent.m_hasKeyIdentifier = true;
        }

        const auto addModifier = [&parameters, &actionEvent](const char* name, AZ::u32 modifier)
        {
            if (const Rml::Variant* value = FindParameter(parameters, name); value && value->Get<int>(0) != 0)
            {
                actionEvent.m_keyModifiers |= modifier;
            }
        };
        addModifier("ctrl_key", CtrlModifier);
        addModifier("shift_key", ShiftModifier);
        addModifier("alt_key", AltModifier);
        addModifier("meta_key", MetaModifier);
        addModifier("caps_lock_key", CapsLockModifier);
        addModifier("num_lock_key", NumLockModifier);
        addModifier("scroll_lock_key", ScrollLockModifier);

        if (const Rml::Variant* value = FindParameter(parameters, "value"))
        {
            actionEvent.m_hasValue = VariantToString(*value, actionEvent.m_value);
        }
        else if (actionElement && actionElement->HasAttribute("value"))
        {
            const Rml::String valueAttribute = actionElement->GetAttribute<Rml::String>("value", "");
            actionEvent.m_value.assign(valueAttribute.begin(), valueAttribute.end());
            actionEvent.m_hasValue = true;
        }

        if (const Rml::Variant* checked = FindParameter(parameters, "checked"))
        {
            actionEvent.m_checked = checked->Get<bool>(false);
            actionEvent.m_hasChecked = true;
        }

        for (const auto& [parameterName, parameterValue] : parameters)
        {
            if (StandardParameterNames.contains(parameterName))
            {
                continue;
            }

            AZStd::string value;
            if (VariantToString(parameterValue, value))
            {
                actionEvent.SetArgument(AZStd::string(parameterName.begin(), parameterName.end()), AZStd::move(value));
            }
        }

        return actionEvent;
    }

    void AtomRmlActionRouter::RegisterDocument(const Rml::ElementDocument* document, AZ::EntityId entityId)
    {
        if (document && entityId.IsValid())
        {
            m_documentEntities.insert_or_assign(document, entityId);
        }
    }

    void AtomRmlActionRouter::UnregisterDocument(const Rml::ElementDocument* document)
    {
        m_documentEntities.erase(document);
    }

    bool AtomRmlActionRouter::DispatchAction(const Rml::String& actionName, Rml::Event& event)
    {
        const Rml::ElementDocument* document = GetOwnerDocument(GetActionElement(event));
        return DispatchAction(document, BuildActionEvent(actionName, event));
    }

    bool AtomRmlActionRouter::DispatchAction(const Rml::ElementDocument* document, AtomRmlActionEvent actionEvent)
    {
        const auto documentIterator = m_documentEntities.find(document);
        if (documentIterator == m_documentEntities.end())
        {
            AZLOG_WARN("Rml action '%s' has no owning Rml Document Asset Ref entity", actionEvent.m_actionName.c_str());
            return false;
        }

        AZLOG_INFO(
            "Rml action '%s' received from event '%s' on entity '%s' element '%s'",
            actionEvent.m_actionName.c_str(),
            actionEvent.m_eventType.c_str(),
            documentIterator->second.ToString().c_str(),
            actionEvent.m_elementId.c_str());

        AtomRmlActionNotificationBus::Event(documentIterator->second, &AtomRmlActionNotificationBus::Events::OnAction, actionEvent);
        return true;
    }

    void AtomRmlActionRouter::Clear()
    {
        m_documentEntities.clear();
    }
} // namespace AtomRml
