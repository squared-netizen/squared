# Squared GUI File Picker developer guide

[Programmer counterpart](../../programmer/squared-gui-file-picker/README.md)

This package is an explicit integration boundary. Core GUI remains portable and
has no HoloDisk edge; the optional file-picker module requires both packages.
The exact dependency graph is in
[file-picker-dependencies.dot](file-picker-dependencies.dot).

`FilePicker` applies Composite by deriving from `Window` and owning only normal
GUI primitives. It borrows two application services: `HoloDrive` supplies
sorted immediate-child enumeration, while `AssetManager` supplies cached typed
file and skin resources. The application still owns mounts, backend texture
creation, and the final selection action.

Directory navigation follows a prepare/commit sequence: list the candidate
path, then replace `current_path`, entries, selection, and child rows only after
success. Rebuilding the vertical list transfers a fresh subtree into the
existing `ScrollPane`; callbacks capture the picker only while that subtree is
owned by it. Enumeration is O(n), lookup by visible name is O(n), and widget
reconstruction is O(n).

The text loader is a Strategy registered under the package-private public asset
type. Skin import delegates to GUI's existing transactional loader. An
AlreadyExists result is accepted so several pickers can share one manager.
Exceptions are converted into `SkinLoadIssue` records at the noexcept helper
boundary.

Rejected alternatives: making all GUI packages depend on HoloDisk; direct host
filesystem enumeration; an independent dialog hierarchy; and embedding SDL or
OpenGL texture loading. Current limitations are synchronous enumeration, no
save/overwrite mode, and no asynchronous previews.
