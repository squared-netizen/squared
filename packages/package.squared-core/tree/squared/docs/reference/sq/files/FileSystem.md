# class FileSystem

*Defined at [files/file_system.cppm:29](../../../../files/file_system.cppm#L29)*



**brief** The framework's porting layer for storage.

 This is one of the few interfaces squared declares on purpose. A second concrete implementation is guaranteed by the architecture rather than speculative: Android internal storage is an asset bundle inside the APK and is not reachable through POSIX, while Termux and desktop Linux are ordinary directories. Everything above this class names a FileType and a relative path and never learns which backend answered.

 Implementations are not required to be thread-safe.



## Functions

### FileSystem

*public void FileSystem()*

*Defined at [files/file_system.cppm:32](../../../../files/file_system.cppm#L32)*

### FileSystem

*public void FileSystem(FileSystem && )*

*Defined at [files/file_system.cppm:37](../../../../files/file_system.cppm#L37)*

### FileSystem

*public void FileSystem(const FileSystem & )*

*Defined at [files/file_system.cppm:35](../../../../files/file_system.cppm#L35)*

### absolute

*public FileHandle absolute(std::string_view path)*



**brief** Return a handle for a path used exactly as given. 

### exists

*public bool exists(FileType type, std::string_view path)*



**brief** Report whether anything exists at a path. 

### external

*public FileHandle external(std::string_view path)*



**brief** Return a handle under shared storage. 

### internal

*public FileHandle internal(std::string_view path)*



**brief** Return a handle under the read-only bundled assets. 

### is_directory

*public bool is_directory(FileType type, std::string_view path)*



**brief** Report whether a path names a directory. 

### length

*public std::uint64_t length(FileType type, std::string_view path)*



**brief** Return a file's size, or zero when it is missing or a directory. 

### list

*public FileError list(FileType type, std::string_view path, std::vector<std::string> & names)*



**brief** Fill names with one entry name per directory entry. 

### local

*public FileHandle local(std::string_view path)*



**brief** Return a handle under this application's private storage. 

### make_directories

*public FileError make_directories(FileType type, std::string_view path)*



**brief** Create a directory and any missing parents. 

### map

*public MapResult map(FileType type, std::string_view path)*



**brief** Map a file to read in place (`MappedFile`): no copy of a file's bytes on the heap where the platform can avoid one.

**type** Which root `path` is under.

**path** The file.

**return** The mapping, or the error (`NotFound`, `IsADirectory`, `NotSupported` where a root has no files to map).

### operator=

*public FileSystem & operator=(FileSystem && )*

*Defined at [files/file_system.cppm:38](../../../../files/file_system.cppm#L38)*

### operator=

*public FileSystem & operator=(const FileSystem & )*

*Defined at [files/file_system.cppm:36](../../../../files/file_system.cppm#L36)*

### read

*public FileReadResult read(FileType type, std::string_view path)*



**brief** Read a whole file. 

### remove

*public FileError remove(FileType type, std::string_view path)*



**brief** Remove a file, or an empty directory. 

### resolve

*public FileHandle resolve(FileType type, std::string_view path)*



**brief** Return a handle under an explicitly chosen root. 

### resolve_platform_path

*public std::string resolve_platform_path(FileType type, std::string_view path)*



**brief** The platform's own path a type and relative path name; empty when there is none (bundled files inside an APK, embedded files, a root not set, or a path that escapes it).

 For what outlives this file system: the App keeps where it saves, and writes there through `FileType::Absolute` from the model's thread after the activity that made this file system is gone.

### write

*public FileError write(FileType type, std::string_view path, std::span<const std::byte> bytes, bool append)*



**brief** Write a whole file, truncating or appending. 

### write_atomically

*public FileError write_atomically(FileType type, std::string_view path, std::span<const std::byte> bytes)*



**brief** Write a whole file so that it holds the old bytes or the new, never a part: written beside it (`<path>.new`), flushed to storage, and renamed over it. What a save that the process may die during needs. The directory must exist.

### ~FileSystem

*public void ~FileSystem()*

*Defined at [files/file_system.cppm:33](../../../../files/file_system.cppm#L33)*

