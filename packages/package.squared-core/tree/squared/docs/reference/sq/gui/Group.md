# class Group

*Defined at [gui/group.cppm:10](../../../../gui/group.cppm#L10)*

Inherits from Actor



## Members

public static const ActorInterfaceId actor_interface_id



## Functions

### Group

*public void Group()*

*Defined at [gui/group.cppm:16](../../../../gui/group.cppm#L16)*

### Group

*public void Group(const Group & )*

*Defined at [gui/group.cppm:20](../../../../gui/group.cppm#L20)*

### operator=

*public Group & operator=(const Group & )*

*Defined at [gui/group.cppm:21](../../../../gui/group.cppm#L21)*

### Group

*public void Group(Group && )*

*Defined at [gui/group.cppm:22](../../../../gui/group.cppm#L22)*

### operator=

*public Group & operator=(Group && )*

*Defined at [gui/group.cppm:23](../../../../gui/group.cppm#L23)*

### actor_interface

*public void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/group.cppm:30](../../../../gui/group.cppm#L30)*

### actor_interface

*public const void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/group.cppm:40](../../../../gui/group.cppm#L40)*

### ~Group

*public void ~Group()*

### act

*public void act(double delta_seconds)*

### hit

*public Actor * hit(float local_x, float local_y, bool require_touchable)*

### add_actor

*public Actor & add_actor(std::unique_ptr<Actor> actor)*



**brief** Transfer ownership of one unparented actor into this group.

**actor** The actor to adopt; must not already have a parent.

**return** Reference to the adopted actor.

### remove_actor

*public std::unique_ptr<Actor> remove_actor(Actor & actor)*



**brief** Remove an immediate child and return its ownership.

**actor** An immediate child actor to detach.

**return** The detached actor, or null when it is not an immediate child.

### clear

*public void clear()*



**brief** Destroy all immediate children. 

### child_count

*public std::size_t child_count()*



**brief** Count immediate children.

**return** Number of immediate children owned by this group.

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



