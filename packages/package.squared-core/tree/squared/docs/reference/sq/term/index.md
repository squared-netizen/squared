# namespace term

## Namespaces

* [detail](detail/index.md)


## Records

* [CellAttributes](CellAttributes.md)
* [CellFlags](CellFlags.md)
* [Cell](Cell.md)
* [Cursor](Cursor.md)
* [ProcessExit](ProcessExit.md)
* [PtyConfig](PtyConfig.md)
* [PtyError](PtyError.md)
* [Pty](Pty.md)
* [Rgb](Rgb.md)
* [TerminalConfig](TerminalConfig.md)
* [TerminalEvents](TerminalEvents.md)
* [TerminalModes](TerminalModes.md)
* [Terminal](Terminal.md)


## Functions

### operator==

*bool operator==(CellAttributes , CellAttributes )*

*Defined at [term/cell_attributes.cppm:28](../../../../term/cell_attributes.cppm#L28)*

### operator==

*bool operator==(CellFlags , CellFlags )*

*Defined at [term/cell_flags.cppm:24](../../../../term/cell_flags.cppm#L24)*

### operator==

*bool operator==(Cursor , Cursor )*

*Defined at [term/cursor.cppm:14](../../../../term/cursor.cppm#L14)*

### operator==

*bool operator==(ProcessExit , ProcessExit )*

*Defined at [term/process_exit.cppm:16](../../../../term/process_exit.cppm#L16)*

### operator==

*bool operator==(Rgb , Rgb )*

*Defined at [term/rgb.cppm:15](../../../../term/rgb.cppm#L15)*

### operator==

*bool operator==(TerminalModes , TerminalModes )*

*Defined at [term/terminal_modes.cppm:31](../../../../term/terminal_modes.cppm#L31)*

### operator==

*bool operator==(const Cell & , const Cell & )*

*Defined at [term/cell.cppm:28](../../../../term/cell.cppm#L28)*

## Enums

| enum class PtyErrorCode |

--

| none |
| already_running |
| no_program |
| open_master |
| open_slave |
| fork |


*Defined at [term/pty_error.cppm:9](../../../../term/pty_error.cppm#L9)*



**brief** Which step of starting a program failed. 

