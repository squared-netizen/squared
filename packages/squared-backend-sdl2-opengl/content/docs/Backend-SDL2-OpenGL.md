# Squared SDL2/OpenGL backend

This module supplies the link-time implementation of Squared Graphics and
Graphics2D for SDL2 with OpenGL ES 2. It owns window/context creation, texture
uploads, atlas file loading, shaders, buffers, drawing, and presentation.

Applications select the backend by including this module in their exact SQ
dependency graph. There is no runtime backend registry, backend lookup, or
virtual dispatch in the frame loop. A platform template must link exactly one
graphics backend.

Portable application and framework code should include only headers from
Squared Graphics, Graphics2D, and Scene2D. SDL and OpenGL headers belong in
this backend package and in the platform template.
