/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace AtomRml
{
    //! Stable O3DE representation of an inline RmlUi action event.
    class AtomRmlActionEvent
    {
    public:
        AZ_TYPE_INFO(AtomRmlActionEvent, "{AA270432-7455-4070-854C-7238049026AD}");
        AZ_CLASS_ALLOCATOR(AtomRmlActionEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        bool HasArgument(const AZStd::string& name) const;
        AZStd::string GetArgument(const AZStd::string& name) const;
        void SetArgument(AZStd::string name, AZStd::string value);

        const AZStd::unordered_map<AZStd::string, AZStd::string>& GetArguments() const;

        AZStd::string m_actionName;
        AZStd::string m_eventType;
        AZStd::string m_documentPath;

        AZStd::string m_elementId;
        AZStd::string m_elementTag;
        AZStd::string m_elementClasses;
        AZStd::string m_targetElementId;
        AZStd::string m_targetElementTag;

        AZStd::string m_value;
        bool m_hasValue = false;
        bool m_checked = false;
        bool m_hasChecked = false;

        AZ::Vector2 m_pointerPosition = AZ::Vector2::CreateZero();
        bool m_hasPointerPosition = false;
        int m_mouseButton = -1;
        bool m_hasMouseButton = false;

        int m_keyIdentifier = 0;
        bool m_hasKeyIdentifier = false;
        AZ::u32 m_keyModifiers = 0;

    private:
        AZStd::unordered_map<AZStd::string, AZStd::string> m_arguments;
    };

    //! Receives actions from the RML document owned by the addressed entity.
    class AtomRmlActionNotifications : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = AZ::EntityId;

        virtual void OnAction(const AtomRmlActionEvent& actionEvent) = 0;
    };

    using AtomRmlActionNotificationBus = AZ::EBus<AtomRmlActionNotifications>;
} // namespace AtomRml
