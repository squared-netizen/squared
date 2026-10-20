# class ToggleButton

*Defined at [gui/toggle_button.hpp:14](../../../../gui/toggle_button.hpp#L14)*



**brief** Two-state button that reports changes through a callback. 



Inherits from Button



## Functions

### ToggleButton

*public void ToggleButton(int text, bool checked)*



**brief** Construct a toggle button.

**text** Optional label text.

**checked** Initial checked state.

### ToggleButton

*public void ToggleButton(const ToggleButton & )*

*Defined at [gui/toggle_button.hpp:29](../../../../gui/toggle_button.hpp#L29)*

 A ButtonGroup holds members by address, so a button is never copied or moved.

### operator=

*public ToggleButton & operator=(const ToggleButton & )*

*Defined at [gui/toggle_button.hpp:30](../../../../gui/toggle_button.hpp#L30)*

### ToggleButton

*public void ToggleButton(ToggleButton && )*

*Defined at [gui/toggle_button.hpp:31](../../../../gui/toggle_button.hpp#L31)*

### operator=

*public ToggleButton & operator=(ToggleButton && )*

*Defined at [gui/toggle_button.hpp:32](../../../../gui/toggle_button.hpp#L32)*

### checked

*public bool checked()*

*Defined at [gui/toggle_button.hpp:44](../../../../gui/toggle_button.hpp#L44)*



**brief** Read the checked state.

**return** Current boolean state.

### ~ToggleButton

*public void ~ToggleButton()*

### set_checked

*public void set_checked(bool checked)*



**brief** Set the checked state.

**checked** New boolean state; invokes the change callback.

### set_on_change

*public void set_on_change(int callback)*



**brief** Set the change callback.

**callback** Action copied into the button; invoked with the new state whenever it changes.

### activate

*protected void activate()*



**brief** Toggle the checked state and invoke the change callback. 

### selected

*protected bool selected()*



**brief** Report the checked state for selected styling.

**return** `true` when the toggle button is checked.



