// sq::FileDialog and its pane's view (file_dialog.cppm says what is here).

module sq.dialogs;

import std;
import sq.files;
import sq.gui;
import sq.stage;

namespace sq
{

namespace
{

/// The dialog showing: one at a time.
std::optional<FileDialog> the_dialog;

constexpr float padding = 14.0F;
constexpr float spacing = 8.0F;

/// `text`, lowercase (ASCII): names and endings compare in any case.
std::string lower( std::string_view text )
{
    std::string out( text );
    for( char &c : out ) {
        c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
    }
    return out;
}

/// Whether `name` ends in one of `extensions` (any case); true when there are none.
bool has_extension( std::string_view name, const std::vector<std::string> &extensions )
{
    if( extensions.empty() ) {
        return true;
    }
    const std::string low = lower( name );
    return std::ranges::any_of( extensions, [&low]( const std::string &e ) {
        const std::string ext = lower( e );
        return low.size() > ext.size() && low.ends_with( ext );
    } );
}

/// A path, with no `.`, `..` or doubled `/` (and no `/` at the end but the top's).
std::string normal( const std::filesystem::path &path )
{
    std::string out = path.lexically_normal().string();
    while( out.size() > 1 && out.ends_with( '/' ) ) {
        out.pop_back();
    }
    return out.empty() ? std::string( "/" ) : out;
}

/// Shown again: the pane closed and opened, built from the dialog as it is now.
void show_again( Stage &stage ) noexcept
{
    static_cast<void>( stage.close() );
    static_cast<void>( stage.open<FileDialogPane>() );
}

/// A file name being typed (save): kept when the field's edit is finished.
void name_committed( void * /*context*/, gui::TextField &field )
{
    if( FileDialog *dialog = FileDialog::current(); dialog != nullptr ) {
        dialog->set_name( field.text() );
    }
}

/// The pane's view: the current dialog, laid out.
std::unique_ptr<gui::Widget> make_view( Stage &stage )
{
    FileDialog *dialog = FileDialog::current();
    if( dialog == nullptr ) {
        return nullptr;
    }
    const FileDialogOptions &options = dialog->options();
    auto table = std::make_unique<gui::Table>();
    table->set_padding( padding );
    table->set_spacing( spacing );
    gui::Table &t = *table;
    const auto across = [&t]( std::unique_ptr<gui::Widget> widget ) -> gui::Widget & {
        gui::Widget &made = *widget;
        t.row();
        t.add( std::move( widget ) ).column_span( 2 ).fill_x();
        return made;
    };

    if( !options.title.empty() ) {
        across( std::make_unique<gui::Label>( options.title ) );
    }

    // The places, a row of buttons.
    auto places = std::make_unique<gui::Table>();
    places->set_spacing( spacing );
    for( const FilePlace &place : options.places ) {
        auto button = std::make_unique<gui::Button>( place.label );
        button->set_on_click( [&stage, path = place.path] {
            FileDialog *d = FileDialog::current();
            if( d != nullptr && d->go( path ) ) {
                show_again( stage );
            }
        } );
        places->add( std::move( button ) );
    }
    across( std::move( places ) );

    // Where: up, and the folder.
    t.row();
    auto up = std::make_unique<gui::Button>( "Up" );
    up->set_on_click( [&stage] {
        FileDialog *d = FileDialog::current();
        if( d != nullptr && d->up() ) {
            show_again( stage );
        }
    } );
    t.add( std::move( up ) );
    t.add( std::make_unique<gui::Label>( dialog->folder() ) ).grow_x().fill_x();

    // What is there: a folder goes in; a file is chosen (open) or named after (save).
    auto list = std::make_unique<gui::Table>();
    list->set_spacing( 4.0F );
    for( const FileDialogEntry &entry : dialog->entries() ) {
        const bool selected = !entry.folder && entry.name == dialog->name();
        auto button = std::make_unique<gui::Button>( entry.folder ? entry.name + "/"
                                                     : selected   ? "> " + entry.name
                                                                  : entry.name );
        button->set_on_click( [&stage, name = entry.name, folder = entry.folder] {
            FileDialog *d = FileDialog::current();
            if( d == nullptr ) {
                return;
            }
            if( folder ) {
                static_cast<void>( d->enter( name ) );
            } else {
                d->set_name( name );
            }
            show_again( stage );
        } );
        list->row();
        list->add( std::move( button ) ).fill_x().grow_x();
    }
    auto scroll = std::make_unique<gui::ScrollPane>();
    scroll->set_content( std::move( list ) );
    t.row();
    t.add( std::move( scroll ) ).column_span( 2 ).grow().fill();

    if( !dialog->problem().empty() ) {
        across( std::make_unique<gui::Label>( dialog->problem() ) );
    }

    // Save: the name, and whether it replaces a file.
    gui::TextField *field = nullptr;
    if( options.mode == FileDialogMode::save ) {
        t.row();
        t.add( std::make_unique<gui::Label>( "Name" ) );
        auto name = std::make_unique<gui::TextField>( dialog->name() );
        field = name.get();
        name->set_on_commit( &name_committed, nullptr );
        t.add( std::move( name ) ).grow_x().fill_x();
        if( dialog->replaces() ) {
            across( std::make_unique<gui::Label>( "Replaces the file there" ) );
        }
    }

    // Cancel, and the choice.
    t.row();
    auto cancel = std::make_unique<gui::Button>( "Cancel" );
    cancel->set_on_click( [&stage] {
        if( FileDialog *d = FileDialog::current(); d != nullptr ) {
            static_cast<void>( d->finish( stage, false ) );
        }
    } );
    t.add( std::move( cancel ) );
    const char *act = options.mode == FileDialogMode::open   ? "Open"
                      : options.mode == FileDialogMode::save ? "Save"
                                                             : "Use this folder";
    auto choose = std::make_unique<gui::Button>( act );
    choose->set_on_click( [&stage, field] {
        FileDialog *d = FileDialog::current();
        if( d == nullptr ) {
            return;
        }
        if( field != nullptr ) {
            d->set_name( field->text() ); // what is typed, finished or not
        }
        static_cast<void>( d->finish( stage, true ) );
    } );
    t.add( std::move( choose ) ).fill_x();
    return table;
}

} // namespace

FileDialog::FileDialog( FileDialogOptions options )
    : options_( std::move( options ) )
    , name_( options_.mode == FileDialogMode::save ? options_.name : std::string() )
{
    std::string start = options_.folder;
    if( start.empty() && !options_.places.empty() ) {
        start = options_.places.front().path;
    }
    if( start.empty() || !go( start ) ) {
        static_cast<void>( go( "/" ) );
    }
}

bool FileDialog::show( Stage &stage, FileDialogOptions options )
{
    if( the_dialog.has_value() ) {
        return false;
    }
    if( options.places.empty() ) {
        options.places = places_of( stage );
    }
    the_dialog.emplace( std::move( options ) );
    static_cast<void>( stage.custom( "file_dialog_view", &make_view ) );
    if( !stage.open<FileDialogPane>() ) {
        the_dialog.reset();
        return false;
    }
    return true;
}

FileDialog *FileDialog::current() noexcept
{
    return the_dialog.has_value() ? &*the_dialog : nullptr;
}

std::vector<FilePlace> FileDialog::places_of( Stage &stage )
{
    std::vector<FilePlace> out;
    if( files::FileSystem *fs = stage.files(); fs != nullptr ) {
        const std::array<std::pair<files::FileType, std::string_view>, 3> roots{
            { { files::FileType::Local, "App" },
              { files::FileType::External, "Shared" },
              { files::FileType::Internal, "Given" } }
        };
        for( const auto &[type, label] : roots ) {
            std::string path = fs->resolve_platform_path( type, "" );
            std::error_code ec;
            if( !path.empty() && std::filesystem::is_directory( path, ec ) ) {
                out.push_back( { std::string( label ), normal( path ) } );
            }
        }
    }
    out.push_back( { "Top", "/" } );
    return out;
}

bool FileDialog::go( std::string_view path )
{
    std::error_code ec;
    const std::filesystem::path where( path );
    if( path.empty() || !where.is_absolute() || !std::filesystem::is_directory( where, ec ) ) {
        return false;
    }
    folder_ = normal( where );
    if( options_.mode == FileDialogMode::open ) {
        name_.clear(); // a file chosen was the other folder's
    }
    refresh();
    return true;
}

bool FileDialog::enter( std::string_view name )
{
    if( name.empty() || name == "." || name == ".." ||
        name.find( '/' ) != std::string_view::npos ) {
        return false;
    }
    return go( ( std::filesystem::path( folder_ ) / name ).string() );
}

bool FileDialog::up()
{
    if( folder_ == "/" ) {
        return false;
    }
    return go( std::filesystem::path( folder_ ).parent_path().string() );
}

void FileDialog::refresh()
{
    entries_.clear();
    problem_.clear();
    std::error_code ec;
    std::filesystem::directory_iterator it( folder_, ec );
    if( ec ) {
        problem_ = "Cannot read this folder: " + ec.message();
        return;
    }
    std::size_t left_out = 0;
    for( const std::filesystem::directory_iterator end; it != end; it.increment( ec ) ) {
        if( ec ) {
            problem_ = "Not all of this folder could be read: " + ec.message();
            break;
        }
        std::string name = it->path().filename().string();
        if( !options_.hidden && name.starts_with( '.' ) ) {
            continue;
        }
        std::error_code type_ec;
        const bool folder = it->is_directory( type_ec ); // a link to a folder is one
        if( !folder && ( options_.mode == FileDialogMode::folder ||
                         !has_extension( name, options_.extensions ) ) ) {
            continue;
        }
        if( entries_.size() == max_entries ) {
            ++left_out;
            continue;
        }
        entries_.push_back( { std::move( name ), folder } );
    }
    if( left_out > 0 ) {
        problem_ = std::format( "{} more not shown", left_out );
    }
    std::ranges::sort( entries_, []( const FileDialogEntry &a, const FileDialogEntry &b ) {
        if( a.folder != b.folder ) {
            return a.folder;
        }
        const std::string la = lower( a.name );
        const std::string lb = lower( b.name );
        return la != lb ? la < lb : a.name < b.name;
    } );
}

void FileDialog::set_name( std::string_view name )
{
    name_ = name;
}

std::optional<std::string> FileDialog::choice() const
{
    const std::filesystem::path here( folder_ );
    switch( options_.mode ) {
        case FileDialogMode::folder:
            return folder_;
        case FileDialogMode::open: {
            if( name_.empty() ) {
                return std::nullopt;
            }
            std::error_code ec;
            const std::filesystem::path file = here / name_;
            if( !std::filesystem::is_regular_file( file, ec ) ) {
                return std::nullopt;
            }
            return file.string();
        }
        case FileDialogMode::save: {
            if( name_.empty() || name_ == "." || name_ == ".." ||
                name_.find( '/' ) != std::string::npos ) {
                return std::nullopt;
            }
            std::string name = name_;
            if( !has_extension( name, options_.extensions ) ) {
                name += options_.extensions.front();
            }
            return ( here / name ).string();
        }
    }
    return std::nullopt;
}

bool FileDialog::replaces() const
{
    if( options_.mode != FileDialogMode::save ) {
        return false;
    }
    const std::optional<std::string> path = choice();
    std::error_code ec;
    return path.has_value() && std::filesystem::exists( *path, ec );
}

bool FileDialog::finish( Stage &stage, bool accept )
{
    const std::optional<std::string> chosen =
        accept ? choice() : std::optional<std::string>( std::string() );
    if( !chosen.has_value() ) {
        return false;
    }
    const FileChosen tell = options_.chosen;
    void *const context = options_.context;
    if( the_dialog.has_value() && &*the_dialog == this ) {
        static_cast<void>( stage.close() );
        the_dialog.reset(); // `this` is gone: only the copies above from here
    }
    if( tell != nullptr ) {
        tell( context, *chosen ); // after: the app may show another
    }
    return true;
}

void FileDialogPane::build( gui::LayoutWriter &root )
{
    root.add_custom( "file_dialog_view" ).grow();
}

} // namespace sq
