# class Actor

*Defined at [gui/actor.cppm:21](../../../../gui/actor.cppm#L21)*

## Functions

### Actor

*public void Actor()*

*Defined at [gui/actor.cppm:24](../../../../gui/actor.cppm#L24)*

### Actor

*public void Actor(const Actor & )*

*Defined at [gui/actor.cppm:27](../../../../gui/actor.cppm#L27)*

### Actor

*public void Actor(Actor && )*

*Defined at [gui/actor.cppm:29](../../../../gui/actor.cppm#L29)*

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

*Defined at [gui/actor.cppm:92](../../../../gui/actor.cppm#L92)*

### actor_interface

*public void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/actor.cppm:156](../../../../gui/actor.cppm#L156)*

### actor_interface

*public const void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/actor.cppm:168](../../../../gui/actor.cppm#L168)*

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

*Defined at [gui/actor.cppm:25](../../../../gui/actor.cppm#L25)*

### operator=

*public Actor & operator=(const Actor & )*

*Defined at [gui/actor.cppm:28](../../../../gui/actor.cppm#L28)*

### operator=

*public Actor & operator=(Actor && )*

*Defined at [gui/actor.cppm:30](../../../../gui/actor.cppm#L30)*

### x

*public float x()*

*Defined at [gui/actor.cppm:74](../../../../gui/actor.cppm#L74)*

### y

*public float y()*

*Defined at [gui/actor.cppm:80](../../../../gui/actor.cppm#L80)*

### width

*public float width()*

*Defined at [gui/actor.cppm:86](../../../../gui/actor.cppm#L86)*

### set_visible

*public void set_visible(bool visible)*

*Defined at [gui/actor.cppm:98](../../../../gui/actor.cppm#L98)*

### visible

*public bool visible()*

*Defined at [gui/actor.cppm:104](../../../../gui/actor.cppm#L104)*

### set_touchable

*public void set_touchable(bool touchable)*

*Defined at [gui/actor.cppm:110](../../../../gui/actor.cppm#L110)*

### touchable

*public bool touchable()*

*Defined at [gui/actor.cppm:116](../../../../gui/actor.cppm#L116)*

### parent

*public Group * parent()*

*Defined at [gui/actor.cppm:122](../../../../gui/actor.cppm#L122)*

### parent

*public const Group * parent()*

*Defined at [gui/actor.cppm:128](../../../../gui/actor.cppm#L128)*



