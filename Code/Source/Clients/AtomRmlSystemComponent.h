/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomRml/AtomRmlBus.h>
#include "Interfaces/AtomRmlFile.h"
#include "Interfaces/AtomRmlEventListener.h"
#include "Interfaces/AtomRmlInput.h"
#include "Interfaces/AtomRmlSystem.h"
#include "AtomRmlActionRouter.h"
#include "AtomRmlDocumentManager.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/PackedVector2.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>

#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Input/Buses/Notifications/InputTextNotificationBus.h>

namespace AtomRml
{
    class AtomRmlDocumentAssetHandler;
    class AtomRmlRenderInterface;

    class AtomRmlSystemComponent
        : public AZ::Component
        , protected AtomRmlRequestBus::Handler
        , protected AZ::SystemTickBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(AtomRmlSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        AtomRmlSystemComponent();
        ~AtomRmlSystemComponent() override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AtomRmlRequestBus interface implementation
        AtomRmlRenderInterface* GetRenderInterface() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        //AZ::SystemTickBus
        void OnSystemTick() override;

    private:
        void LoadConfiguredFonts();

        AtomRmlFile m_fileInterface;
        AtomRmlActionRouter m_actionRouter;
        AtomRmlDocumentManager m_documentManager;
        AtomRmlEventListenerInstancer m_eventListenerInstancer;
        AtomRmlInput m_inputInterface;
        AtomRmlSystem m_systemInterface;
        AZStd::unique_ptr<AtomRmlDocumentAssetHandler> m_documentAssetHandler;
        AZStd::unique_ptr<AtomRmlRenderInterface> m_renderInterface;
    };

} // namespace AtomRml
