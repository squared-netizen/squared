export module sq.dialogs:file_dialog;

import std;
import sq.gui;
import sq.stage;

export namespace sq
{

/** @brief What a file dialog chooses. */
enum class FileDialogMode : std::uint8_t {
    open,   ///< an existing file (Import...)
    save,   ///< a folder and a new file's name (Export...)
    folder, ///< a folder
};

/** @brief A place the dialog offers as a button: a label and a folder. */
struct FilePlace final
{
        std::string label;
        std::string path;
};

/** @brief What a folder shows: a name, and whether it is a folder. */
struct FileDialogEntry final
{
        std::string name;
        bool folder = false;
};

/**
 * @brief Told what was chosen: the whole path, or empty when the dialog was
 * cancelled. On the UI thread, once, as the dialog closes.
 */
using FileChosen = void ( * )( void *context, std::string_view path ) noexcept;

/** @brief What a dialog is asked for. */
struct FileDialogOptions final
{
        FileDialogMode mode = FileDialogMode::open;
        /// Its heading (`Export preferences`).
        std::string title;
        /// Where it starts; empty: the first place's folder.
        std::string folder;
        /// A file name to start with (save): the person can change it.
        std::string name;
        /// The files shown (open) and the one written (save), by ending
        /// (`.sq`), any case; empty: every file. A saved name with none of
        /// them gets the first.
        std::vector<std::string> extensions;
        /// The places offered; empty: the app's own (`FileDialog::places_of`).
        std::vector<FilePlace> places;
        /// Folders and files whose names start with a dot.
        bool hidden = false;
        FileChosen chosen = nullptr;
        void *context = nullptr;
};

/**
 * @brief A file dialog: the folders and files of one folder at a time, the
 * app's places, a name to save as; what is chosen goes to the app through
 * `chosen`. Any app, any screen:
 *
 * ```cpp
 * sq::FileDialogOptions options;
 * options.mode = sq::FileDialogMode::save;
 * options.title = "Export preferences";
 * options.name = "dev.squared.sqide-preferences-2026-11-25.sq";
 * options.extensions = { ".sq" };
 * options.chosen = &exported;   // void exported( void *, std::string_view path ) noexcept
 * sq::FileDialog::show( app.stage, std::move( options ) );
 * ```
 *
 * `show` opens it as a pane (`FileDialogPane`, modal, over the screen
 * shown); one at a time. The rest is the dialog's state, which the pane
 * shows and a test drives: real paths, read with std::filesystem, on the UI
 * thread (a folder is listed when it is entered, not watched).
 */
class FileDialog final
{
    public:
        /** @brief The most entries a folder shows; the rest are left out, and said. */
        static constexpr std::size_t max_entries = 2000;

        explicit FileDialog( FileDialogOptions options );

        /**
         * @brief Show a dialog over `stage`'s screen; false when one is
         * showing already or the pane cannot open (the problem is reported).
         * The dialog registers its pane's view with the Stage.
         */
        static bool show( Stage &stage, FileDialogOptions options );

        /** @brief The dialog showing, or nullptr. */
        [[nodiscard]] static FileDialog *current() noexcept;

        /**
         * @brief The app's places: its files (Local), shared storage
         * (External), the files the app was given (Internal), each where the
         * platform has one; and the file system's top.
         */
        [[nodiscard]] static std::vector<FilePlace> places_of( Stage &stage );

        [[nodiscard]] const FileDialogOptions &options() const noexcept { return options_; }
        [[nodiscard]] const std::string &folder() const noexcept { return folder_; }
        /** @brief The folder's folders, then its files, each by name, any case. */
        [[nodiscard]] std::span<const FileDialogEntry> entries() const noexcept { return entries_; }
        /** @brief Why the folder shows nothing, or what it left out; empty when nothing went wrong. */
        [[nodiscard]] const std::string &problem() const noexcept { return problem_; }
        /** @brief The name chosen: a file selected (open), the name typed (save). */
        [[nodiscard]] const std::string &name() const noexcept { return name_; }

        /** @brief Show `path`'s folder; false (the folder kept) when it is not one. */
        bool go( std::string_view path );
        /** @brief Into the folder `name`, here. */
        bool enter( std::string_view name );
        /** @brief Up to the folder this one is in; false at the top. */
        bool up();
        /** @brief Read the folder again. */
        void refresh();
        /** @brief Choose the file `name` (open), or set the name to save as (save). */
        void set_name( std::string_view name );

        /**
         * @brief What would be chosen now: the file (open), the folder and the
         * name with its ending (save), the folder (folder); nothing while
         * there is none (no file selected, an empty name, a name with `/`).
         */
        [[nodiscard]] std::optional<std::string> choice() const;
        /** @brief Whether saving now would replace a file there. */
        [[nodiscard]] bool replaces() const;

        /**
         * @brief Done: the app told the choice (or that it was cancelled),
         * the pane closed and the dialog gone. False, and nothing done, when
         * `accept` and there is no choice.
         */
        bool finish( Stage &stage, bool accept );

    private:
        FileDialogOptions options_;
        std::string folder_;
        std::vector<FileDialogEntry> entries_;
        std::string problem_;
        std::string name_;
};

/**
 * @brief The file dialog's pane (`FileDialog::show` opens it): modal, in the
 * middle; its view is the current dialog's.
 */
struct FileDialogPane final
{
        static constexpr std::string_view name = "file_dialog";
        static constexpr gui::PanePlace place = gui::PanePlace::center;
        static constexpr bool modal = true;
        static void build( gui::LayoutWriter &root );
};

} // namespace sq
