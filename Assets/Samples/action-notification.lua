-- Listen for actions emitted by the Rml Document Asset Ref on this entity.
-- Assign Samples/buttons.rml to the component and enable Auto Load.

local AtomRmlActionNotificationSample = {}

function AtomRmlActionNotificationSample:OnActivate()
    self.actionHandler = AtomRmlActionNotificationBus.Connect(self, self.entityId)
end

function AtomRmlActionNotificationSample:OnAction(actionEvent)
    local message = "[AtomRmlActionNotificationSample] action=" .. actionEvent.ActionName
        .. " event=" .. actionEvent.EventType
        .. " element=" .. actionEvent.ElementId

    if actionEvent:HasArgument("variant") then
        message = message .. " variant=" .. actionEvent:GetArgument("variant")
    end

    Debug.Log(message)
end

function AtomRmlActionNotificationSample:OnDeactivate()
    if self.actionHandler then
        self.actionHandler:Disconnect()
        self.actionHandler = nil
    end
end

return AtomRmlActionNotificationSample
