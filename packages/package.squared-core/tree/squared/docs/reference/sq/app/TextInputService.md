# class TextInputService

*Defined at [app/src/text_input_service.hpp:18](../../../../app/src/text_input_service.hpp#L18)*



**brief** Portable application-to-platform soft-keyboard boundary.

 This interface is implemented by the platform adapter. Application and GUI code obtains a pointer through Application::set_text_input_service and never depends on SDL or Android keyboard APIs. All methods are called by the widget framework on the application thread; the implementation owns platform IPC.



## Functions

### start

*public void start(const TextInputRequest & request)*



**brief** Show the soft keyboard for one focused control.

**request** The target rectangle and requested keyboard purpose.

### update_area

*public void update_area(const TextInputArea & area)*



**brief** Reposition the soft keyboard when the control moves or resizes.

**area** The updated on-screen target rectangle.

### stop

*public void stop()*



**brief** Hide the soft keyboard. Idempotent when already hidden. 

### active

*public bool active()*



**brief** Report whether platform text input is active.

**return** true while the soft keyboard is shown for a control.

### ~TextInputService

*public void ~TextInputService()*

*Defined at [app/src/text_input_service.hpp:21](../../../../app/src/text_input_service.hpp#L21)*



