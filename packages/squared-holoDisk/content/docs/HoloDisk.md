# Squared HoloDisk

Squared HoloDisk is an optional, standalone ZIP cartridge extension. It has no
dependency on Squared Application, Data, Graphics, SDL, Lua, or another
Squared module. Its only implementation dependency is the vendored miniz
source shipped inside its SQ package.

## Boundary

`HoloDriveFactory` creates a drive. From that point onward, `HoloDrive` is the
only operational API boundary. Disk images, mounts, streams, host files, and
ZIP implementation details remain private to the drive.

The compact boundary supports:

- creating an empty emulated HoloDisk;
- loading an existing ZIP HoloDisk;
- mounting and unmounting disks in one virtual path namespace;
- opening, streaming, seeking, and closing mounted files;
- listing immediate directory children;
- materializing the current emulated state to a ZIP HoloDisk; and
- discarding a loaded or emulated disk.

## Storage behavior

ZIP directory metadata is indexed when a disk is loaded, but file payloads are
not expanded into RAM. Archive reads are streamed through miniz. Mutations are
stored in a drive-owned scratch directory and are not written back implicitly.
`write_holodisk()` creates a temporary ZIP beside the destination and installs
it only after finalization succeeds. Destroying the drive discards every
unmaterialized mutation.

All archive paths are portable relative paths. Absolute paths, backslashes,
empty segments, and `.` or `..` traversal segments are rejected.

## Ownership and limits

Clients receive only opaque `DiskId`, `MountId`, and `FileId` values. A drive
owns the corresponding state. `DriveOptions` bounds disk, mount, open-file,
entry, per-file, and expanded-data counts to keep untrusted cartridges from
creating unbounded work.
