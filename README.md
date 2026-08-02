# Squared

Squared is a modular C++20 application framework distributed as immutable
`.sq` packages. It targets Android, Linux, and Windows while keeping platform
and frontend selection outside portable framework APIs.

This repository is the authoritative development home for:

- Squared Application
- Squared Data
- Squared Graphics
- Squared Graphics2D
- Squared Math
- Squared Messaging
- Squared Scene2D
- Squared Time

The initial source snapshot is extracted unchanged from Squared Project
Generator 0.6.0-dev.6. The corresponding packages remain frozen in
`squared-pg` as bootstrap and integration fixtures.

## Build and test

Run the complete host test workflow from the repository root:

```sh
cmake --workflow --preset test
```

The host suite intentionally tests the portable framework slices. SDL2 and
OpenGL integration remains covered by generated Android project smoke tests.

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

## Repository boundary

This repository does not own project templates, generator code, the private
Lua toolchain, Gradle orchestration, or the `.sq` archive implementation.
Those remain in `squared-pg`.

The framework repository contains no replacement package or project-builder
scripts. Framework packages are built, verified, registered, resolved, and
composed using the installed `squared-pg` commands.
