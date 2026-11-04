# class TelegraphLine

*Defined at [messaging/telegraph.cppm:586](../../../../messaging/telegraph.cppm#L586)*



**brief** The Telegraph's storage and its two ends: the line between the UI station and the model's. The App owns one, made once at start (about 90 KB at the default knobs, every array inside it, and two `eventfd`s); the model is handed `model()`, the Stage `ui()`, and neither can reach the other's. A `ModelRunner` runs the model's end.



## Functions

### TelegraphLine

*public void TelegraphLine()*

### TelegraphLine

*public void TelegraphLine(TelegraphLine && )*

*Defined at [messaging/telegraph.cppm:592](../../../../messaging/telegraph.cppm#L592)*

### TelegraphLine

*public void TelegraphLine(const TelegraphLine & )*

*Defined at [messaging/telegraph.cppm:590](../../../../messaging/telegraph.cppm#L590)*

### model

*public Telegraph & model()*

*Defined at [messaging/telegraph.cppm:596](../../../../messaging/telegraph.cppm#L596)*

### operator=

*public TelegraphLine & operator=(TelegraphLine && )*

*Defined at [messaging/telegraph.cppm:593](../../../../messaging/telegraph.cppm#L593)*

### operator=

*public TelegraphLine & operator=(const TelegraphLine & )*

*Defined at [messaging/telegraph.cppm:591](../../../../messaging/telegraph.cppm#L591)*

### ui

*public TelegraphUi & ui()*

*Defined at [messaging/telegraph.cppm:597](../../../../messaging/telegraph.cppm#L597)*

### ~TelegraphLine

*public void ~TelegraphLine()*

*Defined at [messaging/telegraph.cppm:594](../../../../messaging/telegraph.cppm#L594)*

