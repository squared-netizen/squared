# Squared Application Package Agent Instructions

## Precedence and safety

This package has a nested `AGENTS.md`, so OpenCode may select it instead of
the repository-root file. Before planning or editing, read
`../../AGENTS.md` completely and obey its ecosystem-wide architecture,
documentation, immutable-package, verification, and Git-safety rules.

Preserve every existing tracked and untracked change. Never reset, clean,
checkout over, delete, stage, commit, push, publish, or overwrite an existing
`.sq` archive without explicit user authorization.

## Package role

Defines the portable application lifecycle and the framework event boundary, including the current text-input additions.

## Documentation task

Document the package as it exists in the current working tree. Read its
`manifest.json`, `history.md`, `TODO.md`, package payload documentation,
public headers, implementations, CMake target, tests, and direct consumers
before writing.

Maintain both:

- root programmer page: `docs/programmer/squared-application/README.md`;
- root developer page: `docs/developer/squared-application/README.md`.

Cross-link the pair. Preserve `content/docs/` because it is packaged into
generated projects. Synchronize its public guidance with the programmer page
without putting private implementation detail into package payload docs.

## Programmer documentation and Doxygen

Document `Application`, lifecycle hooks, `Event` and every event payload/variant, text-input contracts, pointer identifiers, resize semantics, and error or shutdown behavior.

For every project-owned public symbol, add accurate Doxygen coverage:
`@brief`, parameters and units/ranges, return values, ownership and lifetime,
preconditions/postconditions, exceptions or errors, thread-safety, and a
minimal C++20 example. State whether a Lua equivalent exists. Do not alter API
declarations merely to simplify documentation and do not document third-party
symbols as Squared APIs.

## Developer documentation

Document event representation and dispatch, lifecycle ordering, ownership, stable pointer IDs, UTF-8 text-input boundaries, backend responsibilities, invariants, and failure behavior.

Name every design pattern actually used, explain why it fits, note alternatives
and deviations, and document data-structure complexity. Keep claims grounded
in code and tests.

## Dependency boundary

Keep Application portable. Do not expose SDL, Android, OpenGL, GUI, or backend objects through its public API.

Derive all dependency statements from the manifest, CMake target, includes,
and tests. If they disagree, report the inconsistency instead of inventing a
resolution.

## Graphviz

Add editable Graphviz sources under
`docs/developer/squared-application/` for:

- application lifecycle and callback order;
- event and text-input flow from backend adapter to application consumer;

Validate every `.dot` file with `dot -Tsvg`. Keep rendered output beneath
`build/docs/` unless repository policy explicitly commits it.

## Verification

Run `squared_application_boundary_test` and every root test that consumes Application events.

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
repository-wide `tools/clang-check.fish --full` audit runs in GitHub Actions and is mandatory for
releases, package-version milestones, public API changes, and broad refactors.

Clang warnings are actionable failures. Agents must report work as awaiting CI until the required
workflow passes. Missing local tools or compilation commands must be reported rather than silently
skipped. Global warning suppression is forbidden; any necessary suppression must be narrow,
justified, documented, and reported. Generated, vendored, and third-party source remains excluded
unless explicitly owned by the task.
<!-- squared-clang-policy:end -->
