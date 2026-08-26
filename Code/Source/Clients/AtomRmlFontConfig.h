/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AtomRml
{
    inline constexpr AZStd::string_view FontSettingsRegistryPath = "/O3DE/AtomRml/Fonts";

    struct AtomRmlFontConfig
    {
        AZStd::string m_name;
        AZStd::string m_path;
        bool m_fallback = false;
        int m_faceIndex = 0;
    };

    //! Reads the merge-friendly, named font entries below /O3DE/AtomRml/Fonts.
    AZStd::vector<AtomRmlFontConfig> LoadFontConfigs(const AZ::SettingsRegistryInterface& settingsRegistry);
} // namespace AtomRml
