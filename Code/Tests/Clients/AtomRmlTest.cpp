/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 Atmosaero
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>

#include <AtomRml/AtomRmlTypeIds.h>
#include <Clients/AtomRmlSystemComponent.h>

namespace AtomRml
{
    TEST(AtomRmlSystemComponentTests, ReflectRegistersSystemComponent)
    {
        AZ::SerializeContext serializeContext;

        AtomRmlSystemComponent::Reflect(&serializeContext);

        EXPECT_NE(serializeContext.FindClassData(azrtti_typeid<AtomRmlSystemComponent>()), nullptr);
    }

    TEST(AtomRmlSystemComponentTests, DeclaresExpectedServiceDependencies)
    {
        AZ::ComponentDescriptor::DependencyArrayType providedServices;
        AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
        AZ::ComponentDescriptor::DependencyArrayType requiredServices;
        AZ::ComponentDescriptor::DependencyArrayType dependentServices;

        AtomRmlSystemComponent::GetProvidedServices(providedServices);
        AtomRmlSystemComponent::GetIncompatibleServices(incompatibleServices);
        AtomRmlSystemComponent::GetRequiredServices(requiredServices);
        AtomRmlSystemComponent::GetDependentServices(dependentServices);

        EXPECT_EQ(providedServices, AZ::ComponentDescriptor::DependencyArrayType{ AZ_CRC_CE("AtomRmlSystemService") });
        EXPECT_EQ(incompatibleServices, AZ::ComponentDescriptor::DependencyArrayType{ AZ_CRC_CE("AtomRmlSystemService") });
        EXPECT_EQ(requiredServices, AZ::ComponentDescriptor::DependencyArrayType{ AZ_CRC_CE("RPISystem") });
        EXPECT_EQ(dependentServices, AZ::ComponentDescriptor::DependencyArrayType{ AZ_CRC_CE("PassTemplatesAutoLoader") });
    }

    TEST(AtomRmlTypeIdTests, ComponentTypeIdsAreDistinct)
    {
        EXPECT_NE(AZ::TypeId::CreateString(AtomRmlSystemComponentTypeId),
            AZ::TypeId::CreateString(AtomRmlEditorSystemComponentTypeId));
        EXPECT_NE(AZ::TypeId::CreateString(AtomRmlModuleInterfaceTypeId),
            AZ::TypeId::CreateString(AtomRmlModuleTypeId));
    }
} // namespace AtomRml

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
