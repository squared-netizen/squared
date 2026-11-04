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

*Defined at [app/src/key_modifiers.cppm:33](../../../../app/src/key_modifiers.cppm#L33)*



**brief** Equal when the same modifiers are held. 

### operator==

*bool operator==(const SafeArea & , const SafeArea & )*

*Defined at [app/safe_area.cppm:30](../../../../app/safe_area.cppm#L30)*

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
| page_up |
| page_down |
| insert |
| f1 |
| f2 |
| f3 |
| f4 |
| f5 |
| f6 |
| f7 |
| f8 |
| f9 |
| f10 |
| f11 |
| f12 |


*Defined at [app/src/key.cppm:19](../../../../app/src/key.cppm#L19)*



**brief** Portable key names for key events.

 The one key vocabulary in squared: the platform layer produces it in an Event, and the GUI routes it to the focused widget unchanged. A key squared does not name arrives as `unknown`, and nothing routes it.

 Letters and digits are not keys here: what a key types arrives as text (`Event::Type::TextInput`), with Ctrl and Alt in the event's modifiers. From `page_up` on (12.3d): the keys a terminal needs that a form does not.

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


*Defined at [app/src/navigation.cppm:14](../../../../app/src/navigation.cppm#L14)*



**brief** Semantic focus and navigation action, from any supported device.

 A D-pad, a gamepad and a keyboard's arrow and Tab keys all produce these. The GUI routes one to the focused widget first, then moves focus with it.

| enum class TextInputPurpose |

--

| normal |
| number |
| email |
| password |


*Defined at [app/src/text_input_purpose.cppm:13](../../../../app/src/text_input_purpose.cppm#L13)*



**brief** Desired soft-keyboard content mode.



**sa** TextInputRequest::purpose

