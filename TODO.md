# Squared Framework TODO

Only cross-package and repository-wide unfinished work belongs here. Keep
package-specific work in the corresponding `packages/*/TODO.md` file.

## Repository ownership

- [ ] Move the maintained `examples/gui-showcase` application to
  `squared-netizen/squared-examples`; reserve `squared-netizen/sandbox` for
  disposable experiments.
- [ ] Before removing the showcase from this repository, migrate its README and
  Doxygen references and retain or extract the smallest self-contained GUI test
  fixture required by framework CI.
- [ ] Audit the framework tree after the move so application-owned examples and
  generated artifacts do not remain in the authoritative package repository.

## Source and package layout

- [ ] Design a repository-wide flattening plan for the excessive
  `packages/<package>/content/modules/<module>/...` authoring depth.
- [ ] Separate the contributor-friendly source layout from the immutable `.sq`
  archive layout; use package staging when necessary instead of making authors
  edit deeply nested payload paths.
- [ ] Preserve public include paths, package coordinates, exact dependency
  semantics, and reproducible content digests during the migration.
- [ ] Migrate one representative package as a measured pilot, update CMake,
  package tooling, tests, and documentation, then apply the proven layout
  consistently.
