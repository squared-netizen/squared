# class MappedFile

*Defined at [files/mapped_file.cppm:21](../../../../files/mapped_file.cppm#L21)*



**brief** A file's bytes, read in place for as long as this lives: a font FreeType reads, a table looked up now and then. Move-only; given back by its destructor.

 In place means the file's own pages: `mmap` of a file, an asset stored uncompressed in the APK. Those pages are clean, so Android can drop them under memory pressure and read them again, and only the parts touched are resident. A compressed asset cannot be mapped: Android unpacks it into memory of its own, and `in_place()` says so.



## Functions

### MappedFile

*public void MappedFile()*

*Defined at [files/mapped_file.cppm:29](../../../../files/mapped_file.cppm#L29)*



**brief** Nothing mapped. 

### MappedFile

*public void MappedFile(MappedFile && other)*

### MappedFile

*public void MappedFile(const MappedFile & )*

*Defined at [files/mapped_file.cppm:40](../../../../files/mapped_file.cppm#L40)*

### MappedFile

*public void MappedFile(std::span<const std::byte> bytes, bool in_place, Release release, void * handle)*



**brief** What a file system hands out: `bytes`, read in place or not, given back by `release( handle, data, size )` (null: nothing to give back, as for embedded files).

### bytes

*public std::span<const std::byte> bytes()*

*Defined at [files/mapped_file.cppm:46](../../../../files/mapped_file.cppm#L46)*



**brief** The file's bytes; empty for an empty file, or nothing mapped. 

### in_place

*public bool in_place()*

*Defined at [files/mapped_file.cppm:52](../../../../files/mapped_file.cppm#L52)*



**brief** The file's own pages, not a copy in memory: false for a compressed asset. 

### operator=

*public MappedFile & operator=(MappedFile && other)*

### operator=

*public MappedFile & operator=(const MappedFile & )*

*Defined at [files/mapped_file.cppm:41](../../../../files/mapped_file.cppm#L41)*

### reset

*public void reset()*



**brief** Give the mapping back now; nothing mapped after. 

### valid

*public bool valid()*

*Defined at [files/mapped_file.cppm:49](../../../../files/mapped_file.cppm#L49)*



**brief** Whether a file is mapped (an empty file is). 

### ~MappedFile

*public void ~MappedFile()*

