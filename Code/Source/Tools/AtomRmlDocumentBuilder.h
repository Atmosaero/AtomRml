/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AzCore/Component/Component.h>

namespace AtomRml
{
    class AtomRmlDocumentBuilderWorker final
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(AtomRmlDocumentBuilderWorker, "{D0E183EA-C88B-4A95-8238-B7C741685F77}");

        void CreateJobs(
            const AssetBuilderSDK::CreateJobsRequest& request,
            AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AssetBuilderSDK::ProcessJobResponse& response);
        void ShutDown() override;

    private:
        bool m_isShuttingDown = false;
    };

    class AtomRmlDocumentBuilderComponent final
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AtomRmlDocumentBuilderComponent, "{A5DB68BD-B415-4C43-90B5-EFF74664567A}");

        static void Reflect(AZ::ReflectContext* context);

        void Activate() override;
        void Deactivate() override;

    private:
        AtomRmlDocumentBuilderWorker m_worker;
    };
} // namespace AtomRml
