# class Stage

*Defined at [gui/stage.hpp:14](../../../../gui/stage.hpp#L14)*



**brief** Root owner for a translation-only two-dimensional actor hierarchy. 



## Functions

### Stage

*public void Stage(float width, float height)*



**brief** Construct a stage with a logical viewport.

**width** Initial logical width.

**height** Initial logical height.

### resize

*public void resize(float width, float height)*



**brief** Update the logical viewport dimensions.

**width** New logical width.

**height** New logical height.

### act

*public void act(double delta_seconds)*



**brief** Advance the entire hierarchy by one frame.

**delta_seconds** Elapsed domain time in seconds.

### add_actor

*public Actor & add_actor(std::unique_ptr<Actor> actor)*



**brief** Adopt an actor into the root group.

**actor** The unparented actor to own.

**return** Reference to the adopted actor.

### hit

*public Actor * hit(float stage_x, float stage_y, bool require_touchable)*



**brief** Hit-test the hierarchy in stage coordinates.

**stage_x** Horizontal position in stage logical units.

**stage_y** Vertical position in stage logical units.

**require_touchable** Ignore actors whose touchable flag is false.

**return** The deepest qualifying actor, or null.

### dispatch_input

*public bool dispatch_input(InputEvent & event, Actor * target)*



**brief** Route input through capture, target, and bubble phases.

 When target is null, pointer events use stage hit testing. Other event types require an explicit target.



**event** The event to dispatch.

**target** Explicit dispatch target, or null for pointer hit testing.

**return** `true` when the event was handled.

### root

*public Group & root()*

*Defined at [gui/stage.hpp:70](../../../../gui/stage.hpp#L70)*



**brief** Access the root group.

**return** Reference to the root group owning every top-level actor.

### root

*public const Group & root()*

*Defined at [gui/stage.hpp:76](../../../../gui/stage.hpp#L76)*



**brief** Access the root group, read-only.

**return** Reference to the root group owning every top-level actor.



