# class ButtonGroup

*Defined at [gui/button_group.cppm:21](../../../../gui/button_group.cppm#L21)*



**brief** Non-owning coordinator for toggle buttons and radio-style choices.

 A button may belong to at most one group. The group and every registered button detach from each other during destruction, so either may be owned by an ordinary Widget subtree without imposing a second ownership hierarchy.



## Functions

### ButtonGroup

*public void ButtonGroup(ButtonGroup && )*

*Defined at [gui/button_group.cppm:38](../../../../gui/button_group.cppm#L38)*

### ButtonGroup

*public void ButtonGroup(const ButtonGroup & )*

*Defined at [gui/button_group.cppm:36](../../../../gui/button_group.cppm#L36)*

### ButtonGroup

*public void ButtonGroup(std::size_t minimum_checked, std::size_t maximum_checked)*



**brief** Construct a checked-count policy.

**minimum_checked** Minimum checked buttons while members exist.

**maximum_checked** Maximum checked buttons; must not be smaller than `minimum_checked`.

**throws**when the limits are reversed.

### add

*public void add(ToggleButton & button)*



**brief** Register a non-owned toggle button.

**button** Button that must outlive the call; duplicate adds are no-ops.

**throws**when the button already belongs to another group.

### checked_button

*public ToggleButton * checked_button()*



**brief** Return the first checked member, or null when none is checked. 

### checked_count

*public std::size_t checked_count()*



**brief** Return the number of currently checked members. 

### clear

*public void clear()*



**brief** Detach every member without destroying any button. 

### operator=

*public ButtonGroup & operator=(ButtonGroup && )*

*Defined at [gui/button_group.cppm:39](../../../../gui/button_group.cppm#L39)*

### operator=

*public ButtonGroup & operator=(const ButtonGroup & )*

*Defined at [gui/button_group.cppm:37](../../../../gui/button_group.cppm#L37)*

### remove

*public bool remove(ToggleButton & button)*



**brief** Detach one registered button.

**button** Candidate member.

**return** true when the button was a member.

### set_limits

*public void set_limits(std::size_t minimum_checked, std::size_t maximum_checked)*



**brief** Replace the checked-count policy and rebalance current members.

**minimum_checked** Minimum checked buttons while members exist.

**maximum_checked** Maximum checked buttons.

**throws**when the limits are reversed.

### size

*public std::size_t size()*

*Defined at [gui/button_group.cppm:68](../../../../gui/button_group.cppm#L68)*



**brief** Return the number of registered buttons. 

### ~ButtonGroup

*public void ~ButtonGroup()*

