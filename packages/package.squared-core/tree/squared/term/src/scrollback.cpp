module sq.term;

import std;

namespace sq::term::detail
{

namespace
{

// A line's length, in its header and footer cells. The header's foreground
// red is 1 when the line wrapped.
std::size_t length_of( const Cell &mark ) noexcept
{
    return mark.code;
}

} // namespace

void Scrollback::reset( std::size_t bytes )
{
    capacity_ = bytes / sizeof( Cell );
    ring_ = capacity_ > 0 ? std::make_unique_for_overwrite<Cell[]>( capacity_ ) : nullptr;
    clear();
}

void Scrollback::clear() noexcept
{
    head_ = 0;
    tail_ = 0;
    wrap_end_ = 0;
    count_ = 0;
    wrapped_ = false;
}

void Scrollback::push( std::span<const Cell> line, bool wrapped ) noexcept
{
    ++pushed_; // counted even when not kept: the numbers stay the lines'
    if( capacity_ < 3 ) {
        return;
    }
    std::size_t length = line.size();
    while( length > 0 && line[length - 1] == Cell{} ) {
        --length;
    }
    length = std::min( length, capacity_ - 2 );
    const std::size_t need = length + 2;
    for( ;; ) {
        if( count_ == 0 ) {
            clear();
        }
        if( !wrapped_ ) {
            if( capacity_ - tail_ >= need ) {
                break;
            }
            wrap_end_ = tail_;
            wrapped_ = true;
            tail_ = 0;
            continue;
        }
        if( head_ - tail_ >= need ) {
            break;
        }
        evict_oldest();
    }
    Cell mark{};
    mark.code = static_cast<char32_t>( length );
    mark.foreground.r = wrapped ? 1 : 0;
    ring_[tail_] = mark;
    std::copy_n( line.begin(), length, &ring_[tail_ + 1] );
    ring_[tail_ + 1 + length] = mark;
    tail_ += need;
    ++count_;
}

void Scrollback::evict_oldest() noexcept
{
    head_ += length_of( ring_[head_] ) + 2;
    --count_;
    if( wrapped_ && head_ >= wrap_end_ ) {
        head_ = 0;
        wrapped_ = false;
    }
}

std::size_t Scrollback::start_of( std::size_t back ) const noexcept
{
    std::size_t end = tail_;
    for( std::size_t i = 0;; ++i ) {
        if( end == 0 ) {
            end = wrap_end_;
        }
        const std::size_t start = end - length_of( ring_[end - 1] ) - 2;
        if( i == back ) {
            return start;
        }
        end = start;
    }
}

Scrollback::Line Scrollback::line( std::size_t back ) const noexcept
{
    const std::size_t start = start_of( back );
    const std::size_t length = length_of( ring_[start] );
    return { { &ring_[start + 1], length }, ring_[start].foreground.r != 0 };
}

} // namespace sq::term::detail
