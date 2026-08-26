/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRmlEventListener.h"

#include "../AtomRmlActionRouter.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Event.h>

namespace AtomRml
{
    namespace
    {
        class AtomRmlEventListener final : public Rml::EventListener
        {
        public:
            AtomRmlEventListener(Rml::String actionName, AtomRmlEventListenerInstancer::ActionHandler actionHandler)
                : m_actionName(AZStd::move(actionName))
                , m_actionHandler(AZStd::move(actionHandler))
            {
            }

            void ProcessEvent(Rml::Event& event) override
            {
                m_actionHandler(m_actionName, event);
            }

            void OnDetach(Rml::Element*) override
            {
                delete this;
            }

        private:
            Rml::String m_actionName;
            AtomRmlEventListenerInstancer::ActionHandler m_actionHandler;
        };

        void DispatchAction(const Rml::String& actionName, Rml::Event& event)
        {
            if (auto* router = AZ::Interface<AtomRmlActionRouterInterface>::Get())
            {
                router->DispatchAction(actionName, event);
            }
            else
            {
                AZLOG_WARN("Rml action '%s' could not be routed because the action router is unavailable", actionName.c_str());
            }
        }
    } // namespace

    AtomRmlEventListenerInstancer::AtomRmlEventListenerInstancer(ActionHandler actionHandler)
        : m_actionHandler(actionHandler ? AZStd::move(actionHandler) : ActionHandler(DispatchAction))
    {
    }

    Rml::EventListener* AtomRmlEventListenerInstancer::InstanceEventListener(const Rml::String& value, Rml::Element*)
    {
        return new AtomRmlEventListener(value, m_actionHandler);
    }
} // namespace AtomRml
