-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT

local AtomRmlDocumentEventSample = {}

function AtomRmlDocumentEventSample:OnActivate()
    self.actionHandler = AtomRmlActionNotificationBus.Connect(self, self.entityId)
end

function AtomRmlDocumentEventSample:OnAction(actionEvent)
    if actionEvent.ActionName == "ToggleTarget" then
        local documentEvent = AtomRmlDocumentEvent()
        documentEvent.EventType = "click"
        documentEvent.TargetElementId = "target-state"
        documentEvent:SetStringParameter("source", "Lua")

        local propagated = AtomRmlDocumentEventBus.Event.DispatchEvent(self.entityId, documentEvent)
        Debug.Log("Lua toggled the target button through RmlUi: " .. tostring(propagated))
    end
end

function AtomRmlDocumentEventSample:OnDeactivate()
    if self.actionHandler then
        self.actionHandler:Disconnect()
    end
end

return AtomRmlDocumentEventSample
