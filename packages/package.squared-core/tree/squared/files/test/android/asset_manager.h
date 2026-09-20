#pragma once
/*
 * Fake AAssetManager for host testing.
 *
 * Declared to match the NDK's <android/asset_manager.h>. The real header was
 * not available when this was written, so the signatures come from the
 * documented API rather than from the header itself - that is the weakest part
 * of this test, and a mismatch would be a compile error on device rather than
 * a silent one.
 */
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifndef AASSET_MODE_UNKNOWN
#define AASSET_MODE_UNKNOWN 0
#define AASSET_MODE_RANDOM 1
#define AASSET_MODE_STREAMING 2
#define AASSET_MODE_BUFFER 3
#endif

struct AAssetManager;
struct AAsset;
struct AAssetDir;

#ifdef __cplusplus
extern "C" {
#endif
AAsset* AAssetManager_open(AAssetManager*, const char*, int);
AAssetDir* AAssetManager_openDir(AAssetManager*, const char*);
const char* AAssetDir_getNextFileName(AAssetDir*);
void AAssetDir_close(AAssetDir*);
int AAsset_read(AAsset*, void*, size_t);
off64_t AAsset_getLength64(AAsset*);
void AAsset_close(AAsset*);
#ifdef __cplusplus
}
#endif
