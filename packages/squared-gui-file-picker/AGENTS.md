# Squared GUI File Picker Agent Instructions

Read `../../AGENTS.md` before editing. This optional integration package owns
the composite HoloDisk-backed file picker. Keep core Squared GUI independent
from HoloDisk. Declare exact GUI and HoloDisk coordinates in the manifest and
mirror them in CMake. Preserve ordinary Widget ownership and injected storage;
do not introduce a second hierarchy or platform filesystem APIs.

Run the file-picker host test, package documentation test, full CMake workflow,
Doxygen, Graphviz validation, and package build verification before release.
Never overwrite an immutable `.sq` coordinate or stage/publish without explicit
authorization.

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
