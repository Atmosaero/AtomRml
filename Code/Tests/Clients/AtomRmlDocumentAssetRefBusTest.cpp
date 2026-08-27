/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomRml/AtomRmlDocumentAssetRefBus.h>
#include <AtomRml/AtomRmlDocumentComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzTest/AzTest.h>

namespace AtomRml
{
    static_assert(AZStd::is_same_v<AtomRmlDocumentAssetRefBus::BusIdType, AZ::EntityId>);

    class DocumentAssetRefBusHandler final
        : public AtomRmlDocumentAssetRefBus::Handler
    {
    public:
        explicit DocumentAssetRefBusHandler(AZ::EntityId entityId)
        {
            BusConnect(entityId);
        }

        ~DocumentAssetRefBusHandler() override
        {
            BusDisconnect();
        }

        void SetPath(const AZ::Data::AssetId& assetId) override
        {
            m_assetId = assetId;
            ++m_setPathCalls;
        }

        void Remove() override
        {
            ++m_removeCalls;
        }

        void Show() override
        {
            ++m_showCalls;
        }

        AZ::Data::AssetId m_assetId;
        int m_setPathCalls = 0;
        int m_removeCalls = 0;
        int m_showCalls = 0;
    };

    TEST(AtomRmlDocumentAssetRefBusTests, AllEventsRouteOnlyToTheAddressedEntity)
    {
        const AZ::EntityId firstEntityId(101);
        const AZ::EntityId secondEntityId(202);
        const AZ::Data::AssetId assetId(AZ::Uuid::CreateRandom(), 7);
        DocumentAssetRefBusHandler firstHandler(firstEntityId);
        DocumentAssetRefBusHandler secondHandler(secondEntityId);

        AtomRmlDocumentAssetRefBus::Event(
            firstEntityId, &AtomRmlDocumentAssetRefBus::Events::SetPath, assetId);
        AtomRmlDocumentAssetRefBus::Event(firstEntityId, &AtomRmlDocumentAssetRefBus::Events::Remove);
        AtomRmlDocumentAssetRefBus::Event(firstEntityId, &AtomRmlDocumentAssetRefBus::Events::Show);

        EXPECT_EQ(firstHandler.m_assetId, assetId);
        EXPECT_EQ(firstHandler.m_setPathCalls, 1);
        EXPECT_EQ(firstHandler.m_removeCalls, 1);
        EXPECT_EQ(firstHandler.m_showCalls, 1);
        EXPECT_EQ(secondHandler.m_setPathCalls, 0);
        EXPECT_EQ(secondHandler.m_removeCalls, 0);
        EXPECT_EQ(secondHandler.m_showCalls, 0);
    }

    TEST(AtomRmlDocumentAssetRefBusTests, BehaviorContextExposesTheThreeEventApi)
    {
        AZ::BehaviorContext behaviorContext;
        AtomRmlDocumentComponent::Reflect(&behaviorContext);

        const auto busIterator = behaviorContext.m_ebuses.find("AtomRmlDocumentAssetRefBus");
        ASSERT_NE(busIterator, behaviorContext.m_ebuses.end());

        const AZ::BehaviorEBus* behaviorBus = busIterator->second;
        EXPECT_NE(behaviorBus->m_events.find("SetPath"), behaviorBus->m_events.end());
        EXPECT_EQ(behaviorBus->m_events.find("SetAutoLoad"), behaviorBus->m_events.end());
        EXPECT_NE(behaviorBus->m_events.find("Remove"), behaviorBus->m_events.end());
        EXPECT_NE(behaviorBus->m_events.find("Show"), behaviorBus->m_events.end());
        EXPECT_EQ(behaviorBus->m_events.size(), 3);
    }
} // namespace AtomRml
