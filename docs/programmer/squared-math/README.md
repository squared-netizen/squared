# Squared Math — Programmer Guide

Squared Math provides compact, platform-neutral mathematical value types used
across the framework: a two-component floating-point vector and a
column-major 4x4 matrix compatible with OpenGL ES uploads. The package is
dependency-free, allocation-free, and intentionally small; camera orientation,
viewport policy, rendering, transforms, and game-specific geometry live in
higher-level modules.

## Package availability

| Module | Version | Requires |
| --- | --- | --- |
| `dev.squarednetizen.squared.math` | `0.6.0-dev.2` | (none) |

The CMake target is `squared_math`.

## Public API overview

| Type | Header | Purpose |
| --- | --- | --- |
| `squared::math::Vector2` | `squared/math/vector2.hpp` | Two-dimensional float vector. |
| `squared::math::Matrix4` | `squared/math/matrix4.hpp` | Column-major 4x4 matrix; identity by default. |

## Vector2

`Vector2` is a plain aggregate of two `float` components, `x` and `y`, each
defaulting to `0.0F`. It is suitable for positions, dimensions, offsets, and
other two-dimensional quantities. There is no ownership, coordinate-system
policy, or hidden state.

```cpp
#include <squared/math/vector2.hpp>

squared::math::Vector2 position{3.0F, -2.0F};
const float x = position.x;  // 3.0F
const float y = position.y;  // -2.0F
```

`Vector2` is a value type: copy it freely and aggregate-initialize it
directly. Component-wise arithmetic and normalization are not part of this
package; keep only the primitive here and put policy in the consumer.

## Matrix4

`Matrix4` stores a column-major 4x4 matrix as 16 contiguous `float` values.
A default-constructed matrix is the identity. `data()` returns a pointer to
the 16 values in column-major order, laid out for direct upload to an OpenGL
uniform without a transpose.

The header documents the transform convention: vectors are treated as columns,
so a transform applies as `M * v`. The package currently exposes construction
and raw data access only — there is no public multiplication or vector-transform
operator. Consumers that need a composed view-projection matrix obtain it from
`OrthographicCamera` in Squared Graphics2D, which folds view and projection
into a single `Matrix4::orthographic` matrix (see
[the math developer page](../developer/squared-math/README.md) for the
composition detail).

```cpp
#include <squared/math/matrix4.hpp>

const squared::math::Matrix4 identity;  // identity matrix

const auto projection = squared::math::Matrix4::orthographic(
    0.0F, 200.0F, 100.0F, 0.0F
);
const float* data = projection.data();  // 16 column-major floats
```

### Orthographic projection

`Matrix4::orthographic(left, right, bottom, top, near_plane = -1.0F,
far_plane = 1.0F)` builds an orthographic projection in world units. The
constructor produces the identity matrix when any dimension is degenerate
(a width, height, or depth with absolute value below `1e-6`), so equal left
and right bounds never produce non-finite values.

```cpp
#include <squared/math/matrix4.hpp>

// Screen-space projection with a top-left origin.
const auto projection = squared::math::Matrix4::orthographic(
    0.0F, 960.0F, 540.0F, 0.0F
);

// Degenerate horizontal bounds: an identity matrix, not a divide-by-zero.
const auto degenerate = squared::math::Matrix4::orthographic(
    1.0F, 1.0F, 0.0F, 100.0F
);
```

`data()` remains valid for the lifetime of the `Matrix4` it belongs to; the
returned pointer refers to the object's own storage.

## Errors and failure behavior

Neither type reports errors. There is no exception channel, and all operations
are `noexcept`. The only defined failure mode is degenerate projection input,
which returns the identity matrix rather than a matrix containing non-finite
values. No operation allocates, so there is no allocation failure.

## Ownership and lifetime

Both types are value types with trivial copy and move semantics; there are no
pointers, no owned resources, and nothing to release. `data()` borrows the
object's internal storage and is invalidated when the `Matrix4` is destroyed
or overwritten.

## Threading

The types keep no global state and mutate no shared resources. Independent
values can be constructed and read from different threads; concurrent
modification of a single shared `Matrix4` must be externally synchronized.

## Lua bindings

None of the types in this package have a Lua 5.4 binding.

## Related documentation

- Package payload: [Math.md](../../../packages/squared-math/content/docs/Math.md)
- Implementation details: [Squared Math — Developer Guide](../developer/squared-math/README.md)
- Consumer documentation: [Squared Graphics2D — Programmer Guide](../squared-graphics2d/README.md)
- Documentation index: [Programmer documentation](../README.md)
