/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>

#include <Clients/AtomRmlSystemComponent.h>
#include <Tools/AtomRmlEditorSystemComponent.h>

namespace AtomRml
{
    TEST(AtomRmlEditorSystemComponentTests, ReflectRegistersEditorComponentAndBase)
    {
        AZ::SerializeContext serializeContext;

        AtomRmlSystemComponent::Reflect(&serializeContext);
        AtomRmlEditorSystemComponent::Reflect(&serializeContext);

        EXPECT_NE(serializeContext.FindClassData(azrtti_typeid<AtomRmlSystemComponent>()), nullptr);
        EXPECT_NE(serializeContext.FindClassData(azrtti_typeid<AtomRmlEditorSystemComponent>()), nullptr);
    }
} // namespace AtomRml

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
