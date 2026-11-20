# The SDK's tools: one language for people, one for tools

**Status: decided 2026-11-20, draft 1.** Every tool of the squared SDK
(sqpg, square, sqmake, sqcart, sqdrive, apkpack, pkg, sqedit, and those to
come) follows this. No tool implements it yet; each takes it up as it is
next changed.

## The rule

| Who writes it | Language | Read with |
| --- | --- | --- |
| **People** | **Lua 5.4** | each tool's own sandboxed loader (below) |
| **Tools, for each other** | **JSON** | `sq.data` (`parse_json`, `write_json`) |

- **People write Lua**: `build.lua`, a project's or a workspace's settings,
  a template's or a kit's description. It reads as plainly as a settings
  format (`name = "sqterm"`, `deps = {"squared"}`), and where a file needs
  logic, the logic is there too. No second language for that.
- **Tools write JSON**: what one tool asks another and its answer (tool
  calls, below), cartridge manifests, package indexes, state a tool keeps
  between runs. Fixed shapes, no comments, one parser (`sq.data`), and
  JSON already where it is used today (sqcart 3.0's manifests, pkg's
  index).
- **No YAML**, and no third format. A file is one or the other, by who
  writes it.

## Lua that people write

- **Lua 5.4**, vendored (as sqmake does: `third_party/lua`, compiled as C++
  so an error unwinds), offline, on the phone as on a desktop.
- **Sandboxed.** A file runs in an environment of its own: the tool's API,
  and Lua's safe libraries only (`string`, `table`, `math`, `utf8`,
  `os.getenv`). No `io`, `os.execute`, `require` or `load`, and none of a
  host program's globals.
- **Settings files take values only.** A file that only describes (a
  workspace's settings, a kit's description) is loaded with no functions
  in reach at all, so it cannot do anything but say what it says. A build
  file (`build.lua`) gets its tool's API.
- **One name each**, `<what>.lua`: `build.lua` (sqmake), and the other
  tools' names decided as they come.

## Tools calling tools

A tool asks another for something the way an agent calls a tool: one JSON
request in, one JSON response out.

```sh
TOOL call < request.json > response.json   # one request, one response
TOOL describe                              # what it can be asked, as JSON
```

The request:

```json
{"sq": 1, "command": "workspace.inputs", "args": {"path": "game.sq"}}
```

The response, either:

```json
{"sq": 1, "ok": true, "result": {"sources": ["src/main.cpp"], "kits": ["sfml"]}}
```

or:

```json
{"sq": 1, "ok": false, "error": {"code": "not_a_workspace", "message": "game.sq has no consumers.square"}}
```

- **`sq`**: the protocol's version, 1. A tool answers a version it does not
  speak with `{"ok": false, "error": {"code": "version", ...}}`.
- **`command`**: `noun.verb` or a single word, the tool's own; `args` an
  object (absent or `{}` when there are none).
- **`ok`**: true with `result` (any JSON), or false with `error`: `code`
  (a short stable word, for programs) and `message` (a sentence, for
  people). The exit status agrees: 0 when `ok`, 1 when not, 2 when the
  request itself could not be read.
- **stdout carries the response alone.** Progress and logs go to stderr,
  so a caller reads one JSON document whatever else happened.
- **`describe`** prints the tool's name, version, the protocol versions it
  speaks, and each command with its arguments and result, each argument
  with its JSON type and whether it is required:

  ```json
  {"sq": 1, "tool": "square", "version": "0.1.0", "speaks": [1],
   "commands": {"workspace.inputs": {"summary": "what a workspace builds from",
                                     "args": {"path": {"type": "string", "required": true}},
                                     "result": {"type": "object"}}}}
  ```

  A caller, or an agent, learns what a tool offers by asking it.
- **In process, the same messages.** A tool's library takes a request and
  returns a response as `sq::data::JsonValue`, so one tool can call
  another's library directly, or run its program, and get the same answer
  either way. The program's `call` is a thin layer over the library's.
- **Dependencies go one way.** A tool calls only tools below it. sqmake is
  called by others and calls none of them (sqmake's spec §1); square calls
  sqmake and sqcart, never the reverse.

## Cartridges: each tool its own part of a manifest

A cartridge (sqcart 3.0) does not know what it is for. Its manifest's
`consumers` object has one member per tool that gives it a meaning, and
only that tool reads it:

```json
{"kind": "workspace",
 "consumers": {"square": {"template": "android", "kits": ["sfml"]},
               "sqterm": {"links": {"bin/cc": "clang"}}}}
```

- **A workspace is a cartridge that square knows is a workspace**:
  `consumers.square` says so, and sqcart keeps it without reading it, as it
  keeps `consumers.sqterm.links` for pkg.
- A tool never reads or changes another tool's member, and keeps the
  members it does not know as they are.

## Where it is used first

- **sqmake**: `build.lua` (Lua, done); `sqmake call` and `describe` (to
  come: build, explain, clean).
- **square**: workspaces as cartridges with `consumers.square`, and the
  calls that drive sqmake and sqcart.
- **pkg**, **sqcart**, **sqdrive**, **apkpack**: `call` and `describe`
  beside their command lines, as each is next changed.
