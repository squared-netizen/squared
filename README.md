# Squared

Squared is a rapidly evolving C++ framework and development ecosystem.

This repository holds squared's cartridges: the framework, `package.squared-core`,
with the templates, kits and assets that sqpg (squared-pg) generates workspaces
from.

**Packages** are squared-core and its add-on packages, `squared-`*name*:
`package.squared-tui` (character-cell programs, `import sq.tui;`) is the
first, with its templates `template.sq_tui.cpp` (a program) and
`template.sq_tui.cpp.edu` (a learner's first screen); more are planned in
`docs/design/`. Nothing else is a package: an application, such
as sqterm, can have a template, and a kit wraps a third-party library.
sqterm's `pkg` installs programs into its own prefix; those are
*sqterm packages*, a different thing that shares the word.

Design documents are in `docs/design/`:
[squared-tui](docs/design/squared-tui.md), character-cell programs.

```text
packages/package.squared-core   the framework (squared/: build.lua, its modules, their tests)
packages/package.squared-tui    sq.tui, on squared-core's sq.term and sq.keys
templates/                      what sqpg makes workspaces from
kits/                           third-party libraries and platform bridges
```

## Status

Squared is undergoing a ground-up rewrite and is currently unstable.

The project is in active development and APIs, architecture, tooling, and
project structure may change substantially without notice. It should not
currently be considered stable.

## Development

The `main` branch contains the current implementation.

Previous experimental implementations are preserved in the repository's
archive branches for historical reference only. They are not supported.

## Contributing

The project is currently maintained primarily by its owner. Contributions
and discussion are welcome, but the architecture is evolving rapidly.
For substantial changes, please open an issue before submitting a pull
request.

See CONTRIBUTING.md for contribution guidelines.

## Support

Only the current implementation on `main` is considered supported.

See SUPPORT.md for the project's support policy.

## License

See LICENSE.
