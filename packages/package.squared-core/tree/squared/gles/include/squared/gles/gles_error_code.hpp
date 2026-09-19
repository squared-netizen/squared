#pragma once

namespace sq::gles {

/** @brief Stable error categories produced by the GL object layer. */
enum class GlesErrorCode {
    None,
    NoContext,
    OutOfMemory,
    CompileFailed,
    LinkFailed,
    InvalidArgument,
    Unsupported,
    GlError
};

/**
 * @brief Return the identifier of an error code as text.
 * @param code Code to name.
 * @return A static string such as "CompileFailed"; never null.
 * @note For logs and teaching output. The text is the enumerator name, not a
 * sentence, so it is stable enough to grep for.
 */
[[nodiscard]] const char* name_of(GlesErrorCode code) noexcept;

}  // namespace sq::gles
