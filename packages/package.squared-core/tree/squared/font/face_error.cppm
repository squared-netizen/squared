export module sq.font:face_error;

import std;

export namespace sq::font
{

/** @brief Which step with a face failed. */
enum class FaceErrorCode : std::uint8_t {
    none,
    /** FreeType itself could not start (out of memory). */
    library,
    /** The file could not be read as a font: missing, or not one FreeType reads. */
    open,
    /** A file holding several faces (a .ttc) has no face of that index. */
    no_such_face,
    /** The face cannot be set to that size (a bitmap font without it), or 0 was asked. */
    size,
    /** No face is open. */
    not_open,
    /** A `FontSet` holds as many faces as it can (`FontSet::max_faces`). */
    set_full,
};

/**
 * @brief Why a `Face` call failed: true when it did. Owns no memory: the
 * text is fixed, `freetype_error` is FreeType's own error code.
 */
struct FaceError
{
        FaceErrorCode code = FaceErrorCode::none;
        int freetype_error = 0;

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return code != FaceErrorCode::none;
        }

        [[nodiscard]] constexpr std::string_view what() const noexcept
        {
            switch( code ) {
                case FaceErrorCode::none:
                    return "no error";
                case FaceErrorCode::library:
                    return "FreeType could not start";
                case FaceErrorCode::open:
                    return "not a font FreeType reads, or not found";
                case FaceErrorCode::no_such_face:
                    return "the file has no face of that index";
                case FaceErrorCode::size:
                    return "the face cannot be set to that size";
                case FaceErrorCode::not_open:
                    return "no face is open";
                case FaceErrorCode::set_full:
                    return "the font set holds as many faces as it can";
            }
            return "unknown";
        }
};

} // namespace sq::font
