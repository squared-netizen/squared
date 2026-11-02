# class Telegraph

*Defined at [messaging/telegraph.cppm:103](../../../../messaging/telegraph.cppm#L103)*

## Functions

### Telegraph

*public void Telegraph(const Telegraph & )*

*Defined at [messaging/telegraph.cppm:106](../../../../messaging/telegraph.cppm#L106)*

### operator=

*public Telegraph & operator=(const Telegraph & )*

*Defined at [messaging/telegraph.cppm:107](../../../../messaging/telegraph.cppm#L107)*

### Telegraph

*public void Telegraph(Telegraph && )*

*Defined at [messaging/telegraph.cppm:108](../../../../messaging/telegraph.cppm#L108)*

### operator=

*public Telegraph & operator=(Telegraph && )*

*Defined at [messaging/telegraph.cppm:109](../../../../messaging/telegraph.cppm#L109)*

### ~Telegraph

*public void ~Telegraph()*

*Defined at [messaging/telegraph.cppm:110](../../../../messaging/telegraph.cppm#L110)*

### state

*public SendResult state(const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:120](../../../../messaging/telegraph.cppm#L120)*

### state

*public SendResult state(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.cppm:130](../../../../messaging/telegraph.cppm#L130)*

### event

*public SendResult event(const K & key)*

*Defined at [messaging/telegraph.cppm:144](../../../../messaging/telegraph.cppm#L144)*

### event

*public SendResult event(const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:152](../../../../messaging/telegraph.cppm#L152)*

### event

*public SendResult event(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.cppm:162](../../../../messaging/telegraph.cppm#L162)*

### answer

*public SendResult answer(const Received & request, const K & key)*

*Defined at [messaging/telegraph.cppm:172](../../../../messaging/telegraph.cppm#L172)*

### answer

*public SendResult answer(const Received & request, const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:181](../../../../messaging/telegraph.cppm#L181)*

### list

*public bool list(const K & rows, S & source)*

*Defined at [messaging/telegraph.cppm:197](../../../../messaging/telegraph.cppm#L197)*

### changed

*public void changed(const K & rows)*

*Defined at [messaging/telegraph.cppm:205](../../../../messaging/telegraph.cppm#L205)*

### changed

*public void changed(const K & rows, std::uint32_t uid)*

*Defined at [messaging/telegraph.cppm:213](../../../../messaging/telegraph.cppm#L213)*

### tick_every

*public void tick_every(std::uint32_t milliseconds)*



**brief** Ask for `tick` every `milliseconds` (0: no tick, the default). `ModelRunner` calls it. 

### tick_interval

*public std::uint32_t tick_interval()*

### start

*public void start(M & model)*

*Defined at [messaging/telegraph.cppm:610](../../../../messaging/telegraph.cppm#L610)*

### deliver

*public std::uint32_t deliver(M & model)*

*Defined at [messaging/telegraph.cppm:618](../../../../messaging/telegraph.cppm#L618)*

### pending

*public std::uint32_t pending()*



**brief** States held back, waiting for room. 

### quitting

*public bool quitting()*



**brief** Whether quit has been asked for: the model saved, and is to stop. 



