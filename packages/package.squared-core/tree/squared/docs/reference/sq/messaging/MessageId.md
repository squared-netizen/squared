# class MessageId

*Defined at [messaging/src/message_id.hpp:16](../../../../messaging/src/message_id.hpp#L16)*



**brief** Stable namespaced identifier for one Telegram kind.

 Valid identifiers contain 1-128 ASCII letters, digits, dots, dashes, underscores, slashes, or colons and include at least one namespace separator (`.`, `/`, or `:`).



## Functions

### MessageId

*public void MessageId()*

*Defined at [messaging/src/message_id.hpp:20](../../../../messaging/src/message_id.hpp#L20)*



**brief** Construct an empty, invalid identifier. 

### MessageId

*public void MessageId(int value)*



**brief** Construct from a candidate namespace string.

**value** Candidate identifier; validated on construction.

### valid

*public bool valid()*



**brief** Validate the stored identifier.

**return** true when the stored value satisfied the identifier rules.

### value

*public int value()*



**brief** Access the stored identifier text.

**return** Text of the candidate identifier.



