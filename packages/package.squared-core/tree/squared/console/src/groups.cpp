#include <squared/console/groups.hpp>

#include <string_view>

namespace sq::console
{

namespace
{

// A line that is itself a dump could queue its own group again, for ever.
bool valid_line( std::string_view line ) noexcept
{
    const std::string_view word = line.substr( 0, line.find( ' ' ) );
    return !line.empty() && word != "dump";
}

bool valid_name( std::string_view name ) noexcept
{
    if( name.empty() || name == "all" || name == "stop" ) {
        return false;
    }
    return name.find_first_of( " \t\"'" ) == std::string_view::npos;
}

} // namespace

Groups::AddResult Groups::add( const char *group, const char *line ) noexcept
{
    if( group == nullptr || line == nullptr || !valid_name( group ) || !valid_line( line ) ) {
        return AddResult::invalid_name;
    }
    std::optional<std::size_t> number = find( group );
    if( number ) {
        for( const GroupLine &existing : lines() ) {
            if( existing.group == *number && std::string_view( existing.line ) == line ) {
                return AddResult::added;
            }
        }
    }
    if( line_count_ == lines_.size() ) {
        return AddResult::lines_full;
    }
    if( !number ) {
        if( name_count_ == names_.size() ) {
            return AddResult::groups_full;
        }
        names_[name_count_] = group;
        number = name_count_;
        ++name_count_;
    }
    lines_[line_count_] = { static_cast<std::uint16_t>( *number ), line };
    ++line_count_;
    return AddResult::added;
}

std::optional<std::size_t> Groups::find( std::string_view name ) const noexcept
{
    for( std::size_t i = 0; i < name_count_; ++i ) {
        if( name == names_[i] ) {
            return i;
        }
    }
    return std::nullopt;
}

} // namespace sq::console
