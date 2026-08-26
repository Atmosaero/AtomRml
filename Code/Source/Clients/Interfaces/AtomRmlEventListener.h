/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/function/function_template.h>

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>

namespace AtomRml
{
    //! Creates listeners for inline RML event attributes such as onclick="StartGame".
    class AtomRmlEventListenerInstancer final : public Rml::EventListenerInstancer
    {
    public:
        using ActionHandler = AZStd::function<void(const Rml::String&, Rml::Event&)>;

        explicit AtomRmlEventListenerInstancer(ActionHandler actionHandler = {});

        Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* element) override;

    private:
        ActionHandler m_actionHandler;
    };
} // namespace AtomRml
