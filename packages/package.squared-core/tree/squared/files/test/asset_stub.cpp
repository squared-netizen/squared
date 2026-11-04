// A fake asset bundle: a fixed map of name to bytes, served the way
// AAssetManager serves one - including the short reads a compressed asset
// produces, which is the behaviour the real reader loop exists for.
#include <android/asset_manager.h>

import std;

namespace assetstub
{

std::map<std::string, std::string> entries;
int short_read_bytes = 0; // 0 = read everything in one call

void reset()
{
    entries.clear();
    short_read_bytes = 0;
}
void add( const std::string &name, const std::string &data )
{
    entries[name] = data;
}

} // namespace assetstub

struct AAsset
{
        const std::string *data;
        size_t offset;
};

struct AAssetDir
{
        std::vector<std::string> names;
        size_t index;
        std::string current;
};

extern "C" {

AAsset *AAssetManager_open( AAssetManager *, const char *name, int )
{
    const auto entry = assetstub::entries.find( name );
    if( entry == assetstub::entries.end() ) {
        return nullptr;
    }
    return new AAsset{ &entry->second, 0 };
}

off64_t AAsset_getLength64( AAsset *asset )
{
    return static_cast<off64_t>( asset->data->size() );
}

int AAsset_read( AAsset *asset, void *out, size_t count )
{
    const size_t remaining = asset->data->size() - asset->offset;
    size_t take = count < remaining ? count : remaining;
    if( assetstub::short_read_bytes > 0 &&
        take > static_cast<size_t>( assetstub::short_read_bytes ) ) {
        take = static_cast<size_t>( assetstub::short_read_bytes );
    }
    std::memcpy( out, asset->data->data() + asset->offset, take );
    asset->offset += take;
    return static_cast<int>( take );
}

const void *AAsset_getBuffer( AAsset *asset )
{
    return asset->data->data();
}

// A compressed asset is unpacked into memory the asset owns: the stub says so
// for names ending .z, as a test of in_place().
int AAsset_isAllocated( AAsset *asset )
{
    for( const auto &entry : assetstub::entries ) {
        if( &entry.second == asset->data ) {
            return entry.first.ends_with( ".z" ) ? 1 : 0;
        }
    }
    return 0;
}

void AAsset_close( AAsset *asset )
{
    delete asset;
}

AAssetDir *AAssetManager_openDir( AAssetManager *, const char *path )
{
    std::string prefix{ path };
    if( !prefix.empty() && prefix.back() != '/' ) {
        prefix += '/';
    }
    if( prefix == "/" ) {
        prefix.clear();
    }

    auto *directory = new AAssetDir{ {}, 0, {} };
    for( const auto &[name, data] : assetstub::entries ) {
        if( name.rfind( prefix, 0 ) != 0 ) {
            continue;
        }
        const std::string tail = name.substr( prefix.size() );
        if( tail.empty() || tail.find( '/' ) != std::string::npos ) {
            continue;
        }
        directory->names.push_back( tail );
    }
    return directory;
}

const char *AAssetDir_getNextFileName( AAssetDir *directory )
{
    if( directory->index >= directory->names.size() ) {
        return nullptr;
    }
    directory->current = directory->names[directory->index++];
    return directory->current.c_str();
}

void AAssetDir_close( AAssetDir *directory )
{
    delete directory;
}

} // extern "C"
