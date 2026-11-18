export module sq.data:src.json_write_options;

export namespace sq::data
{

/** @brief Formatting controls for deterministic JSON serialization. */
struct JsonWriteOptions
{
        /** @brief Emit two-space-indented output when true. */
        bool pretty{ false };

        /** @brief Append a final newline when true. */
        bool newline_at_end{ false };

        /**
     * @brief Write the canonical form of RFC 8785 (JSON Canonicalization
     * Scheme): members ordered by their names' UTF-16 code units, numbers
     * as ECMAScript writes them, the fewest escapes, no whitespace. The same
     * value always gives the same bytes, for hashing and signing. pretty and
     * newline_at_end are ignored. An integer beyond +/-(2^53 - 1) is an error: JCS
     * writes every number as a double.
     */
        bool canonical{ false };
};

} // namespace sq::data
