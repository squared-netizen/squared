# class Application

*Defined at [app/application.cppm:22](../../../../app/application.cppm#L22)*

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

*Defined at [app/application.cppm:25](../../../../app/application.cppm#L25)*

### set_text_input_service

*public void set_text_input_service(TextInputService * )*

*Defined at [app/application.cppm:28](../../../../app/application.cppm#L28)*

### set_report

*public void set_report(Report report)*

*Defined at [app/application.cppm:34](../../../../app/application.cppm#L34)*

### pause

*public void pause()*

*Defined at [app/application.cppm:69](../../../../app/application.cppm#L69)*

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

*Defined at [app/application.cppm:72](../../../../app/application.cppm#L72)*

### foreground

*public void foreground()*

*Defined at [app/application.cppm:78](../../../../app/application.cppm#L78)*

### background

*public void background()*

*Defined at [app/application.cppm:84](../../../../app/application.cppm#L84)*

### dispose

*public void dispose()*



**brief** Release logical and framework resources before destruction. 

### set_saved_state

*public void set_saved_state(std::span<const std::byte> saved)*

*Defined at [app/application.cppm:97](../../../../app/application.cppm#L97)*

### save_state

*public std::size_t save_state(std::span<std::byte> out)*

*Defined at [app/application.cppm:106](../../../../app/application.cppm#L106)*

### take_background_request

*public bool take_background_request()*

*Defined at [app/application.cppm:117](../../../../app/application.cppm#L117)*

### hides_bars

*public bool hides_bars()*

*Defined at [app/application.cppm:120](../../../../app/application.cppm#L120)*

### low_memory

*public void low_memory()*

*Defined at [app/application.cppm:123](../../../../app/application.cppm#L123)*

### quit_requested

*public bool quit_requested()*



**brief** Report whether application logic requested shutdown.

**return** `true` when the platform should end the event loop.

### resize

*public void resize(int , int )*

*Defined at [app/application.cppm:126](../../../../app/application.cppm#L126)*

### safe_area_changed

*public void safe_area_changed(const SafeArea & area)*

*Defined at [app/application.cppm:138](../../../../app/application.cppm#L138)*

### surface_created

*public void surface_created(graphics::Context & )*

*Defined at [app/application.cppm:141](../../../../app/application.cppm#L141)*

### surface_destroyed

*public void surface_destroyed()*

*Defined at [app/application.cppm:148](../../../../app/application.cppm#L148)*



