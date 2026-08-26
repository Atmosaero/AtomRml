/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomRml/AtomRmlActionBus.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/containers/unordered_map.h>

#include <RmlUi/Core/Types.h>

namespace Rml
{
    class ElementDocument;
    class Event;
} // namespace Rml

namespace AtomRml
{
    AtomRmlActionEvent BuildActionEvent(const Rml::String& actionName, Rml::Event& event);

    class AtomRmlActionRouterInterface
    {
    public:
        AZ_RTTI(AtomRmlActionRouterInterface, "{CA076B6C-3B17-4F6B-844F-A52999C10001}");

        virtual ~AtomRmlActionRouterInterface() = default;
        virtual void RegisterDocument(const Rml::ElementDocument* document, AZ::EntityId entityId) = 0;
        virtual void UnregisterDocument(const Rml::ElementDocument* document) = 0;
        virtual bool DispatchAction(const Rml::String& actionName, Rml::Event& event) = 0;
    };

    class AtomRmlActionRouter final : public AtomRmlActionRouterInterface
    {
    public:
        void RegisterDocument(const Rml::ElementDocument* document, AZ::EntityId entityId) override;
        void UnregisterDocument(const Rml::ElementDocument* document) override;
        bool DispatchAction(const Rml::String& actionName, Rml::Event& event) override;
        bool DispatchAction(const Rml::ElementDocument* document, AtomRmlActionEvent actionEvent);
        void Clear();

    private:
        AZStd::unordered_map<const Rml::ElementDocument*, AZ::EntityId> m_documentEntities;
    };
} // namespace AtomRml
