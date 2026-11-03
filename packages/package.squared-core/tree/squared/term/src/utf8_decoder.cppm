export module sq.term:src.utf8_decoder;

import std;

export namespace sq::term::detail
{

/**
 * UTF-8, one byte at a time, as a stream delivers it: a character may be
 * split between two reads. Well-formed per Unicode's table 3-7 (no overlong
 * forms, no surrogates, nothing past U+10FFFF); each ill-formed part becomes
 * one U+FFFD, the "maximal subpart" rule, as xterm and most terminals do.
 * Four bytes of state.
 */
class Utf8Decoder
{
    public:
        static constexpr char32_t replacement = U'�';

        enum class Step : std::uint8_t {
            /** The character is not complete yet. */
            more,
            /** `code` is a character. */
            code,
            /** `code` is U+FFFD for what came before this byte, which starts again. */
            code_then_again,
        };

        struct Result
        {
                Step step = Step::more;
                char32_t code = 0;
        };

        [[nodiscard]] constexpr Result push( std::uint8_t byte ) noexcept
        {
            if( need_ == 0 ) {
                return first( byte );
            }
            if( byte < low_ || byte > high_ ) {
                need_ = 0;
                return { Step::code_then_again, replacement };
            }
            code_ = ( code_ << 6U ) | ( byte & 0x3FU );
            low_ = 0x80;
            high_ = 0xBF;
            if( --need_ == 0 ) {
                return { Step::code, code_ };
            }
            return {};
        }

        /** True between the bytes of one character. */
        [[nodiscard]] constexpr bool pending() const noexcept { return need_ != 0; }

        constexpr void reset() noexcept { need_ = 0; }

    private:
        [[nodiscard]] constexpr Result first( std::uint8_t byte ) noexcept
        {
            if( byte < 0x80 ) {
                return { Step::code, byte };
            }
            low_ = 0x80;
            high_ = 0xBF;
            if( byte >= 0xC2 && byte <= 0xDF ) {
                need_ = 1;
                code_ = byte & 0x1FU;
            } else if( byte >= 0xE0 && byte <= 0xEF ) {
                need_ = 2;
                code_ = byte & 0x0FU;
                if( byte == 0xE0 ) {
                    low_ = 0xA0;
                } else if( byte == 0xED ) {
                    high_ = 0x9F;
                }
            } else if( byte >= 0xF0 && byte <= 0xF4 ) {
                need_ = 3;
                code_ = byte & 0x07U;
                if( byte == 0xF0 ) {
                    low_ = 0x90;
                } else if( byte == 0xF4 ) {
                    high_ = 0x8F;
                }
            } else {
                return { Step::code, replacement };
            }
            return {};
        }

        char32_t code_ = 0;
        std::uint8_t need_ = 0;
        std::uint8_t low_ = 0x80;
        std::uint8_t high_ = 0xBF;
};

} // namespace sq::term::detail
