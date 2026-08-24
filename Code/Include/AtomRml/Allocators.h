/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 Reece Hagan
 *
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>

namespace AtomRml
{
    AZ_CHILD_ALLOCATOR_WITH_NAME(AtomRmlRenderAllocator, "AtomRmlRenderAllocator", "{2C939D55-D099-47D8-922C-E0FDACF83EFD}", AZ::SystemAllocator);
}