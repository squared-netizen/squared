# class TelegraphLine

*Defined at [messaging/telegraph.hpp:488](../../../../messaging/telegraph.hpp#L488)*



**brief** The Telegraph's storage and its two ends: the line between the UI station and the model's. The App owns one, made once at start (about 90 KB at the default knobs, every array inside it, and two `eventfd`s); the model is handed `model()`, the Stage `ui()`, and neither can reach the other's. A `ModelRunner` runs the model's end.



## Functions

### TelegraphLine

*public void TelegraphLine()*

### TelegraphLine

*public void TelegraphLine(const TelegraphLine & )*

*Defined at [messaging/telegraph.hpp:492](../../../../messaging/telegraph.hpp#L492)*

### operator=

*public TelegraphLine & operator=(const TelegraphLine & )*

*Defined at [messaging/telegraph.hpp:493](../../../../messaging/telegraph.hpp#L493)*

### TelegraphLine

*public void TelegraphLine(TelegraphLine && )*

*Defined at [messaging/telegraph.hpp:494](../../../../messaging/telegraph.hpp#L494)*

### operator=

*public TelegraphLine & operator=(TelegraphLine && )*

*Defined at [messaging/telegraph.hpp:495](../../../../messaging/telegraph.hpp#L495)*

### ~TelegraphLine

*public void ~TelegraphLine()*

*Defined at [messaging/telegraph.hpp:496](../../../../messaging/telegraph.hpp#L496)*

### model

*public Telegraph & model()*

*Defined at [messaging/telegraph.hpp:498](../../../../messaging/telegraph.hpp#L498)*

### ui

*public TelegraphUi & ui()*

*Defined at [messaging/telegraph.hpp:499](../../../../messaging/telegraph.hpp#L499)*



