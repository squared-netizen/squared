# class Actor

*Defined at [gui/actor.hpp:22](../../../../gui/actor.hpp#L22)*



**brief** Base node with parent-relative bounds and frame traversal.

 Coordinates supplied to hit() are local to the actor. Position is expressed in the parent coordinate system. The first GUI slice intentionally has translation-only hierarchy semantics; transforms and rendering arrive in a later compatible layer.



## Functions

### Actor

*public void Actor()*

*Defined at [gui/actor.hpp:25](../../../../gui/actor.hpp#L25)*

### Actor

*public void Actor(const Actor & )*

*Defined at [gui/actor.hpp:28](../../../../gui/actor.hpp#L28)*

### Actor

*public void Actor(Actor && )*

*Defined at [gui/actor.hpp:30](../../../../gui/actor.hpp#L30)*

### act

*public void act(double delta_seconds)*



**brief** Advance this actor by one frame.

**delta_seconds** Elapsed domain time in seconds.

### contains

*protected bool contains(float local_x, float local_y)*



**brief** Test whether a local coordinate falls inside this actor.

**local_x** Horizontal coordinate local to this actor.

**local_y** Vertical coordinate local to this actor.

**return** true when the point lies within the actor's rectangle.

### height

*public float height()*

*Defined at [gui/actor.hpp:93](../../../../gui/actor.hpp#L93)*



**brief** Read the height.

**return** Height in logical units.

### actor_interface

*public void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/actor.hpp:157](../../../../gui/actor.hpp#L157)*



**brief** Recover a derived interface published by this actor.

**id** Identifier of the requested interface.

**return** Pointer to the requested interface, or null when this actor does not publish it. The base implementation publishes nothing.

**note** Prefer actor_cast(), which supplies the identifier and restores the pointer type. Overrides must answer their own identifier and otherwise delegate to their direct base, so an interface published anywhere in the chain stays reachable.

### actor_interface

*public const void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/actor.hpp:169](../../../../gui/actor.hpp#L169)*



**brief** Recover a derived interface published by this actor, read-only.

**id** Identifier of the requested interface.

**return** Pointer to the requested interface, or null when this actor does not publish it.

### hit

*public Actor * hit(float local_x, float local_y, bool require_touchable)*



**brief** Return the deepest eligible actor at a local coordinate.

**local_x** Horizontal coordinate local to this actor.

**local_y** Vertical coordinate local to this actor.

**require_touchable** Ignore actors whose touchable flag is false.

**return** The actor at the coordinate, or null when nothing qualifies.

### set_bounds

*public void set_bounds(float x, float y, float width, float height)*



**brief** Set position and size in parent-local logical units.

**x** Horizontal position.

**y** Vertical position.

**width** New width; non-negative.

**height** New height; non-negative.

### set_position

*public void set_position(float x, float y)*



**brief** Set position in parent-local logical units.

**x** Horizontal position.

**y** Vertical position.

### set_size

*public void set_size(float width, float height)*



**brief** Set the extent in logical units.

**width** New width; non-negative.

**height** New height; non-negative.

### add_input_listener

*public InputListenerId add_input_listener(InputListener listener, bool capture)*



**brief** Add a regular or capture listener owned by this actor.

**listener** Callback invoked during input dispatch.

**capture** `true` registers the listener for the capture phase.

**return** A stable actor-local identifier usable with remove_input_listener.

### remove_input_listener

*public bool remove_input_listener(InputListenerId id)*



**brief** Remove one listener by its stable actor-local identifier.

**id** Identifier returned by add_input_listener.

**return** `true` when the identifier matched a live listener.

### input_event

*protected void input_event(InputEvent & event)*



**brief** Handle target/bubble input before regular external listeners.

**event** The event currently in the target or bubble phase.

### ~Actor

*public void ~Actor()*

*Defined at [gui/actor.hpp:26](../../../../gui/actor.hpp#L26)*

### operator=

*public Actor & operator=(const Actor & )*

*Defined at [gui/actor.hpp:29](../../../../gui/actor.hpp#L29)*

### operator=

*public Actor & operator=(Actor && )*

*Defined at [gui/actor.hpp:31](../../../../gui/actor.hpp#L31)*

### x

*public float x()*

*Defined at [gui/actor.hpp:75](../../../../gui/actor.hpp#L75)*



**brief** Read the horizontal position.

**return** Horizontal position in parent-local units.

### y

*public float y()*

*Defined at [gui/actor.hpp:81](../../../../gui/actor.hpp#L81)*



**brief** Read the vertical position.

**return** Vertical position in parent-local units.

### width

*public float width()*

*Defined at [gui/actor.hpp:87](../../../../gui/actor.hpp#L87)*



**brief** Read the width.

**return** Width in logical units.

### set_visible

*public void set_visible(bool visible)*

*Defined at [gui/actor.hpp:99](../../../../gui/actor.hpp#L99)*



**brief** Control whether the actor participates in frames and hit tests.

**visible** `false` hides the actor and excludes it from hit tests.

### visible

*public bool visible()*

*Defined at [gui/actor.hpp:105](../../../../gui/actor.hpp#L105)*



**brief** Read the visibility state.

**return** true while the actor participates in frames and hit tests.

### set_touchable

*public void set_touchable(bool touchable)*

*Defined at [gui/actor.hpp:111](../../../../gui/actor.hpp#L111)*



**brief** Control whether the actor can receive pointer hit tests.

**touchable** `false` skips the actor during hit testing.

### touchable

*public bool touchable()*

*Defined at [gui/actor.hpp:117](../../../../gui/actor.hpp#L117)*



**brief** Read the touchable state.

**return** true while the actor participates in hit testing.

### parent

*public Group * parent()*

*Defined at [gui/actor.hpp:123](../../../../gui/actor.hpp#L123)*



**brief** Access the owning group.

**return** Owning group, or null for a root-owned actor.

### parent

*public const Group * parent()*

*Defined at [gui/actor.hpp:129](../../../../gui/actor.hpp#L129)*



**brief** Access the owning group, read-only.

**return** Owning group, or null for a root-owned actor.



