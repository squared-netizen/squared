# Squared HoloDisk Package Agent Instructions

## Precedence and safety

This package has a nested `AGENTS.md`, so OpenCode may select it instead of
the repository-root file. Before planning or editing, read
`../../AGENTS.md` completely and obey its ecosystem-wide architecture,
documentation, immutable-package, verification, and Git-safety rules.

Preserve every existing tracked and untracked change. Never reset, clean,
checkout over, delete, stage, commit, push, publish, or overwrite an existing
`.sq` archive without explicit user authorization.

## Package role

Provides the optional emulated and ZIP-backed cartridge drive through the `HoloDrive` boundary.

## Documentation task

Document the package as it exists in the current working tree. Read its
`manifest.json`, `history.md`, `TODO.md`, package payload documentation,
public headers, implementations, CMake target, tests, and direct consumers
before writing.

Maintain both:

- root programmer page: `docs/programmer/squared-holoDisk/README.md`;
- root developer page: `docs/developer/squared-holoDisk/README.md`.

Cross-link the pair. Preserve `content/docs/` because it is packaged into
generated projects. Synchronize its public guidance with the programmer page
without putting private implementation detail into package payload docs.

## Programmer documentation and Doxygen

Document drive factories, mounting and unmounting, cartridge/file operations, path rules, read/write results, materialization, ownership, limits, error states, cleanup, and examples.

For every project-owned public symbol, add accurate Doxygen coverage:
`@brief`, parameters and units/ranges, return values, ownership and lifetime,
preconditions/postconditions, exceptions or errors, thread-safety, and a
minimal C++20 example. State whether a Lua equivalent exists. Do not alter API
declarations merely to simplify documentation and do not document third-party
symbols as Squared APIs.

## Developer documentation

Document ZIP and scratch storage ownership, miniz boundary, path normalization and traversal defense, streamed reads, write materialization, resource limits, transactionality, cleanup, complexity, and failure recovery.

Name every design pattern actually used, explain why it fits, note alternatives
and deviations, and document data-structure complexity. Keep claims grounded
in code and tests.

## Dependency boundary

HoloDisk is independent and optional. Do not make GUI or another core package depend on it before an explicit integration milestone.

Derive all dependency statements from the manifest, CMake target, includes,
and tests. If they disagree, report the inconsistency instead of inventing a
resolution.

## Graphviz

Add editable Graphviz sources under
`docs/developer/squared-holoDisk/` for:

- drive creation/mount/unmount lifecycle;
- ZIP read and scratch-write flow;
- path validation and rejection flow;

Validate every `.dot` file with `dot -Tsvg`. Keep rendered output beneath
`build/docs/` unless repository policy explicitly commits it.

## Verification

Run `squared_holodisk_test`, including unsafe paths, limits, cleanup, and malformed cartridges.

Also run the repository's documentation generation, package-doc validation,
`git diff --check`, and relevant root CTest targets. Report exact commands,
results, warnings, and anything that could not run. Before any package version
change, present the immutable-version and downstream-coordinate impact to the
user.

<!-- squared-clang-policy:begin -->
## Mandatory Clang workflow

This package inherits the repository-root `.clang-format`, `.clang-tidy`, and `AGENTS.md`.
Whenever project-owned C or C++ files in this package change, agents must configure a compilation
database and run narrow formatter and Clang-Tidy checks on touched files when practical. The
repository-wide `tools/clang-check.fish --full` audit runs in GitHub Actions and reports existing
baseline debt. It must run for releases, package-version milestones, public API changes, and broad
refactors, but remains advisory until the repository-wide baseline is clean.

New warnings in files touched by the task are actionable failures. Existing unrelated baseline
debt does not block an otherwise successful build or task. Agents must report narrow-check results
and the latest CI audit result. Missing tools or compilation commands must be reported rather than
silently skipped. Global warning suppression is forbidden; any necessary suppression must be
narrow, justified, documented, and reported. Generated, vendored, and third-party source remains
excluded unless explicitly owned by the task.
<!-- squared-clang-policy:end -->
