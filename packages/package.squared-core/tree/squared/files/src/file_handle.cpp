#include <squared/files/file_handle.hpp>

#include <squared/files/file_error_code.hpp>
#include <squared/files/file_system.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sq::files {

namespace {

/** @brief Return the offset just past the last separator, or zero. */
std::size_t name_offset(std::string_view path) noexcept
{
    const std::size_t separator = path.find_last_of('/');
    return separator == std::string_view::npos ? 0 : separator + 1;
}

/** @brief Join two path fragments with a single separator. */
std::string join(std::string_view left, std::string_view right)
{
    if (left.empty()) return std::string{right};
    if (right.empty()) return std::string{left};

    std::string result;
    result.reserve(left.size() + right.size() + 1);
    result.assign(left);
    if (result.back() != '/') result.push_back('/');
    if (right.front() == '/') {
        result.append(right.substr(1));
    } else {
        result.append(right);
    }
    return result;
}

FileError unbound()
{
    return FileError{
        .code = FileErrorCode::InvalidPath,
        .message = "file handle is not bound to a file system",
        .path = {}
    };
}

}  // namespace

FileHandle::FileHandle(
    FileSystem& system,
    FileType type,
    std::string path
) noexcept
    : system_(&system)
    , type_(type)
    , path_(std::move(path))
{
}

std::string_view FileHandle::name() const noexcept
{
    return std::string_view{path_}.substr(name_offset(path_));
}

std::string_view FileHandle::extension() const noexcept
{
    const std::string_view final_name = name();
    const std::size_t dot = final_name.find_last_of('.');
    if (dot == std::string_view::npos || dot == 0) return {};
    return final_name.substr(dot + 1);
}

std::string_view FileHandle::name_without_extension() const noexcept
{
    const std::string_view final_name = name();
    const std::size_t dot = final_name.find_last_of('.');
    if (dot == std::string_view::npos || dot == 0) return final_name;
    return final_name.substr(0, dot);
}

FileHandle FileHandle::child(std::string_view name) const
{
    if (system_ == nullptr) return {};
    return FileHandle{*system_, type_, join(path_, name)};
}

FileHandle FileHandle::sibling(std::string_view name) const
{
    return parent().child(name);
}

FileHandle FileHandle::parent() const
{
    if (system_ == nullptr) return {};
    const std::size_t offset = name_offset(path_);
    if (offset == 0) return FileHandle{*system_, type_, std::string{}};
    return FileHandle{*system_, type_, path_.substr(0, offset - 1)};
}

bool FileHandle::exists() const noexcept
{
    return system_ != nullptr && system_->exists(type_, path_);
}

bool FileHandle::is_directory() const noexcept
{
    return system_ != nullptr && system_->is_directory(type_, path_);
}

std::uint64_t FileHandle::length() const noexcept
{
    return system_ == nullptr ? 0U : system_->length(type_, path_);
}

FileReadResult FileHandle::read_bytes() const
{
    if (system_ == nullptr) return FileReadResult{.bytes = {}, .error = unbound()};
    return system_->read(type_, path_);
}

FileTextResult FileHandle::read_string() const
{
    FileReadResult raw = read_bytes();
    if (!raw) return FileTextResult{.text = {}, .error = std::move(raw.error)};

    FileTextResult result;
    result.text.resize(raw.bytes.size());
    for (std::size_t index = 0; index < raw.bytes.size(); ++index) {
        result.text[index] = static_cast<char>(raw.bytes[index]);
    }
    return result;
}

FileError FileHandle::write_bytes(
    std::span<const std::byte> bytes,
    bool append
) const
{
    if (system_ == nullptr) return unbound();

    // libGDX creates missing parent directories on write, and callers expect
    // it. Done here rather than in a backend so every backend inherits it.
    // Costs one directory walk per write; priority 2 over priority 3, and it
    // allocates nothing beyond the resolved path.
    const FileHandle directory = parent();
    if (!directory.path().empty()) {
        if (FileError error = directory.make_directories()) return error;
    }

    return system_->write(type_, path_, bytes, append);
}

FileError FileHandle::write_string(std::string_view text, bool append) const
{
    const auto* first = reinterpret_cast<const std::byte*>(text.data());
    return write_bytes(std::span<const std::byte>{first, text.size()}, append);
}

FileError FileHandle::list(std::vector<FileHandle>& entries) const
{
    entries.clear();
    if (system_ == nullptr) return unbound();

    std::vector<std::string> names;
    FileError error = system_->list(type_, path_, names);
    if (error) return error;

    entries.reserve(names.size());
    for (const std::string& entry : names) {
        entries.emplace_back(*system_, type_, join(path_, entry));
    }
    return {};
}

FileError FileHandle::make_directories() const
{
    if (system_ == nullptr) return unbound();
    return system_->make_directories(type_, path_);
}

FileError FileHandle::remove() const
{
    if (system_ == nullptr) return unbound();
    return system_->remove(type_, path_);
}

bool FileHandle::operator==(const FileHandle& other) const noexcept
{
    return system_ == other.system_
        && type_ == other.type_
        && path_ == other.path_;
}

}  // namespace sq::files
