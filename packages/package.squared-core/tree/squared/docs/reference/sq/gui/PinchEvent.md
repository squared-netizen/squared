# struct PinchEvent

*Defined at [gui/src/pinch_event.cppm:16](../../../../gui/src/pinch_event.cppm#L16)*



**brief** A two-finger pinch, offered to the widget under its middle (`Widget::pinch`). The Stage recognises it; a widget that takes it at `begin` gets its `change`s and its `end`, and the UI is not zoomed.



## Members

public PinchPhase phase

public float scale

public float x

public float y



