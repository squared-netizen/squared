# Squared Framework Agent Instructions

## Repository role

This repository is the authoritative development home for the Squared C++20
framework and its immutable `.sq@@ module packages. It owns framework source,
public headers, host tests, package manifests, package histories and TODOs,
the GUI showcase source, and built package archives. It does not own project
generation or generated application repositories.

The working tree may contain a large, tested, uncommitted milestone. Preserve
all existing tracked and untracked work. Never reset, clean, checkout over,
delete, or regenerate files merely to make the tree resemble `origin/main`.

## Current documentation milestone

Document the complete current working tree, including the Application text
input additions, Scene2D input/focus work, Graphics and Graphics2D changes,
Squared GUI through `0.6.0-dev.10`, transactional skin loading,
the pinned `gdx-holo` assets, and the GUI showcase.

Do not create Text Editor, Console, Canvas, or File Dialog applications yet.
Component-focused examples wait until the full widget library is complete.
Record that gate in the roadmap. Text Editor and Console must eventually be
Composite widgets assembled from primitive Squared UI widgets, not a parallel
class hierarchy. File Dialog must use primitive widgets before later HoloDisk
integration.

This pass is documentation and documentation infrastructure work. Do not
change runtime behavior unless a build-breaking documentation integration
error requires a minimal fix; report any such fix separately.

## Required first steps

1. Read `README.md`, `CHANGELOG.md`, every package `manifest.json`,
   `history.md` and `TODO.md`, the existing package docs, public headers,
   CMake files, and relevant tests.
2. Run `git status --short --branch`, `git diff --stat`,
   `git diff --check`, and list untracked files.
3. Treat the existing diff as user-owned source of truth.
4. Determine actual module dependencies from manifests and CMake. Never guess
   dependency edges.
5. Present a short documentation plan and package-version impact before
   changing any immutable package coordinate or rebuilding `dist/*.sq`.

## C++ and Lua standards

- Target C++20 exclusively.
- Prefer the C++ standard library over custom or external utilities.
- Use RAII, value semantics, `std::unique_ptr` for exclusive ownership,
  immutable shared resources where justified, and no manual `new`/`delete`.
- Apply `const`, `noexcept`, and `[[nodiscard]]` accurately.
- Target Lua 5.4 and keep the C++/Lua boundary explicit and narrow.
- Do not introduce a dependency during documentation work.

## Documentation architecture

Maintain two parallel top-level trees:

- `docs/programmer/` — public API usage for game/application programmers.
- `docs/developer/` — implementation, maintenance, and extension details.

Each tree must contain a `README.md` index. Mirror subsystem paths where
practical, for example:

- `docs/programmer/gui/GUI.md`
- `docs/developer/gui/GUI.md`

Every paired page must link to its counterpart.

### Programmer documentation

For every public class, struct, enum, function, method, callback, configuration
entry, lifecycle hook, and Lua binding, document:

- purpose;
- parameters and units;
- return value;
- ownership and lifetime;
- preconditions and postconditions;
- exceptions or error results;
- threading expectations;
- a minimal C++20 example;
- the Lua equivalent, or an explicit statement that no Lua binding exists.

Do not expose private members, internal containers, implementation algorithms,
or backend details in the programmer tree.

### Developer documentation

For every subsystem, document:

- architecture and dependency boundaries;
- ownership and threading model;
- invariants and failure behavior;
- data structures, complexity, and selection rationale;
- algorithms and important execution order;
- design patterns by name, why they fit, alternatives rejected, and any
  deviation from the textbook form;
- limitations and technical debt.

Patterns already relevant to GUI include Composite, Strategy, Facade, Template
Method, and Observer. Confirm each against the implementation before
documenting it.

Existing `packages/*/content/docs/` files are package payloads. Preserve them
until their packaging role is audited. Cross-link or synchronize them with the
new top-level trees; do not silently move or delete them.

## Doxygen

Add or update a root `Doxyfile`. Enable Markdown and Graphviz support with
`HAVE_DOT = YES`. Keep generated output under `build/docs/` and do not
commit generated HTML or SVG unless an existing repository policy requires it.

Audit every project-owned public header. Add useful Doxygen comments without
changing declarations or ABI. Public-symbol comments must cover:

- `@brief`;
- `@param` including units, valid ranges, and ownership;
- `@return`;
- `@throws` or an explicit no-throw contract where useful;
- lifetime and thread-safety constraints;
- preconditions and postconditions.

Do not document third-party code. Keep comments factual and derived from
implementations and tests. Configure warnings so undocumented public API and
broken references are visible. Do not hide warnings merely to obtain a green
run.

## Graphviz

Store editable `.dot@@ sources under `docs/developer/architecture/` and
subsystem directories. At minimum provide:

- the framework package dependency graph;
- the GUI implementation dependency graph;
- the GUI input/focus event-flow graph;
- the skin-loading transaction graph.

Derive package edges from exact `manifest.json` requirements. Distinguish
compile-time/package dependencies from application-owned runtime adapters.
HoloDisk remains optional and must not be shown as a GUI dependency before the
actual integration exists.

Validate every graph with `dot -Tsvg`.

## Immutable package rules

- Never overwrite an existing published `.sq` archive or reuse its version.
- Documentation or Doxygen changes inside package content change package bytes.
- Before versioning affected packages, show the user the impact.
- If authorized, advance versions, dependency coordinates, manifests,
  histories, changelog entries, tests, and new archives consistently.
- Do not sync current framework source backward into frozen `squared-pg`
  bootstrap fixtures.

## Verification

Use existing repository workflows first. At minimum run:

```sh
git diff --check
cmake --workflow --preset test
doxygen Doxyfile
dot -Tsvg docs/developer/architecture/package-dependencies.dot -o build/docs/package-dependencies.svg
```

Also run package-doc tests and each additional Graphviz command introduced by
the change. If Android-only validation cannot run, report the exact unrun
command and reason. Never claim a test passed without its output.

## Git safety

- Work on the current repository; never create another repository.
- Do not initialize Git.
- Do not stage, commit, push, tag, publish, or open a pull request unless the
  user explicitly asks after reviewing the diff.
- Do not discard or rewrite existing work.
- End with changed files, tests run, failures, warnings, and package-version
  consequences.

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
