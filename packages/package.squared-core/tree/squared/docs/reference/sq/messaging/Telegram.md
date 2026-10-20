# class Telegram

*Defined at [messaging/telegram.hpp:20](../../../../messaging/telegram.hpp#L20)*



**brief** Owned immutable-access message envelope.

 A receiver identifies directed delivery. An absent receiver requests broadcast delivery to subscribers of message(). Payloads are owned JSON values and therefore never contain process-local pointers.



## Functions

### Telegram

*public void Telegram(MessageId message, sq::data::JsonValue payload, int sender, int receiver, int correlation, bool receipt_requested)*



**brief** Construct an owned immutable-access message envelope.

**message** Message kind used for subscription matching.

**payload** Application data carried by the envelope.

**sender** Origin endpoint, when directed.

**receiver** Destination endpoint for directed delivery; an absent receiver requests broadcast delivery to subscribers.

**correlation** Application-owned correlation tag.

**receipt_requested** Whether a queued return receipt is requested.

### message

*public const MessageId & message()*



**brief** Access the message kind.

**return** Message kind used for subscription matching.

### payload

*public const sq::data::JsonValue & payload()*



**brief** Access the owned application payload.

**return** Reference to the stored payload; invalidated by destruction.

### sender

*public const int & sender()*



**brief** Access the origin endpoint.

**return** Sender endpoint when set, otherwise std::nullopt.

### receiver

*public const int & receiver()*



**brief** Access the destination endpoint.

**return** Receiver endpoint for directed delivery, else broadcast.

### correlation

*public int correlation()*



**brief** Access the correlation tag.

**return** Application-owned correlation tag.

### receipt_requested

*public bool receipt_requested()*



**brief** Report whether a return receipt was requested.

**return** true when a queued return receipt was requested.

### directed

*public bool directed()*



**brief** Report directed delivery.

**return** true for directed, false for broadcast delivery.

### valid

*public bool valid()*



**brief** Validate the envelope.

**return** true when message and any endpoint identifiers are valid.



