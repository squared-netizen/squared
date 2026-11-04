// Every public type in sq::files.
// Prefer a narrower include in headers of your own.
// android_asset_file_system.hpp is deliberately not included here: it names
// AAssetManager, and this header is included on every platform.

export module sq.files;

export import :android_asset_file_system;
export import :embedded_file_system;
export import :file_handle;
export import :file_system;
export import :mapped_file;
export import :posix_file_system;
export import :src.file_error;
export import :src.file_error_code;
export import :src.file_read_result;
export import :src.file_text_result;
export import :src.file_type;
