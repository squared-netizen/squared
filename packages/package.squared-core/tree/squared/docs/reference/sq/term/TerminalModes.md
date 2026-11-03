# struct TerminalModes

*Defined at [term/terminal_modes.cppm:16](../../../../term/terminal_modes.cppm#L16)*



**brief** The modes a program has set that change what the terminal's owner does: how keys are sent, whether the cursor is drawn, how a paste is sent.

 ECMA-48 modes and the DEC and xterm ones sqterm takes (decided 2026-11-02: the alternate screen, scroll regions, cursor keys and visibility, bracketed paste).



## Members

public bool application_cursor_keys

public bool cursor_visible

public bool bracketed_paste

public bool alternate_screen

public bool autowrap

public bool insert



