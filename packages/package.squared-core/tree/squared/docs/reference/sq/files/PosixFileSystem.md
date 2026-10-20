# class PosixFileSystem

*Defined at [files/posix_file_system.hpp:42](../../../../files/posix_file_system.hpp#L42)*



**brief** FileSystem over ordinary directories.

 This is the backend for Termux and desktop Linux, and it is the one used on Android for Local and External storage. Android Internal storage lives inside the APK and needs the asset backend instead.

 Internal is refused for writes on every platform, so a build that works against a directory in Termux behaves the same on a device where the same files are read-only inside an APK.



Inherits from FileSystem



## Functions

### PosixFileSystem

*public void PosixFileSystem(PosixFileSystemRoots roots)*



**brief** Bind each root to a directory.

**roots** Directories for Internal, Local and External. An empty root makes every path under that type fail with NotSupported rather than resolving somewhere unintended.

### exists

*public bool exists(FileType type, int path)*

### is_directory

*public bool is_directory(FileType type, int path)*

### length

*public int length(FileType type, int path)*

### read

*public FileReadResult read(FileType type, int path)*

### write

*public FileError write(FileType type, int path, int bytes, bool append)*

### list

*public FileError list(FileType type, int path, int & names)*

### make_directories

*public FileError make_directories(FileType type, int path)*

### remove

*public FileError remove(FileType type, int path)*

### resolve_platform_path

*public int resolve_platform_path(FileType type, int path)*



**brief** Return the platform path a type and relative path resolve to.

**type** Root to resolve against.

**path** Relative path, or the whole path for FileType::Absolute.

**return** The resolved path, or an empty string when the root is unset or the relative path escapes it.

**note** Exposed for diagnostics and tests. Code above the file system should not need it.



