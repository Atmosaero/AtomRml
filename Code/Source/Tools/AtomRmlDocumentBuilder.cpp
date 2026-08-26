/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AtomRmlDocumentBuilder.h"

#include <AtomRml/AtomRmlDocumentAsset.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AtomRml
{
    void AtomRmlDocumentBuilderWorker::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest& request,
        AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& platform : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor job;
            job.m_jobKey = "AtomRml Document";
            job.SetPlatformIdentifier(platform.m_identifier.c_str());
            response.m_createJobOutputs.push_back(AZStd::move(job));
        }
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void AtomRmlDocumentBuilderWorker::ProcessJob(
        const AssetBuilderSDK::ProcessJobRequest& request,
        AssetBuilderSDK::ProcessJobResponse& response)
    {
        AssetBuilderSDK::JobCancelListener cancelListener(request.m_jobId);
        if (m_isShuttingDown || cancelListener.IsCancelled())
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), fileName);
        AZStd::string destinationPath;
        AzFramework::StringFunc::Path::ConstructFull(
            request.m_tempDirPath.c_str(), fileName.c_str(), destinationPath, true);

        AZ::IO::LocalFileIO fileIo;
        if (fileIo.Copy(request.m_fullPath.c_str(), destinationPath.c_str()) != AZ::IO::ResultCode::Success)
        {
            AZLOG_ERROR("Failed to copy RML document '%s'", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AssetBuilderSDK::JobProduct product(fileName);
        product.m_productAssetType = azrtti_typeid<AtomRmlDocumentAsset>();
        product.m_productSubID = 0;
        product.m_dependenciesHandled = true;
        response.m_outputProducts.push_back(AZStd::move(product));
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    void AtomRmlDocumentBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void AtomRmlDocumentBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AtomRmlDocumentBuilderComponent, AZ::Component>()
                ->Version(1)
                ->Attribute(
                    AZ::Edit::Attributes::SystemComponentTags,
                    AZStd::vector<AZ::Crc32>{ AssetBuilderSDK::ComponentTags::AssetBuilder });
        }
    }

    void AtomRmlDocumentBuilderComponent::Activate()
    {
        AssetBuilderSDK::AssetBuilderDesc descriptor;
        descriptor.m_name = "AtomRml Document Builder";
        descriptor.m_patterns.emplace_back(
            "*.rml", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        descriptor.m_busId = azrtti_typeid<AtomRmlDocumentBuilderWorker>();
        descriptor.m_version = 1;
        descriptor.m_createJobFunction = AZStd::bind(
            &AtomRmlDocumentBuilderWorker::CreateJobs, &m_worker,
            AZStd::placeholders::_1, AZStd::placeholders::_2);
        descriptor.m_processJobFunction = AZStd::bind(
            &AtomRmlDocumentBuilderWorker::ProcessJob, &m_worker,
            AZStd::placeholders::_1, AZStd::placeholders::_2);
        descriptor.m_flags = AssetBuilderSDK::AssetBuilderDesc::BF_EmitsNoDependencies;

        m_worker.BusConnect(descriptor.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(
            &AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, descriptor);
    }

    void AtomRmlDocumentBuilderComponent::Deactivate()
    {
        m_worker.BusDisconnect();
    }
} // namespace AtomRml
