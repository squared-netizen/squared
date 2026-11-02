# class Group

*Defined at [gui/group.cppm:10](../../../../gui/group.cppm#L10)*



**brief** Actor that owns children in deterministic insertion order. 



Inherits from Actor



## Members

public static const ActorInterfaceId actor_interface_id



## Functions

### Group

*public void Group()*

*Defined at [gui/group.cppm:16](../../../../gui/group.cppm#L16)*

### Group

*public void Group(Group && )*

*Defined at [gui/group.cppm:22](../../../../gui/group.cppm#L22)*

### Group

*public void Group(const Group & )*

*Defined at [gui/group.cppm:20](../../../../gui/group.cppm#L20)*

 Children point back at their parent, so a group is never copied or moved.

### act

*public void act(double delta_seconds)*

### actor_interface

*public const void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/group.cppm:40](../../../../gui/group.cppm#L40)*



**brief** Publish the Group interface to actor_cast(), read-only.

**id** Identifier of the requested interface.

**return** This group for the Group identifier, otherwise the base answer.

### actor_interface

*public void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/group.cppm:30](../../../../gui/group.cppm#L30)*



**brief** Publish the Group interface to actor_cast().

**id** Identifier of the requested interface.

**return** This group for the Group identifier, otherwise the base answer.

### add_actor

*public Actor & add_actor(std::unique_ptr<Actor> actor)*



**brief** Transfer ownership of one unparented actor into this group.

**actor** The actor to adopt; must not already have a parent.

**return** Reference to the adopted actor.

### child_at

*public Actor * child_at(std::size_t index)*



**brief** Return the child at an insertion-order index.

**index** Zero-based child index.

**return** The child, or null when the index is out of range.

### child_at

*public const Actor * child_at(std::size_t index)*



**brief** Return the child at an insertion-order index, read-only.

**index** Zero-based child index.

**return** The child, or null when the index is out of range.

### child_count

*public std::size_t child_count()*



**brief** Count immediate children.

**return** Number of immediate children owned by this group.

### clear

*public void clear()*



**brief** Destroy all immediate children. 

### hit

*public Actor * hit(float local_x, float local_y, bool require_touchable)*

### operator=

*public Group & operator=(Group && )*

*Defined at [gui/group.cppm:23](../../../../gui/group.cppm#L23)*

### operator=

*public Group & operator=(const Group & )*

*Defined at [gui/group.cppm:21](../../../../gui/group.cppm#L21)*

### remove_actor

*public std::unique_ptr<Actor> remove_actor(Actor & actor)*



**brief** Remove an immediate child and return its ownership.

**actor** An immediate child actor to detach.

**return** The detached actor, or null when it is not an immediate child.

### ~Group

*public void ~Group()*

