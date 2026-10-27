# class FileHandle

*Defined at [files/file_handle.hpp:35](../../../../files/file_handle.hpp#L35)*



**brief** A named location, modelled on libGDX's FileHandle.

 A handle is a value: a file system, a root type, and a relative path. It is 48 bytes, it holds no descriptor, and constructing one touches nothing, so building handles in a loop costs only the path string. Nothing is opened until an operation is called, and nothing stays open after one returns.

 The file system is referenced, not owned, and must outlive every handle resolved from it. Handles are freely copyable and comparable.



**note** Every failing operation returns its failure. None of them throw, and none of them assert on a missing file — a missing file is an ordinary result here, not a programmer error.



## Functions

### FileHandle

*public void FileHandle(FileSystem & system, FileType type, std::string path)*



**brief** Bind a handle to a file system, a root, and a relative path.

**system** File system that outlives this handle.

**type** Root the path is relative to.

**path** Relative path using '/' separators; empty means the root.

**note** Prefer FileSystem::internal() and its siblings, which supply the first two arguments.

### FileHandle

*public void FileHandle()*

*Defined at [files/file_handle.hpp:39](../../../../files/file_handle.hpp#L39)*



**brief** Construct an invalid handle that answers no operation. 

### name

*public std::string_view name()*



**brief** Return the final path segment, extension included. 

### extension

*public std::string_view extension()*



**brief** Return the extension without its dot, or empty when there is none. 

### name_without_extension

*public std::string_view name_without_extension()*



**brief** Return the final path segment without its extension. 

### child

*public FileHandle child(std::string_view name)*



**brief** Return a handle to an entry inside this directory.

**name** Relative path to append; '/' separators are accepted.

**return** A handle under the same file system and root. No check is made that this handle is a directory or that the child exists.

### sibling

*public FileHandle sibling(std::string_view name)*



**brief** Return a handle to an entry beside this one.

**name** Relative path to append to this handle's parent.

**return** A handle under the same file system and root.

### parent

*public FileHandle parent()*



**brief** Return a handle to the containing directory.

**return** The parent handle, or a handle to the root when this is already a top-level entry.

### exists

*public bool exists()*



**brief** Report whether anything exists at this path. 

### is_directory

*public bool is_directory()*



**brief** Report whether this path names a directory. 

### length

*public std::uint64_t length()*



**brief** Return the file's size in bytes.

**return** Size in bytes, or zero when the file is missing, unreadable, or a directory. Use exists() to tell an empty file from a missing one.

### valid

*public bool valid()*

*Defined at [files/file_handle.hpp:52](../../../../files/file_handle.hpp#L52)*



**brief** Report whether this handle is bound to a file system. 

### type

*public FileType type()*

*Defined at [files/file_handle.hpp:55](../../../../files/file_handle.hpp#L55)*



**brief** Return the root this handle's path is relative to. 

### path

*public const std::string & path()*

*Defined at [files/file_handle.hpp:58](../../../../files/file_handle.hpp#L58)*



**brief** Return the relative path, with '/' separators. 

### read_bytes

*public FileReadResult read_bytes()*



**brief** Read the whole file.

**return** The bytes, or a FileError describing why not.

### read_string

*public FileTextResult read_string()*



**brief** Read the whole file as text.

**return** The contents, or a FileError describing why not. The bytes are returned unvalidated and untranscoded.

### write_bytes

*public FileError write_bytes(std::span<const std::byte> bytes, bool append)*



**brief** Write bytes to this path, creating the file if needed.

**bytes** Contents to write.

**append** Append rather than truncate.

**return** A no-error FileError on success, otherwise the failure.

**note** Missing parent directories are created first, as libGDX does.

**note** Writing to FileType::Internal fails with ReadOnly on every platform, so a build that works in Termux does not surprise you on a device where assets live inside the APK.

### write_string

*public FileError write_string(std::string_view text, bool append)*



**brief** Write text to this path, creating the file if needed.

**text** Contents to write.

**append** Append rather than truncate.

**return** A no-error FileError on success, otherwise the failure.

### write_atomically

*public FileError write_atomically(std::span<const std::byte> bytes)*



**brief** Write bytes to this path so that it holds the old bytes or the new, never a part (`FileSystem::write_atomically`): for a save the process may die during. Missing parent directories are created first.

**return** A no-error FileError on success, otherwise the failure; the file as it was, when there was one.

### write_atomically

*public FileError write_atomically(std::string_view text)*



**brief** Write text so, as `write_atomically( bytes )`. 

### list

*public FileError list(std::vector<FileHandle> & entries)*



**brief** List the entries of this directory.

**entries** Cleared, then filled with one handle per entry, in no guaranteed order. `.` and `..` are excluded.

**return** A no-error FileError on success, otherwise the failure.

**note** The caller owns the vector so it can be reused across calls, which is why this is not a value-returning list().

### make_directories

*public FileError make_directories()*



**brief** Create this directory and any missing parents.

**return** A no-error FileError on success, including when the directory already exists.

### remove

*public FileError remove()*



**brief** Remove this file, or this directory when it is empty.

**return** A no-error FileError on success, NotFound when nothing was there.

### operator==

*public bool operator==(const FileHandle & other)*



**brief** Compare two handles by file system, root and path. 



