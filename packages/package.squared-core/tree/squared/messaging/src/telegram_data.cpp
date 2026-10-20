#include <squared/messaging/telegram_data.hpp>

#include <cstring>

namespace sq::messaging
{

std::optional<DataRef> TelegramData::reserve( std::size_t length ) noexcept
{
    if( length > storage_.size() - used_ || used_ + length > UINT32_MAX ) {
        return std::nullopt;
    }
    const DataRef where{ static_cast<std::uint32_t>( used_ ),
                         static_cast<std::uint32_t>( length ) };
    used_ += length;
    return where;
}

std::optional<DataRef> TelegramData::append( std::span<const std::byte> bytes ) noexcept
{
    const auto where = reserve( bytes.size() );
    if( where && !bytes.empty() ) {
        std::memcpy( storage_.data() + where->offset, bytes.data(), bytes.size() );
    }
    return where;
}

std::span<const std::byte> TelegramData::at( DataRef where ) const noexcept
{
    if( where.offset > used_ || where.length > used_ - where.offset ) {
        return {};
    }
    return storage_.subspan( where.offset, where.length );
}

std::span<std::byte> TelegramData::writable( DataRef where ) noexcept
{
    if( where.offset > used_ || where.length > used_ - where.offset ) {
        return {};
    }
    return storage_.subspan( where.offset, where.length );
}

} // namespace sq::messaging
