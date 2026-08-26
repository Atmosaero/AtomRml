/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AtomRmlSystemComponent.h"
#include "Assets/AtomRmlDocumentAssetHandler.h"
#include "AtomRmlFontConfig.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <AtomRml/AtomRmlTypeIds.h>
#include <Render/AtomRmlFeatureProcessor.h>
#include <Render/AtomRmlParentPass.h>
#include <Render/AtomRmlChildPass.h>
#include <Render/AtomRmlRenderInterface.h>

#include <RmlUi/Core.h>

#include "../AtomRmlBudget.h"
AZ_DEFINE_BUDGET(AtomRmlBudget);

namespace AtomRml
{
    AZ_COMPONENT_IMPL(AtomRmlSystemComponent, "AtomRmlSystemComponent",
        AtomRmlSystemComponentTypeId);

    void AtomRmlSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AtomRmlSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }

        AtomRmlFeatureProcessor::Reflect(context);
    }

    void AtomRmlSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomRmlSystemService"));
    }

    void AtomRmlSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomRmlSystemService"));
    }

    void AtomRmlSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void AtomRmlSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("PassTemplatesAutoLoader"));
    }

    AtomRmlSystemComponent::AtomRmlSystemComponent() = default;

    AtomRmlSystemComponent::~AtomRmlSystemComponent() = default;

    AtomRmlRenderInterface* AtomRmlSystemComponent::GetRenderInterface()
    {
        return m_renderInterface.get();
    }

    void AtomRmlSystemComponent::Init()
    {
    }

    void AtomRmlSystemComponent::Activate()
    {
        AZ::SystemTickBus::Handler::BusConnect();
        AtomRmlRequestBus::Handler::BusConnect();

        AZ_Assert(AtomRmlInterface::Get() == nullptr, "AtomRmlInterface is already registered");
        AtomRmlInterface::Register(this);

        AZ_Assert(
            AZ::Interface<AtomRmlActionRouterInterface>::Get() == nullptr,
            "AtomRmlActionRouterInterface is already registered");
        AZ::Interface<AtomRmlActionRouterInterface>::Register(&m_actionRouter);

        m_documentAssetHandler = AZStd::make_unique<AtomRmlDocumentAssetHandler>();
        m_documentAssetHandler->Register();

        m_renderInterface = AZStd::make_unique<AtomRmlRenderInterface>();
        Rml::SetRenderInterface(m_renderInterface.get());
        m_fileInterface.Init();
        m_inputInterface.Init();
        m_systemInterface.Init();

        if (!Rml::Initialise())
        {
            AZLOG_ERROR("Failed to initialise RmlUi");
            return;
        }

        Rml::Factory::RegisterEventListenerInstancer(&m_eventListenerInstancer);

        LoadConfiguredFonts();
        
        // Register pass classes
        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        passSystem->AddPassCreator(AZ::Name("AtomRmlParentPass"), &AtomRmlParentPass::Create);
        passSystem->AddPassCreator(AZ::Name("AtomRmlChildPass"), &AtomRmlChildPass::Create);

        AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<AtomRmlFeatureProcessor>();
    }

    void AtomRmlSystemComponent::Deactivate()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        m_inputInterface.Shutdown();
        m_fileInterface.Shutdown();
        m_systemInterface.Shutdown();

        AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<AtomRmlFeatureProcessor>();

        Rml::Shutdown();

        m_actionRouter.Clear();
        if (AZ::Interface<AtomRmlActionRouterInterface>::Get() == &m_actionRouter)
        {
            AZ::Interface<AtomRmlActionRouterInterface>::Unregister(&m_actionRouter);
        }

        if (m_renderInterface)
        {
            m_renderInterface.reset();
        }

        if (m_documentAssetHandler)
        {
            m_documentAssetHandler->Unregister();
            m_documentAssetHandler.reset();
        }

        if (AtomRmlInterface::Get() == this)
        {
            AtomRmlInterface::Unregister(this);
        }
        AtomRmlRequestBus::Handler::BusDisconnect();
    }

    void AtomRmlSystemComponent::OnSystemTick()
    {
        const auto numCtxs = Rml::GetNumContexts();
        for (auto i = 0; i < numCtxs; ++i)
        {
            auto ctx = Rml::GetContext(i);
            if (ctx == nullptr)
                continue;

            ctx->Update();
        }
    }

    void AtomRmlSystemComponent::LoadConfiguredFonts()
    {
        const AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            AZLOG_WARN("Settings Registry is unavailable; no RmlUi fonts were loaded");
            return;
        }

        const AZStd::vector<AtomRmlFontConfig> fontConfigs = LoadFontConfigs(*settingsRegistry);
        if (fontConfigs.empty())
        {
            AZLOG_WARN("No fonts are configured below %.*s", AZ_STRING_ARG(FontSettingsRegistryPath));
            return;
        }

        for (const AtomRmlFontConfig& fontConfig : fontConfigs)
        {
            if (fontConfig.m_path.empty() || fontConfig.m_faceIndex < 0)
            {
                AZLOG_WARN(
                    "Ignoring invalid font entry '%s' below %.*s",
                    fontConfig.m_name.c_str(), AZ_STRING_ARG(FontSettingsRegistryPath));
                continue;
            }

            if (!Rml::LoadFontFace(
                    fontConfig.m_path.c_str(), fontConfig.m_fallback, Rml::Style::FontWeight::Auto,
                    fontConfig.m_faceIndex))
            {
                AZLOG_WARN(
                    "Failed to load configured font '%s' from '%s' (faceIndex: %d, fallback: %s)",
                    fontConfig.m_name.c_str(), fontConfig.m_path.c_str(), fontConfig.m_faceIndex,
                    fontConfig.m_fallback ? "true" : "false");
            }
        }
    }
} // namespace AtomRml
