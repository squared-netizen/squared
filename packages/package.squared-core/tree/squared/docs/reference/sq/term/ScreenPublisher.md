# class ScreenPublisher

*Defined at [term/screen_publisher.cppm:32](../../../../term/screen_publisher.cppm#L32)*



**brief** A terminal's screen written as a snapshot, on the model's thread, for a `ScreenView` on the UI's (the app model's third channel: the model publishes it through the Telegraph, `Telegraph::snapshot` and `publish`).

 Each publish is the whole screen (12 bytes a cell: 24 KB for 50 x 40), because the UI may skip publishes: what it reads must stand alone. What lets the UI redraw only what changed is a version for each row, moved on when the row changed since the publish before (or every row, when the size or the view moved): the UI redraws the rows whose version is not the one it drew.

 The view can be scrolled back into the scrollback (`scroll_back`): the rows shown are then the history's, then the screen's, and the cursor is hidden.



 sq::term::ScreenPublisher publisher;

 auto room = t.snapshot( screen, publisher.bytes_for( terminal ) );

 t.publish( screen, publisher.write( terminal, room ) );





## Functions

### bytes_for

*public std::size_t bytes_for(const Terminal & terminal)*

*Defined at [term/screen_publisher.cppm:36](../../../../term/screen_publisher.cppm#L36)*

 The bytes `write` needs for `terminal` now. 

### scroll_back

*public void scroll_back(std::size_t lines)*

*Defined at [term/screen_publisher.cppm:52](../../../../term/screen_publisher.cppm#L52)*

 The view `lines` back into the scrollback (0: the screen itself, as the default); held to what the scrollback has at each write.

### scrolled_back

*public std::size_t scrolled_back()*

*Defined at [term/screen_publisher.cppm:53](../../../../term/screen_publisher.cppm#L53)*

### write

*public std::size_t write(Terminal & terminal, std::span<std::byte> out)*

 The screen into `out` (at least `bytes_for( terminal )`), the rows marked dirty given new versions, and the terminal's dirty marks cleared. The bytes written; 0 when `out` is too small.

