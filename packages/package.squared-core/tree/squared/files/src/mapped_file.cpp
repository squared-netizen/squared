module sq.files;

import std;

namespace sq::files
{

MappedFile::MappedFile( std::span<const std::byte> bytes, bool in_place, Release release,
                        void *handle ) noexcept
    : data_( bytes.data() )
    , size_( bytes.size() )
    , release_( release )
    , handle_( handle )
    , in_place_( in_place )
    , valid_( true )
{}

MappedFile::~MappedFile()
{
    reset();
}

MappedFile::MappedFile( MappedFile &&other ) noexcept
    : data_( std::exchange( other.data_, nullptr ) )
    , size_( std::exchange( other.size_, 0 ) )
    , release_( std::exchange( other.release_, nullptr ) )
    , handle_( std::exchange( other.handle_, nullptr ) )
    , in_place_( std::exchange( other.in_place_, false ) )
    , valid_( std::exchange( other.valid_, false ) )
{}

MappedFile &MappedFile::operator=( MappedFile &&other ) noexcept
{
    if( this != &other ) {
        reset();
        data_ = std::exchange( other.data_, nullptr );
        size_ = std::exchange( other.size_, 0 );
        release_ = std::exchange( other.release_, nullptr );
        handle_ = std::exchange( other.handle_, nullptr );
        in_place_ = std::exchange( other.in_place_, false );
        valid_ = std::exchange( other.valid_, false );
    }
    return *this;
}

void MappedFile::reset() noexcept
{
    if( release_ != nullptr ) {
        release_( handle_, data_, size_ );
    }
    data_ = nullptr;
    size_ = 0;
    release_ = nullptr;
    handle_ = nullptr;
    in_place_ = false;
    valid_ = false;
}

} // namespace sq::files
