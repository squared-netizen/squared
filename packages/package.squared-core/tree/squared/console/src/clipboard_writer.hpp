#pragma once

#include <string_view>

namespace sq::console
{

/**
 * @brief Where the `copy` command writes: three functions and their context,
 * so the Console needs no clipboard of its own and copies through no buffer.
 *
 * `begin` starts a clip and returns false when there is nowhere to write;
 * `append` adds text, as often as needed; `end` hands the clip over and
 * returns false if that failed. On Android the template's JNI writer builds a
 * Java string piece by piece; a test collects into a string.
 */
struct ClipboardWriter final
{
        void *context{ nullptr };
        bool ( *begin )( void *context ) noexcept { nullptr };
        void ( *append )( void *context, std::string_view text ) noexcept { nullptr };
        bool ( *end )( void *context ) noexcept { nullptr };
};

} // namespace sq::console
