#pragma once

#include <string_view>

namespace sq::app
{

/**
 * @brief Where an application says that something went wrong: a function
 * and its context, given by the platform (on Android: the system log, and the
 * Console when it is built in). Empty does nothing.
 */
struct Report final
{
        void ( *write )( void *context, std::string_view line ) noexcept = nullptr;
        void *context = nullptr;

        /** @brief Say `line`, when there is somewhere to say it. */
        void operator()( std::string_view line ) const noexcept
        {
            if( write != nullptr ) {
                write( context, line );
            }
        }
};

} // namespace sq::app
