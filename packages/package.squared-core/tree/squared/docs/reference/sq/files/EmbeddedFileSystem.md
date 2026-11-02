# class EmbeddedFileSystem

*Defined at [files/embedded_file_system.cppm:38](../../../../files/embedded_file_system.cppm#L38)*

Inherits from FileSystem



## Functions

### EmbeddedFileSystem

*public void EmbeddedFileSystem(std::span<const EmbeddedFile> files)*

*Defined at [files/embedded_file_system.cppm:41](../../../../files/embedded_file_system.cppm#L41)*

### exists

*public bool exists(FileType type, std::string_view path)*

### files

*public std::span<const EmbeddedFile> files()*

*Defined at [files/embedded_file_system.cppm:79](../../../../files/embedded_file_system.cppm#L79)*

### is_directory

*public bool is_directory(FileType type, std::string_view path)*

### length

*public std::uint64_t length(FileType type, std::string_view path)*

### read

*public FileReadResult read(FileType type, std::string_view path)*



**brief** A copy of the file's bytes; `NotFound` for none, `NotSupported` outside `Internal`. 

### write

*public FileError write(FileType type, std::string_view path, std::span<const std::byte> bytes, bool append)*



**brief** Refused: `ReadOnly`. 

### write_atomically

*public FileError write_atomically(FileType type, std::string_view path, std::span<const std::byte> bytes)*



**brief** Refused: `ReadOnly`. 

### list

*public FileError list(FileType type, std::string_view path, std::vector<std::string> & names)*



**brief** The names directly under `path`, files and directories, sorted, each once. 

### make_directories

*public FileError make_directories(FileType type, std::string_view path)*



**brief** Refused: `ReadOnly`. 

### remove

*public FileError remove(FileType type, std::string_view path)*



**brief** Refused: `ReadOnly`. 

### resolve_platform_path

*public std::string resolve_platform_path(FileType type, std::string_view path)*



**brief** Empty: embedded files have no path of their own. 



