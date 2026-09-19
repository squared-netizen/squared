/*
 * The single translation unit that instantiates stb_image.
 *
 * Kept apart from squared's own sources so it can be compiled with warnings
 * off. stb_image is not ours to keep clean, and squared builds with
 * -Wconversion, under which it emits hundreds of warnings that would bury a
 * real one.
 *
 * Configured down to what squared needs: no stdio, because every byte arrives
 * through sq::files; PNG only, because that is what the asset pipeline
 * produces. Both trim code out of the APK.
 */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ONLY_PNG
#define STBI_ASSERT(x) ((void)0)
#include "stb_image.h"
