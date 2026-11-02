# class Renderer

*Defined at [console/renderer.cppm:29](../../../../console/renderer.cppm#L29)*



**brief** Draws a Console with GLES 3.0: one shader, one 8 KB font texture, and one batch of 512 characters (16 KB) refilled as often as a frame needs.

 Built only with the gles graphics backend. Everything it needs is made in create(), with the context current; draw() allocates nothing. It sets the GL state it draws with (blending on; scissor, depth and culling off) and leaves no program or array buffer bound; whatever draws next sets its own.

 ```cpp sq::console::Renderer renderer; if( !renderer.create() ) { ... }                    // once the context is current renderer.draw( console, frame, pixel_width, pixel_height ); // after the app's frame ```

 When the context is lost with its objects, call invalidate() and then create() again; destroy() releases them while the context is current.



## Members

public static const std::size_t batch_quads



## Functions

### Renderer

*public void Renderer()*

*Defined at [console/renderer.cppm:35](../../../../console/renderer.cppm#L35)*

### Renderer

*public void Renderer(Renderer && )*

*Defined at [console/renderer.cppm:40](../../../../console/renderer.cppm#L40)*

### Renderer

*public void Renderer(const Renderer & )*

*Defined at [console/renderer.cppm:38](../../../../console/renderer.cppm#L38)*

### create

*public bool create()*



**brief** Compile the shader, upload the font, make the buffers. Needs a current context. False if any of it failed; nothing is left behind.

### destroy

*public void destroy()*



**brief** Delete the GL objects. The context must be current. 

### draw

*public void draw(const Console & console, const Frame & frame, int width, int height)*



**brief** Draw `console` into `frame`; the viewport is `width` by `height` pixels. Does nothing until create() has succeeded.

### invalidate

*public void invalidate()*



**brief** The context has gone with its objects: forget them without deleting. 

### operator=

*public Renderer & operator=(Renderer && )*

*Defined at [console/renderer.cppm:41](../../../../console/renderer.cppm#L41)*

### operator=

*public Renderer & operator=(const Renderer & )*

*Defined at [console/renderer.cppm:39](../../../../console/renderer.cppm#L39)*

### valid

*public bool valid()*

*Defined at [console/renderer.cppm:61](../../../../console/renderer.cppm#L61)*

### ~Renderer

*public void ~Renderer()*

