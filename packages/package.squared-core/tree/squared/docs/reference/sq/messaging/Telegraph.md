# class Telegraph

*Defined at [messaging/telegraph.hpp:95](../../../../messaging/telegraph.hpp#L95)*



**brief** The model's end of the Telegraph: what a model sees, on its thread.

 It sends `state` and `event` by key, declares virtualized lists, and runs the model's side of a turn (`deliver`): lifecycle signals, held-back states, then every telegram from the UI. A sending mistake (a wrong type, a verb the key does not carry, a missing or extra uid) does not compile.



## Functions

### Telegraph

*public void Telegraph(const Telegraph & )*

*Defined at [messaging/telegraph.hpp:98](../../../../messaging/telegraph.hpp#L98)*

### operator=

*public Telegraph & operator=(const Telegraph & )*

*Defined at [messaging/telegraph.hpp:99](../../../../messaging/telegraph.hpp#L99)*

### Telegraph

*public void Telegraph(Telegraph && )*

*Defined at [messaging/telegraph.hpp:100](../../../../messaging/telegraph.hpp#L100)*

### operator=

*public Telegraph & operator=(Telegraph && )*

*Defined at [messaging/telegraph.hpp:101](../../../../messaging/telegraph.hpp#L101)*

### ~Telegraph

*public void ~Telegraph()*

*Defined at [messaging/telegraph.hpp:102](../../../../messaging/telegraph.hpp#L102)*

### state

*public SendResult state(const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:107](../../../../messaging/telegraph.hpp#L107)*



**brief** `state <key> <value>`. When the ring is full it is held back, coalesced: `pending`. 

### state

*public SendResult state(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.hpp:117](../../../../messaging/telegraph.hpp#L117)*



**brief** `state <key>[uid] <value>`, for a key that takes a uid. 

### event

*public SendResult event(const K & key)*

*Defined at [messaging/telegraph.hpp:127](../../../../messaging/telegraph.hpp#L127)*



**brief** `event <key>`, for a key with no value. Never held back: `full` when there is no room. 

### event

*public SendResult event(const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:135](../../../../messaging/telegraph.hpp#L135)*



**brief** `event <key> <value>`. 

### event

*public SendResult event(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.hpp:145](../../../../messaging/telegraph.hpp#L145)*



**brief** `event <key>[uid] <value>`, for a key that takes a uid. 

### answer

*public SendResult answer(const Received & request, const K & key)*

*Defined at [messaging/telegraph.hpp:155](../../../../messaging/telegraph.hpp#L155)*



**brief** `event <key>` answering `request` (a `do`): it carries the request's correlation. 

### answer

*public SendResult answer(const Received & request, const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:164](../../../../messaging/telegraph.hpp#L164)*



**brief** `event <key> <value>` answering `request`. 

### list

*public bool list(const K & rows, S & source)*

*Defined at [messaging/telegraph.hpp:180](../../../../messaging/telegraph.hpp#L180)*



**brief** A virtualized list: `rows` (a key taking a uid: the row) and where its rows come from. Its count is sent at once. False when `TELEGRAPH_LISTS` lists exist already, or this one does.

### changed

*public void changed(const K & rows)*

*Defined at [messaging/telegraph.hpp:188](../../../../messaging/telegraph.hpp#L188)*



**brief** The list's rows changed (a sort, an insert, a removal): what changed on screen is sent. 

### changed

*public void changed(const K & rows, std::uint32_t uid)*

*Defined at [messaging/telegraph.hpp:196](../../../../messaging/telegraph.hpp#L196)*



**brief** One item's fields changed: published again if it is on screen. 

### tick_every

*public void tick_every(std::uint32_t milliseconds)*



**brief** Ask for `tick` every `milliseconds` (0: no tick, the default). Run by the model thread (6.6c). 

### tick_interval

*public std::uint32_t tick_interval()*

### start

*public void start(M & model)*

*Defined at [messaging/telegraph.hpp:468](../../../../messaging/telegraph.hpp#L468)*



**brief** Calls `model.start(*this)`, if it has one. Once, before `deliver`. 

 ---- the templates, now that the line is complete --------------------------------------------

### deliver

*public std::uint32_t deliver(M & model)*

*Defined at [messaging/telegraph.hpp:476](../../../../messaging/telegraph.hpp#L476)*



**brief** One turn of the model's side: the lifecycle signals (low memory first; background and foreground in the order that ends where the app is now; quit last, after a save), then the held-back states, then each telegram from the UI: a list's `view` to its list, everything else to `model.receive(*this, telegram)`. The telegrams handled.

### pending

*public std::uint32_t pending()*



**brief** States held back, waiting for room. 

### quitting

*public bool quitting()*



**brief** Whether quit has been asked for: the model saved, and is to stop. 



