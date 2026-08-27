-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT

local AtomRmlDocumentManagerSample = {}

function AtomRmlDocumentManagerSample:OnActivate()
    self.documentEntityId = AtomRmlDocumentManagerBus.Broadcast.FindLoadedDocumentByPathName(
        "Assets/Samples/aurora-command.rml",
        true)

    if self.documentEntityId:IsValid() then
        Debug.Log("AtomRml manager loaded Aurora Command on entity " .. tostring(self.documentEntityId))
    else
        Debug.Log("AtomRml manager could not load Aurora Command")
    end
end

return AtomRmlDocumentManagerSample
