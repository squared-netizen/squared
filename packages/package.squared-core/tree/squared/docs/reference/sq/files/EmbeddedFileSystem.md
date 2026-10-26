# class EmbeddedFileSystem

*Defined at [files/embedded_file_system.hpp:40](../../../../files/embedded_file_system.hpp#L40)*



**brief** A read-only FileSystem over files compiled into the program (`tools/embed_files.sh` writes them): what the Stage's default skin is read through, so the atlas, font and skin loaders, and a texture's reload after a lost context, work as they do on bundled files.

 Only `Internal` holds anything; a directory is any path some file is under. Writing is refused. Holds a view of the table, which must outlive it: a generated table is a constant, so it does. A lookup is a linear scan: embedded tables are a handful of files.



Inherits from FileSystem



## Functions

### EmbeddedFileSystem

*public void EmbeddedFileSystem(std::span<const EmbeddedFile> files)*

*Defined at [files/embedded_file_system.hpp:43](../../../../files/embedded_file_system.hpp#L43)*

### exists

*public bool exists(FileType type, std::string_view path)*

### files

*public std::span<const EmbeddedFile> files()*

*Defined at [files/embedded_file_system.hpp:73](../../../../files/embedded_file_system.hpp#L73)*



**brief** The table it reads. 

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

### list

*public FileError list(FileType type, std::string_view path, std::vector<std::string> & names)*



**brief** The names directly under `path`, files and directories, sorted, each once. 

### make_directories

*public FileError make_directories(FileType type, std::string_view path)*



**brief** Refused: `ReadOnly`. 

### remove

*public FileError remove(FileType type, std::string_view path)*



**brief** Refused: `ReadOnly`. 



