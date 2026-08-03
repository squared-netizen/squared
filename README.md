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
- Squared HoloDisk (optional)
- Squared Math
- Squared Messaging
- Squared Scene2D
- Squared Time

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
layout, focus, pointer capture, controls, text editing, framework event
routing, and Graphics2D region drawing remain portable and require no window
or GPU. Its packaged skin fixture uses CC0 Kenney UI Pack images.

## Build packages

Package sources retain the public layout consumed by Squared Project
Generator:

```sh
squared-pg package build \
  packages/squared-math \
  dist/squared-math-0.6.0-dev.1.sq

squared-pg package add \
  dist/squared-math-0.6.0-dev.1.sq
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

## Optional HoloDisk extension

`dev.squarednetizen.squared.holodisk` provides emulated and ZIP-backed
cartridges without becoming a required I/O layer. It has no dependency on any
other Squared package. A factory creates a `HoloDrive`; that drive then owns
disk loading, mounting, mounted file operations, ZIP materialization, and
cleanup behind one compact API boundary.

## Optional GUI extension

`dev.squarednetizen.squared.gui` provides a compact retained-mode widget tree
over Scene2D. `Ui` consumes the framework's existing `application::Event`
directly, including stable pointer IDs and resize events; it does not create a
second event bus. A frontend implements its small painter contract with
portable Graphics and Graphics2D types. GUI itself has no SDL, OpenGL,
Android, HoloDisk, or template dependency. Projects add it explicitly through
the generator's optional project-module workflow.

## Repository boundary

This repository does not own project templates, generator code, the private
Lua toolchain, Gradle orchestration, or the `.sq` archive implementation.
Those remain in `squared-pg`.

The framework repository contains no replacement package or project-builder
scripts. Framework packages are built, verified, registered, resolved, and
composed using the installed `squared-pg` commands.
