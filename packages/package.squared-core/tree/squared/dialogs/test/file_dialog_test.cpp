// sq::FileDialog without a window: a folder listed (folders first, by name in
// any case; hidden names and other endings left out), gone into and out of,
// and what each mode chooses; the app told the choice, or that it was
// cancelled.

#include <stdio.h>

import sq.app;
import sq.dialogs;
import sq.stage;

import std;

namespace
{

#define CHECK( ... )                                                                               \
    do {                                                                                           \
        if( !( __VA_ARGS__ ) ) {                                                                   \
            std::fprintf( stderr, "file_dialog_test.cpp:%d: %s\n", __LINE__, #__VA_ARGS__ );       \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

sq::App the_app;

std::string told;
int tells = 0;
void chosen( void *context, std::string_view path ) noexcept
{
    told = path;
    tells += *static_cast<int *>( context );
}

void touch( const std::filesystem::path &file )
{
    std::ofstream( file ) << "x";
}

std::vector<std::string> names( const sq::FileDialog &dialog )
{
    std::vector<std::string> out;
    for( const sq::FileDialogEntry &e : dialog.entries() ) {
        out.push_back( e.folder ? e.name + "/" : e.name );
    }
    return out;
}

} // namespace

void squared_main( sq::App & /*app*/ )
{}

int main()
{
    const char *tmp = std::getenv( "TMPDIR" );
    const std::filesystem::path base =
        std::filesystem::path( tmp != nullptr ? tmp : "/tmp" ) /
        ( "sq-file-dialog-test-" + std::to_string( std::random_device{}() ) );
    const std::filesystem::path dir = base / "exports";
    std::filesystem::create_directories( dir / "sub" );
    std::filesystem::create_directories( dir / "Zed" );
    std::filesystem::create_directories( dir / ".cache" );
    touch( dir / "b.sq" );
    touch( dir / "A.SQ" );
    touch( dir / "notes.txt" );
    touch( dir / ".hidden.sq" );
    touch( dir / "sq" ); // the ending alone is no name
    int one = 1;

    // Open: folders first, then the .sq files, any case; nothing chosen yet.
    {
        sq::FileDialogOptions options;
        options.folder = dir.string() + "/";
        options.extensions = { ".sq" };
        options.chosen = &chosen;
        options.context = &one;
        sq::FileDialog dialog( std::move( options ) );
        CHECK( dialog.folder() == dir.string() );
        CHECK( names( dialog ) == std::vector<std::string>{ "sub/", "Zed/", "A.SQ", "b.sq" } );
        CHECK( dialog.problem().empty() && !dialog.choice().has_value() );
        dialog.set_name( "b.sq" );
        CHECK( dialog.choice() == ( dir / "b.sq" ).string() );
        dialog.set_name( "missing.sq" );
        CHECK( !dialog.choice().has_value() );

        // In and out; a folder left clears the file chosen in it.
        dialog.set_name( "b.sq" );
        CHECK( dialog.enter( "sub" ) && dialog.folder() == ( dir / "sub" ).string() );
        CHECK( dialog.entries().empty() && dialog.name().empty() );
        CHECK( dialog.up() && dialog.folder() == dir.string() );
        CHECK( !dialog.enter( ".." ) && !dialog.enter( "a/b" ) && !dialog.enter( "b.sq" ) );
        CHECK( !dialog.go( "relative/path" ) && !dialog.go( ( dir / "nothing" ).string() ) );
        CHECK( dialog.go( "/" ) && dialog.folder() == "/" && !dialog.up() );
        CHECK( dialog.go( dir.string() ) );

        // Hidden ones, when asked for.
        sq::FileDialogOptions all;
        all.folder = dir.string();
        all.extensions = { ".sq" };
        all.hidden = true;
        CHECK(
            names( sq::FileDialog( std::move( all ) ) ) ==
            std::vector<std::string>{ ".cache/", "sub/", "Zed/", ".hidden.sq", "A.SQ", "b.sq" } );

        // Chosen: told the path; cancelled: told nothing. Not shown, so no pane.
        dialog.set_name( "A.SQ" );
        CHECK( dialog.finish( the_app.stage, true ) && told == ( dir / "A.SQ" ).string() &&
               tells == 1 );
        CHECK( dialog.finish( the_app.stage, false ) && told.empty() && tells == 2 );
        dialog.set_name( "missing.sq" );
        CHECK( !dialog.finish( the_app.stage, true ) && tells == 2 );
        CHECK( sq::FileDialog::current() == nullptr );
    }

    // Save: the name gets the ending; what it would replace is said.
    {
        sq::FileDialogOptions options;
        options.mode = sq::FileDialogMode::save;
        options.folder = dir.string();
        options.name = "export";
        options.extensions = { ".sq" };
        sq::FileDialog dialog( std::move( options ) );
        CHECK( dialog.name() == "export" && dialog.choice() == ( dir / "export.sq" ).string() );
        CHECK( !dialog.replaces() );
        dialog.set_name( "b" );
        CHECK( dialog.choice() == ( dir / "b.sq" ).string() && dialog.replaces() );
        dialog.set_name( "Mine.SQ" );
        CHECK( dialog.choice() == ( dir / "Mine.SQ" ).string() );
        for( const char *bad : { "", ".", "..", "a/b" } ) {
            dialog.set_name( bad );
            CHECK( !dialog.choice().has_value() && !dialog.replaces() );
        }
    }

    // Folder: folders only; the folder shown is the choice.
    {
        sq::FileDialogOptions options;
        options.mode = sq::FileDialogMode::folder;
        options.folder = dir.string();
        sq::FileDialog dialog( std::move( options ) );
        CHECK( names( dialog ) == std::vector<std::string>{ "sub/", "Zed/" } );
        CHECK( dialog.choice() == dir.string() );
    }

    // No folder given and no places: the top. A folder that cannot be: the top.
    {
        sq::FileDialog top( sq::FileDialogOptions{} );
        CHECK( top.folder() == "/" );
        sq::FileDialogOptions gone;
        gone.folder = ( base / "not-there" ).string();
        CHECK( sq::FileDialog( std::move( gone ) ).folder() == "/" );
    }

    // The app's places, with no window: the top alone.
    const std::vector<sq::FilePlace> places = sq::FileDialog::places_of( the_app.stage );
    CHECK( !places.empty() && places.back().path == "/" );

    std::filesystem::remove_all( base );
    std::puts( "file_dialog_test: ok" );
    return 0;
}
