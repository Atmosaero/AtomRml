/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Clients/Interfaces/AtomRmlEventListener.h>

#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/Event.h>

namespace AtomRml
{
    TEST(AtomRmlEventListenerTests, InlineEventValueIsReportedAsActionName)
    {
        Rml::String receivedAction;
        Rml::String receivedEventType;
        AtomRmlEventListenerInstancer instancer(
            [&receivedAction, &receivedEventType](const Rml::String& actionName, Rml::Event& event)
            {
                receivedAction = actionName;
                receivedEventType = event.GetType();
            });

        Rml::EventListener* listener = instancer.InstanceEventListener("StartGame", nullptr);
        ASSERT_NE(listener, nullptr);

        Rml::Dictionary parameters;
        Rml::Event clickEvent(nullptr, Rml::EventId::Click, "click", parameters, true);
        listener->ProcessEvent(clickEvent);

        EXPECT_EQ(receivedAction, "StartGame");
        EXPECT_EQ(receivedEventType, "click");

        listener->OnDetach(nullptr);
    }
} // namespace AtomRml
