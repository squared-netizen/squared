# class ToggleButton

*Defined at [gui/toggle_button.cppm:12](../../../../gui/toggle_button.cppm#L12)*



**brief** Two-state button that reports changes through a callback. 



Inherits from Button



## Functions

### ToggleButton

*public void ToggleButton(ToggleButton && )*

*Defined at [gui/toggle_button.cppm:29](../../../../gui/toggle_button.cppm#L29)*

### ToggleButton

*public void ToggleButton(const ToggleButton & )*

*Defined at [gui/toggle_button.cppm:27](../../../../gui/toggle_button.cppm#L27)*

 A ButtonGroup holds members by address, so a button is never copied or moved.

### ToggleButton

*public void ToggleButton(std::string text, bool checked)*



**brief** Construct a toggle button.

**text** Optional label text.

**checked** Initial checked state.

### activate

*protected void activate()*



**brief** Toggle the checked state and invoke the change callback. 

### checked

*public bool checked()*

*Defined at [gui/toggle_button.cppm:42](../../../../gui/toggle_button.cppm#L42)*



**brief** Read the checked state.

**return** Current boolean state.

### operator=

*public ToggleButton & operator=(ToggleButton && )*

*Defined at [gui/toggle_button.cppm:30](../../../../gui/toggle_button.cppm#L30)*

### operator=

*public ToggleButton & operator=(const ToggleButton & )*

*Defined at [gui/toggle_button.cppm:28](../../../../gui/toggle_button.cppm#L28)*

### selected

*protected bool selected()*



**brief** Report the checked state for selected styling.

**return** `true` when the toggle button is checked.

### set_checked

*public void set_checked(bool checked)*



**brief** Set the checked state.

**checked** New boolean state; invokes the change callback.

### set_on_change

*public void set_on_change(ChangeCallback callback)*



**brief** Set the change callback.

**callback** Action copied into the button; invoked with the new state whenever it changes.

### ~ToggleButton

*public void ~ToggleButton()*

