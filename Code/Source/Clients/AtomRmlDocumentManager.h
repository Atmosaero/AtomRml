/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomRml/AtomRmlDocumentManagerBus.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class Entity;
    class ReflectContext;
} // namespace AZ

namespace AtomRml
{
    class AtomRmlDocumentComponent;

    class AtomRmlDocumentManagerInterface
    {
    public:
        AZ_RTTI(AtomRmlDocumentManagerInterface, "{8C72140D-9047-46C0-87A5-79E71E16876B}");

        virtual ~AtomRmlDocumentManagerInterface() = default;
        virtual void RegisterComponent(AtomRmlDocumentComponent* component) = 0;
        virtual void UnregisterComponent(AtomRmlDocumentComponent* component) = 0;
    };

    class AtomRmlDocumentManager final
        : public AtomRmlDocumentManagerBus::Handler
        , public AtomRmlDocumentManagerInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(AtomRmlDocumentManager, AZ::SystemAllocator);

        AtomRmlDocumentManager() = default;
        ~AtomRmlDocumentManager() override;

        static void Reflect(AZ::ReflectContext* context);

        void Activate();
        void Deactivate();

        AZ::EntityId FindLoadedDocumentByPathName(
            const AZStd::string& documentPathname, bool loadIfNotFound = false) override;
        AZ::EntityId FindLoadedDocument(
            const AZ::Data::AssetId& documentAssetId, bool loadIfNotFound = false) override;

        void RegisterComponent(AtomRmlDocumentComponent* component) override;
        void UnregisterComponent(AtomRmlDocumentComponent* component) override;

    private:
        AZ::Data::AssetId ResolveDocumentAssetId(const AZStd::string& documentPathname) const;

        AZStd::unordered_map<AZ::EntityId, AtomRmlDocumentComponent*> m_components;
        AZStd::vector<AZStd::unique_ptr<AZ::Entity>> m_ownedDocumentEntities;
        bool m_isActive = false;
    };
} // namespace AtomRml
