# class TelegraphLine

*Defined at [messaging/telegraph.hpp:525](../../../../messaging/telegraph.hpp#L525)*



**brief** The Telegraph's storage and its two ends: the line between the UI station and the model's. The App owns one, made once at start (about 90 KB at the default knobs, every array inside it, and two `eventfd`s); the model is handed `model()`, the Stage `ui()`, and neither can reach the other's. A `ModelRunner` runs the model's end.



## Functions

### TelegraphLine

*public void TelegraphLine()*

### TelegraphLine

*public void TelegraphLine(const TelegraphLine & )*

*Defined at [messaging/telegraph.hpp:529](../../../../messaging/telegraph.hpp#L529)*

### operator=

*public TelegraphLine & operator=(const TelegraphLine & )*

*Defined at [messaging/telegraph.hpp:530](../../../../messaging/telegraph.hpp#L530)*

### TelegraphLine

*public void TelegraphLine(TelegraphLine && )*

*Defined at [messaging/telegraph.hpp:531](../../../../messaging/telegraph.hpp#L531)*

### operator=

*public TelegraphLine & operator=(TelegraphLine && )*

*Defined at [messaging/telegraph.hpp:532](../../../../messaging/telegraph.hpp#L532)*

### ~TelegraphLine

*public void ~TelegraphLine()*

*Defined at [messaging/telegraph.hpp:533](../../../../messaging/telegraph.hpp#L533)*

### model

*public Telegraph & model()*

*Defined at [messaging/telegraph.hpp:535](../../../../messaging/telegraph.hpp#L535)*

### ui

*public TelegraphUi & ui()*

*Defined at [messaging/telegraph.hpp:536](../../../../messaging/telegraph.hpp#L536)*



