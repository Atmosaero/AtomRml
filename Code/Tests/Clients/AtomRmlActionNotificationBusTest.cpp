/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomRml/AtomRmlActionBus.h>
#include <AtomRml/AtomRmlDocumentComponent.h>

#include <Clients/AtomRmlActionRouter.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzTest/AzTest.h>

#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/Event.h>

namespace AtomRml
{
    static_assert(AZStd::is_same_v<AtomRmlActionNotificationBus::BusIdType, AZ::EntityId>);

    class ActionNotificationHandler final : public AtomRmlActionNotificationBus::Handler
    {
    public:
        explicit ActionNotificationHandler(AZ::EntityId entityId)
        {
            BusConnect(entityId);
        }

        ~ActionNotificationHandler() override
        {
            BusDisconnect();
        }

        void OnAction(const AtomRmlActionEvent& actionEvent) override
        {
            m_lastEvent = actionEvent;
            ++m_actionCount;
        }

        AtomRmlActionEvent m_lastEvent;
        int m_actionCount = 0;
    };

    TEST(AtomRmlActionNotificationBusTests, RoutesOnlyToTheEntityOwningTheDocument)
    {
        const AZ::EntityId firstEntityId(101);
        const AZ::EntityId secondEntityId(202);
        ActionNotificationHandler firstHandler(firstEntityId);
        ActionNotificationHandler secondHandler(secondEntityId);
        AtomRmlActionRouter router;
        const auto* document = reinterpret_cast<const Rml::ElementDocument*>(1);

        router.RegisterDocument(document, firstEntityId);

        AtomRmlActionEvent actionEvent;
        actionEvent.m_actionName = "StartGame";
        actionEvent.m_eventType = "click";

        EXPECT_TRUE(router.DispatchAction(document, actionEvent));
        EXPECT_EQ(firstHandler.m_actionCount, 1);
        EXPECT_EQ(firstHandler.m_lastEvent.m_actionName, "StartGame");
        EXPECT_EQ(secondHandler.m_actionCount, 0);

        router.UnregisterDocument(document);
        EXPECT_FALSE(router.DispatchAction(document, actionEvent));
        EXPECT_EQ(firstHandler.m_actionCount, 1);
    }

    TEST(AtomRmlActionNotificationBusTests, ConvertsInputAndCustomArguments)
    {
        Rml::Dictionary parameters;
        parameters["mouse_x"] = 320;
        parameters["mouse_y"] = 180;
        parameters["button"] = 0;
        parameters["ctrl_key"] = 1;
        parameters["value"] = "purchase";
        parameters["item-id"] = "health_potion";
        parameters["count"] = 3;
        Rml::Event clickEvent(nullptr, Rml::EventId::Click, "click", parameters, true);

        const AtomRmlActionEvent actionEvent = BuildActionEvent("BuyItem", clickEvent);

        EXPECT_EQ(actionEvent.m_actionName, "BuyItem");
        EXPECT_EQ(actionEvent.m_eventType, "click");
        EXPECT_TRUE(actionEvent.m_elementId.empty());
        EXPECT_TRUE(actionEvent.m_hasPointerPosition);
        EXPECT_EQ(actionEvent.m_pointerPosition, AZ::Vector2(320.0f, 180.0f));
        EXPECT_TRUE(actionEvent.m_hasMouseButton);
        EXPECT_EQ(actionEvent.m_mouseButton, 0);
        EXPECT_EQ(actionEvent.m_keyModifiers, 1u);
        EXPECT_TRUE(actionEvent.m_hasValue);
        EXPECT_EQ(actionEvent.m_value, "purchase");
        EXPECT_EQ(actionEvent.GetArgument("item-id"), "health_potion");
        EXPECT_EQ(actionEvent.GetArgument("count"), "3");
        EXPECT_FALSE(actionEvent.HasArgument("missing"));
    }

    TEST(AtomRmlActionNotificationBusTests, BehaviorContextExposesNotificationBusAndPayload)
    {
        AZ::BehaviorContext behaviorContext;
        AtomRmlDocumentComponent::Reflect(&behaviorContext);

        EXPECT_NE(behaviorContext.m_ebuses.find("AtomRmlActionNotificationBus"), behaviorContext.m_ebuses.end());
        EXPECT_NE(behaviorContext.FindClassByTypeId(azrtti_typeid<AtomRmlActionEvent>()), nullptr);
    }
} // namespace AtomRml
