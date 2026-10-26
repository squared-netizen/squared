# namespace app

## Records

* [Application](Application.md)
* [Event](Event.md)
* [KeyModifiers](KeyModifiers.md)
* [Report](Report.md)
* [Runtime](Runtime.md)
* [SafeArea](SafeArea.md)
* [TextInputArea](TextInputArea.md)
* [TextInputRequest](TextInputRequest.md)
* [TextInputService](TextInputService.md)


## Functions

### operator==

*bool operator==(KeyModifiers , KeyModifiers )*

*Defined at [app/src/key_modifiers.hpp:33](../../../../app/src/key_modifiers.hpp#L33)*



**brief** Equal when the same modifiers are held. 

### operator==

*bool operator==(const SafeArea & , const SafeArea & )*

*Defined at [app/safe_area.hpp:30](../../../../app/safe_area.hpp#L30)*

## Enums

| enum class Key |

--

| unknown |
| left |
| right |
| up |
| down |
| home |
| end |
| backspace |
| delete_key |
| enter |
| space |
| tab |
| escape |


*Defined at [app/src/key.hpp:15](../../../../app/src/key.hpp#L15)*



**brief** Portable key names for key events.

 The one key vocabulary in squared: the platform layer produces it in an Event, and the GUI routes it to the focused widget unchanged. A key squared does not name arrives as `unknown`, and nothing routes it.

| enum class Navigation |

--

| unknown |
| left |
| right |
| up |
| down |
| next |
| previous |
| activate |
| cancel |


*Defined at [app/src/navigation.hpp:14](../../../../app/src/navigation.hpp#L14)*



**brief** Semantic focus and navigation action, from any supported device.

 A D-pad, a gamepad and a keyboard's arrow and Tab keys all produce these. The GUI routes one to the focused widget first, then moves focus with it.

| enum class TextInputPurpose |

--

| normal |
| number |
| email |
| password |


*Defined at [app/src/text_input_purpose.hpp:13](../../../../app/src/text_input_purpose.hpp#L13)*



**brief** Desired soft-keyboard content mode.



**sa** TextInputRequest::purpose

