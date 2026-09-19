# squared &mdash; developer documentation

For people maintaining and extending squared. Programmer-facing API
documentation is in [../programmer/README.md](../programmer/README.md).

## Cross-cutting

- [File layout](file-layout.md) &mdash; the one-type-per-file rule and the
  self-containment invariant
- [Memory profile](memory-profile.md) &mdash; measured `sizeof` of every core
  type, and what allocates
- [Priority audit](priority-audit.md) &mdash; where the tree contradicts the
  project's own priority order
- [Standard library deviations](standard-library-deviations.md)
- [Extension policy](extension-policy.md) &mdash; why squared adds fewer
  extension points than a linked library would, and what that obliges
- [Design patterns](design-patterns.md)
- [Build and toolchain](build-and-toolchain.md)
- [Refactor notes](refactor-notes.md) &mdash; how the split was done and how it
  was verified

## Modules

- [app](app.md)
- [assets](assets.md)
- [data](data.md)
- [files](files.md)
- [gles](gles.md)
- [graphics](graphics.md)
- [graphics2d](graphics2d.md)
- [scene2d](scene2d.md)
- [gui](gui.md)
- [messaging](messaging.md)
- [math](math.md)
- [time](time.md)
