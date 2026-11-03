# struct Cell

*Defined at [term/cell.cppm:19](../../../../term/cell.cppm#L19)*



**brief** One character cell of a terminal: 12 bytes.

 The character, its two colours in 24-bit RGB, how it is drawn, and what the colours and width mean (decided 2026-11-02: colours inline, over a smaller cell with a table of colours beside the screen). A blank is a space in the default colours: `Cell{}`.



## Members

public char32_t code

public Rgb foreground

public Rgb background

public CellAttributes attributes

public CellFlags flags



