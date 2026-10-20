# class Group

*Defined at [gui/group.hpp:13](../../../../gui/group.hpp#L13)*



**brief** Actor that owns children in deterministic insertion order. 



Inherits from Actor



## Members

public static const int actor_interface_id



## Functions

### Group

*public void Group()*

*Defined at [gui/group.hpp:19](../../../../gui/group.hpp#L19)*

### Group

*public void Group(const Group & )*

*Defined at [gui/group.hpp:23](../../../../gui/group.hpp#L23)*

 Children point back at their parent, so a group is never copied or moved.

### operator=

*public Group & operator=(const Group & )*

*Defined at [gui/group.hpp:24](../../../../gui/group.hpp#L24)*

### Group

*public void Group(Group && )*

*Defined at [gui/group.hpp:25](../../../../gui/group.hpp#L25)*

### operator=

*public Group & operator=(Group && )*

*Defined at [gui/group.hpp:26](../../../../gui/group.hpp#L26)*

### actor_interface

*public const void * actor_interface(int id)*

*Defined at [gui/group.hpp:43](../../../../gui/group.hpp#L43)*



**brief** Publish the Group interface to actor_cast(), read-only.

**id** Identifier of the requested interface.

**return** This group for the Group identifier, otherwise the base answer.

### ~Group

*public void ~Group()*

### act

*public void act(double delta_seconds)*

### actor_interface

*public void * actor_interface(int id)*

*Defined at [gui/group.hpp:33](../../../../gui/group.hpp#L33)*



**brief** Publish the Group interface to actor_cast().

**id** Identifier of the requested interface.

**return** This group for the Group identifier, otherwise the base answer.

### hit

*public Actor * hit(float local_x, float local_y, bool require_touchable)*

### add_actor

*public Actor & add_actor(int actor)*



**brief** Transfer ownership of one unparented actor into this group.

**actor** The actor to adopt; must not already have a parent.

**return** Reference to the adopted actor.

### remove_actor

*public int remove_actor(Actor & actor)*



**brief** Remove an immediate child and return its ownership.

**actor** An immediate child actor to detach.

**return** The detached actor, or null when it is not an immediate child.

### clear

*public void clear()*



**brief** Destroy all immediate children. 

### child_count

*public int child_count()*



**brief** Count immediate children.

**return** Number of immediate children owned by this group.

### child_at

*public Actor * child_at(int index)*



**brief** Return the child at an insertion-order index.

**index** Zero-based child index.

**return** The child, or null when the index is out of range.

### child_at

*public const Actor * child_at(int index)*



**brief** Return the child at an insertion-order index, read-only.

**index** Zero-based child index.

**return** The child, or null when the index is out of range.



