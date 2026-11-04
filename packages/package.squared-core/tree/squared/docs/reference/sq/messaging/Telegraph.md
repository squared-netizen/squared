# class Telegraph

*Defined at [messaging/telegraph.cppm:106](../../../../messaging/telegraph.cppm#L106)*



**brief** The model's end of the Telegraph: what a model sees, on its thread.

 It sends `state` and `event` by key, declares virtualized lists, and runs the model's side of a turn (`deliver`): lifecycle signals, held-back states, then every telegram from the UI. A sending mistake (a wrong type, a verb the key does not carry, a missing or extra uid) does not compile.



## Functions

### Telegraph

*public void Telegraph(Telegraph && )*

*Defined at [messaging/telegraph.cppm:111](../../../../messaging/telegraph.cppm#L111)*

### Telegraph

*public void Telegraph(const Telegraph & )*

*Defined at [messaging/telegraph.cppm:109](../../../../messaging/telegraph.cppm#L109)*

### answer

*public SendResult answer(const Received & request, const K & key)*

*Defined at [messaging/telegraph.cppm:175](../../../../messaging/telegraph.cppm#L175)*



**brief** `event <key>` answering `request` (a `do`): it carries the request's correlation. 

### answer

*public SendResult answer(const Received & request, const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:184](../../../../messaging/telegraph.cppm#L184)*



**brief** `event <key> <value>` answering `request`. 

### changed

*public void changed(const K & rows)*

*Defined at [messaging/telegraph.cppm:208](../../../../messaging/telegraph.cppm#L208)*



**brief** The list's rows changed (a sort, an insert, a removal): what changed on screen is sent. 

### changed

*public void changed(const K & rows, std::uint32_t uid)*

*Defined at [messaging/telegraph.cppm:216](../../../../messaging/telegraph.cppm#L216)*



**brief** One item's fields changed: published again if it is on screen. 

### deliver

*public std::uint32_t deliver(M & model)*

*Defined at [messaging/telegraph.cppm:688](../../../../messaging/telegraph.cppm#L688)*



**brief** One turn of the model's side: the lifecycle signals (low memory first; background and foreground in the order that ends where the app is now; quit last, after a save), then the held-back states, then each telegram from the UI: a list's `view` to its list, everything else to `model.receive(*this, telegram)`. The telegrams handled.

### event

*public SendResult event(const K & key)*

*Defined at [messaging/telegraph.cppm:147](../../../../messaging/telegraph.cppm#L147)*



**brief** `event <key>`, for a key with no value. Never held back: with no room, the model's thread waits up to `TELEGRAPH_WAIT_MS`, then `full`.

### event

*public SendResult event(const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:155](../../../../messaging/telegraph.cppm#L155)*



**brief** `event <key> <value>`. 

### event

*public SendResult event(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.cppm:165](../../../../messaging/telegraph.cppm#L165)*



**brief** `event <key>[uid] <value>`, for a key that takes a uid. 

### list

*public bool list(const K & rows, S & source)*

*Defined at [messaging/telegraph.cppm:200](../../../../messaging/telegraph.cppm#L200)*



**brief** A virtualized list: `rows` (a key taking a uid: the row) and where its rows come from. Its count is sent at once. False when `TELEGRAPH_LISTS` lists exist already, or this one does.

### operator=

*public Telegraph & operator=(Telegraph && )*

*Defined at [messaging/telegraph.cppm:112](../../../../messaging/telegraph.cppm#L112)*

### operator=

*public Telegraph & operator=(const Telegraph & )*

*Defined at [messaging/telegraph.cppm:110](../../../../messaging/telegraph.cppm#L110)*

### pending

*public std::uint32_t pending()*



**brief** States held back, waiting for room. 

### publish

*public bool publish(const SnapshotKey & key, std::size_t bytes)*



**brief** The first `bytes` of what `snapshot( key, ... )` gave are the newest: the UI's next read of `key` takes them, whatever it missed before. Wakes the UI if it sleeps. False for a key never given room.

### quitting

*public bool quitting()*



**brief** Whether quit has been asked for: the model saved, and is to stop. 

### read_watched

*public void read_watched(M & model)*

*Defined at [messaging/telegraph.cppm:744](../../../../messaging/telegraph.cppm#L744)*



**brief** Calls `model.readable(*this, fd)` for each watched descriptor found ready since the last call, if the model has `readable`. `ModelRunner` calls it, after `deliver`.

### snapshot

*public std::span<std::byte> snapshot(const SnapshotKey & key, std::size_t bytes)*



**brief** Room for the snapshot `key`: at least `bytes`, the model's to fill, then `publish`. It grows (allocates, on the model's thread) only past every size asked for before. Empty when `TELEGRAPH_SNAPSHOTS` other snapshots exist already.

### start

*public void start(M & model)*

*Defined at [messaging/telegraph.cppm:680](../../../../messaging/telegraph.cppm#L680)*



**brief** Calls `model.start(*this)`, if it has one. Once, before `deliver`. 

 ---- the templates, now that the line is complete --------------------------------------------

### state

*public SendResult state(const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:123](../../../../messaging/telegraph.cppm#L123)*



**brief** `state <key> <value>`. When the ring is full it is held back, coalesced: `pending`. When the held-back table is full too, the model's thread waits up to `TELEGRAPH_WAIT_MS` for the UI to drain, then `full`.

### state

*public SendResult state(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.cppm:133](../../../../messaging/telegraph.cppm#L133)*



**brief** `state <key>[uid] <value>`, for a key that takes a uid. 

### tick_every

*public void tick_every(std::uint32_t milliseconds)*



**brief** Ask for `tick` every `milliseconds` (0: no tick, the default). `ModelRunner` calls it. 

### tick_interval

*public std::uint32_t tick_interval()*

### unwatch

*public void unwatch(int fd)*



**brief** Stop watching `fd`; nothing for one not watched. 

### watch

*public bool watch(int fd)*



**brief** Wake the model when `fd` has something to read (or has closed): its `readable( Telegraph &, int fd )` is called in the next turn, after `receive`. For a pty, a socket, a pipe; the model keeps the descriptor, and unwatches it before closing it. False for a model with no `readable`, a negative `fd`, or `TELEGRAPH_WATCHES` watched already; true when `fd` is watched already.

### watching

*public std::size_t watching()*



**brief** How many descriptors are watched. 

### ~Telegraph

*public void ~Telegraph()*

*Defined at [messaging/telegraph.cppm:113](../../../../messaging/telegraph.cppm#L113)*

