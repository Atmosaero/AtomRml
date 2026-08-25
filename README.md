# AtomRml

Adds RmlUi to O3DE.

> [!WARNING]
> This gem isn't finished, you will experience issues.

Development repository: [Atmosaero/AtomRml](https://github.com/Atmosaero/AtomRml).

## Dependencies

AtomRml builds against RmlUi 6.2, pinned to commit
`2230d1a6e8e0848ed87a5761e2a5160b2a175ba4` for reproducible builds.

## Document component

Add **AtomRml Document** to an entity and select a processed `.rml` file in
the **RML document** asset picker. The component accepts only
`AtomRmlDocumentAsset` products; it does not store or resolve a free-form path.

When the entity activates, the component asynchronously loads the asset through
the O3DE Asset Manager and creates the RmlUi document in the primary AtomRml
context. **Auto show** controls whether the document is shown immediately.
Relative stylesheets and textures are resolved from the selected document's
asset path. `Assets/Samples/hello.rml` is included as a minimal smoke-test UI.

## Tests

Runtime and editor smoke tests are available on Windows and Linux as the
`AtomRml.Tests` and `AtomRml.Editor.Tests` CMake targets. After building the
targets, run them through CTest with `ctest -C profile -R AtomRml` from the
configured O3DE build directory.
