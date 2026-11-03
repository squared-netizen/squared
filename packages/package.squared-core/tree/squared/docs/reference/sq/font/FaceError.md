# struct FaceError

*Defined at [font/face_error.cppm:27](../../../../font/face_error.cppm#L27)*



**brief** Why a `Face` call failed: true when it did. Owns no memory: the text is fixed, `freetype_error` is FreeType's own error code.



## Members

public FaceErrorCode code

public int freetype_error



## Functions

### operator bool

*public bool operator bool()*

*Defined at [font/face_error.cppm:32](../../../../font/face_error.cppm#L32)*

### what

*public std::string_view what()*

*Defined at [font/face_error.cppm:37](../../../../font/face_error.cppm#L37)*

