/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 Reece Hagan
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
#pragma once

namespace AtomRml
{
    // System Component TypeIds
    inline constexpr const char* AtomRmlSystemComponentTypeId = "{21F186B4-A900-4519-BEC6-68B95C051C76}";
    inline constexpr const char* AtomRmlEditorSystemComponentTypeId = "{C2F18746-632D-4462-A552-848C59B92FE8}";
    inline constexpr const char* AtomRmlDocumentComponentTypeId = "{46A72870-9723-4F7A-BD29-54689F8C5C8A}";

    // Module derived classes TypeIds
    inline constexpr const char* AtomRmlModuleInterfaceTypeId = "{05420FC3-1248-48D7-8D92-9D2DEB355887}";
    inline constexpr const char* AtomRmlModuleTypeId = "{9B6485B8-F2D3-4C2B-A117-D41B9BC77848}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* AtomRmlEditorModuleTypeId = AtomRmlModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* AtomRmlRequestsTypeId = "{F98BDA8E-450C-47D4-9CB7-1C179B6A91DF}";
} // namespace AtomRml
