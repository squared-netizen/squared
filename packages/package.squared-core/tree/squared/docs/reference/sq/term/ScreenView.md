# class ScreenView

*Defined at [term/screen_view.cppm:24](../../../../term/screen_view.cppm#L24)*



**brief** A screen snapshot as the UI reads it: its size, each row's cells and version, the cursor, the modes, the title. A view over the bytes it was made from; nothing is copied.



 if( const auto shot = t.snapshot( screen ) ) {

     if( const auto view = sq::term::ScreenView::read( shot->bytes ) ) {

         for( std::uint16_t r = 0; r < view->rows(); ++r ) {

             if( view->version( r ) != drawn[r] ) { draw_row( r, view->row( r ) ); }

         }

     }

 }





## Functions

### columns

*public std::uint16_t columns()*

*Defined at [term/screen_view.cppm:31](../../../../term/screen_view.cppm#L31)*

### cursor

*public Cursor cursor()*

*Defined at [term/screen_view.cppm:39](../../../../term/screen_view.cppm#L39)*

### cursor_shown

*public bool cursor_shown()*

*Defined at [term/screen_view.cppm:41](../../../../term/screen_view.cppm#L41)*

 The cursor is to be drawn: the program shows it and the view is not scrolled back. 

### history_size

*public std::size_t history_size()*

*Defined at [term/screen_view.cppm:50](../../../../term/screen_view.cppm#L50)*

### modes

*public TerminalModes modes()*

*Defined at [term/screen_view.cppm:45](../../../../term/screen_view.cppm#L45)*

### read

*public static std::optional<ScreenView> read(std::span<const std::byte> bytes)*

 The view, when `bytes` are a whole screen snapshot of this layout; else nothing. 

### row

*public std::span<const Cell> row(std::uint16_t row)*

*Defined at [term/screen_view.cppm:33](../../../../term/screen_view.cppm#L33)*

### rows

*public std::uint16_t rows()*

*Defined at [term/screen_view.cppm:32](../../../../term/screen_view.cppm#L32)*

### scrolled_back

*public std::size_t scrolled_back()*

*Defined at [term/screen_view.cppm:51](../../../../term/screen_view.cppm#L51)*

### title

*public std::string_view title()*

*Defined at [term/screen_view.cppm:46](../../../../term/screen_view.cppm#L46)*

### version

*public std::uint32_t version(std::uint16_t row)*

 Moves on each time the row changes. 

