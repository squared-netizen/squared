#include <squared/files/file_system.hpp>

#include <squared/files/file_handle.hpp>
#include <squared/files/file_type.hpp>

#include <string>
#include <string_view>

namespace sq::files {

FileHandle FileSystem::internal(std::string_view path)
{
    return resolve(FileType::Internal, path);
}

FileHandle FileSystem::local(std::string_view path)
{
    return resolve(FileType::Local, path);
}

FileHandle FileSystem::external(std::string_view path)
{
    return resolve(FileType::External, path);
}

FileHandle FileSystem::absolute(std::string_view path)
{
    return resolve(FileType::Absolute, path);
}

FileHandle FileSystem::resolve(FileType type, std::string_view path)
{
    return FileHandle{*this, type, std::string{path}};
}

}  // namespace sq::files
