# class ActorRoot

*Defined at [gui/src/detail/actor_root.cppm:18](../../../../gui/src/detail/actor_root.cppm#L18)*



**brief** The root of the widget tree the Ui holds: its group, hit testing, and input's capture, target and bubble phases. Internal: an app reaches widgets through `sq::Stage` (was `gui::Stage` until 7.7, renamed so the one Stage is the app's).



## Functions

### ActorRoot

*public void ActorRoot(float width, float height)*



**brief** Construct a root with a logical viewport.

**width** Initial logical width.

**height** Initial logical height.

### act

*public void act(double delta_seconds)*



**brief** Advance the entire hierarchy by one frame.

**delta_seconds** Elapsed domain time in seconds.

### add_actor

*public Actor & add_actor(std::unique_ptr<Actor> actor)*



**brief** Adopt an actor into the root group.

**actor** The unparented actor to own.

**return** Reference to the adopted actor.

### dispatch_input

*public bool dispatch_input(InputEvent & event, Actor * target)*



**brief** Route input through capture, target, and bubble phases.

 When target is null, pointer events use root hit testing. Other event types require an explicit target.



**event** The event to dispatch.

**target** Explicit dispatch target, or null for pointer hit testing.

**return** `true` when the event was handled.

### hit

*public Actor * hit(float stage_x, float stage_y, bool require_touchable)*



**brief** Hit-test the hierarchy in root coordinates.

**stage_x** Horizontal position in stage logical units.

**stage_y** Vertical position in stage logical units.

**require_touchable** Ignore actors whose touchable flag is false.

**return** The deepest qualifying actor, or null.

### resize

*public void resize(float width, float height)*



**brief** Update the logical viewport dimensions.

**width** New logical width.

**height** New logical height.

### root

*public Group & root()*

*Defined at [gui/src/detail/actor_root.cppm:74](../../../../gui/src/detail/actor_root.cppm#L74)*



**brief** Access the root group.

**return** Reference to the root group owning every top-level actor.

### root

*public const Group & root()*

*Defined at [gui/src/detail/actor_root.cppm:80](../../../../gui/src/detail/actor_root.cppm#L80)*



**brief** Access the root group, read-only.

**return** Reference to the root group owning every top-level actor.

