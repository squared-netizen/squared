# class ButtonGroup

*Defined at [gui/button_group.hpp:19](../../../../gui/button_group.hpp#L19)*



**brief** Non-owning coordinator for toggle buttons and radio-style choices.

 A button may belong to at most one group. The group and every registered button detach from each other during destruction, so either may be owned by an ordinary Widget subtree without imposing a second ownership hierarchy.



## Functions

### ButtonGroup

*public void ButtonGroup(int minimum_checked, int maximum_checked)*



**brief** Construct a checked-count policy.

**minimum_checked** Minimum checked buttons while members exist.

**maximum_checked** Maximum checked buttons; must not be smaller than `minimum_checked`.

**throws**when the limits are reversed.

### ButtonGroup

*public void ButtonGroup(const ButtonGroup & )*

*Defined at [gui/button_group.hpp:34](../../../../gui/button_group.hpp#L34)*

### operator=

*public ButtonGroup & operator=(const ButtonGroup & )*

*Defined at [gui/button_group.hpp:35](../../../../gui/button_group.hpp#L35)*

### ButtonGroup

*public void ButtonGroup(ButtonGroup && )*

*Defined at [gui/button_group.hpp:36](../../../../gui/button_group.hpp#L36)*

### operator=

*public ButtonGroup & operator=(ButtonGroup && )*

*Defined at [gui/button_group.hpp:37](../../../../gui/button_group.hpp#L37)*

### ~ButtonGroup

*public void ~ButtonGroup()*

### add

*public void add(ToggleButton & button)*



**brief** Register a non-owned toggle button.

**button** Button that must outlive the call; duplicate adds are no-ops.

**throws**when the button already belongs to another group.

### remove

*public bool remove(ToggleButton & button)*



**brief** Detach one registered button.

**button** Candidate member.

**return** true when the button was a member.

### clear

*public void clear()*



**brief** Detach every member without destroying any button. 

### set_limits

*public void set_limits(int minimum_checked, int maximum_checked)*



**brief** Replace the checked-count policy and rebalance current members.

**minimum_checked** Minimum checked buttons while members exist.

**maximum_checked** Maximum checked buttons.

**throws**when the limits are reversed.

### checked_count

*public int checked_count()*



**brief** Return the number of currently checked members. 

### checked_button

*public ToggleButton * checked_button()*



**brief** Return the first checked member, or null when none is checked. 

### size

*public int size()*

*Defined at [gui/button_group.hpp:66](../../../../gui/button_group.hpp#L66)*



**brief** Return the number of registered buttons. 



