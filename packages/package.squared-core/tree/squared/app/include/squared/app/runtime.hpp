#pragma once

namespace sq::graphics {
class Context;
}  // namespace sq::graphics

namespace sq::files {
class FileSystem;
}  // namespace sq::files

namespace sq::assets {
class AssetManager;
}  // namespace sq::assets

namespace sq::app {

/**
 * @brief The services only the platform layer can construct.
 *
 * Handed to Application::create() once, at startup. The application keeps
 * references to whichever members it uses.
 *
 * **Admission rule.** Something belongs here only if its lifetime is the whole
 * process *and* only the platform layer can build it. The rendering context
 * needs the native window. The file system needs the asset manager and the
 * app's private storage path, which only the platform knows. The asset
 * manager needs that file system. All three pass.
 *
 * A sprite batch, a skin or a UI does not pass: the application can make its
 * own, and putting them here would turn this into the object every subsystem
 * reaches into. That rule is what keeps a bundle of three references from
 * becoming a Service Locator.
 *
 * **Non-owning.** Every member is a reference to an object the platform layer
 * owns for the life of the process, so this is three pointers wide and cheap
 * to pass. It holds references rather than owning because the file system is
 * polymorphic - a POSIX one in Termux, an asset-backed one on Android - and
 * owning it would mean a heap allocation or a template on the platform type.
 *
 * Not global. Nothing reaches in by type. It is passed explicitly, once.
 */
struct Runtime final {
    /** @brief The rendering context. Survives surface loss; see Context. */
    graphics::Context& graphics;

    /**
     * @brief Storage. Internal is the read-only asset bundle, Local is the
     * app's private writable storage, External is shared storage.
     */
    files::FileSystem& files;

    /** @brief The typed asset cache, reading through `files`. */
    assets::AssetManager& assets;
};

}  // namespace sq::app
