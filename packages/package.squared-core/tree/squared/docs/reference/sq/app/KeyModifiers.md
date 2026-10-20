# struct KeyModifiers

*Defined at [app/src/key_modifiers.hpp:12](../../../../app/src/key_modifiers.hpp#L12)*



**brief** The modifier keys held during a key or navigation event.

 One byte: four one-bit fields. Read them as fields (`modifiers.shift`) and build with designated initializers (`KeyModifiers{ .shift = true }`).



## Members

public bool shift

public bool control

public bool alt

public bool meta



## Functions

### any

*public bool any()*

*Defined at [app/src/key_modifiers.hpp:27](../../../../app/src/key_modifiers.hpp#L27)*



**brief** Report whether any modifier is held.

**return** true when at least one field is set.



