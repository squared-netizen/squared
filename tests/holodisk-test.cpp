#include "squared/holodisk/asset_manager.hpp"
#include "squared/holodisk/holodrive.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

std::vector<std::byte> read_host_bytes(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    require(stream.good(), "host archive did not open");
    const auto size = stream.tellg();
    require(size >= 0, "host archive size is invalid");
    std::vector<std::byte> result(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(
        reinterpret_cast<char*>(result.data()),
        static_cast<std::streamsize>(size)
    );
    require(stream.good(), "host archive bytes did not read");
    return result;
}

struct TextAsset final {
    std::string text;
};

struct DependentAsset final {
    AssetHandle<TextAsset> text;
};

struct CyclicAsset final {};

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

    {
        AssetManager assets(*drive);
        require(assets.register_loader<TextAsset>([](
            AssetLoadContext& context,
            std::string_view path
        ) -> Result<AssetHandle<TextAsset>> {
            auto source = context.read_bytes(path);
            if (!source) return Result<AssetHandle<TextAsset>>::failure(source.error());
            return Result<AssetHandle<TextAsset>>::success(
                std::make_shared<TextAsset>(TextAsset{std::string(
                    reinterpret_cast<const char*>(source.value().data()),
                    source.value().size()
                )})
            );
        }), "text asset loader did not register");
        require(assets.register_loader<DependentAsset>([](
            AssetLoadContext& context,
            std::string_view path
        ) -> Result<AssetHandle<DependentAsset>> {
            auto text = context.load<TextAsset>(path);
            if (!text) {
                return Result<AssetHandle<DependentAsset>>::failure(text.error());
            }
            return Result<AssetHandle<DependentAsset>>::success(
                std::make_shared<DependentAsset>(DependentAsset{text.value()})
            );
        }), "dependent asset loader did not register");
        require(assets.register_loader<CyclicAsset>([](
            AssetLoadContext& context,
            std::string_view path
        ) -> Result<AssetHandle<CyclicAsset>> {
            return context.load<CyclicAsset>(path);
        }), "cyclic asset loader did not register");

        auto first = assets.load<TextAsset>("/game/config/settings.txt");
        auto cached = assets.load<TextAsset>("/game/config/settings.txt");
        require(first && cached && first.value() == cached.value() &&
                    first.value()->text == "portable-holodisk",
                "typed asset was not cached");
        auto dependent = assets.load<DependentAsset>("/game/config/settings.txt");
        require(dependent && dependent.value()->text == first.value(),
                "typed dependency was not recorded");
        auto busy_asset = assets.unload<TextAsset>("/game/config/settings.txt");
        require(!busy_asset && busy_asset.error().code == ErrorCode::Busy,
                "asset dependency did not protect unload");
        require(assets.unload<DependentAsset>("/game/config/settings.txt"),
                "dependent asset did not unload");

        auto changed = drive->open(
            "/game/config/settings.txt", OpenMode::WriteTruncate
        );
        require(changed, "asset source did not reopen for reload");
        require(drive->write(changed.value(), bytes("reloaded")),
                "asset source update failed");
        require(drive->close(changed.value()), "updated asset source did not close");
        require(assets.reload<TextAsset>("/game/config/settings.txt"),
                "typed asset did not reload");
        auto refreshed = assets.load<TextAsset>("/game/config/settings.txt");
        require(refreshed && refreshed.value()->text == "reloaded" &&
                    first.value()->text == "portable-holodisk",
                "reload did not preserve old handles and replace the cache");

        auto cycle = assets.load<CyclicAsset>("/game/config/settings.txt");
        require(!cycle && cycle.error().code == ErrorCode::DependencyCycle,
                "asset dependency cycle was not rejected");
        require(assets.unload<TextAsset>("/game/config/settings.txt"),
                "reloaded asset did not unload");
    }
    {
        AssetManagerOptions limited_options;
        limited_options.maximum_asset_bytes = 4;
        AssetManager limited(*drive, limited_options);
        require(limited.register_loader<TextAsset>([](
            AssetLoadContext& context,
            std::string_view path
        ) -> Result<AssetHandle<TextAsset>> {
            auto source = context.read_bytes(path);
            if (!source) return Result<AssetHandle<TextAsset>>::failure(source.error());
            return Result<AssetHandle<TextAsset>>::success(
                std::make_shared<TextAsset>()
            );
        }), "limited asset loader did not register");
        auto too_large = limited.load<TextAsset>("/game/config/settings.txt");
        require(!too_large && too_large.error().code == ErrorCode::LimitExceeded,
                "asset byte limit was not enforced");
        auto missing_loader = limited.load<DependentAsset>(
            "/game/config/settings.txt"
        );
        require(!missing_loader &&
                    missing_loader.error().code == ErrorCode::LoaderNotFound,
                "missing asset loader was not reported");
    }

    auto listed = drive->list("/game/config");
    require(listed && listed.value().size() == 1, "directory list is incomplete");
    require(listed.value().front().name == "settings.txt" &&
                !listed.value().front().directory && listed.value().front().size == 8,
            "directory entry is incorrect");
    require(drive->unmount(mounted.value()), "emulated HoloDisk did not unmount");

    const fs::path archive = root / "game.holodisk";
    require(drive->write_holodisk(disk, archive.string()),
            "emulated HoloDisk was not materialized");
    require(fs::is_regular_file(archive), "materialized HoloDisk is absent");
    require(drive->discard_holodisk(disk), "emulated HoloDisk was not discarded");

    const auto archive_bytes = read_host_bytes(archive);
    auto memory_loaded = drive->load_holodisk(
        std::span<const std::byte>(archive_bytes)
    );
    require(memory_loaded, "memory-backed HoloDisk did not load");
    auto memory_mount = drive->mount(
        memory_loaded.value(), "/memory", MountAccess::ReadOnly
    );
    require(memory_mount, "memory-backed HoloDisk did not mount");
    auto memory_file = drive->open("/memory/config/settings.txt", OpenMode::Read);
    require(memory_file && read_all(*drive, memory_file.value()) == "reloaded",
            "memory-backed HoloDisk contents differ");
    require(drive->close(memory_file.value()), "memory-backed file did not close");
    require(drive->unmount(memory_mount.value()), "memory HoloDisk did not unmount");
    require(drive->discard_holodisk(memory_loaded.value()),
            "memory HoloDisk did not discard");

    auto container = drive->create_holodisk();
    require(container, "nested-archive container was not created");
    auto container_mount = drive->mount(
        container.value(), "/container", MountAccess::ReadWrite
    );
    require(container_mount, "nested-archive container did not mount");
    auto nested_file = drive->open("/container/game.zip", OpenMode::WriteTruncate);
    require(nested_file, "nested archive file did not open");
    require(drive->write(nested_file.value(), archive_bytes),
            "nested archive bytes did not write");
    require(drive->close(nested_file.value()), "nested archive file did not close");
    {
        AssetManager assets(*drive);
        require(assets.register_loader<TextAsset>([](
            AssetLoadContext& context,
            std::string_view path
        ) -> Result<AssetHandle<TextAsset>> {
            auto source = context.read_bytes(path);
            if (!source) return Result<AssetHandle<TextAsset>>::failure(source.error());
            return Result<AssetHandle<TextAsset>>::success(
                std::make_shared<TextAsset>(TextAsset{std::string(
                    reinterpret_cast<const char*>(source.value().data()),
                    source.value().size()
                )})
            );
        }), "nested text loader did not register");
        require(assets.mount_archive("/container/game.zip", "/nested"),
                "nested archive did not mount through AssetManager");
        auto nested = assets.load<TextAsset>("/nested/config/settings.txt");
        require(nested && nested.value()->text == "reloaded",
                "asset did not load from nested archive");
        auto nested_busy = assets.unmount_archive("/nested");
        require(!nested_busy && nested_busy.error().code == ErrorCode::Busy,
                "cached nested asset did not protect archive mount");
        assets.clear();
        require(assets.unmount_archive("/nested"),
                "nested archive did not unmount after cache clear");
    }
    require(drive->unmount(container_mount.value()),
            "nested-archive container did not unmount");
    require(drive->discard_holodisk(container.value()),
            "nested-archive container did not discard");

    auto loaded = drive->load_holodisk(archive.string());
    require(static_cast<bool>(loaded), "materialized HoloDisk did not reload");
    auto read_mount = drive->mount(loaded.value(), "/cartridge", MountAccess::ReadOnly);
    require(static_cast<bool>(read_mount), "loaded HoloDisk did not mount");
    auto rejected = drive->open("/cartridge/config/settings.txt", OpenMode::Append);
    require(!rejected && rejected.error().code == ErrorCode::ReadOnly,
            "read-only mount accepted a writer");
    auto reader = drive->open("/cartridge/config/settings.txt", OpenMode::Read);
    require(static_cast<bool>(reader), "loaded HoloDisk file did not open");
    auto middle = drive->seek(reader.value(), 2, SeekOrigin::Begin);
    require(middle && middle.value() == 2, "archive seek failed");
    require(read_all(*drive, reader.value()) == "loaded",
            "archive streaming result differs");
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
