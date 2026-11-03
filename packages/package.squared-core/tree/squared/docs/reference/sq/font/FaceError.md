# struct FaceError

*Defined at [font/face_error.cppm:29](../../../../font/face_error.cppm#L29)*



**brief** Why a `Face` call failed: true when it did. Owns no memory: the text is fixed, `freetype_error` is FreeType's own error code.



## Members

public FaceErrorCode code

public int freetype_error



## Functions

### operator bool

*public bool operator bool()*

*Defined at [font/face_error.cppm:34](../../../../font/face_error.cppm#L34)*

### what

*public std::string_view what()*

*Defined at [font/face_error.cppm:39](../../../../font/face_error.cppm#L39)*

