# struct Line

*Defined at [console/src/line.hpp:16](../../../../console/src/line.hpp#L16)*



**brief** One line of console text, as the scrollback hands it out.

 `text` points into the scrollback's storage: it is valid until the next push() or clear(), which may overwrite it. Copy it out to keep it.



## Members

public std::string_view text

public Level level



