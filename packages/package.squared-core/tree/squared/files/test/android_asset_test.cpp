#include <squared/files/android_asset_file_system.hpp>
#include <squared/files/files.hpp>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace assetstub {
void reset();
void add(const std::string& name, const std::string& data);
extern int short_read_bytes;
}

using namespace sq::files;

int main()
{
    assetstub::reset();
    assetstub::add("skin/uiskin.atlas", "uiskin.png\nsize: 256,128\n");
    assetstub::add("skin/uiskin.json", "{}");
    assetstub::add("skin/uiskin.png", std::string(4096, '\xAB'));

    const char* tmpdir = std::getenv("TMPDIR");
    const std::string base =
        std::string{tmpdir != nullptr ? tmpdir : "."} + "/sq_asset_test";

    AAssetManager* fake = nullptr;   // the stub ignores it
    AndroidAssetFileSystem fs{*fake, AndroidStorageRoots{
        .local_root = base + "/files", .external_root = {}}};

    // --- Internal reads come from the bundle ------------------------------
    {
        FileHandle atlas = fs.internal("skin/uiskin.atlas");
        assert(atlas.exists());
        assert(!atlas.is_directory());
        assert(atlas.length() == 25);

        auto text = atlas.read_string();
        assert(text);
        assert(text.text.find("uiskin.png") != std::string::npos);

        assert(!fs.internal("skin/missing.png").exists());
        assert(fs.internal("skin/missing.png").read_bytes().error.code
               == FileErrorCode::NotFound);
    }

    // --- a compressed asset returns short reads; the loop must cope -------
    {
        assetstub::short_read_bytes = 100;
        auto bytes = fs.internal("skin/uiskin.png").read_bytes();
        assert(bytes);
        assert(bytes.bytes.size() == 4096);   // not 100
        assetstub::short_read_bytes = 0;
    }

    // --- Internal is a directory when it has entries ----------------------
    {
        assert(fs.internal("skin").is_directory());
        assert(fs.internal("skin").exists());

        std::vector<FileHandle> entries;
        assert(!fs.internal("skin").list(entries));
        assert(entries.size() == 3);
        for (const FileHandle& entry : entries) {
            assert(entry.path().rfind("skin/", 0) == 0);
            assert(entry.exists());
        }
    }

    // --- Internal refuses every write path --------------------------------
    {
        assert(fs.internal("skin/new.png").write_string("x").code
               == FileErrorCode::ReadOnly);
        assert(fs.internal("skin").make_directories().code
               == FileErrorCode::ReadOnly);
        assert(fs.internal("skin/uiskin.png").remove().code
               == FileErrorCode::ReadOnly);
    }

    // --- an asset path that escapes the bundle is refused -----------------
    {
        assert(!fs.internal("../secret").exists());
        assert(fs.internal("../secret").read_bytes().error.code
               == FileErrorCode::NotSupported);
        assert(!fs.internal("/etc/passwd").exists());
    }

    // --- Local is ordinary storage, writable, no permission ---------------
    {
        FileHandle save = fs.local("saves/slot1.json");
        assert(!save.write_string("{\"level\":3}"));
        assert(save.exists());
        assert(save.read_string().text == "{\"level\":3}");

        std::vector<FileHandle> saves;
        assert(!fs.local("saves").list(saves));
        assert(saves.size() == 1);

        assert(!save.remove());
        assert(!fs.local("saves").remove());
        assert(!fs.local("").remove());
    }

    // --- External unset fails cleanly rather than resolving somewhere -----
    {
        assert(!fs.external("anything").exists());
        assert(fs.external("anything").write_string("x").code
               == FileErrorCode::NotSupported);
    }

    // --- nested trees, answered from the build-time index -----------------
    {
        assetstub::reset();
        assetstub::add("skins/default/skin/uiskin.atlas", "atlas");
        assetstub::add("skins/default/skin/uiskin.png", "png");
        assetstub::add("skins/default/skin/default.fnt", "fnt");
        assetstub::add("audio/sfx/ui/click.ogg", "ogg");
        assetstub::add("readme.txt", "hi");
        assetstub::add("SQ-INF/index",
            "skins/default/skin/uiskin.atlas\n"
            "skins/default/skin/uiskin.png\n"
            "skins/default/skin/default.fnt\n"
            "audio/sfx/ui/click.ogg\n"
            "readme.txt\n");

        // the case AAssetDir gets wrong: a directory holding only a directory
        assert(fs.internal("skins").is_directory());
        assert(fs.internal("skins").exists());
        assert(fs.internal("skins/default").is_directory());

        // a file is not a directory, and a missing path is neither
        assert(!fs.internal("skins/default/skin/uiskin.png").is_directory());
        assert(fs.internal("skins/default/skin/uiskin.png").exists());
        assert(!fs.internal("skins/nope").exists());

        // listing returns subdirectories, not only files
        std::vector<FileHandle> top;
        assert(!fs.internal("").list(top));
        std::vector<std::string> names;
        for (const auto& entry : top) names.emplace_back(entry.name());
        auto has = [&](const char* n) {
            for (const auto& x : names) if (x == n) return true;
            return false;
        };
        assert(has("skins") && has("audio") && has("readme.txt"));
        assert(!has("SQ-INF"));            // bundle metadata is hidden
        assert(names.size() == 3);         // no duplicates from shared prefixes

        std::vector<FileHandle> skins;
        assert(!fs.internal("skins").list(skins));
        assert(skins.size() == 1 && skins[0].name() == "default");

        std::vector<FileHandle> leaf;
        assert(!fs.internal("skins/default/skin").list(leaf));
        assert(leaf.size() == 3);

        // deep reads are unaffected by the index
        assert(fs.internal("audio/sfx/ui/click.ogg").read_string().text
               == "ogg");

        assert(fs.internal("nowhere").list(leaf).code
               == FileErrorCode::NotFound);
    }

    // --- no index: degraded to files-only, never broken -------------------
    {
        assetstub::reset();
        assetstub::add("skins/default/skin/uiskin.png", "png");
        assetstub::add("icon.png", "i");

        std::vector<FileHandle> root;
        assert(!fs.internal("").list(root));
        assert(root.size() == 1 && root[0].name() == "icon.png");

        // files still read at any depth
        assert(fs.internal("skins/default/skin/uiskin.png").read_string().text
               == "png");
        // and the known limitation is exactly what it was
        assert(!fs.internal("skins").is_directory());
    }

    std::printf("android assets: all assertions passed\n");
    return 0;
}
