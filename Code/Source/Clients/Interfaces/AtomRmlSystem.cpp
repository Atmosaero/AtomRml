/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRmlSystem.h"

#include <RmlUi/Core.h>

#include <AzCore/Console/ILogger.h>

using namespace AtomRml;

void AtomRmlSystem::Init()
{
    Rml::SetSystemInterface(this);
}

void AtomRmlSystem::Shutdown()
{
}

bool AtomRmlSystem::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
    switch (type)
    {
    case Rml::Log::LT_ALWAYS:
    case Rml::Log::LT_INFO:
        AZLOG(AtomRml, "%s", message.c_str());
        break;
    case Rml::Log::LT_ASSERT:
    case Rml::Log::LT_ERROR:
        AZLOG_ERROR("RmlUi: %s", message.c_str());
        break;
    case Rml::Log::LT_WARNING:
        AZLOG_WARN("RmlUi: %s", message.c_str());
        break;
    case Rml::Log::LT_DEBUG:
        AZLOG(AtomRml, "%s", message.c_str());
        break;
    case Rml::Log::LT_MAX:
    default:
        break;
    }

    return false;
}
