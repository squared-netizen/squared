# class TelegraphLine

*Defined at [messaging/telegraph.cppm:529](../../../../messaging/telegraph.cppm#L529)*



**brief** The Telegraph's storage and its two ends: the line between the UI station and the model's. The App owns one, made once at start (about 90 KB at the default knobs, every array inside it, and two `eventfd`s); the model is handed `model()`, the Stage `ui()`, and neither can reach the other's. A `ModelRunner` runs the model's end.



## Functions

### TelegraphLine

*public void TelegraphLine()*

### TelegraphLine

*public void TelegraphLine(TelegraphLine && )*

*Defined at [messaging/telegraph.cppm:535](../../../../messaging/telegraph.cppm#L535)*

### TelegraphLine

*public void TelegraphLine(const TelegraphLine & )*

*Defined at [messaging/telegraph.cppm:533](../../../../messaging/telegraph.cppm#L533)*

### model

*public Telegraph & model()*

*Defined at [messaging/telegraph.cppm:539](../../../../messaging/telegraph.cppm#L539)*

### operator=

*public TelegraphLine & operator=(TelegraphLine && )*

*Defined at [messaging/telegraph.cppm:536](../../../../messaging/telegraph.cppm#L536)*

### operator=

*public TelegraphLine & operator=(const TelegraphLine & )*

*Defined at [messaging/telegraph.cppm:534](../../../../messaging/telegraph.cppm#L534)*

### ui

*public TelegraphUi & ui()*

*Defined at [messaging/telegraph.cppm:540](../../../../messaging/telegraph.cppm#L540)*

### ~TelegraphLine

*public void ~TelegraphLine()*

*Defined at [messaging/telegraph.cppm:537](../../../../messaging/telegraph.cppm#L537)*

