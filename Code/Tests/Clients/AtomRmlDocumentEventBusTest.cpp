/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomRml/AtomRmlDocumentComponent.h>
#include <AtomRml/AtomRmlDocumentEventBus.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzTest/AzTest.h>

namespace AtomRml
{
    static_assert(AZStd::is_same_v<AtomRmlDocumentEventBus::BusIdType, AZ::EntityId>);

    class DocumentEventBusHandler final
        : public AtomRmlDocumentEventBus::Handler
    {
    public:
        explicit DocumentEventBusHandler(AZ::EntityId entityId)
        {
            BusConnect(entityId);
        }

        ~DocumentEventBusHandler() override
        {
            BusDisconnect();
        }

        bool DispatchEvent(const AtomRmlDocumentEvent& documentEvent) override
        {
            m_event = documentEvent;
            ++m_dispatchCalls;
            return m_result;
        }

        AtomRmlDocumentEvent m_event;
        int m_dispatchCalls = 0;
        bool m_result = true;
    };

    TEST(AtomRmlDocumentEventTests, ParametersKeepTheirRmlVariantTypes)
    {
        AtomRmlDocumentEvent documentEvent;
        documentEvent.SetStringParameter("value", "ready");
        documentEvent.SetNumberParameter("progress", 0.75f);
        documentEvent.SetBooleanParameter("enabled", true);

        EXPECT_EQ(documentEvent.GetStringParameters().at("value"), "ready");
        EXPECT_FLOAT_EQ(documentEvent.GetNumberParameters().at("progress"), 0.75f);
        EXPECT_TRUE(documentEvent.GetBooleanParameters().at("enabled"));

        documentEvent.SetNumberParameter("value", 42.0f);
        EXPECT_FALSE(documentEvent.GetStringParameters().contains("value"));
        EXPECT_FLOAT_EQ(documentEvent.GetNumberParameters().at("value"), 42.0f);
    }

    TEST(AtomRmlDocumentEventBusTests, DispatchRoutesOnlyToTheAddressedEntityAndReturnsTheResult)
    {
        const AZ::EntityId firstEntityId(101);
        const AZ::EntityId secondEntityId(202);
        DocumentEventBusHandler firstHandler(firstEntityId);
        DocumentEventBusHandler secondHandler(secondEntityId);

        AtomRmlDocumentEvent documentEvent;
        documentEvent.m_eventType = "gamestatechanged";
        documentEvent.m_targetElementId = "status";
        documentEvent.SetStringParameter("message", "Ready");

        bool result = false;
        AtomRmlDocumentEventBus::EventResult(
            result, firstEntityId, &AtomRmlDocumentEventBus::Events::DispatchEvent, documentEvent);

        EXPECT_TRUE(result);
        EXPECT_EQ(firstHandler.m_dispatchCalls, 1);
        EXPECT_EQ(firstHandler.m_event.m_eventType, "gamestatechanged");
        EXPECT_EQ(firstHandler.m_event.m_targetElementId, "status");
        EXPECT_EQ(firstHandler.m_event.GetStringParameters().at("message"), "Ready");
        EXPECT_EQ(secondHandler.m_dispatchCalls, 0);
    }

    TEST(AtomRmlDocumentEventBusTests, BehaviorContextExposesLuaPayloadAndDispatchBus)
    {
        AZ::BehaviorContext behaviorContext;
        AtomRmlDocumentComponent::Reflect(&behaviorContext);

        const auto classIterator = behaviorContext.m_classes.find("AtomRmlDocumentEvent");
        ASSERT_NE(classIterator, behaviorContext.m_classes.end());
        EXPECT_NE(classIterator->second->m_methods.find("SetStringParameter"), classIterator->second->m_methods.end());
        EXPECT_NE(classIterator->second->m_methods.find("SetNumberParameter"), classIterator->second->m_methods.end());
        EXPECT_NE(classIterator->second->m_methods.find("SetBooleanParameter"), classIterator->second->m_methods.end());

        const auto busIterator = behaviorContext.m_ebuses.find("AtomRmlDocumentEventBus");
        ASSERT_NE(busIterator, behaviorContext.m_ebuses.end());
        EXPECT_NE(busIterator->second->m_events.find("DispatchEvent"), busIterator->second->m_events.end());
        EXPECT_EQ(busIterator->second->m_events.size(), 1);
    }
} // namespace AtomRml
