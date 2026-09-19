# math

Small value types used across the framework.

Every type below lives in its own header. Include exactly the ones a
translation unit names; `squared/math/math.hpp` pulls in all of them
and exists for convenience, not for use inside headers of your own.

Developer counterpart: [../developer/math.md](../developer/math.md)

| Type | Header | Purpose |
|---|---|---|
| `Matrix4` | `squared/math/matrix4.hpp` | Column-major four-by-four matrix compatible with OpenGL ES |
| `Vector2` | `squared/math/vector2.hpp` | Two-dimensional floating-point vector |
