export module sq.term:src.scrollback;

export import :cell;
import std;

export namespace sq::term::detail
{

/**
 * The lines that scrolled off the top of the main screen, in one ring of
 * cells allocated when its size is set: each line with its trailing blanks
 * trimmed, between a header and a footer cell that hold its length (so the
 * ring is walked from either end), oldest out to make room. A line is never
 * split across the ring's end: what is left there is skipped. 24 bytes a
 * line and 12 a cell.
 *
 * `line( back )` walks back from the newest, one step a line: reading the
 * lines a scrolled-back view shows costs as many steps as it is scrolled.
 */
class Scrollback
{
    public:
        struct Line
        {
                std::span<const Cell> cells;
                /** It went on into the next line by autowrap. */
                bool wrapped = false;
        };

        Scrollback() = default;

        /** Room for `bytes` (rounded down to whole cells), empty. 0: keeps nothing. */
        void reset( std::size_t bytes );

        /** A line, its trailing blanks trimmed; the oldest lines go to make room. */
        void push( std::span<const Cell> line, bool wrapped ) noexcept;

        /** Every line forgotten; the memory kept. */
        void clear() noexcept;

        [[nodiscard]] std::size_t size() const noexcept { return count_; }
        [[nodiscard]] std::size_t capacity_bytes() const noexcept
        {
            return capacity_ * sizeof( Cell );
        }

        /** Line `back`, 0 the newest; `back` is below `size()`. */
        [[nodiscard]] Line line( std::size_t back ) const noexcept;

    private:
        void evict_oldest() noexcept;

        std::unique_ptr<Cell[]> ring_;
        std::size_t capacity_ = 0;
        // The lines are [head_, tail_) or, when wrapped_, [head_, wrap_end_)
        // then [0, tail_).
        std::size_t head_ = 0;
        std::size_t tail_ = 0;
        std::size_t wrap_end_ = 0;
        std::size_t count_ = 0;
        bool wrapped_ = false;
};

} // namespace sq::term::detail
