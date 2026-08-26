/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzTest/AzTest.h>

#include <AtomRml/AtomRmlTypeIds.h>
#include <AtomRml/AtomRmlDocumentAsset.h>
#include <AtomRml/AtomRmlDocumentComponent.h>
#include <Clients/AtomRmlSystemComponent.h>
#include <Clients/AtomRmlFontConfig.h>
#include <Render/AtomRmlPassBus.h>

namespace AtomRml
{
    class MockAtomRmlPassHandler final
        : public AtomRmlPassRequestBus::Handler
    {
    public:
        AtomRmlParentPass* GetParentPass() override
        {
            ++m_requestCount;
            return nullptr;
        }

        int m_requestCount = 0;
    };

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

    TEST(AtomRmlFontConfigTests, GemAndProjectEntriesMergeAndLoadInStableOrder)
    {
        AZ::SettingsRegistryImpl settingsRegistry;
        ASSERT_TRUE(settingsRegistry.MergeSettings(
            R"({"O3DE":{"AtomRml":{"Fonts":{"20_ProjectFallback":{"path":"Fonts/ProjectFallback.ttc","fallback":true,"faceIndex":2}}}}})",
            AZ::SettingsRegistryInterface::Format::JsonMergePatch));
        ASSERT_TRUE(settingsRegistry.MergeSettings(
            R"({"O3DE":{"AtomRml":{"Fonts":{"10_GemRegular":{"path":"Fonts/GemRegular.ttf","fallback":false,"faceIndex":0}}}}})",
            AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        const AZStd::vector<AtomRmlFontConfig> fontConfigs = LoadFontConfigs(settingsRegistry);

        ASSERT_EQ(fontConfigs.size(), 2);
        EXPECT_EQ(fontConfigs[0].m_name, "10_GemRegular");
        EXPECT_EQ(fontConfigs[0].m_path, "Fonts/GemRegular.ttf");
        EXPECT_FALSE(fontConfigs[0].m_fallback);
        EXPECT_EQ(fontConfigs[0].m_faceIndex, 0);
        EXPECT_EQ(fontConfigs[1].m_name, "20_ProjectFallback");
        EXPECT_EQ(fontConfigs[1].m_path, "Fonts/ProjectFallback.ttc");
        EXPECT_TRUE(fontConfigs[1].m_fallback);
        EXPECT_EQ(fontConfigs[1].m_faceIndex, 2);
    }

    TEST(AtomRmlDocumentComponentTests, UsesDedicatedDocumentAssetType)
    {
        EXPECT_FALSE(azrtti_typeid<AtomRmlDocumentAsset>().IsNull());
        EXPECT_NE(azrtti_typeid<AtomRmlDocumentAsset>(), azrtti_typeid<AZ::Data::AssetData>());
    }

    TEST(AtomRmlDocumentComponentTests, HasNoEntityServiceRequirements)
    {
        AZ::ComponentDescriptor::DependencyArrayType requiredServices;

        AtomRmlDocumentComponent::GetRequiredServices(requiredServices);

        EXPECT_TRUE(requiredServices.empty());
    }

    TEST(AtomRmlDocumentComponentTests, PreventsMultipleDocumentReferencesOnAnEntity)
    {
        AZ::ComponentDescriptor::DependencyArrayType providedServices;
        AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;

        AtomRmlDocumentComponent::GetProvidedServices(providedServices);
        AtomRmlDocumentComponent::GetIncompatibleServices(incompatibleServices);

        const AZ::ComponentServiceType documentService = AZ_CRC_CE("AtomRmlDocumentService");
        EXPECT_EQ(providedServices, AZ::ComponentDescriptor::DependencyArrayType{ documentService });
        EXPECT_EQ(incompatibleServices, AZ::ComponentDescriptor::DependencyArrayType{ documentService });
    }

    TEST(AtomRmlTypeIdTests, ComponentTypeIdsAreDistinct)
    {
        EXPECT_NE(AZ::TypeId::CreateString(AtomRmlSystemComponentTypeId),
            AZ::TypeId::CreateString(AtomRmlEditorSystemComponentTypeId));
        EXPECT_NE(AZ::TypeId::CreateString(AtomRmlSystemComponentTypeId),
            AZ::TypeId::CreateString(AtomRmlDocumentComponentTypeId));
        EXPECT_NE(AZ::TypeId::CreateString(AtomRmlModuleInterfaceTypeId),
            AZ::TypeId::CreateString(AtomRmlModuleTypeId));
    }

    TEST(AtomRmlPassBusTests, RoutesBySceneAndStopsRoutingAfterDisconnect)
    {
        const AZ::RPI::SceneId firstSceneId = AZ::Uuid::CreateRandom();
        const AZ::RPI::SceneId secondSceneId = AZ::Uuid::CreateRandom();
        MockAtomRmlPassHandler firstHandler;
        MockAtomRmlPassHandler secondHandler;

        firstHandler.BusConnect(firstSceneId);
        secondHandler.BusConnect(secondSceneId);

        AtomRmlParentPass* result = nullptr;
        AtomRmlPassRequestBus::EventResult(
            result, firstSceneId, &AtomRmlPassRequestBus::Events::GetParentPass);

        EXPECT_EQ(firstHandler.m_requestCount, 1);
        EXPECT_EQ(secondHandler.m_requestCount, 0);
        EXPECT_TRUE(AtomRmlPassRequestBus::HasHandlers(firstSceneId));
        EXPECT_TRUE(AtomRmlPassRequestBus::HasHandlers(secondSceneId));

        firstHandler.BusDisconnect();
        EXPECT_FALSE(AtomRmlPassRequestBus::HasHandlers(firstSceneId));
        EXPECT_TRUE(AtomRmlPassRequestBus::HasHandlers(secondSceneId));

        secondHandler.BusDisconnect();
        EXPECT_FALSE(AtomRmlPassRequestBus::HasHandlers(secondSceneId));
    }
} // namespace AtomRml

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
