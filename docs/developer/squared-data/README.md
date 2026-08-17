# Squared Data — Developer Guide

Squared Data is the strict deterministic JSON service of the framework. It
exposes owned values, strict RFC 8259 parsing with configurable resource
limits, and deterministic serialization through one public header, while all
parser work is delegated to a pinned private copy of yyjson 0.12.0. The
third-party boundary is deliberately narrow: yyjson types appear only inside
`src/json.cpp` and the vendored build target, never in public declarations.

- Programmer counterpart: [Squared Data — Programmer Guide](../programmer/squared-data/README.md)
- Package payload: [Data.md](../../packages/squared-data/content/docs/Data.md)
- Documentation index: [Developer documentation](../README.md)
- Diagrams:
  - [JSON parse ownership and error flow](json-parse-error-flow.dot)
  - [JSON serialization flow](json-serialization-flow.dot)

## Dependency boundary

The manifest declares `module.requires` as `[]`: no other Squared module and
no third-party package requirement. The CMake target `squared_data` is a
`STATIC` library that links a private `squared_data_yyjson` target compiled
from `content/third_party/yyjson-0.12.0`; `yyjson` sources ship inside the
package, so the native build is offline and repeatable.

The public header `include/squared/data/json.hpp` includes only C++ standard
headers (`<map>`, `<variant>`, `<vector>`, `<string>`, `<string_view>`,
`<cstdint>`, `<cstddef>`). `src/json.cpp` is the only translation unit that
includes `yyjson.h`. The Doxyfile excludes `yyjson_*` symbols from generated
documentation, keeping the parser out of the documented public surface.

## Architecture

| Component | Source | Responsibility |
| --- | --- | --- |
| `squared::data::JsonValue` | `include/squared/data/json.hpp` | Owned value over a `std::variant` discriminated union. |
| `JsonParseOptions`, `JsonWriteOptions` | `include/squared/data/json.hpp` | Per-call resource limits and formatting policy. |
| `JsonError`, `JsonErrorCode`, result types | `include/squared/data/json.hpp` | Stable structured failure and success channels. |
| `parse_json` | `src/json.cpp` | Strict parse via yyjson plus recursive conversion into `JsonValue`. |
| `write_json` | `src/json.cpp` | Deterministic build and serialize via yyjson mutable documents. |
| yyjson 0.12.0 | `content/third_party/yyjson-0.12.0/` | Vendored parser/serializer; private build dependency. |

## Ownership and threading

- `JsonValue` owns its storage entirely: `Storage` is a
  `std::variant<std::nullptr_t, bool, std::int64_t, std::uint64_t, double,
  std::string, Array, Object>` with a `nullptr` default. Copying a `JsonValue`
  deep-copies the contained value; there is no shared or borrowed payload.
- `Object` is `std::map<std::string, JsonValue, std::less<>>`, so lookups and
  iteration use bytewise UTF-8 key ordering and `find(std::string_view)` does
  not construct a temporary key.
- Parse and write manage yyjson documents with RAII: `yyjson_doc_free` runs
  after conversion, and `yyjson_mut_doc_free` runs after serialization even on
  the error paths. The serialized buffer returned by
  `yyjson_mut_write_opts` is copied into `std::string` and released with
  `std::free`.
- All allocations during conversion are funneled through standard containers;
  `std::bad_alloc` is caught and mapped to `JsonErrorCode::AllocationFailure`.
- The functions keep no global mutable state and are reentrant. Concurrent
  use of one mutable `JsonValue` needs external synchronization; independent
  values are safe.

## Invariants and failure behavior

- Determinism: object keys serialize in bytewise UTF-8 order; signed,
  unsigned, and real number categories are preserved end to end; repeated
  parse/write round trips are byte-stable.
- Strictness: trailing garbage, comments, trailing commas, single-quoted
  strings, byte-order marks, invalid UTF-8, and non-finite numbers are
  rejected at parse; duplicate keys are rejected unless explicitly allowed,
  in which case the last value wins.
- Limits: input is checked against `maximum_bytes` (default 8 MiB) before
  parsing, and conversion enforces `maximum_depth` (default 128) recursively.
- `JsonParseResult::value` is meaningful only when `error` is empty; the
  same holds for `JsonWriteResult::text`.
- Writing refuses non-finite `double` values (`NonFiniteNumber`) and invalid
  UTF-8 strings; the conversion step reports the first offending member.
- Parse and write never throw; every failure is a returned `JsonError`.

## Data structures and complexity

- `JsonValue::type()` is `static_cast<Type>(storage_.index())`, O(1).
- `Object` is a balanced tree with bytewise-ordered keys: member lookup and
  insertion are O(log n), and iteration is in key order. `find` uses the
  transparent comparator to look up a `std::string_view` without allocation.
- Duplicate-key detection during conversion uses a per-object
  `std::unordered_set<std::string>`, so each object pays O(m) space for m
  keys during parse; the set is released with the conversion frame.
- `parse_json` is O(n) in input size (yyjson read plus one traversal);
  `write_json` is O(total output size) plus the build pass.
- Results and errors are small value types; `JsonError` owns its message.

## Algorithms and execution order

`parse_json`:

1. Reject input larger than `maximum_bytes` with `InputTooLarge` before
   touching yyjson.
2. `yyjson_read_opts` with `YYJSON_READ_NOFLAG` (strict RFC 8259; comments,
   trailing commas, single quotes, BOMs, invalid UTF-8, and non-finite numbers
   fail here).
3. On failure, map the yyjson read error to `JsonErrorCode::Syntax` with the
   zero-based `pos` as `byte_offset`; `yyjson_locate_pos` computes one-based
   `line` and `column`.
4. On success, `convert_value` walks the document recursively starting at
   depth 1, mapping yyjson categories to `JsonValue` categories (`uint` ->
   `UnsignedInteger`, `sint` -> `SignedInteger`, `real` -> `Real` with a
   finiteness check). Objects are converted key by key: keys are tracked in an
   `unordered_set`, and a repeat key is rejected with `DuplicateKey` when
   configured. Array/object conversion is depth-guarded before descent.
5. `yyjson_doc_free(document)` runs before returning in every path.

`write_json`:

1. `yyjson_mut_doc_new` allocates a mutable document.
2. `convert_for_writing` mirrors the `JsonValue` tree into yyjson mutable
   values; a non-finite `Real` fails with `NonFiniteNumber`.
3. `yyjson_mut_write_opts` serializes with `YYJSON_WRITE_NOFLAG`, adding
   `YYJSON_WRITE_PRETTY_TWO_SPACES` and/or `YYJSON_WRITE_NEWLINE_AT_END` from
   the options. The yyjson write error maps to `NonFiniteNumber` for
   `YYJSON_WRITE_ERROR_NAN_OR_INF`, otherwise to `InvalidValue`.
4. The output is copied into `JsonWriteResult::text` and the buffer freed.

## Design patterns

- **Wrapper/Adapter facade over yyjson** — every public type and function is a
  Squared type; yyjson structures never cross the module boundary. This pins
  the third-party dependency and keeps the public surface stable.
- **Value Object** — `JsonValue` is an immutable-access owned value over a
  discriminated union; copying transfers no aliasing and no lifetime coupling,
  which matches how payloads are passed between framework modules.
- **Resource Acquisition Is Initialization (RAII)** — yyjson document
  lifetimes are managed by explicit free calls on every path (including
  exceptions), and the vendored library is compiled behind its own target so
  allocation policy stays internal.
- **Strategy via options structs** — `JsonParseOptions` and `JsonWriteOptions`
  parameterize strictness and formatting without branching APIs.

Alternatives rejected: exposing yyjson types directly (rejected — couples
callers to a pinned parser version and leaks backend detail); a streaming
reader (rejected for now — `TODO.md` evaluates it for large assets later);
a callback-free lenient parse mode (rejected — strictness is the module's
contract, with an explicit opt-in for duplicate keys only).

## Limitations and technical debt

- The default 8 MiB input limit and 128 depth are fixed defaults, not
  environment-wide policy; `TODO.md` records configurable resource limits for
  untrusted data as unfinished work.
- No typed conversion helpers exist: callers must combine `find` with
  `*_if()` and map missing-key/type mismatches themselves. `TODO.md` tracks
  typed helpers with explicit range and missing-field errors.
- Parsing duplicates keys into an auxiliary set per object, so very large
  objects pay temporary space; allocation failure is reported, not retried.
- Streaming JSON input is not yet supported; whole documents are parsed.
- The pinned yyjson 0.12.0 version is a deliberate frozen dependency; an
  upgrade requires re-verification of strictness and determinism behavior.
