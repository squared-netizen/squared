# class Dialog

*Defined at [gui/dialog.hpp:17](../../../../gui/dialog.hpp#L17)*



**brief** Modal Window with separate content and action-button tables.



Inherits from Window



## Functions

### Dialog

*public void Dialog(std::string title, ResultCallback result)*



**brief** Construct a dialog.

**title** Title-bar text.

**result** Result action; invoked when any button is chosen.

### dialog_content

*public Table & dialog_content()*

*Defined at [gui/dialog.hpp:34](../../../../gui/dialog.hpp#L34)*



**brief** Access the dialog content table.

**return** Reference to the content table for the message body.

### button_table

*public Table & button_table()*

*Defined at [gui/dialog.hpp:40](../../../../gui/dialog.hpp#L40)*



**brief** Access the action-button table.

**return** Reference to the table receiving configured buttons.

### text

*public Dialog & text(std::string text)*



**brief** Append a message paragraph.

**text** Message text to display.

**return** This dialog for chaining.

### button

*public Dialog & button(std::string text, std::string result)*



**brief** Append an action button.

**text** Button label.

**result** Result name reported when the button is chosen.

**return** This dialog for chaining.

### set_cancel_on_escape

*public void set_cancel_on_escape(bool enabled)*

*Defined at [gui/dialog.hpp:68](../../../../gui/dialog.hpp#L68)*



**brief** Control Escape-to-cancel behavior.

**enabled** true reports a cancel result when Escape is pressed.

### set_on_result

*public void set_on_result(ResultCallback result)*



**brief** Set the result action.

**result** Action copied into the dialog; invoked with the chosen result name.

### escape_closes

*protected bool escape_closes()*



**brief** Report Escape closing for dialogs.

**return** true, unless cancel-on-escape was disabled.



