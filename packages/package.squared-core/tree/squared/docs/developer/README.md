# squared &mdash; developer documentation

For people maintaining and extending squared. Programmer-facing API
documentation is in [../programmer/README.md](../programmer/README.md).

## Cross-cutting

- [File layout](file-layout.md) &mdash; where files go in a module, the
  one-type-per-file rule and the self-containment invariant
- [Memory profile](memory-profile.md) &mdash; measured `sizeof` of every core
  type, and what allocates
- [Priority audit](priority-audit.md) &mdash; where the tree contradicts the
  project's own priority order
- [Standard library deviations](standard-library-deviations.md)
- [Extension policy](extension-policy.md) &mdash; why squared adds fewer
  extension points than a linked library would, and what that obliges
- [Design patterns](design-patterns.md)
- [Build and toolchain](build-and-toolchain.md)
- [Code style](code-style.md) &mdash; CDDA's formatting rules, as
  `squared/.clang-format`, and where squared differs
- [Refactor notes](refactor-notes.md) &mdash; how the split was done and how it
  was verified (history: written before the Sprint 1 layout move)
- [Tools](tools.md) &mdash; the scripts in `squared/tools/` and the workspace
  wrappers

## Modules

- [app](app.md)
- [assets](assets.md)
- [data](data.md)
- [dev](dev.md) &mdash; loopback log sink and crash reporter (development only)
- [files](files.md)
  - [asset-index](asset-index.md) &mdash; how nested APK assets are listed
- [gles](gles.md)
- [graphics](graphics.md) &mdash; includes the former `graphics2d`
- [gui](gui.md) &mdash; includes the former `scene2d`
- [messaging](messaging.md)
- [math](math.md)
- [time](time.md)
