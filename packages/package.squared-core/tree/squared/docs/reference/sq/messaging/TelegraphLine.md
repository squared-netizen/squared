# class TelegraphLine

*Defined at [messaging/telegraph.hpp:407](../../../../messaging/telegraph.hpp#L407)*



**brief** The Telegraph's storage and its two ends: the line between the UI station and the model's. The App owns one, made once at start (about 90 KB at the default knobs, every array inside it); the model is handed `model()`, the Stage `ui()`, and neither can reach the other's.



## Functions

### TelegraphLine

*public void TelegraphLine()*

### TelegraphLine

*public void TelegraphLine(const TelegraphLine & )*

*Defined at [messaging/telegraph.hpp:411](../../../../messaging/telegraph.hpp#L411)*

### operator=

*public TelegraphLine & operator=(const TelegraphLine & )*

*Defined at [messaging/telegraph.hpp:412](../../../../messaging/telegraph.hpp#L412)*

### TelegraphLine

*public void TelegraphLine(TelegraphLine && )*

*Defined at [messaging/telegraph.hpp:413](../../../../messaging/telegraph.hpp#L413)*

### operator=

*public TelegraphLine & operator=(TelegraphLine && )*

*Defined at [messaging/telegraph.hpp:414](../../../../messaging/telegraph.hpp#L414)*

### ~TelegraphLine

*public void ~TelegraphLine()*

*Defined at [messaging/telegraph.hpp:415](../../../../messaging/telegraph.hpp#L415)*

### model

*public Telegraph & model()*

*Defined at [messaging/telegraph.hpp:417](../../../../messaging/telegraph.hpp#L417)*

### ui

*public TelegraphUi & ui()*

*Defined at [messaging/telegraph.hpp:418](../../../../messaging/telegraph.hpp#L418)*



