#include <squared/console/commands.hpp>

#include <cstring>

namespace sq::console
{

Commands::AddResult Commands::add( const char *name, const char *help, Handler handler,
                                   void *context ) noexcept
{
    if( name == nullptr || name[0] == '\0' || handler == nullptr ||
        std::strpbrk( name, " \t\"" ) != nullptr ) {
        return AddResult::invalid_name;
    }
    if( find( name ) != nullptr ) {
        return AddResult::duplicate_name;
    }
    if( count_ == table_.size() ) {
        return AddResult::table_full;
    }
    table_[count_++] = Command{ name, help != nullptr ? help : "", handler, context };
    return AddResult::added;
}

const Command *Commands::find( std::string_view name ) const noexcept
{
    for( std::size_t i = 0; i < count_; ++i ) {
        if( name == table_[i].name ) {
            return &table_[i];
        }
    }
    return nullptr;
}

} // namespace sq::console
