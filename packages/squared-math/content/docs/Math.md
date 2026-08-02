# Squared Math

Squared Math provides small platform-neutral C++20 primitives shared by
framework modules. It does not depend on SDL, OpenGL, Lua, JSON, Android, or
another Squared package.

## Vector2

`squared::math::Vector2` is a lightweight two-component floating-point value.
It is suitable for positions, dimensions, offsets, and other two-dimensional
quantities without imposing ownership or coordinate-system policy.

## Matrix4

`squared::math::Matrix4` stores a column-major four-by-four matrix compatible
with OpenGL ES. A default-constructed matrix is the identity matrix.

`Matrix4::orthographic` creates an orthographic projection from explicit
horizontal, vertical, near, and far bounds. Degenerate bounds return an
identity matrix rather than producing non-finite values.

Squared Math intentionally contains only generally reusable primitives.
Camera orientation, viewport policy, rendering, transforms, collision, and
game-specific geometry belong in higher-level modules.
