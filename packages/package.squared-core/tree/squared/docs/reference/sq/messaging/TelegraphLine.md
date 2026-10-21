# class TelegraphLine

*Defined at [messaging/telegraph.hpp:468](../../../../messaging/telegraph.hpp#L468)*



**brief** The Telegraph's storage and its two ends: the line between the UI station and the model's. The App owns one, made once at start (about 90 KB at the default knobs, every array inside it, and two `eventfd`s); the model is handed `model()`, the Stage `ui()`, and neither can reach the other's. A `ModelRunner` runs the model's end.



## Functions

### TelegraphLine

*public void TelegraphLine()*

### TelegraphLine

*public void TelegraphLine(const TelegraphLine & )*

*Defined at [messaging/telegraph.hpp:472](../../../../messaging/telegraph.hpp#L472)*

### operator=

*public TelegraphLine & operator=(const TelegraphLine & )*

*Defined at [messaging/telegraph.hpp:473](../../../../messaging/telegraph.hpp#L473)*

### TelegraphLine

*public void TelegraphLine(TelegraphLine && )*

*Defined at [messaging/telegraph.hpp:474](../../../../messaging/telegraph.hpp#L474)*

### operator=

*public TelegraphLine & operator=(TelegraphLine && )*

*Defined at [messaging/telegraph.hpp:475](../../../../messaging/telegraph.hpp#L475)*

### ~TelegraphLine

*public void ~TelegraphLine()*

*Defined at [messaging/telegraph.hpp:476](../../../../messaging/telegraph.hpp#L476)*

### model

*public Telegraph & model()*

*Defined at [messaging/telegraph.hpp:478](../../../../messaging/telegraph.hpp#L478)*

### ui

*public TelegraphUi & ui()*

*Defined at [messaging/telegraph.hpp:479](../../../../messaging/telegraph.hpp#L479)*



