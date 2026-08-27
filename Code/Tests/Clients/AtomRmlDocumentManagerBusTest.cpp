/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomRml/AtomRmlDocumentManagerBus.h>

#include <Clients/AtomRmlDocumentManager.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzTest/AzTest.h>

namespace AtomRml
{
    class DocumentManagerHandler final : public AtomRmlDocumentManagerBus::Handler
    {
    public:
        AZ::EntityId FindLoadedDocumentByPathName(
            const AZStd::string& documentPathname, bool loadIfNotFound) override
        {
            m_pathname = documentPathname;
            m_loadIfNotFound = loadIfNotFound;
            return m_result;
        }

        AZ::EntityId FindLoadedDocument(
            const AZ::Data::AssetId& documentAssetId, bool loadIfNotFound) override
        {
            m_assetId = documentAssetId;
            m_loadIfNotFound = loadIfNotFound;
            return m_result;
        }

        AZStd::string m_pathname;
        AZ::Data::AssetId m_assetId;
        bool m_loadIfNotFound = false;
        AZ::EntityId m_result{ 707 };
    };

    TEST(AtomRmlDocumentManagerBusTests, BroadcastRoutesPathAndLoadFlag)
    {
        DocumentManagerHandler handler;
        handler.BusConnect();

        AZ::EntityId result;
        AtomRmlDocumentManagerBus::BroadcastResult(
            result,
            &AtomRmlDocumentManagerBus::Events::FindLoadedDocumentByPathName,
            "Assets/UI/Main.rml",
            true);

        EXPECT_EQ(result, handler.m_result);
        EXPECT_EQ(handler.m_pathname, "Assets/UI/Main.rml");
        EXPECT_TRUE(handler.m_loadIfNotFound);

        handler.BusDisconnect();
    }

    TEST(AtomRmlDocumentManagerBusTests, BroadcastRoutesAssetIdAndLoadFlag)
    {
        DocumentManagerHandler handler;
        handler.BusConnect();
        const AZ::Data::AssetId assetId(AZ::Uuid::CreateRandom(), 9);

        AZ::EntityId result;
        AtomRmlDocumentManagerBus::BroadcastResult(
            result,
            &AtomRmlDocumentManagerBus::Events::FindLoadedDocument,
            assetId,
            true);

        EXPECT_EQ(result, handler.m_result);
        EXPECT_EQ(handler.m_assetId, assetId);
        EXPECT_TRUE(handler.m_loadIfNotFound);

        handler.BusDisconnect();
    }

    TEST(AtomRmlDocumentManagerBusTests, BehaviorContextExposesTheSingleEventApi)
    {
        AZ::BehaviorContext behaviorContext;
        AtomRmlDocumentManager::Reflect(&behaviorContext);

        const auto busIterator = behaviorContext.m_ebuses.find("AtomRmlDocumentManagerBus");
        ASSERT_NE(busIterator, behaviorContext.m_ebuses.end());
        const AZ::BehaviorEBus* behaviorBus = busIterator->second;
        ASSERT_NE(behaviorBus, nullptr);
        EXPECT_EQ(behaviorBus->m_events.size(), 2);
        EXPECT_NE(
            behaviorBus->m_events.find("FindLoadedDocumentByPathName"),
            behaviorBus->m_events.end());
        EXPECT_NE(behaviorBus->m_events.find("FindLoadedDocument"), behaviorBus->m_events.end());
    }
} // namespace AtomRml
