# Squared

Squared is a modular C++20 application framework distributed as immutable
`.sq` packages. It targets Android, Linux, and Windows while keeping platform
and frontend selection outside portable framework APIs.

This repository is the authoritative development home for:

- Squared Application
- Squared SDL2/OpenGL Backend
- Squared Data
- Squared Graphics
- Squared Graphics2D
- Squared GUI (optional)
- Squared GUI File Picker (optional HoloDisk integration)
- Squared HoloDisk (optional)
- Squared Math
- Squared Messaging
- Squared Scene2D
- Squared Time

## Package-owned planning and history

Every source package directory owns two governance documents:

- `history.md` is an append-only, versioned record of completed package work.
- `TODO.md` contains only unfinished package work, with the immediate priority
  under `## Next`.

When work is completed, remove its checkbox from `TODO.md` and record the
result under the released version in that package's `history.md`. These files
live beside `manifest.json`; they govern the package source without being
copied into generated applications or changing an immutable `.sq` payload.
The host test workflow rejects packages that omit or fail to maintain them.

The initial source snapshot was extracted from Squared Project Generator
0.6.0-dev.6. Framework development now advances here while the corresponding
packages in `squared-pg` remain frozen bootstrap and integration fixtures.

## Build and test

Run the complete host test workflow from the repository root:

```sh
cmake --workflow --preset test
```

The host suite tests the portable framework slices and verifies that Graphics
and Graphics2D headers compile without SDL or OpenGL headers. SDL2/OpenGL
integration remains covered by generated Android project smoke tests.

Squared HoloDisk is independently tested as a ZIP cartridge drive and links
only its own sources and vendored miniz implementation.

Squared GUI is independently tested with a recording painter, so skins,
layout, focus, pointer capture, controls, keyboard/IME text editing, framework event
routing, and Graphics2D region drawing remain portable and require no window
or GPU. Nine-patch drawables preserve corners while scaling window, dialog,
and control backgrounds. Floating windows support styled close controls,
edge/corner resizing, minimum sizes, and viewport constraints. Its packaged
skin fixture uses CC0 Kenney UI Pack images and libGDX-compatible split and
padding metadata.

## Build packages

Package sources retain the public layout consumed by Squared Project
Generator:

```sh
squared-pg package build \
  packages/squared-math \
  dist/squared-math-0.6.0-dev.2.sq

squared-pg package add \
  dist/squared-math-0.6.0-dev.2.sq
```

`package build` validates the resulting archive. `package add` validates it
again and transactionally registers it. Published package contents are
immutable; changes require a new version.

## Graphics backend boundary

Squared Graphics and Graphics2D expose portable C++ contracts. Platform
templates select exactly one implementation package at link time. The first
implementation is `dev.squarednetizen.squared.backend.sdl2-opengl`, which owns
SDL2 window/context operations, OpenGL ES resources, and platform asset image
loading. This selection adds no runtime backend registry or per-draw virtual
dispatch.

On Android, the platform adapter stops rendering at the background lifecycle
boundary and marks Graphics2D resources stale without issuing unsafe GL calls.
Foreground recovery reactivates SDL's context or creates a replacement,
advances the portable context generation, and validates or rebuilds texture,
atlas, shader, and buffer objects from retained recipes.
Texture owners select a backend-neutral recovery recipe: reload an asset,
retain RGBA pixels, regenerate pixels through a synchronous callback, or
discard the resource. The GUI showcase regenerates cached text, avoiding
permanent CPU-side copies of its glyph textures.

## Optional HoloDisk extension

The optional `dev.squarednetizen.squared.gui.file-picker` module requires both
GUI and HoloDisk. Adding that coordinate through `squared-pg` resolves the exact
GUI dev.15 and HoloDisk dev.3 packages automatically; core GUI remains usable
without HoloDisk. The picker browses `HoloDrive`, loads typed selections and the
packaged gdx-holo JSON through `AssetManager`, and receives backend drawable and
font resolvers from the application.

`dev.squarednetizen.squared.holodisk` provides emulated and ZIP-backed
cartridges without becoming a required I/O layer. It has no dependency on any
other Squared package. A factory creates a `HoloDrive`; that drive then owns
disk loading, mounting, mounted file operations, ZIP materialization, and
cleanup behind one compact API boundary.

Its application-owned `AssetManager` adds synchronous typed loader strategies,
bounded byte reads, shared caching, dependency/cycle tracking, transactional
reload, and explicit unload without learning any Graphics or GUI type. It can
load a ZIP directly from bounded memory and own read-only nested mounts, so a
pinned archive such as GUI's `gdx-skins.zip` need not be extracted into package
paths. Graphics2D and GUI will register their loaders in later milestones.

## Optional GUI extension

`dev.squarednetizen.squared.gui` provides a compact retained-mode widget tree
over Scene2D. `Ui` consumes the framework's existing `application::Event`
directly, including stable pointer IDs and resize events; it does not create a
second event bus. A frontend implements its small painter contract with
portable Graphics and Graphics2D types. GUI itself has no SDL, OpenGL,
Android, HoloDisk, or template dependency. Projects add it explicitly through
the generator's optional project-module workflow. Its libGDX-inspired `Table`,
`Window`, and `Dialog` layer provides grid constraints, floating panels, and
modal workflows without crossing that portable boundary.

`examples/gui-showcase` contains a complete visual Android test application.
It presents every current widget across multiple movable and resizable windows,
with launcher buttons, modal confirmation and about dialogs, scrolling content,
the packaged nine-patch skin, and an SDL_ttf/SpriteBatch painter frontend.
Its input path is portable above the adapter: Scene2D dispatches capture,
target, and bubble phases, while GUI provides modifier-aware traversal,
modal focus trapping, directional keyboard/controller navigation, activation,
and cancellation.

GUI package installation also carries the SHA-256-pinned gdx-skins archive
into generated-project assets. The showcase uses the selected gdx-holo theme
through GUI's bounded, transactional memory loader. HoloDisk's AssetManager
can mount the pinned archive. The optional file-picker module now loads the
selected skin JSON through that manager while retaining application-injected
atlas, texture, and bitmap-font resolvers.

## Repository boundary

This repository does not own project templates, generator code, the private
Lua toolchain, Gradle orchestration, or the `.sq` archive implementation.
Those remain in `squared-pg`.

The framework repository contains no replacement package or project-builder
scripts. Framework packages are built, verified, registered, resolved, and
composed using the installed `squared-pg` commands.
