# class FileSystem

*Defined at [files/file_system.hpp:30](../../../../files/file_system.hpp#L30)*



**brief** The framework's porting layer for storage.

 This is one of the few interfaces squared declares on purpose. A second concrete implementation is guaranteed by the architecture rather than speculative: Android internal storage is an asset bundle inside the APK and is not reachable through POSIX, while Termux and desktop Linux are ordinary directories. Everything above this class names a FileType and a relative path and never learns which backend answered.

 Implementations are not required to be thread-safe.



## Functions

### FileSystem

*public void FileSystem()*

*Defined at [files/file_system.hpp:33](../../../../files/file_system.hpp#L33)*

### ~FileSystem

*public void ~FileSystem()*

*Defined at [files/file_system.hpp:34](../../../../files/file_system.hpp#L34)*

### FileSystem

*public void FileSystem(const FileSystem & )*

*Defined at [files/file_system.hpp:36](../../../../files/file_system.hpp#L36)*

### operator=

*public FileSystem & operator=(const FileSystem & )*

*Defined at [files/file_system.hpp:37](../../../../files/file_system.hpp#L37)*

### FileSystem

*public void FileSystem(FileSystem && )*

*Defined at [files/file_system.hpp:38](../../../../files/file_system.hpp#L38)*

### internal

*public FileHandle internal(std::string_view path)*



**brief** Return a handle under the read-only bundled assets. 

### local

*public FileHandle local(std::string_view path)*



**brief** Return a handle under this application's private storage. 

### external

*public FileHandle external(std::string_view path)*



**brief** Return a handle under shared storage. 

### absolute

*public FileHandle absolute(std::string_view path)*



**brief** Return a handle for a path used exactly as given. 

### operator=

*public FileSystem & operator=(FileSystem && )*

*Defined at [files/file_system.hpp:39](../../../../files/file_system.hpp#L39)*

### resolve

*public FileHandle resolve(FileType type, std::string_view path)*



**brief** Return a handle under an explicitly chosen root. 

### exists

*public bool exists(FileType type, std::string_view path)*



**brief** Report whether anything exists at a path. 

### is_directory

*public bool is_directory(FileType type, std::string_view path)*



**brief** Report whether a path names a directory. 

### length

*public std::uint64_t length(FileType type, std::string_view path)*



**brief** Return a file's size, or zero when it is missing or a directory. 

### read

*public FileReadResult read(FileType type, std::string_view path)*



**brief** Read a whole file. 

### write

*public FileError write(FileType type, std::string_view path, std::span<const std::byte> bytes, bool append)*



**brief** Write a whole file, truncating or appending. 

### list

*public FileError list(FileType type, std::string_view path, std::vector<std::string> & names)*



**brief** Fill names with one entry name per directory entry. 

### make_directories

*public FileError make_directories(FileType type, std::string_view path)*



**brief** Create a directory and any missing parents. 

### remove

*public FileError remove(FileType type, std::string_view path)*



**brief** Remove a file, or an empty directory. 



