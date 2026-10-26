# class Application

*Defined at [app/application.hpp:27](../../../../app/application.hpp#L27)*



**brief** Developer-owned application behind a platform-neutral lifecycle.

 The generated SDL adapter owns process and platform setup. Developer code implements this interface and uses Squared framework services for ordinary application behavior.



## Functions

### create

*public bool create(Runtime & runtime)*



**brief** Initialize logical state once.



**runtime** The platform services: rendering context, file system and asset manager. Keep references to the members you use; the objects they refer to live for the whole process.

**return** `true` when the application may enter its event loop.

**note** Only create() receives the whole runtime. render() still takes the context alone, because a frame should not be reaching into the file system or the asset cache sixty times a second.

### handle_event

*public void handle_event(const Event & event)*



**brief** Receive one platform-neutral input or lifecycle event.

**event** Portable event produced by the platform adapter.

### ~Application

*public void ~Application()*

*Defined at [app/application.hpp:30](../../../../app/application.hpp#L30)*

### set_text_input_service

*public void set_text_input_service(TextInputService * )*

*Defined at [app/application.hpp:33](../../../../app/application.hpp#L33)*

 Supply the portable application-to-platform text-input service. 

### set_report

*public void set_report(Report report)*

*Defined at [app/application.hpp:39](../../../../app/application.hpp#L39)*



**brief** Where to say what went wrong. Given before create(), and again whenever the platform changes it.

### pause

*public void pause()*

*Defined at [app/application.hpp:74](../../../../app/application.hpp#L74)*



**brief** Notify the application that frame updates are pausing. 

### update

*public void update(std::chrono::nanoseconds delta)*



**brief** Advance logical state once for the current frame.

**delta** Elapsed domain time since the previous update, in nanoseconds.

### render

*public void render(graphics::Context & graphics)*



**brief** Render one frame using the active graphics context.

**graphics** The active platform rendering context.

### resume

*public void resume()*

*Defined at [app/application.hpp:77](../../../../app/application.hpp#L77)*



**brief** Notify the application that frame updates are resuming. 

### foreground

*public void foreground()*

*Defined at [app/application.hpp:83](../../../../app/application.hpp#L83)*



**brief** The application is visible again (Android's start). Not called before create(); the first start precedes it.

### background

*public void background()*

*Defined at [app/application.hpp:89](../../../../app/application.hpp#L89)*



**brief** The application is no longer visible (Android's stop): it may never come back, so this is where state is made safe.

### dispose

*public void dispose()*



**brief** Release logical and framework resources before destruction. 

### low_memory

*public void low_memory()*

*Defined at [app/application.hpp:92](../../../../app/application.hpp#L92)*



**brief** The system is short of memory: drop what can be made again. 

### quit_requested

*public bool quit_requested()*



**brief** Report whether application logic requested shutdown.

**return** `true` when the platform should end the event loop.

### resize

*public void resize(int , int )*

*Defined at [app/application.hpp:95](../../../../app/application.hpp#L95)*



**brief** Notify the application of current drawable dimensions. 

### safe_area_changed

*public void safe_area_changed(const SafeArea & area)*

*Defined at [app/application.hpp:107](../../../../app/application.hpp#L107)*



**brief** Tell the application which edges of the drawable are covered.

 Called when the platform first measures the bars and again whenever they change (rotation, a hidden navigation bar). Not called on a platform with nothing covering the window, so the default - ignore it - is right for a desktop application.



**area** Pixels covered along each edge of the drawable.

### surface_created

*public void surface_created(graphics::Context & )*

*Defined at [app/application.hpp:110](../../../../app/application.hpp#L110)*



**brief** Restore or validate GPU resources after a surface becomes usable. 

### surface_destroyed

*public void surface_destroyed()*

*Defined at [app/application.hpp:117](../../../../app/application.hpp#L117)*



**brief** Mark GPU resources stale after rendering becomes unavailable.

 Implementations must not assume graphics commands are legal here.



