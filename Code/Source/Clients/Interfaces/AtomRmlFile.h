/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RmlUi/Core/FileInterface.h>

namespace AtomRml
{
   class AtomRmlFile final
        : public Rml::FileInterface
   {
   public:
       void Init();
       void Shutdown();

       //File interface
       Rml::FileHandle Open(const Rml::String& path) override;
       void Close(Rml::FileHandle file) override;
       size_t Read(void* buffer, size_t size, Rml::FileHandle file) override;
       bool Seek(Rml::FileHandle file, long offset, int origin) override;
       size_t Tell(Rml::FileHandle file) override;
       size_t Length(Rml::FileHandle file) override;
       bool LoadFile(const Rml::String& path, Rml::String& out_data) override;
   };
}