# struct CellFlags

*Defined at [term/cell_flags.cppm:15](../../../../term/cell_flags.cppm#L15)*



**brief** What a cell's colours and width mean.

 A colour given as "the default" (SGR 39, 49, or nothing yet) is the terminal's own foreground or background, whatever the theme makes it, not an RGB value: the flags say so, and the cell's `Rgb` is then unused.



## Members

public bool default_foreground

public bool default_background

public bool wide

public bool wide_tail



