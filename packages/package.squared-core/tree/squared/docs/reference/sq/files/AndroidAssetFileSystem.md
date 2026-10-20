# class AndroidAssetFileSystem

*Defined at [files/android_asset_file_system.hpp:58](../../../../files/android_asset_file_system.hpp#L58)*



**brief** FileSystem over an APK's assets and the app's private storage.

 The second implementation the porting layer was shaped for. `Internal` reads through AAssetManager, because assets inside an APK are zip entries rather than files and no POSIX call can reach them. Everything writable is an ordinary directory and is delegated to PosixFileSystem.

 The asset manager is referenced, not owned, and comes from `ANativeActivity::assetManager`. It must outlive this file system.



**note** Names on Android are confusing and worth stating once. Android's "internal storage" is this class's Local; this class's Internal is the read only asset bundle, which Android does not call storage at all.



Inherits from FileSystem



## Functions

### AndroidAssetFileSystem

*public void AndroidAssetFileSystem(AAssetManager & assets, AndroidStorageRoots roots)*



**brief** Bind to an asset manager and the writable roots.

**assets** Asset manager from the activity; must outlive this.

**roots** Directories for Local and External.

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



