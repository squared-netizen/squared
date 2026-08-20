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
