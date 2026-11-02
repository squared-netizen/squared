# class Application

*Defined at [app/application.cppm:22](../../../../app/application.cppm#L22)*



**brief** Developer-owned application behind a platform-neutral lifecycle.

 The generated SDL adapter owns process and platform setup. Developer code implements this interface and uses Squared framework services for ordinary application behavior.



## Members

public static const std::size_t saved_state_bytes



## Functions

### background

*public void background()*

*Defined at [app/application.cppm:84](../../../../app/application.cppm#L84)*



**brief** The application is no longer visible (Android's stop): it may never come back, so this is where state is made safe.

### create

*public bool create(Runtime & runtime)*



**brief** Initialize logical state once.



**runtime** The platform services: rendering context, file system and asset manager. Keep references to the members you use; the objects they refer to live for the whole process.

**return** `true` when the application may enter its event loop.

**note** Only create() receives the whole runtime. render() still takes the context alone, because a frame should not be reaching into the file system or the asset cache sixty times a second.

### dispose

*public void dispose()*



**brief** Release logical and framework resources before destruction. 

### foreground

*public void foreground()*

*Defined at [app/application.cppm:78](../../../../app/application.cppm#L78)*



**brief** The application is visible again (Android's start). Not called before create(); the first start precedes it.

### handle_event

*public void handle_event(const Event & event)*



**brief** Receive one platform-neutral input or lifecycle event.

**event** Portable event produced by the platform adapter.

### hides_bars

*public bool hides_bars()*

*Defined at [app/application.cppm:120](../../../../app/application.cppm#L120)*



**brief** Whether the system bars should be hidden (immersive): read on each frame. 

### low_memory

*public void low_memory()*

*Defined at [app/application.cppm:123](../../../../app/application.cppm#L123)*



**brief** The system is short of memory: drop what can be made again. 

### pause

*public void pause()*

*Defined at [app/application.cppm:69](../../../../app/application.cppm#L69)*



**brief** Notify the application that frame updates are pausing. 

### quit_requested

*public bool quit_requested()*



**brief** Report whether application logic requested shutdown.

**return** `true` when the platform should end the event loop.

### render

*public void render(graphics::Context & graphics)*



**brief** Render one frame using the active graphics context.

**graphics** The active platform rendering context.

### resize

*public void resize(int , int )*

*Defined at [app/application.cppm:126](../../../../app/application.cppm#L126)*



**brief** Notify the application of current drawable dimensions. 

### resume

*public void resume()*

*Defined at [app/application.cppm:72](../../../../app/application.cppm#L72)*



**brief** Notify the application that frame updates are resuming. 

### safe_area_changed

*public void safe_area_changed(const SafeArea & area)*

*Defined at [app/application.cppm:138](../../../../app/application.cppm#L138)*



**brief** Tell the application which edges of the drawable are covered.

 Called when the platform first measures the bars and again whenever they change (rotation, a hidden navigation bar). Not called on a platform with nothing covering the window, so the default - ignore it - is right for a desktop application.



**area** Pixels covered along each edge of the drawable.

### save_state

*public std::size_t save_state(std::span<std::byte> out)*

*Defined at [app/application.cppm:106](../../../../app/application.cppm#L106)*



**brief** Write what should come back with the activity into `out` (Android's save instance state); the bytes written, 0 for nothing.

### set_report

*public void set_report(Report report)*

*Defined at [app/application.cppm:34](../../../../app/application.cppm#L34)*



**brief** Where to say what went wrong. Given before create(), and again whenever the platform changes it.

### set_saved_state

*public void set_saved_state(std::span<const std::byte> saved)*

*Defined at [app/application.cppm:97](../../../../app/application.cppm#L97)*



**brief** What the platform kept for this activity, given before each create(): on Android, its saved instance state. Android keeps it only when it ended the process itself (memory, the freezer) or made the activity again; a swipe from the recent apps, a finish, a force stop or a reboot leaves it empty: a new launch. Valid until create() returns.

### set_text_input_service

*public void set_text_input_service(TextInputService * )*

*Defined at [app/application.cppm:28](../../../../app/application.cppm#L28)*

 Supply the portable application-to-platform text-input service. 

### surface_created

*public void surface_created(graphics::Context & )*

*Defined at [app/application.cppm:141](../../../../app/application.cppm#L141)*



**brief** Restore or validate GPU resources after a surface becomes usable. 

### surface_destroyed

*public void surface_destroyed()*

*Defined at [app/application.cppm:148](../../../../app/application.cppm#L148)*



**brief** Mark GPU resources stale after rendering becomes unavailable.

 Implementations must not assume graphics commands are legal here.

### take_background_request

*public bool take_background_request()*

*Defined at [app/application.cppm:117](../../../../app/application.cppm#L117)*



**brief** Whether the application asked to go to the background (back at its root screen): true once for each time it asked. The platform then moves its task back, as Home does.

### update

*public void update(std::chrono::nanoseconds delta)*



**brief** Advance logical state once for the current frame.

**delta** Elapsed domain time since the previous update, in nanoseconds.

### ~Application

*public void ~Application()*

*Defined at [app/application.cppm:25](../../../../app/application.cppm#L25)*

