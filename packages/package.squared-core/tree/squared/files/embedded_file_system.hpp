#pragma once

#include <squared/files/file_system.hpp>
#include <squared/files/src/file_error.hpp>
#include <squared/files/src/file_read_result.hpp>
#include <squared/files/src/file_type.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sq::files
{

/**
 * @brief One file compiled into the program: its path under `Internal`
 * (`skins/default/skin/uiskin.json`: no leading or trailing '/') and its
 * bytes, both in read-only data.
 */
struct EmbeddedFile final
{
        std::string_view path;
        std::span<const unsigned char> bytes;
};

/**
 * @brief A read-only FileSystem over files compiled into the program
 * (`tools/embed_files.sh` writes them): what the Stage's default skin is read
 * through, so the atlas, font and skin loaders, and a texture's reload after
 * a lost context, work as they do on bundled files.
 *
 * Only `Internal` holds anything; a directory is any path some file is under.
 * Writing is refused. Holds a view of the table, which must outlive it: a
 * generated table is a constant, so it does. A lookup is a linear scan:
 * embedded tables are a handful of files.
 */
class EmbeddedFileSystem final : public FileSystem
{
    public:
        explicit EmbeddedFileSystem( std::span<const EmbeddedFile> files ) noexcept
            : files_( files )
        {}

        [[nodiscard]] bool exists( FileType type, std::string_view path ) const noexcept override;

        [[nodiscard]] bool is_directory( FileType type,
                                         std::string_view path ) const noexcept override;

        [[nodiscard]] std::uint64_t length( FileType type,
                                            std::string_view path ) const noexcept override;

        /** @brief A copy of the file's bytes; `NotFound` for none, `NotSupported` outside `Internal`. */
        [[nodiscard]] FileReadResult read( FileType type, std::string_view path ) const override;

        /** @brief Refused: `ReadOnly`. */
        [[nodiscard]] FileError write( FileType type, std::string_view path,
                                       std::span<const std::byte> bytes, bool append ) override;

        /** @brief The names directly under `path`, files and directories, sorted, each once. */
        [[nodiscard]] FileError list( FileType type, std::string_view path,
                                      std::vector<std::string> &names ) const override;

        /** @brief Refused: `ReadOnly`. */
        [[nodiscard]] FileError make_directories( FileType type, std::string_view path ) override;

        /** @brief Refused: `ReadOnly`. */
        [[nodiscard]] FileError remove( FileType type, std::string_view path ) override;

        /** @brief The table it reads. */
        [[nodiscard]] std::span<const EmbeddedFile> files() const noexcept { return files_; }

    private:
        [[nodiscard]] const EmbeddedFile *find( FileType type,
                                                std::string_view path ) const noexcept;

        std::span<const EmbeddedFile> files_;
};

} // namespace sq::files
