#pragma once

namespace sq::data {

/** @brief Formatting controls for deterministic JSON serialization. */
struct JsonWriteOptions {
    /** @brief Emit two-space-indented output when true. */
    bool pretty{false};

    /** @brief Append a final newline when true. */
    bool newline_at_end{false};
};

}  // namespace sq::data
