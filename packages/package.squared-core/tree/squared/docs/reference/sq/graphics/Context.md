# class Context

*Defined at [graphics/context.hpp:18](../../../../graphics/context.hpp#L18)*



**brief** Own the native window and rendering context selected at link time.

 The generated platform layer owns this object. Application rendering code receives higher-level graphics objects and does not present the window directly.



## Functions

### Context

*public void Context()*

*Defined at [graphics/context.hpp:21](../../../../graphics/context.hpp#L21)*

### Context

*public void Context(const Context & )*

*Defined at [graphics/context.hpp:22](../../../../graphics/context.hpp#L22)*

### operator=

*public Context & operator=(const Context & )*

*Defined at [graphics/context.hpp:23](../../../../graphics/context.hpp#L23)*

### Context

*public void Context(Context && )*

*Defined at [graphics/context.hpp:24](../../../../graphics/context.hpp#L24)*

### operator=

*public Context & operator=(Context && )*

*Defined at [graphics/context.hpp:25](../../../../graphics/context.hpp#L25)*

### ~Context

*public void ~Context()*

### create

*public bool create(const ContextConfig & config)*



**brief** Adopt or create a window and make its rendering context current.



**config** Native window handle, or the size and title for a backend that creates its own.

**return** `true` when the window and context are ready.

### destroy

*public void destroy()*



**brief** Destroy the context and window. 

### suspend

*public void suspend()*



**brief** Release the native rendering context while retaining the window.

 Callers may release GPU objects before this call while the context is current. On Android this is driven by the platform layer losing the surface, not by application code.

### resume

*public bool resume()*



**brief** Create a new rendering context for the retained window.

 A successful restoration advances `generation()`. Applications then receive `surface_created()` and restore their GPU objects.



**return** `true` when a usable context was activated.

### set_native_window

*public void set_native_window(void * native_window)*



**brief** Adopt a new native window before the next resume().

**native_window** Handle the platform layer received, or null.

**note** Android destroys the window on backgrounding and hands back a different one, so suspend() and resume() are not enough on their own: the platform layer calls this between them. A backend with no such concept ignores it.

### refresh_viewport

*public void refresh_viewport()*



**brief** Refresh drawable dimensions and the backend viewport. 

### clear

*public void clear(Color color)*



**brief** Clear the active color buffer.

**color** Fill color for the clear.

### present

*public bool present()*



**brief** Present the completed frame.

**return** `false` when the surface was lost and the frame did not reach the display. Stop drawing and wait for the platform layer to resume; `generation()` tells you whether GPU objects have to be rebuilt.

**note** The return value is the only notice some devices give that the surface has gone, which is why it is [[nodiscard]]. Discarding it shows up as an application frozen on its last frame after a phone call.

### valid

*public bool valid()*



**brief** Return whether a usable context exists.

**return** `true` after `create()` or a successful `resume()`.

### pixel_width

*public int pixel_width()*



**brief** Return the current framebuffer width in pixels.

**return** Physical pixel width of the active drawable.

### pixel_height

*public int pixel_height()*



**brief** Return the current framebuffer height in pixels.

**return** Physical pixel height of the active drawable.

### generation

*public int generation()*



**brief** Monotonic identity of the current native rendering context.

**return** Generation counter advanced by every successful activation.

### resources_preserved

*public bool resources_preserved()*



**brief** Return whether the GPU objects an application owns are still valid.



**return** `false` only when objects that existed before have been lost and must be rebuilt.



**note** This answers the question a caller actually asks - "do I need to rebuild?" - rather than "did a resume preserve anything". The two differ exactly once, at the first `create()`: there was no previous context, so nothing was lost, and this returns `true`. Reporting `false` there made every application reload its assets before drawing its first frame.



