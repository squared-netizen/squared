# class Matrix4

*Defined at [math/matrix4.hpp:17](../../../../math/matrix4.hpp#L17)*



**brief** Column-major four-by-four matrix compatible with OpenGL ES.

 Storage is column-major with `values_` laid out for direct upload to an OpenGL uniform. Vectors are treated as columns, so a transform applies as `M * v`. Handedness is inherited from the consuming camera and projection conventions rather than imposed here. All operations are value types and never allocate.



## Functions

### Matrix4

*public void Matrix4()*

*Defined at [math/matrix4.hpp:21](../../../../math/matrix4.hpp#L21)*



**brief** Construct an identity matrix. 

### data

*public const float * data()*

*Defined at [math/matrix4.hpp:47](../../../../math/matrix4.hpp#L47)*



**brief** Read the contiguous OpenGL-compatible matrix data.

**return** Pointer to 16 column-major values; valid for this object's lifetime.

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



