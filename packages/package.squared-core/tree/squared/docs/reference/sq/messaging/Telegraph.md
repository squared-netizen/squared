# class Telegraph

*Defined at [messaging/telegraph.cppm:103](../../../../messaging/telegraph.cppm#L103)*



**brief** The model's end of the Telegraph: what a model sees, on its thread.

 It sends `state` and `event` by key, declares virtualized lists, and runs the model's side of a turn (`deliver`): lifecycle signals, held-back states, then every telegram from the UI. A sending mistake (a wrong type, a verb the key does not carry, a missing or extra uid) does not compile.



## Functions

### Telegraph

*public void Telegraph(Telegraph && )*

*Defined at [messaging/telegraph.cppm:108](../../../../messaging/telegraph.cppm#L108)*

### Telegraph

*public void Telegraph(const Telegraph & )*

*Defined at [messaging/telegraph.cppm:106](../../../../messaging/telegraph.cppm#L106)*

### answer

*public SendResult answer(const Received & request, const K & key)*

*Defined at [messaging/telegraph.cppm:172](../../../../messaging/telegraph.cppm#L172)*



**brief** `event <key>` answering `request` (a `do`): it carries the request's correlation. 

### answer

*public SendResult answer(const Received & request, const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:181](../../../../messaging/telegraph.cppm#L181)*



**brief** `event <key> <value>` answering `request`. 

### changed

*public void changed(const K & rows)*

*Defined at [messaging/telegraph.cppm:205](../../../../messaging/telegraph.cppm#L205)*



**brief** The list's rows changed (a sort, an insert, a removal): what changed on screen is sent. 

### changed

*public void changed(const K & rows, std::uint32_t uid)*

*Defined at [messaging/telegraph.cppm:213](../../../../messaging/telegraph.cppm#L213)*



**brief** One item's fields changed: published again if it is on screen. 

### deliver

*public std::uint32_t deliver(M & model)*

*Defined at [messaging/telegraph.cppm:618](../../../../messaging/telegraph.cppm#L618)*



**brief** One turn of the model's side: the lifecycle signals (low memory first; background and foreground in the order that ends where the app is now; quit last, after a save), then the held-back states, then each telegram from the UI: a list's `view` to its list, everything else to `model.receive(*this, telegram)`. The telegrams handled.

### event

*public SendResult event(const K & key)*

*Defined at [messaging/telegraph.cppm:144](../../../../messaging/telegraph.cppm#L144)*



**brief** `event <key>`, for a key with no value. Never held back: with no room, the model's thread waits up to `TELEGRAPH_WAIT_MS`, then `full`.

### event

*public SendResult event(const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:152](../../../../messaging/telegraph.cppm#L152)*



**brief** `event <key> <value>`. 

### event

*public SendResult event(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.cppm:162](../../../../messaging/telegraph.cppm#L162)*



**brief** `event <key>[uid] <value>`, for a key that takes a uid. 

### list

*public bool list(const K & rows, S & source)*

*Defined at [messaging/telegraph.cppm:197](../../../../messaging/telegraph.cppm#L197)*



**brief** A virtualized list: `rows` (a key taking a uid: the row) and where its rows come from. Its count is sent at once. False when `TELEGRAPH_LISTS` lists exist already, or this one does.

### operator=

*public Telegraph & operator=(Telegraph && )*

*Defined at [messaging/telegraph.cppm:109](../../../../messaging/telegraph.cppm#L109)*

### operator=

*public Telegraph & operator=(const Telegraph & )*

*Defined at [messaging/telegraph.cppm:107](../../../../messaging/telegraph.cppm#L107)*

### pending

*public std::uint32_t pending()*



**brief** States held back, waiting for room. 

### quitting

*public bool quitting()*



**brief** Whether quit has been asked for: the model saved, and is to stop. 

### start

*public void start(M & model)*

*Defined at [messaging/telegraph.cppm:610](../../../../messaging/telegraph.cppm#L610)*



**brief** Calls `model.start(*this)`, if it has one. Once, before `deliver`. 

 ---- the templates, now that the line is complete --------------------------------------------

### state

*public SendResult state(const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:120](../../../../messaging/telegraph.cppm#L120)*



**brief** `state <key> <value>`. When the ring is full it is held back, coalesced: `pending`. When the held-back table is full too, the model's thread waits up to `TELEGRAPH_WAIT_MS` for the UI to drain, then `full`.

### state

*public SendResult state(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.cppm:130](../../../../messaging/telegraph.cppm#L130)*



**brief** `state <key>[uid] <value>`, for a key that takes a uid. 

### tick_every

*public void tick_every(std::uint32_t milliseconds)*



**brief** Ask for `tick` every `milliseconds` (0: no tick, the default). `ModelRunner` calls it. 

### tick_interval

*public std::uint32_t tick_interval()*

### ~Telegraph

*public void ~Telegraph()*

*Defined at [messaging/telegraph.cppm:110](../../../../messaging/telegraph.cppm#L110)*

