# class Matrix4

*Defined at [math/matrix4.cppm:17](../../../../math/matrix4.cppm#L17)*

## Functions

### Matrix4

*public void Matrix4()*

*Defined at [math/matrix4.cppm:21](../../../../math/matrix4.cppm#L21)*

### data

*public const float * data()*

*Defined at [math/matrix4.cppm:47](../../../../math/matrix4.cppm#L47)*

### orthographic

*public static Matrix4 orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane)*



**brief** Create an orthographic projection matrix.



**left** Near-plane left boundary in world units.

**right** Near-plane right boundary in world units.

**bottom** Top-plane bottom boundary in world units.

**top** Top-plane top boundary in world units.

**near_plane** Depth of the near plane.

**far_plane** Depth of the far plane.

**return** An orthographic projection, or the identity matrix when the bounds are degenerate.



