export module sq.data:src.json_write_result;

export import :src.json_error;
import std;

export namespace sq::data
{

/** @brief Result of serializing one owned JSON value. */
struct JsonWriteResult
{
        /** @brief Serialized UTF-8 text; valid only when error is empty. */
        std::string text;

        /** @brief Serialization failure, or a no-error JsonError on success. */
        JsonError error;

        /** @brief Return whether serialization succeeded. */
        [[nodiscard]] explicit operator bool() const noexcept { return !error; }
};

} // namespace sq::data
