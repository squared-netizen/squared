#include <squared/console/args.hpp>

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <system_error>

namespace sq::console
{

bool Args::parse( std::string_view line ) noexcept
{
    count_ = 0;
    std::size_t at = 0;
    while( at < line.size() ) {
        if( line[at] == ' ' || line[at] == '\t' ) {
            ++at;
            continue;
        }
        if( count_ == words_.size() ) {
            count_ = 0;
            return false;
        }
        std::size_t start = at;
        std::size_t stop = 0;
        if( line[at] == '"' ) {
            start = at + 1;
            const auto close = line.find( '"', start );
            stop = close == std::string_view::npos ? line.size() : close;
            at = close == std::string_view::npos ? line.size() : close + 1;
        } else {
            while( at < line.size() && line[at] != ' ' && line[at] != '\t' ) {
                ++at;
            }
            stop = at;
        }
        words_[count_++] = line.substr( start, stop - start );
    }
    return true;
}

std::string_view Args::text( std::size_t index ) const noexcept
{
    return index < count_ ? words_[index] : std::string_view{};
}

std::int64_t Args::integer( std::size_t index, std::int64_t fallback ) const noexcept
{
    std::string_view word = text( index );
    bool negative = false;
    if( !word.empty() && ( word[0] == '-' || word[0] == '+' ) ) {
        negative = word[0] == '-';
        word.remove_prefix( 1 );
    }
    int base = 10;
    if( word.size() > 2 && word[0] == '0' && ( word[1] == 'x' || word[1] == 'X' ) ) {
        base = 16;
        word.remove_prefix( 2 );
    }
    std::int64_t value = 0;
    const auto [end, error] =
        std::from_chars( word.data(), word.data() + word.size(), value, base );
    if( word.empty() || error != std::errc{} || end != word.data() + word.size() ) {
        return fallback;
    }
    return negative ? -value : value;
}

double Args::real( std::size_t index, double fallback ) const noexcept
{
    // strtod needs a terminated string; a number longer than this is not one
    // a person typed.
    const std::string_view word = text( index );
    char copy[64];
    if( word.empty() || word.size() >= sizeof( copy ) ) {
        return fallback;
    }
    std::copy( word.begin(), word.end(), copy );
    copy[word.size()] = '\0';
    char *end = nullptr;
    const double value = std::strtod( copy, &end );
    return end == copy + word.size() ? value : fallback;
}

} // namespace sq::console
