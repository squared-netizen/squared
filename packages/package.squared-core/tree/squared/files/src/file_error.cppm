export module sq.files:src.file_error;

export import :src.file_error_code;
import std;

export namespace sq::files
{

/** @brief Structured file system failure information. */
struct FileError
{
        /** @brief Failure category; None means no error. */
        FileErrorCode code{ FileErrorCode::None };

        /** @brief Human-readable diagnostic message. */
        std::string message;

        /** @brief Resolved path the operation was attempting, when known. */
        std::string path;

        /** @brief Return whether this structure represents a failure. */
        [[nodiscard]] explicit operator bool() const noexcept
        {
            return code != FileErrorCode::None;
        }
};

} // namespace sq::files
