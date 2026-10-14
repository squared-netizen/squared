#pragma once

// Every public type in sq::files.
// Prefer a narrower include in headers of your own.

// android_asset_file_system.hpp is deliberately not included here: it names
// AAssetManager, and this header is included on every platform.
#include <squared/files/file_handle.hpp>
#include <squared/files/file_system.hpp>
#include <squared/files/posix_file_system.hpp>
#include <squared/files/src/file_error.hpp>
#include <squared/files/src/file_error_code.hpp>
#include <squared/files/src/file_read_result.hpp>
#include <squared/files/src/file_text_result.hpp>
#include <squared/files/src/file_type.hpp>
