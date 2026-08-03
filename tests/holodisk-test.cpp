#include "squared/holodisk/holodrive.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

namespace {

namespace fs = std::filesystem;
using namespace squared::holodisk;

[[noreturn]] void fail(std::string_view message)
{
    std::cerr << message << '\n';
    std::exit(1);
}

template<typename Condition>
void require(Condition&& condition, std::string_view message)
{
    if (!static_cast<bool>(condition)) {
        fail(message);
    }
}

std::span<const std::byte> bytes(std::string_view text)
{
    return {reinterpret_cast<const std::byte*>(text.data()), text.size()};
}

std::string read_all(HoloDrive& drive, FileId file)
{
    std::string result;
    std::array<std::byte, 3> buffer{};
    while (true) {
        auto read = drive.read(file, buffer);
        require(static_cast<bool>(read), "streaming read failed");
        if (read.value() == 0) {
            return result;
        }
        result.append(reinterpret_cast<const char*>(buffer.data()), read.value());
    }
}

}  // namespace

int main()
{
    std::error_code ignored;
    auto temporary = fs::temp_directory_path(ignored);
    if (ignored) {
        ignored.clear();
        temporary = fs::current_path();
    }
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path root = temporary /
        ("squared-holodisk-test-" + std::to_string(nonce));
    fs::remove_all(root, ignored);
    fs::create_directories(root);

    auto factory = make_standard_holodrive_factory();
    require(factory != nullptr, "factory was not created");
    DriveOptions options;
    options.scratch_directory = (root / "scratch").string();
    options.maximum_open_files = 2;
    auto created_drive = factory->create(options);
    require(static_cast<bool>(created_drive), "drive was not created");
    auto drive = std::move(created_drive).value();

    auto created_disk = drive->create_holodisk();
    require(static_cast<bool>(created_disk), "emulated HoloDisk was not created");
    const auto disk = created_disk.value();
    auto mounted = drive->mount(disk, "/game", MountAccess::ReadWrite);
    require(static_cast<bool>(mounted), "emulated HoloDisk was not mounted");

    auto opened = drive->open("/game/config/settings.txt", OpenMode::ReadWrite);
    require(static_cast<bool>(opened), "emulated HoloDisk file was not opened");
    const auto file = opened.value();
    auto wrote = drive->write(file, bytes("portable-holodisk"));
    require(wrote && wrote.value() == 17, "emulated HoloDisk write failed");
    const auto busy_unmount = drive->unmount(mounted.value());
    require(!busy_unmount && busy_unmount.error().code == ErrorCode::Busy,
            "unmount did not protect an open file");
    auto rewind = drive->seek(file, 0, SeekOrigin::Begin);
    require(rewind && rewind.value() == 0, "seek failed");
    require(read_all(*drive, file) == "portable-holodisk", "read/write contents differ");
    require(drive->close(file), "file did not close");

    auto listed = drive->list("/game/config");
    require(listed && listed.value().size() == 1, "directory list is incomplete");
    require(listed.value().front().name == "settings.txt" &&
                !listed.value().front().directory && listed.value().front().size == 17,
            "directory entry is incorrect");
    require(drive->unmount(mounted.value()), "emulated HoloDisk did not unmount");

    const fs::path archive = root / "game.holodisk";
    require(drive->write_holodisk(disk, archive.string()),
            "emulated HoloDisk was not materialized");
    require(fs::is_regular_file(archive), "materialized HoloDisk is absent");
    require(drive->discard_holodisk(disk), "emulated HoloDisk was not discarded");

    auto loaded = drive->load_holodisk(archive.string());
    require(static_cast<bool>(loaded), "materialized HoloDisk did not reload");
    auto read_mount = drive->mount(loaded.value(), "/cartridge", MountAccess::ReadOnly);
    require(static_cast<bool>(read_mount), "loaded HoloDisk did not mount");
    auto rejected = drive->open("/cartridge/config/settings.txt", OpenMode::Append);
    require(!rejected && rejected.error().code == ErrorCode::ReadOnly,
            "read-only mount accepted a writer");
    auto reader = drive->open("/cartridge/config/settings.txt", OpenMode::Read);
    require(static_cast<bool>(reader), "loaded HoloDisk file did not open");
    auto middle = drive->seek(reader.value(), 9, SeekOrigin::Begin);
    require(middle && middle.value() == 9, "archive seek failed");
    require(read_all(*drive, reader.value()) == "holodisk", "archive streaming result differs");
    require(drive->close(reader.value()), "archive stream did not close");
    require(drive->unmount(read_mount.value()), "loaded HoloDisk did not unmount");
    require(drive->discard_holodisk(loaded.value()), "loaded HoloDisk did not discard");

    const fs::path replacement = root / "replacement.holodisk";
    auto empty = drive->create_holodisk();
    require(empty && drive->write_holodisk(empty.value(), replacement.string()),
            "empty HoloDisk did not materialize");
    auto exists = drive->write_holodisk(empty.value(), replacement.string());
    require(!exists && exists.error().code == ErrorCode::AlreadyExists,
            "destination replacement was not explicit");
    WriteOptions replace;
    replace.replace_existing = true;
    require(drive->write_holodisk(empty.value(), replacement.string(), replace),
            "explicit HoloDisk replacement failed");

    drive.reset();
    require(!fs::exists(root / "scratch" / "squared-holodrive-1"),
            "drive scratch state survived drive destruction");
    fs::remove_all(root, ignored);
    return 0;
}
