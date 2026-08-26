/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRmlFontConfig.h"

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/sort.h>

namespace AtomRml
{
    AZStd::vector<AtomRmlFontConfig> LoadFontConfigs(const AZ::SettingsRegistryInterface& settingsRegistry)
    {
        AZStd::vector<AtomRmlFontConfig> fontConfigs;
        const AZStd::string registryPath(FontSettingsRegistryPath);

        settingsRegistry.Visit(
            [&fontConfigs, &registryPath, &settingsRegistry](
                const AZ::SettingsRegistryInterface::VisitArgs& args,
                AZ::SettingsRegistryInterface::VisitAction action)
            {
                if (action != AZ::SettingsRegistryInterface::VisitAction::Begin ||
                    args.m_type != AZ::SettingsRegistryInterface::Type::Object || args.m_jsonKeyPath == registryPath)
                {
                    return AZ::SettingsRegistryInterface::VisitResponse::Continue;
                }

                const AZStd::string entryPath(args.m_jsonKeyPath);
                const size_t separator = entryPath.rfind('/');
                if (separator == AZStd::string::npos || entryPath.substr(0, separator) != registryPath)
                {
                    return AZ::SettingsRegistryInterface::VisitResponse::Continue;
                }

                AtomRmlFontConfig config;
                config.m_name = args.m_fieldName;
                settingsRegistry.Get(config.m_path, entryPath + "/path");
                settingsRegistry.Get(config.m_fallback, entryPath + "/fallback");

                AZ::s64 faceIndex = 0;
                settingsRegistry.Get(faceIndex, entryPath + "/faceIndex");
                if (faceIndex >= 0 && faceIndex <= AZStd::numeric_limits<int>::max())
                {
                    config.m_faceIndex = aznumeric_cast<int>(faceIndex);
                }
                else
                {
                    config.m_faceIndex = -1;
                }

                fontConfigs.push_back(AZStd::move(config));
                return AZ::SettingsRegistryInterface::VisitResponse::Continue;
            },
            registryPath);

        AZStd::sort(
            fontConfigs.begin(), fontConfigs.end(),
            [](const AtomRmlFontConfig& left, const AtomRmlFontConfig& right)
            {
                return left.m_name < right.m_name;
            });
        return fontConfigs;
    }
} // namespace AtomRml
