# class InputEvent

*Defined at [gui/src/input_event.hpp:22](../../../../gui/src/input_event.hpp#L22)*



**brief** Mutable event routed through a Stage actor path.

 Source fields are supplied by the caller. Stage supplies target, current actor, phase, and actor-local pointer coordinates during dispatch.



## Members

public InputType type

public int pointer_id

public float stage_x

public float stage_y

public int button

public Key key

public KeyModifiers modifiers

public Navigation navigation

public int input_device_id

public bool repeat



## Functions

### handle

*public void handle()*



**brief** Mark the event handled without stopping propagation. 

### stop

*public void stop()*

*Defined at [gui/src/input_event.hpp:59](../../../../gui/src/input_event.hpp#L59)*



**brief** Stop propagation; later phases and listeners are skipped. 

### cancel

*public void cancel()*

*Defined at [gui/src/input_event.hpp:67](../../../../gui/src/input_event.hpp#L67)*



**brief** Handle and stop the event, recording the current actor.

 Cancellation is used when a pointer contact is abandoned (for example after a gesture is cancelled by the OS) so actors can release capture.

### handled

*public bool handled()*

*Defined at [gui/src/input_event.hpp:81](../../../../gui/src/input_event.hpp#L81)*



**brief** Read the handled flag.

**return** true when any listener or handler handled the event.

### stopped

*public bool stopped()*

*Defined at [gui/src/input_event.hpp:87](../../../../gui/src/input_event.hpp#L87)*



**brief** Read the stopped flag.

**return** true when propagation has been stopped or cancelled.

### cancelled

*public bool cancelled()*

*Defined at [gui/src/input_event.hpp:93](../../../../gui/src/input_event.hpp#L93)*



**brief** Read the cancelled flag.

**return** true when the event was cancelled.

### target

*public Actor * target()*

*Defined at [gui/src/input_event.hpp:99](../../../../gui/src/input_event.hpp#L99)*



**brief** Read the dispatch target.

**return** Stage-selected or caller-supplied dispatch target.

### current_target

*public Actor * current_target()*

*Defined at [gui/src/input_event.hpp:105](../../../../gui/src/input_event.hpp#L105)*



**brief** Read the actor currently receiving the event.

**return** Actor receiving the event during this dispatch step.

### handled_by

*public Actor * handled_by()*

*Defined at [gui/src/input_event.hpp:111](../../../../gui/src/input_event.hpp#L111)*



**brief** Read the actor that first handled or cancelled the event.

**return** Actor that first handled or cancelled the event, or null.

### phase

*public InputPhase phase()*

*Defined at [gui/src/input_event.hpp:117](../../../../gui/src/input_event.hpp#L117)*



**brief** Read the current dispatch phase.

**return** Current capture, target, or bubble phase.

### local_x

*public float local_x()*

*Defined at [gui/src/input_event.hpp:123](../../../../gui/src/input_event.hpp#L123)*



**brief** Read the horizontal position relative to the current target.

**return** Local horizontal position in logical units.

### local_y

*public float local_y()*

*Defined at [gui/src/input_event.hpp:129](../../../../gui/src/input_event.hpp#L129)*



**brief** Read the vertical position relative to the current target.

**return** Local vertical position in logical units.



