# struct Event

*Defined at [app/event.hpp:21](../../../../app/event.hpp#L21)*



**brief** Platform-neutral application event.

 The platform layer converts what the system reports into this compact representation before application code receives it. Keys, modifiers and navigation use the same types the GUI routes, so nothing converts them again on the way to a widget.



## Members

public Type type

public std::int64_t pointer_id

public float x

public float y

public int width

public int height

public Key key

public KeyModifiers modifiers

public Navigation navigation

public std::int32_t input_device_id

public bool repeat

public std::string text

public int editing_start

public int editing_length



## Enums

| enum class Type |

--

| QuitRequested |
| PointerDown |
| PointerMove |
| PointerUp |
| BackRequested |
| Pause |
| Resume |
| Resize |
| KeyDown |
| KeyUp |
| NavigationInput |
| TextInput |
| TextEditing |
| TextInputShown |
| TextInputHidden |


*Defined at [app/event.hpp:24](../../../../app/event.hpp#L24)*



**brief** Supported event categories for the Phase 5 boundary. 



