# struct ScreenHeader

*Defined at [term/src/screen_layout.cppm:17](../../../../../term/src/screen_layout.cppm#L17)*

 A screen snapshot's bytes: this header, then a version for each row (`std::uint32_t`), then the rows' cells, row by row. Written by `ScreenPublisher` on the model's thread, read by `ScreenView` on the UI's; both are this file, so the two never disagree.



## Members

public std::uint32_t layout

public std::uint16_t columns

public std::uint16_t rows

public Cursor cursor

public TerminalModes modes

public std::uint8_t title_size

public std::uint16_t reserved

public std::uint32_t history_size

public std::uint32_t scrolled_back

public std::uint64_t top_line

public array title

public static const std::uint32_t current



