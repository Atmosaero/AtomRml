/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 Reece Hagan
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
#include "AtomRmlSystemComponent.h"
#include "Assets/AtomRmlDocumentAssetHandler.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <AtomRml/AtomRmlTypeIds.h>
#include <Console/AtomRmlConsoleDocument.h>
#include <Render/AtomRmlFeatureProcessor.h>
#include <Render/AtomRmlParentPass.h>
#include <Render/AtomRmlChildPass.h>
#include <Render/AtomRmlRenderInterface.h>

#include <RmlUi/Core.h>

#include "../RmlBudget.h"
AZ_DEFINE_BUDGET(RmlBudget);

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

        m_documentAssetHandler = AZStd::make_unique<AtomRmlDocumentAssetHandler>();
        m_documentAssetHandler->Register();

        m_renderInterface = AZStd::make_unique<AtomRmlRenderInterface>();
        Rml::SetRenderInterface(m_renderInterface.get());
        m_fileInterface.Init();
        m_inputInterface.Init();
        m_systemInterface.Init();

        if (!Rml::Initialise())
        {
            AZ_Error("AtomRml", false, "Failed to initialise RmlUi");
            return;
        }

        Rml::LoadFontFace("Fonts/Roboto-Regular.ttf");
        Rml::LoadFontFace("Fonts/Roboto-Bold.ttf");
        Rml::LoadFontFace("Fonts/Roboto-Italic.ttf");
        Rml::LoadFontFace("Fonts/LatoLatin-Regular.ttf");
        Rml::LoadFontFace("Fonts/LatoLatin-Italic.ttf");
        Rml::LoadFontFace("Console/JetBrainsMono-Regular.ttf");
        Rml::LoadFontFace("Fonts/NotoSansJP-VariableFont_wght.ttf", true);
        
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
} // namespace AtomRml
