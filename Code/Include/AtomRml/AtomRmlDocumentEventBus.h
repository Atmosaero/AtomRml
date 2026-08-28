/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace AtomRml
{
    //! A custom DOM event sent to a loaded RmlUi document.
    class AtomRmlDocumentEvent
    {
    public:
        AZ_TYPE_INFO(AtomRmlDocumentEvent, "{6E5F8962-57D8-4C61-9BA4-41551F408EDB}");
        AZ_CLASS_ALLOCATOR(AtomRmlDocumentEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        void SetStringParameter(AZStd::string name, AZStd::string value);
        void SetNumberParameter(AZStd::string name, float value);
        void SetBooleanParameter(AZStd::string name, bool value);
        void ClearParameters();

        const AZStd::unordered_map<AZStd::string, AZStd::string>& GetStringParameters() const;
        const AZStd::unordered_map<AZStd::string, float>& GetNumberParameters() const;
        const AZStd::unordered_map<AZStd::string, bool>& GetBooleanParameters() const;

        AZStd::string m_eventType;
        //! Empty targets the document itself. Otherwise, this is resolved with GetElementById.
        AZStd::string m_targetElementId;

    private:
        void EraseParameter(const AZStd::string& name);

        AZStd::unordered_map<AZStd::string, AZStd::string> m_stringParameters;
        AZStd::unordered_map<AZStd::string, float> m_numberParameters;
        AZStd::unordered_map<AZStd::string, bool> m_booleanParameters;
    };

    //! Dispatches custom RmlUi DOM events to the document owned by the addressed entity.
    class AtomRmlDocumentEventRequests
        : public AZ::ComponentBus
    {
    public:
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = AZ::EntityId;

        //! Returns true when the event reached the DOM and was not stopped during propagation.
        virtual bool DispatchEvent(const AtomRmlDocumentEvent& documentEvent) = 0;
    };

    using AtomRmlDocumentEventBus = AZ::EBus<AtomRmlDocumentEventRequests>;
} // namespace AtomRml
