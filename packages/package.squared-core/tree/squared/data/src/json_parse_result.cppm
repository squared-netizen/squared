export module sq.data:src.json_parse_result;

export import :json_value;
export import :src.json_error;

export namespace sq::data
{

/** @brief Result of parsing one complete RFC 8259 JSON document. */
struct JsonParseResult
{
        /** @brief Parsed value; valid only when error is empty. */
        JsonValue value;

        /** @brief Parse failure, or a no-error JsonError on success. */
        JsonError error;

        /** @brief Return whether parsing succeeded. */
        [[nodiscard]] explicit operator bool() const noexcept { return !error; }
};

} // namespace sq::data
