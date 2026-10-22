#include <squared/console/telegraph_commands.hpp>
#include <squared/messaging/name_table.hpp>
#include <squared/messaging/telegram_text.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace sq::console
{

namespace
{

using messaging::BoundEntry;
using messaging::TelegraphUi;
using messaging::write_telegram;

constexpr const char *telegraph_usage =
    "telegraph [state]: the capacities and their highest fill; state: every value";
constexpr const char *telegraph_help =
    "telegraph [state]: capacities and highest fills; state: every value";

void print_ring( Console &console, const char *direction,
                 const messaging::RingStats &ring ) noexcept
{
    console.printf( Level::info, "%-13s %u of %u telegrams at most, %u of %u data bytes", direction,
                    ring.highest_slots, ring.slots, ring.highest_data, ring.data_bytes );
}

void print_stats( Console &console, const TelegraphUi &ui ) noexcept
{
    print_ring( console, "to the model:", ui.to_model_stats() );
    print_ring( console, "to the UI:", ui.to_ui_stats() );
    const messaging::CacheStats cache = ui.cache_stats();
    console.printf( Level::info, "%-13s %u of %u entries, %u bound", "cache:", cache.used,
                    cache.entries, cache.bound );
    console.printf( Level::info, "%-13s text %u of %u bytes, %u at most", "", cache.text_used,
                    cache.text_bytes, cache.highest_text );
    console.printf( Level::info, "%-13s %u evicted for room, %u dropped", "", cache.evicted,
                    cache.dropped );
    const messaging::ModelStats model = ui.model_stats();
    const auto pending = static_cast<unsigned>( messaging::telegraph_size::pending );
    const auto lists = static_cast<unsigned>( messaging::telegraph_size::lists );
    console.printf( Level::info, "%-13s %u now, %u at most, of %u", "held back:", model.held,
                    model.highest_held, pending );
    console.printf( Level::info, "%-13s %u of %u", "lists:", model.lists, lists );
    const messaging::WaitStats waits = ui.wait_stats();
    const Level level = waits.full != 0 ? Level::warning : Level::info;
    console.printf( level, "%-13s waited %u times, refused %u (full)", "model sends:", waits.waits,
                    waits.full );
}

// The order `telegraph state` prints in: by name id, then uid. The same names
// give the same ids on every run, so two dumps compare line for line.
bool before( messaging::Address left, messaging::Address right ) noexcept
{
    if( left.name.id != right.name.id ) {
        return left.name.id < right.name.id;
    }
    return left.uid < right.uid;
}

// The valued entry that comes next after `previous` (all of them, from the
// first, when there is none): a selection walk, O(n) a line and nothing
// allocated, for a command that runs when asked.
std::optional<BoundEntry> next_value( const TelegraphUi &ui,
                                      const std::optional<messaging::Address> &previous ) noexcept
{
    std::optional<BoundEntry> best;
    std::uint32_t from = 0;
    for( ;; ) {
        const std::optional<BoundEntry> found = ui.cached_from( from );
        if( !found.has_value() ) {
            return best;
        }
        from = found->entry + 1;
        const bool valued = found->value.has_value();
        const bool after = !previous.has_value() || before( *previous, found->address );
        if( valued && after && ( !best.has_value() || before( found->address, best->address ) ) ) {
            best = found;
        }
    }
}

void print_state( Console &console, const TelegraphUi &ui ) noexcept
{
    // Names are not registered yet, so each shows as # and 16 hex digits.
    const messaging::NameTable names( std::span<std::byte>{} );
    std::array<char, format_bytes> line{};
    std::uint32_t shown = 0;
    std::optional<messaging::Address> previous;
    for( ;; ) {
        const std::optional<BoundEntry> value = next_value( ui, previous );
        if( !value.has_value() || !value->value.has_value() ) {
            break;
        }
        previous = value->address;
        const messaging::CachedValue &cached = *value->value;
        const std::size_t length = write_telegram( cached.telegram, cached.data, names, line );
        if( length != 0 ) {
            console.print( { line.data(), length } );
        } else {
            const std::size_t address = names.format( value->address, line );
            console.printf( Level::warning, "state %.*s: %zu bytes of value, longer than a line",
                            static_cast<int>( address ), line.data(), cached.data.size() );
        }
        ++shown;
    }
    std::uint32_t waiting = 0;
    std::uint32_t from = 0;
    for( ;; ) {
        const std::optional<BoundEntry> found = ui.cached_from( from );
        if( !found.has_value() ) {
            break;
        }
        from = found->entry + 1;
        if( !found->value.has_value() ) {
            ++waiting;
        }
    }
    console.printf( Level::info, "telegraph: %u values; %u bound, with no value yet", shown,
                    waiting );
}

void telegraph( Console &console, const Args &args, void *context ) noexcept
{
    const auto &ui = *static_cast<const TelegraphUi *>( context );
    if( args.size() == 1 ) {
        print_stats( console, ui );
    } else if( args.size() == 2 && args.text( 1 ) == "state" ) {
        print_state( console, ui );
    } else {
        console.print( telegraph_usage, Level::error );
    }
}

} // namespace

std::size_t add_telegraph_commands( Console &console, messaging::TelegraphUi &ui ) noexcept
{
    std::size_t refused = 0;
    const auto added = console.add( "telegraph", telegraph_help, telegraph, &ui );
    if( added != Commands::AddResult::added ) {
        ++refused;
    }
    if( console.add_to_group( "telegraph", "telegraph" ) != Groups::AddResult::added ) {
        ++refused;
    }
    if( console.add_to_group( "state", "telegraph state" ) != Groups::AddResult::added ) {
        ++refused;
    }
    return refused;
}

} // namespace sq::console
