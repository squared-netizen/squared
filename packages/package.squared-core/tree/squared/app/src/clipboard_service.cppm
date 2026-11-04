export module sq.app:src.clipboard_service;

import std;

export namespace sq::app
{

/**
 * @brief The system clipboard's text, for the application: what a paste
 * reads and a copy writes.
 *
 * Implemented by the platform adapter (Android's ClipboardManager, through
 * JNI); an application gets it through Application::set_clipboard_service
 * (a squared App passes it to its Stage: `stage.clipboard()`), and has none
 * where the platform gives none. Called on the application thread.
 */
class ClipboardService
{
    public:
        virtual ~ClipboardService() = default;

        /**
         * @brief The clipboard's text as UTF-8, into `out`: whole
         * characters, in order, until one does not fit. The bytes the whole
         * text takes, whatever `out` holds; 0 when the clipboard holds no
         * text. Call it with an empty `out` for the size, then with that much.
         */
        [[nodiscard]] virtual std::size_t text( std::span<char> out ) = 0;

        /** @brief The clipboard holds `utf8` now. False when the platform refused. */
        virtual bool set_text( std::string_view utf8 ) = 0;
};

} // namespace sq::app
