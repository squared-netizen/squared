# class TelegraphLine

*Defined at [messaging/telegraph.cppm:556](../../../../messaging/telegraph.cppm#L556)*



**brief** The Telegraph's storage and its two ends: the line between the UI station and the model's. The App owns one, made once at start (about 90 KB at the default knobs, every array inside it, and two `eventfd`s); the model is handed `model()`, the Stage `ui()`, and neither can reach the other's. A `ModelRunner` runs the model's end.



## Functions

### TelegraphLine

*public void TelegraphLine()*

### TelegraphLine

*public void TelegraphLine(TelegraphLine && )*

*Defined at [messaging/telegraph.cppm:562](../../../../messaging/telegraph.cppm#L562)*

### TelegraphLine

*public void TelegraphLine(const TelegraphLine & )*

*Defined at [messaging/telegraph.cppm:560](../../../../messaging/telegraph.cppm#L560)*

### model

*public Telegraph & model()*

*Defined at [messaging/telegraph.cppm:566](../../../../messaging/telegraph.cppm#L566)*

### operator=

*public TelegraphLine & operator=(TelegraphLine && )*

*Defined at [messaging/telegraph.cppm:563](../../../../messaging/telegraph.cppm#L563)*

### operator=

*public TelegraphLine & operator=(const TelegraphLine & )*

*Defined at [messaging/telegraph.cppm:561](../../../../messaging/telegraph.cppm#L561)*

### ui

*public TelegraphUi & ui()*

*Defined at [messaging/telegraph.cppm:567](../../../../messaging/telegraph.cppm#L567)*

### ~TelegraphLine

*public void ~TelegraphLine()*

*Defined at [messaging/telegraph.cppm:564](../../../../messaging/telegraph.cppm#L564)*

