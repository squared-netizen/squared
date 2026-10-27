# class Application

*Defined at [app/application.hpp:29](../../../../app/application.hpp#L29)*



**brief** Developer-owned application behind a platform-neutral lifecycle.

 The generated SDL adapter owns process and platform setup. Developer code implements this interface and uses Squared framework services for ordinary application behavior.



## Members

public static const std::size_t saved_state_bytes



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

*Defined at [app/application.hpp:32](../../../../app/application.hpp#L32)*

### set_text_input_service

*public void set_text_input_service(TextInputService * )*

*Defined at [app/application.hpp:35](../../../../app/application.hpp#L35)*

 Supply the portable application-to-platform text-input service. 

### set_report

*public void set_report(Report report)*

*Defined at [app/application.hpp:41](../../../../app/application.hpp#L41)*



**brief** Where to say what went wrong. Given before create(), and again whenever the platform changes it.

### pause

*public void pause()*

*Defined at [app/application.hpp:76](../../../../app/application.hpp#L76)*



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

*Defined at [app/application.hpp:79](../../../../app/application.hpp#L79)*



**brief** Notify the application that frame updates are resuming. 

### foreground

*public void foreground()*

*Defined at [app/application.hpp:85](../../../../app/application.hpp#L85)*



**brief** The application is visible again (Android's start). Not called before create(); the first start precedes it.

### background

*public void background()*

*Defined at [app/application.hpp:91](../../../../app/application.hpp#L91)*



**brief** The application is no longer visible (Android's stop): it may never come back, so this is where state is made safe.

### dispose

*public void dispose()*



**brief** Release logical and framework resources before destruction. 

### set_saved_state

*public void set_saved_state(std::span<const std::byte> saved)*

*Defined at [app/application.hpp:104](../../../../app/application.hpp#L104)*



**brief** What the platform kept for this activity, given before each create(): on Android, its saved instance state. Android keeps it only when it ended the process itself (memory, the freezer) or made the activity again; a swipe from the recent apps, a finish, a force stop or a reboot leaves it empty: a new launch. Valid until create() returns.

### save_state

*public std::size_t save_state(std::span<std::byte> out)*

*Defined at [app/application.hpp:113](../../../../app/application.hpp#L113)*



**brief** Write what should come back with the activity into `out` (Android's save instance state); the bytes written, 0 for nothing.

### take_background_request

*public bool take_background_request()*

*Defined at [app/application.hpp:124](../../../../app/application.hpp#L124)*



**brief** Whether the application asked to go to the background (back at its root screen): true once for each time it asked. The platform then moves its task back, as Home does.

### hides_bars

*public bool hides_bars()*

*Defined at [app/application.hpp:127](../../../../app/application.hpp#L127)*



**brief** Whether the system bars should be hidden (immersive): read on each frame. 

### low_memory

*public void low_memory()*

*Defined at [app/application.hpp:130](../../../../app/application.hpp#L130)*



**brief** The system is short of memory: drop what can be made again. 

### quit_requested

*public bool quit_requested()*



**brief** Report whether application logic requested shutdown.

**return** `true` when the platform should end the event loop.

### resize

*public void resize(int , int )*

*Defined at [app/application.hpp:133](../../../../app/application.hpp#L133)*



**brief** Notify the application of current drawable dimensions. 

### safe_area_changed

*public void safe_area_changed(const SafeArea & area)*

*Defined at [app/application.hpp:145](../../../../app/application.hpp#L145)*



**brief** Tell the application which edges of the drawable are covered.

 Called when the platform first measures the bars and again whenever they change (rotation, a hidden navigation bar). Not called on a platform with nothing covering the window, so the default - ignore it - is right for a desktop application.



**area** Pixels covered along each edge of the drawable.

### surface_created

*public void surface_created(graphics::Context & )*

*Defined at [app/application.hpp:148](../../../../app/application.hpp#L148)*



**brief** Restore or validate GPU resources after a surface becomes usable. 

### surface_destroyed

*public void surface_destroyed()*

*Defined at [app/application.hpp:155](../../../../app/application.hpp#L155)*



**brief** Mark GPU resources stale after rendering becomes unavailable.

 Implementations must not assume graphics commands are legal here.



