# class Renderer

*Defined at [console/renderer.hpp:32](../../../../console/renderer.hpp#L32)*



**brief** Draws a Console with GLES 3.0: one shader, one 8 KB font texture, and one batch of 512 characters (16 KB) refilled as often as a frame needs.

 Built only with the gles graphics backend. Everything it needs is made in create(), with the context current; draw() allocates nothing. It sets the GL state it draws with (blending on; scissor, depth and culling off) and leaves no program or array buffer bound; whatever draws next sets its own.

 ```cpp sq::console::Renderer renderer; if( !renderer.create() ) { ... }                    // once the context is current renderer.draw( console, frame, pixel_width, pixel_height ); // after the app's frame ```

 When the context is lost with its objects, call invalidate() and then create() again; destroy() releases them while the context is current.



## Members

public static const std::size_t batch_quads



## Functions

### Renderer

*public void Renderer()*

*Defined at [console/renderer.hpp:38](../../../../console/renderer.hpp#L38)*

### Renderer

*public void Renderer(const Renderer & )*

*Defined at [console/renderer.hpp:41](../../../../console/renderer.hpp#L41)*

### operator=

*public Renderer & operator=(const Renderer & )*

*Defined at [console/renderer.hpp:42](../../../../console/renderer.hpp#L42)*

### Renderer

*public void Renderer(Renderer && )*

*Defined at [console/renderer.hpp:43](../../../../console/renderer.hpp#L43)*

### operator=

*public Renderer & operator=(Renderer && )*

*Defined at [console/renderer.hpp:44](../../../../console/renderer.hpp#L44)*

### ~Renderer

*public void ~Renderer()*

### create

*public bool create()*



**brief** Compile the shader, upload the font, make the buffers. Needs a current context. False if any of it failed; nothing is left behind.

### draw

*public void draw(const Console & console, const Frame & frame, int width, int height)*



**brief** Draw `console` into `frame`; the viewport is `width` by `height` pixels. Does nothing until create() has succeeded.

### invalidate

*public void invalidate()*



**brief** The context has gone with its objects: forget them without deleting. 

### destroy

*public void destroy()*



**brief** Delete the GL objects. The context must be current. 

### valid

*public bool valid()*

*Defined at [console/renderer.hpp:64](../../../../console/renderer.hpp#L64)*



