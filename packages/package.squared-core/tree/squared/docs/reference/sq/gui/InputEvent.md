# class InputEvent

*Defined at [gui/src/input_event.cppm:19](../../../../gui/src/input_event.cppm#L19)*



**brief** Mutable event routed through a Stage actor path.

 Source fields are supplied by the caller. Stage supplies target, current actor, phase, and actor-local pointer coordinates during dispatch.



## Members

public InputType type

public std::int64_t pointer_id

public float stage_x

public float stage_y

public int button

public Key key

public KeyModifiers modifiers

public Navigation navigation

public std::int32_t input_device_id

public bool repeat



## Functions

### cancel

*public void cancel()*

*Defined at [gui/src/input_event.cppm:64](../../../../gui/src/input_event.cppm#L64)*



**brief** Handle and stop the event, recording the current actor.

 Cancellation is used when a pointer contact is abandoned (for example after a gesture is cancelled by the OS) so actors can release capture.

### cancelled

*public bool cancelled()*

*Defined at [gui/src/input_event.cppm:90](../../../../gui/src/input_event.cppm#L90)*



**brief** Read the cancelled flag.

**return** true when the event was cancelled.

### current_target

*public Actor * current_target()*

*Defined at [gui/src/input_event.cppm:102](../../../../gui/src/input_event.cppm#L102)*



**brief** Read the actor currently receiving the event.

**return** Actor receiving the event during this dispatch step.

### handle

*public void handle()*



**brief** Mark the event handled without stopping propagation. 

### handled

*public bool handled()*

*Defined at [gui/src/input_event.cppm:78](../../../../gui/src/input_event.cppm#L78)*



**brief** Read the handled flag.

**return** true when any listener or handler handled the event.

### handled_by

*public Actor * handled_by()*

*Defined at [gui/src/input_event.cppm:108](../../../../gui/src/input_event.cppm#L108)*



**brief** Read the actor that first handled or cancelled the event.

**return** Actor that first handled or cancelled the event, or null.

### local_x

*public float local_x()*

*Defined at [gui/src/input_event.cppm:120](../../../../gui/src/input_event.cppm#L120)*



**brief** Read the horizontal position relative to the current target.

**return** Local horizontal position in logical units.

### local_y

*public float local_y()*

*Defined at [gui/src/input_event.cppm:126](../../../../gui/src/input_event.cppm#L126)*



**brief** Read the vertical position relative to the current target.

**return** Local vertical position in logical units.

### phase

*public InputPhase phase()*

*Defined at [gui/src/input_event.cppm:114](../../../../gui/src/input_event.cppm#L114)*



**brief** Read the current dispatch phase.

**return** Current capture, target, or bubble phase.

### stop

*public void stop()*

*Defined at [gui/src/input_event.cppm:56](../../../../gui/src/input_event.cppm#L56)*



**brief** Stop propagation; later phases and listeners are skipped. 

### stopped

*public bool stopped()*

*Defined at [gui/src/input_event.cppm:84](../../../../gui/src/input_event.cppm#L84)*



**brief** Read the stopped flag.

**return** true when propagation has been stopped or cancelled.

### target

*public Actor * target()*

*Defined at [gui/src/input_event.cppm:96](../../../../gui/src/input_event.cppm#L96)*



**brief** Read the dispatch target.

**return** Stage-selected or caller-supplied dispatch target.

