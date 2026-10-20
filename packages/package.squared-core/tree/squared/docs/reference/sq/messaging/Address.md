# struct Address

*Defined at [messaging/address.hpp:19](../../../../messaging/address.hpp#L19)*



**brief** Where a telegram goes: a name (the schema) and a uid (which object).

 `player.hp` is a name with no uid; `item[17].name` is the name `item.name` about the object with uid 17, the model's own id for it. A uid of 0 means none. Sixteen bytes; the telegram envelope packs it tighter.



## Members

public Name name

public int uid



