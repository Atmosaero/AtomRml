# AtomRml

AtomRml is a community-developed integration of [RmlUi](https://github.com/mikke89/RmlUi)
for Open 3D Engine. It provides asset-driven RmlUi documents, Atom rendering,
input handling, and O3DE component and scripting APIs while following O3DE
development and coding standards.

AtomRml is open-source software distributed under the [MIT License](LICENSE).

> [!WARNING]
> AtomRml is under active development. APIs and behavior may evolve between releases.

Development repository: [Atmosaero/AtomRml](https://github.com/Atmosaero/AtomRml).

## Dependencies

AtomRml builds against RmlUi 6.2, pinned to commit
`2230d1a6e8e0848ed87a5761e2a5160b2a175ba4` for reproducible builds.

## Document component

Add **Rml Document Asset Ref** to an entity and select a processed `.rml` file
in **Document pathname**. The component accepts only
`AtomRmlDocumentAsset` products; it does not store or resolve a free-form path.

With **Auto Load** enabled, the component asynchronously loads the asset through
the O3DE Asset Manager and creates and displays the RmlUi document in the primary
AtomRml context when the entity activates.
Relative stylesheets and textures are resolved from the selected document's
asset path. `Assets/Samples/hello.rml` is included as a minimal smoke-test UI.

## Fonts

AtomRml loads fonts from the merge-friendly `/O3DE/AtomRml/Fonts` Settings
Registry object. Each named entry contains `path`, `fallback`, and `faceIndex`.
The name controls load order, so numeric prefixes are recommended. Gems and
projects can add entries without replacing existing ones:

```json
{
    "O3DE": {
        "AtomRml": {
            "Fonts": {
                "20_ProjectFallback": {
                    "path": "Fonts/ProjectFallback.ttc",
                    "fallback": true,
                    "faceIndex": 2
                }
            }
        }
    }
}
```

Font paths are product paths resolved by AtomRml's RmlUi file interface. The gem
ships registry entries for O3DE's Vera and Vera Mono fonts. The former built-in
console and its bundled font have been removed; RmlUi's debugger remains
available with `F9`.

## Debug logging

Detailed AtomRml lifecycle, rendering, resource, and RmlUi informational logs
use the tagged O3DE logger and are disabled by default. Enable or disable them
from the console with:

```text
LoggerSystemComponent.EnableLog AtomRml
LoggerSystemComponent.DisableLog AtomRml
```

Warnings and errors are always reported through `AZLOG_WARN` and `AZLOG_ERROR`.

## Document Asset Ref API

`AtomRmlDocumentAssetRefBus` is addressed by `EntityId`; every call targets the
entity containing the Rml Document Asset Ref component. It exposes `SetPath`
with an `AssetId`, `SetAutoLoad`, `Remove`, and `Show` to C++, Lua, and Script
Canvas.

## Action notifications

Inline event values are action names. For example, the following button emits
`BuyItem` when clicked and exposes `item-id` as a custom argument:

```xml
<button id="buy-button" onclick="BuyItem" data-item-id="health_potion">Buy</button>
```

Subscribe to `AtomRmlActionNotificationBus` at the entity containing the Rml
Document Asset Ref component. `OnAction` receives an `AtomRmlActionEvent` with
the action and event names, source and target element identity, input value,
checked state, pointer/key data, modifier flags, and custom arguments through
`HasArgument` and `GetArgument`.

```lua
function Example:OnActivate()
    self.actionHandler = AtomRmlActionNotificationBus.Connect(self, self.entityId)
end

function Example:OnAction(event)
    Debug.Log(event.ActionName .. " from " .. event.ElementId)
end
```

`Assets/Samples/action-notification.lua` and `Assets/Samples/buttons.rml`
provide a complete sample. No RmlUi DOM pointers cross the public bus boundary.

## Tests

Runtime and editor smoke tests are available on Windows and Linux as the
`AtomRml.Tests` and `AtomRml.Editor.Tests` CMake targets. After building the
targets, run them through CTest with `ctest -C profile -R AtomRml` from the
configured O3DE build directory.

## Acknowledgements

AtomRml is based on the original TuRml project created by [Reece Hagan (Tuyuji)](https://github.com/Tuyuji). Special thanks to Reece for developing the original O3DE and RmlUi integration and making it available to the community.
