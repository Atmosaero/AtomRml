-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT

-- Assign Samples/virtual-joystick.rml to Rml Document Asset Ref on this entity.
-- The native RmlUi handle moves each knob; Lua receives normalized gameplay axes.

local AtomRmlVirtualJoystickSample = {}

local maximumTravel = 57.0

local function clampAxis(value)
    return math.max(-1.0, math.min(1.0, value / maximumTravel))
end

function AtomRmlVirtualJoystickSample:OnActivate()
    self.actionHandler = AtomRmlActionNotificationBus.Connect(self, self.entityId)
end

function AtomRmlVirtualJoystickSample:OnAction(actionEvent)
    if actionEvent.ActionName == "LeftJoystickMoved"
        or actionEvent.ActionName == "RightJoystickMoved" then
        local x = tonumber(actionEvent:GetArgument("handle_x")) or 0.0
        local y = tonumber(actionEvent:GetArgument("handle_y")) or 0.0
        local side = actionEvent.ActionName == "LeftJoystickMoved" and "left" or "right"

        Debug.Log(string.format(
            "[AtomRmlVirtualJoystickSample] %s x=%.2f y=%.2f",
            side,
            clampAxis(x),
            clampAxis(y)))
    elseif actionEvent.ActionName == "LeftJoystickReleased"
        or actionEvent.ActionName == "RightJoystickReleased" then
        local side = actionEvent.ActionName == "LeftJoystickReleased" and "left" or "right"
        Debug.Log("[AtomRmlVirtualJoystickSample] " .. side .. " released")
    end
end

function AtomRmlVirtualJoystickSample:OnDeactivate()
    if self.actionHandler then
        self.actionHandler:Disconnect()
        self.actionHandler = nil
    end
end

return AtomRmlVirtualJoystickSample
