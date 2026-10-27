# class TelegraphLine

*Defined at [messaging/telegraph.hpp:538](../../../../messaging/telegraph.hpp#L538)*



**brief** The Telegraph's storage and its two ends: the line between the UI station and the model's. The App owns one, made once at start (about 90 KB at the default knobs, every array inside it, and two `eventfd`s); the model is handed `model()`, the Stage `ui()`, and neither can reach the other's. A `ModelRunner` runs the model's end.



## Functions

### TelegraphLine

*public void TelegraphLine()*

### TelegraphLine

*public void TelegraphLine(const TelegraphLine & )*

*Defined at [messaging/telegraph.hpp:542](../../../../messaging/telegraph.hpp#L542)*

### operator=

*public TelegraphLine & operator=(const TelegraphLine & )*

*Defined at [messaging/telegraph.hpp:543](../../../../messaging/telegraph.hpp#L543)*

### TelegraphLine

*public void TelegraphLine(TelegraphLine && )*

*Defined at [messaging/telegraph.hpp:544](../../../../messaging/telegraph.hpp#L544)*

### operator=

*public TelegraphLine & operator=(TelegraphLine && )*

*Defined at [messaging/telegraph.hpp:545](../../../../messaging/telegraph.hpp#L545)*

### ~TelegraphLine

*public void ~TelegraphLine()*

*Defined at [messaging/telegraph.hpp:546](../../../../messaging/telegraph.hpp#L546)*

### model

*public Telegraph & model()*

*Defined at [messaging/telegraph.hpp:548](../../../../messaging/telegraph.hpp#L548)*

### ui

*public TelegraphUi & ui()*

*Defined at [messaging/telegraph.hpp:549](../../../../messaging/telegraph.hpp#L549)*



