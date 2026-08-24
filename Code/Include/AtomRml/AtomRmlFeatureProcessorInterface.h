/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 Reece Hagan
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
#pragma once

#include <AzCore/base.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace Rml
{
    class Context;
}

namespace AtomRml
{
    class AtomRml;
    class AtomRmlConsoleOverlay;

    using AtomRmlHandle = AZStd::shared_ptr<AtomRml>;

    class AtomRmlFeatureProcessorInterface
        : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(AtomRmlFeatureProcessorInterface, "{2460040A-0B9F-4A1D-9E41-B81273EB0A08}", AZ::RPI::FeatureProcessor);

        virtual Rml::Context* GetContext() = 0;
        virtual void GetChildPasses(AZStd::function<void(class AtomRmlChildPass*)> fn) = 0;
    };
}
