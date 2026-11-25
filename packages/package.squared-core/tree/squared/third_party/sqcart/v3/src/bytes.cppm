// SPDX-License-Identifier: MIT
//
// sq.cart:bytes — where a cartridge's bytes come from (a file, read with
// pread so several threads can share one Archive, or a memory image), the
// little-endian fields of Appendix A, and arithmetic that cannot wrap (§3:
// every sum a reader forms from the file is checked).

module;

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

export module sq.cart:bytes;

import std;
import :error;

namespace sq::cart {

[[nodiscard]] constexpr std::uint16_t u16(const std::byte* p) noexcept {
    return static_cast<std::uint16_t>(std::to_integer<unsigned>(p[0]) | std::to_integer<unsigned>(p[1]) << 8);
}
[[nodiscard]] constexpr std::uint32_t u32(const std::byte* p) noexcept {
    std::uint32_t v = 0;
    for (int i = 3; i >= 0; --i) v = v << 8 | std::to_integer<std::uint32_t>(p[i]);
    return v;
}
[[nodiscard]] constexpr std::uint64_t u64(const std::byte* p) noexcept {
    std::uint64_t v = 0;
    for (int i = 7; i >= 0; --i) v = v << 8 | std::to_integer<std::uint64_t>(p[i]);
    return v;
}
[[nodiscard]] constexpr std::int64_t i64(const std::byte* p) noexcept { return static_cast<std::int64_t>(u64(p)); }

[[nodiscard]] inline bool all_zero(std::span<const std::byte> bytes) noexcept {
    return std::ranges::all_of(bytes, [](std::byte b) { return b == std::byte{0}; });
}

/// a + b, or nothing when it would wrap.
[[nodiscard]] constexpr std::optional<std::uint64_t> add(std::uint64_t a, std::uint64_t b) noexcept {
    if (b > std::numeric_limits<std::uint64_t>::max() - a) return std::nullopt;
    return a + b;
}
[[nodiscard]] constexpr std::optional<std::uint64_t> mul(std::uint64_t a, std::uint64_t b) noexcept {
    if (a != 0 && b > std::numeric_limits<std::uint64_t>::max() / a) return std::nullopt;
    return a * b;
}

/// Random access to a cartridge.
class Source {
public:
    virtual ~Source() = default;
    [[nodiscard]] std::uint64_t size() const noexcept { return size_; }
    /// Exactly out.size() bytes at `offset`.
    [[nodiscard]] virtual Result<void> read(std::uint64_t offset, std::span<std::byte> out) const = 0;

    /// The bytes at [offset, offset + n) in place, without a copy, where the
    /// source can give them (a memory image, a mapped file); else nothing,
    /// and the caller reads them.
    [[nodiscard]] virtual std::optional<std::span<const std::byte>> view(std::uint64_t offset, std::uint64_t n) const {
        static_cast<void>(offset), static_cast<void>(n);
        return std::nullopt;
    }

    [[nodiscard]] Result<std::vector<std::byte>> read(std::uint64_t offset, std::uint64_t n) const {
        const auto end = add(offset, n);
        if (!end || *end > size_) return fail(Code::malformed, "a read past the end of the cartridge");
        std::vector<std::byte> out(static_cast<std::size_t>(n));
        if (auto r = read(offset, out); !r) return std::unexpected(r.error());
        return out;
    }

protected:
    std::uint64_t size_ = 0;
};

class MemorySource final : public Source {
public:
    explicit MemorySource(std::span<const std::byte> bytes) : bytes_(bytes) { size_ = bytes.size(); }
    Result<void> read(std::uint64_t offset, std::span<std::byte> out) const override {
        const auto end = add(offset, out.size());
        if (!end || *end > size_) return fail(Code::malformed, "a read past the end of the cartridge");
        std::ranges::copy(bytes_.subspan(static_cast<std::size_t>(offset), out.size()), out.begin());
        return {};
    }
    std::optional<std::span<const std::byte>> view(std::uint64_t offset, std::uint64_t n) const override {
        const auto end = add(offset, n);
        if (!end || *end > size_) return std::nullopt;
        return bytes_.subspan(static_cast<std::size_t>(offset), static_cast<std::size_t>(n));
    }

private:
    std::span<const std::byte> bytes_;
};

class FileSource final : public Source {
public:
    static Result<std::unique_ptr<Source>> open(const std::filesystem::path& path) {
        const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0) return fail(Code::io, std::format("cannot open {}: {}", path.string(), std::strerror(errno)));
        struct stat st{};
        if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
            ::close(fd);
            return fail(Code::io, std::format("{} is not a regular file", path.string()));
        }
        return std::unique_ptr<Source>(new FileSource(fd, static_cast<std::uint64_t>(st.st_size)));
    }
    ~FileSource() override {
        if (map_ != nullptr) ::munmap(map_, static_cast<std::size_t>(size_));
        ::close(fd_);
    }
    /// From the file mapped read-only at open (indexes and string tables are
    /// read in place, §3.5). A writer only ever appends, truncates bytes past
    /// the last commit, or renames a new file over this one, so the bytes a
    /// reader's commits reach never change under the mapping.
    std::optional<std::span<const std::byte>> view(std::uint64_t offset, std::uint64_t n) const override {
        const auto end = add(offset, n);
        if (map_ == nullptr || !end || *end > size_) return std::nullopt;
        return std::span<const std::byte>(static_cast<const std::byte*>(map_) + offset, static_cast<std::size_t>(n));
    }
    FileSource(const FileSource&) = delete;
    FileSource& operator=(const FileSource&) = delete;

    Result<void> read(std::uint64_t offset, std::span<std::byte> out) const override {
        const auto end = add(offset, out.size());
        if (!end || *end > size_) return fail(Code::malformed, "a read past the end of the cartridge");
        std::size_t done = 0;
        while (done < out.size()) {
            const ssize_t got = ::pread(fd_, out.data() + done, out.size() - done, static_cast<off_t>(offset + done));
            if (got < 0 && errno == EINTR) continue;
            if (got <= 0) {
                return fail(Code::io, got < 0 ? std::format("cannot read the cartridge: {}", std::strerror(errno))
                                              : std::string("the cartridge shrank while it was read"));
            }
            done += static_cast<std::size_t>(got);
        }
        return {};
    }

private:
    FileSource(int fd, std::uint64_t size) : fd_(fd) {
        size_ = size;
        if (size > 0) {
            void* m = ::mmap(nullptr, static_cast<std::size_t>(size), PROT_READ, MAP_SHARED, fd, 0);
            if (m != MAP_FAILED) map_ = m;  // else everything is read with pread
        }
    }
    int fd_;
    void* map_ = nullptr;
};

}  // namespace sq::cart
