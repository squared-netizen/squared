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
 * `visit` finds its first line the same way, then walks forward (each line's
 * header holds its length), so a run of lines costs one walk, not one each.
 *
 * Every line pushed is counted (`pushed`), kept or not: the terminal numbers
 * its lines by it, so a line's number stays with it as more scroll off.
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

        /** Lines pushed since it was made, the evicted and the cleared among them. */
        [[nodiscard]] std::uint64_t pushed() const noexcept { return pushed_; }

        /**
         * Lines `back` to `newest_back` (newest_back <= back < size()), the
         * oldest first, each given to `visit( const Line & )`. `back` steps to
         * the first, one for each after. `visit` by value: it is called once
         * a line, so it is never forwarded (a lambda capturing by reference
         * is a pointer or two).
         */
        template<typename F>
        void visit( std::size_t back, std::size_t newest_back, F visit ) const
        {
            std::size_t start = start_of( back );
            for( std::size_t left = back - newest_back + 1; left > 0; --left ) {
                const std::size_t length = ring_[start].code;
                visit( Line{ { &ring_[start + 1], length }, ring_[start].foreground.r != 0 } );
                start += length + 2;
                if( wrapped_ && start == wrap_end_ ) {
                    start = 0; // the lines go on at the ring's start
                }
            }
        }

    private:
        void evict_oldest() noexcept;
        /// Where line `back`'s header cell is.
        [[nodiscard]] std::size_t start_of( std::size_t back ) const noexcept;

        std::unique_ptr<Cell[]> ring_;
        std::size_t capacity_ = 0;
        // The lines are [head_, tail_) or, when wrapped_, [head_, wrap_end_)
        // then [0, tail_).
        std::size_t head_ = 0;
        std::size_t tail_ = 0;
        std::size_t wrap_end_ = 0;
        std::size_t count_ = 0;
        std::uint64_t pushed_ = 0;
        bool wrapped_ = false;
};

} // namespace sq::term::detail
