# class ToggleButton

*Defined at [gui/toggle_button.cppm:12](../../../../gui/toggle_button.cppm#L12)*

Inherits from Button



## Functions

### ToggleButton

*public void ToggleButton(std::string text, bool checked)*



**brief** Construct a toggle button.

**text** Optional label text.

**checked** Initial checked state.

### ToggleButton

*public void ToggleButton(const ToggleButton & )*

*Defined at [gui/toggle_button.cppm:27](../../../../gui/toggle_button.cppm#L27)*

### operator=

*public ToggleButton & operator=(const ToggleButton & )*

*Defined at [gui/toggle_button.cppm:28](../../../../gui/toggle_button.cppm#L28)*

### ToggleButton

*public void ToggleButton(ToggleButton && )*

*Defined at [gui/toggle_button.cppm:29](../../../../gui/toggle_button.cppm#L29)*

### operator=

*public ToggleButton & operator=(ToggleButton && )*

*Defined at [gui/toggle_button.cppm:30](../../../../gui/toggle_button.cppm#L30)*

### checked

*public bool checked()*

*Defined at [gui/toggle_button.cppm:42](../../../../gui/toggle_button.cppm#L42)*

### ~ToggleButton

*public void ~ToggleButton()*

### set_checked

*public void set_checked(bool checked)*



**brief** Set the checked state.

**checked** New boolean state; invokes the change callback.

### set_on_change

*public void set_on_change(ChangeCallback callback)*



**brief** Set the change callback.

**callback** Action copied into the button; invoked with the new state whenever it changes.

### activate

*protected void activate()*



**brief** Toggle the checked state and invoke the change callback. 

### selected

*protected bool selected()*



**brief** Report the checked state for selected styling.

**return** `true` when the toggle button is checked.



