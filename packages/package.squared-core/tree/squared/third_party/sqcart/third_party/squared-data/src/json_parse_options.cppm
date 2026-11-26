export module sq.data:src.json_parse_options;

import std;

export namespace sq::data
{

/** @brief Limits and strictness applied to one parse operation. */
struct JsonParseOptions
{
        /** @brief Maximum accepted document size in bytes (default 8 MiB). */
        std::size_t maximum_bytes{ std::size_t{ 8 } * 1024U * 1024U };

        /** @brief Maximum nesting depth of arrays and objects (default 128). */
        std::size_t maximum_depth{ 128 };

        /** @brief Whether duplicate object keys are errors (default true). */
        bool reject_duplicate_keys{ true };

        /**
     * @brief Accept only I-JSON (RFC 7493): also refuse integers beyond
     * +/-(2^53 - 1), which a double cannot hold exactly (default false).
     *
     * Duplicate keys, invalid UTF-8 and lone surrogates are refused anyway
     * (the first unless reject_duplicate_keys is false).
     */
        bool ijson{ false };
};

} // namespace sq::data
