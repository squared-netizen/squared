export module sq.files:mapped_file;

export import :src.file_error;

import std;

export namespace sq::files
{

/**
 * @brief A file's bytes, read in place for as long as this lives: a font
 * FreeType reads, a table looked up now and then. Move-only; given back by
 * its destructor.
 *
 * In place means the file's own pages: `mmap` of a file, an asset stored
 * uncompressed in the APK. Those pages are clean, so Android can drop them
 * under memory pressure and read them again, and only the parts touched are
 * resident. A compressed asset cannot be mapped: Android unpacks it into
 * memory of its own, and `in_place()` says so.
 */
class MappedFile final
{
    public:
        /** @brief How the system that made a mapping gives it back. */
        using Release = void ( * )( void *handle, const std::byte *data,
                                    std::size_t size ) noexcept;

        /** @brief Nothing mapped. */
        MappedFile() noexcept = default;

        /**
         * @brief What a file system hands out: `bytes`, read in place or
         * not, given back by `release( handle, data, size )` (null: nothing
         * to give back, as for embedded files).
         */
        MappedFile( std::span<const std::byte> bytes, bool in_place, Release release,
                    void *handle ) noexcept;

        ~MappedFile();
        MappedFile( const MappedFile & ) = delete;
        MappedFile &operator=( const MappedFile & ) = delete;
        MappedFile( MappedFile &&other ) noexcept;
        MappedFile &operator=( MappedFile &&other ) noexcept;

        /** @brief The file's bytes; empty for an empty file, or nothing mapped. */
        [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return { data_, size_ }; }

        /** @brief Whether a file is mapped (an empty file is). */
        [[nodiscard]] bool valid() const noexcept { return valid_; }

        /** @brief The file's own pages, not a copy in memory: false for a compressed asset. */
        [[nodiscard]] bool in_place() const noexcept { return in_place_; }

        /** @brief Give the mapping back now; nothing mapped after. */
        void reset() noexcept;

    private:
        const std::byte *data_ = nullptr;
        std::size_t size_ = 0;
        Release release_ = nullptr;
        void *handle_ = nullptr;
        bool in_place_ = false;
        bool valid_ = false;
};

/** @brief What `map` gives: the mapping, or why there is none. */
struct MapResult
{
        MappedFile file;
        FileError error;

        [[nodiscard]] explicit operator bool() const noexcept { return !error; }
};

} // namespace sq::files
