# namespace files

## Records

* [AndroidAssetFileSystem](AndroidAssetFileSystem.md)
* [AndroidStorageRoots](AndroidStorageRoots.md)
* [EmbeddedFileSystem](EmbeddedFileSystem.md)
* [EmbeddedFile](EmbeddedFile.md)
* [FileError](FileError.md)
* [FileHandle](FileHandle.md)
* [FileReadResult](FileReadResult.md)
* [FileSystem](FileSystem.md)
* [FileTextResult](FileTextResult.md)
* [PosixFileSystemRoots](PosixFileSystemRoots.md)
* [PosixFileSystem](PosixFileSystem.md)


## Enums

| enum class FileErrorCode |

--

| None |
| NotFound |
| NotADirectory |
| IsADirectory |
| PermissionDenied |
| ReadOnly |
| TooLarge |
| AlreadyExists |
| InvalidPath |
| NotSupported |
| IoFailure |


*Defined at [files/src/file_error_code.hpp:9](../../../../files/src/file_error_code.hpp#L9)*



**brief** Stable error categories produced by file system operations. 

| enum class FileType |

--

| Internal |
| Local |
| External |
| Absolute |


*Defined at [files/src/file_type.hpp:17](../../../../files/src/file_type.hpp#L17)*



**brief** Where a path is rooted.

 The same relative path means a different file under each type, and the platform decides what each root is. Android internal is the read-only asset bundle inside the APK, which is not a POSIX directory; on a desktop or in Termux it is a directory beside the executable. Code above the file system names a type and a relative path and never learns which it got.

