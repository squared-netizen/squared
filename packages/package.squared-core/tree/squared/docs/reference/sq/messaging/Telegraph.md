# class Telegraph

*Defined at [messaging/telegraph.hpp:110](../../../../messaging/telegraph.hpp#L110)*



**brief** The model's end of the Telegraph: what a model sees, on its thread.

 It sends `state` and `event` by key, declares virtualized lists, and runs the model's side of a turn (`deliver`): lifecycle signals, held-back states, then every telegram from the UI. A sending mistake (a wrong type, a verb the key does not carry, a missing or extra uid) does not compile.



## Functions

### Telegraph

*public void Telegraph(const Telegraph & )*

*Defined at [messaging/telegraph.hpp:113](../../../../messaging/telegraph.hpp#L113)*

### operator=

*public Telegraph & operator=(const Telegraph & )*

*Defined at [messaging/telegraph.hpp:114](../../../../messaging/telegraph.hpp#L114)*

### Telegraph

*public void Telegraph(Telegraph && )*

*Defined at [messaging/telegraph.hpp:115](../../../../messaging/telegraph.hpp#L115)*

### operator=

*public Telegraph & operator=(Telegraph && )*

*Defined at [messaging/telegraph.hpp:116](../../../../messaging/telegraph.hpp#L116)*

### ~Telegraph

*public void ~Telegraph()*

*Defined at [messaging/telegraph.hpp:117](../../../../messaging/telegraph.hpp#L117)*

### state

*public SendResult state(const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:127](../../../../messaging/telegraph.hpp#L127)*



**brief** `state <key> <value>`. When the ring is full it is held back, coalesced: `pending`. When the held-back table is full too, the model's thread waits up to `TELEGRAPH_WAIT_MS` for the UI to drain, then `full`.

### state

*public SendResult state(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.hpp:137](../../../../messaging/telegraph.hpp#L137)*



**brief** `state <key>[uid] <value>`, for a key that takes a uid. 

### event

*public SendResult event(const K & key)*

*Defined at [messaging/telegraph.hpp:151](../../../../messaging/telegraph.hpp#L151)*



**brief** `event <key>`, for a key with no value. Never held back: with no room, the model's thread waits up to `TELEGRAPH_WAIT_MS`, then `full`.

### event

*public SendResult event(const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:159](../../../../messaging/telegraph.hpp#L159)*



**brief** `event <key> <value>`. 

### event

*public SendResult event(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.hpp:169](../../../../messaging/telegraph.hpp#L169)*



**brief** `event <key>[uid] <value>`, for a key that takes a uid. 

### answer

*public SendResult answer(const Received & request, const K & key)*

*Defined at [messaging/telegraph.hpp:179](../../../../messaging/telegraph.hpp#L179)*



**brief** `event <key>` answering `request` (a `do`): it carries the request's correlation. 

### answer

*public SendResult answer(const Received & request, const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:188](../../../../messaging/telegraph.hpp#L188)*



**brief** `event <key> <value>` answering `request`. 

### list

*public bool list(const K & rows, S & source)*

*Defined at [messaging/telegraph.hpp:204](../../../../messaging/telegraph.hpp#L204)*



**brief** A virtualized list: `rows` (a key taking a uid: the row) and where its rows come from. Its count is sent at once. False when `TELEGRAPH_LISTS` lists exist already, or this one does.

### changed

*public void changed(const K & rows)*

*Defined at [messaging/telegraph.hpp:212](../../../../messaging/telegraph.hpp#L212)*



**brief** The list's rows changed (a sort, an insert, a removal): what changed on screen is sent. 

### changed

*public void changed(const K & rows, std::uint32_t uid)*

*Defined at [messaging/telegraph.hpp:220](../../../../messaging/telegraph.hpp#L220)*



**brief** One item's fields changed: published again if it is on screen. 

### tick_every

*public void tick_every(std::uint32_t milliseconds)*



**brief** Ask for `tick` every `milliseconds` (0: no tick, the default). `ModelRunner` calls it. 

### tick_interval

*public std::uint32_t tick_interval()*

### start

*public void start(M & model)*

*Defined at [messaging/telegraph.hpp:542](../../../../messaging/telegraph.hpp#L542)*



**brief** Calls `model.start(*this)`, if it has one. Once, before `deliver`. 

 ---- the templates, now that the line is complete --------------------------------------------

### deliver

*public std::uint32_t deliver(M & model)*

*Defined at [messaging/telegraph.hpp:550](../../../../messaging/telegraph.hpp#L550)*



**brief** One turn of the model's side: the lifecycle signals (low memory first; background and foreground in the order that ends where the app is now; quit last, after a save), then the held-back states, then each telegram from the UI: a list's `view` to its list, everything else to `model.receive(*this, telegram)`. The telegrams handled.

### pending

*public std::uint32_t pending()*



**brief** States held back, waiting for room. 

### quitting

*public bool quitting()*



**brief** Whether quit has been asked for: the model saved, and is to stop. 



