# class PosixFileSystem

*Defined at [files/posix_file_system.cppm:40](../../../../files/posix_file_system.cppm#L40)*

Inherits from FileSystem



## Functions

### PosixFileSystem

*public void PosixFileSystem(PosixFileSystemRoots roots)*



**brief** Bind each root to a directory.

**roots** Directories for Internal, Local and External. An empty root makes every path under that type fail with NotSupported rather than resolving somewhere unintended.

### exists

*public bool exists(FileType type, std::string_view path)*

### is_directory

*public bool is_directory(FileType type, std::string_view path)*

### length

*public std::uint64_t length(FileType type, std::string_view path)*

### read

*public FileReadResult read(FileType type, std::string_view path)*

### write

*public FileError write(FileType type, std::string_view path, std::span<const std::byte> bytes, bool append)*

### write_atomically

*public FileError write_atomically(FileType type, std::string_view path, std::span<const std::byte> bytes)*



**brief** Written beside it (`.new`), flushed, renamed over it, its directory flushed. 

### list

*public FileError list(FileType type, std::string_view path, std::vector<std::string> & names)*

### make_directories

*public FileError make_directories(FileType type, std::string_view path)*

### remove

*public FileError remove(FileType type, std::string_view path)*

### resolve_platform_path

*public std::string resolve_platform_path(FileType type, std::string_view path)*



**brief** Return the platform path a type and relative path resolve to.

**type** Root to resolve against.

**path** Relative path, or the whole path for FileType::Absolute.

**return** The resolved path, or an empty string when the root is unset or the relative path escapes it.



