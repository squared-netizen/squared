# Squared Math — Developer Guide

Squared Math is the dependency-free numerical primitive layer of the
framework. It ships two value types — `Vector2` and `Matrix4` — built around
a single implementation translation unit. The module's role is deliberately
minimal: it stores and projects; policy such as camera orientation, viewport
conventions, and transform composition belongs to consumers such as
Squared Graphics2D.

- Programmer counterpart: [Squared Math — Programmer Guide](../programmer/squared-math/README.md)
- Package payload: [Math.md](../../packages/squared-math/content/docs/Math.md)
- Documentation index: [Developer documentation](../README.md)
- Diagrams:
  - [Matrix4 transform composition and Graphics2D consumers](matrix4-composition.dot)
  - [Framework package dependency graph](../architecture/package-dependencies.dot)

## Dependency boundary

The manifest declares `module.requires` as `[]`; the package requires no
other Squared module and no third-party library. The CMake target
`squared_math` is a `STATIC` library compiling `src/matrix4.cpp` with only
`cxx_std_20` and the `include/` directory. `vector2.hpp` includes nothing
beyond its own declarations, and `matrix4.hpp` includes only `<array>`. There
is no SDL, OpenGL, Lua, JSON, or Android coupling.

The package is consumed by Squared Graphics2D, whose manifest requires
`dev.squarednetizen.squared.math` exactly at `0.6.0-dev.2`;
`orthographic_camera.hpp` includes both math headers. No other Squared module
depends on math at this milestone.

## Architecture

| Component | Source | Responsibility |
| --- | --- | --- |
| `squared::math::Vector2` | `include/squared/math/vector2.hpp` | Two-float value aggregate. |
| `squared::math::Matrix4` | `include/squared/math/matrix4.hpp` | Column-major identity matrix and orthographic projection. |
| `Matrix4::orthographic` | `src/matrix4.cpp` | Projection construction with degenerate-bounds guard. |

## Ownership and threading

Both types are immutable-by-convention value types: plain data members with
no heap, no pointers, no shared state, and no resource to release. Copy and
move are trivial. `Matrix4::data()` returns a pointer into the object's own
`std::array<float, 16>`, valid until the object is destroyed or overwritten.

The package keeps no global state and is reentrant. Independent values may be
used from any thread; a single value mutated concurrently needs external
synchronization. Nothing here starts a thread or performs I/O.

## Invariants and failure behavior

- A default `Matrix4` is exactly the identity: ones on the main diagonal
  (`values_[0]`, `values_[5]`, `values_[10]`, `values_[15]`), zeros elsewhere.
- `orthographic` computes `width = right - left`, `height = top - bottom`,
  and `depth = far_plane - near_plane`. If any of these three has absolute
  value below `1e-6`, the function returns a fresh identity matrix, so
  degenerate input can never yield division by zero or non-finite output.
- The projection maps `left` to `-1` and `right` to `+1` on the x axis
  (scale `2/width`, translate `-(right+left)/width`), `bottom`/`top` to
  `-1`/`+1` on the y axis, and near/far to `-1`/`+1` on z (scale `-2/depth`).
  The last column is homogeneous `(0, 0, 0, 1)`.
- All member functions are `noexcept`; there is no exception or error channel.

## Data structures and complexity

- `Vector2` is two `float` members: O(1) size and O(1) access.
- `Matrix4` is one `std::array<float, 16>`. `orthographic` writes the fixed
  16-element projection pattern; `data()` is O(1). There is no dynamic
  allocation anywhere, so all operations are constant-time.
- Column-major layout (`values_[col * 4 + row]`) is chosen for direct
  `glUniformMatrix4fv` upload with `GL_FALSE` transpose, matching the GL
  convention and avoiding a transposed copy.

## Algorithms and execution order

`Matrix4::orthographic`:

1. Compute `width`, `height`, and `depth` from the bounds.
2. Guard: if any absolute value is below `1e-6`, return the identity
   `Matrix4{}`.
3. Otherwise build the projection with scale terms `2/width`, `2/height`,
   `-2/depth` and translation terms `-(right+left)/width`,
   `-(top+bottom)/height`, `-(far_plane+near_plane)/depth`.

The single consumer, `OrthographicCamera::update()`, composes the view and
projection into one matrix by computing camera-relative bounds and calling
`Matrix4::orthographic` once. For a `TopLeft` origin the vertical bounds are
reversed (`position.y + half_height` to `position.y - half_height`), which
flips the y axis into screen space. There is no literal matrix multiplication;
the composed result is a single projection built from combined bounds. This
is why the package ships no public multiplication operator yet — the current
consumer needs only one orthographic construction per update.

## Design patterns

- **Value Object (immutable value)** — both types are plain, copyable,
  constexpr-friendly values with no identity; `Matrix4` exposes no mutators
  after construction (the private `values` array and private constructor
  prevent external mutation of the layout). This fits the framework's
  value-oriented, allocation-free math layer.
- **Static factory** — `Matrix4::orthographic` is a named constructor that
  encapsulates the projection recipe and the degenerate-bounds guard, keeping
  the constructor surface minimal.
- **Provider/consumer composition (view-projection folding)** — the Graphics2D
  camera composes transforms at the policy layer rather than through a
  generic matrix-multiply API; the math layer stays primitive.

Alternatives rejected: a full vector/matrix algebra library with
multiplication, inversion, and quaternions (rejected — `TODO.md` defers all
geometry primitives until concrete framework consumers exist); a dynamic
matrix type with heap storage (rejected — fixed size keeps operations
allocation-free and cache-local); exposing a mutable element accessor
(rejected — the current consumers need only read-only `data()`).

## Limitations and technical debt

- **No public matrix composition or transform application**: the `M * v`
  convention is documented in the header, but no multiplication, inversion,
  or vector-transform operator exists. `TODO.md` records explicitly
  documenting the coordinate and multiplication conventions as next work.
- **Focused numerical tests are pending**: `TODO.md` lists singular, boundary,
  and floating-point tolerance cases as unfinished; the current test suite
  covers identity, a representative orthographic projection, and the
  degenerate-bounds path.
- The degenerate guard uses a fixed `1e-6` epsilon rather than a
  scale-dependent tolerance, which is a simplification for the single
  projection consumer.
- No geometry primitives beyond `Vector2` and `Matrix4` exist; any addition
  must be justified by a concrete framework consumer.
