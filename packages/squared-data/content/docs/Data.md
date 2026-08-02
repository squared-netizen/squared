---
title: {{PROJECT_TITLE}} Data
tags:
  - cpp
  - json
  - persistence
---

# Data and JSON

The independent Squared Data module provides the public native JSON API
`squared::data`, declared in `include/squared/data/json.hpp`. It owns parsed
values independently of its pinned yyjson backend, so application code does
not depend on parser-specific types.

## Strict defaults

`parse_json` accepts one RFC 8259 document. It rejects comments, trailing
commas, single quotes, byte-order marks, invalid UTF-8, non-finite numbers,
and duplicate keys. The default input limit is 8 MiB and the default nesting
limit is 128.

Duplicate keys may be allowed explicitly through `JsonParseOptions` for
compatibility imports. The last value then wins. Avoid that mode for new
persistence formats.

## Deterministic output

`write_json` emits object keys in bytewise order and preserves signed integer,
unsigned integer, and real-number categories. Compact output is the default.
Pretty output uses two spaces, with an optional final newline.

These guarantees make the API suitable for manifests, skins, editor
documents, and application persistence. Domain schemas, migrations, atomic
file replacement, and a Lua facade are separate layers and are not implied by
the low-level JSON parser.

## Offline dependency

The Squared Data `.sq` package contains the verified yyjson 0.12.0 source
under `third_party/yyjson-0.12.0/` and its MIT notice under `licenses/`.
Generated projects receive both through dependency composition. No network
resolution is required during the native build.

## Visible runtime diagnostic

The generated sample reads
`app/src/main/assets/diagnostics/json-ttf-status.json` through SDL, parses it
strictly, serializes it deterministically, parses that output again, and
renders the JSON-provided message with SDL_ttf. Optional `textColor` and
`panelColor` fields accept `#RRGGBB` or `#RRGGBBAA`.

The bundled DejaVu Sans Mono font makes the diagnostic completely offline.
Its license is copied to `licenses/DejaVu-Fonts-LICENSE.txt`. A colored panel
and structured log entry remain available if JSON, font loading, glyph
rendering, or texture upload fails.

The generated application also renders a multiline diagnostic report with
literal `PASS`, `FAIL`, and `PENDING` values. A compact JSON Telegram dump is
shown on screen and mirrored into the native diagnostic log.

The current candidate bundles DejaVu Sans Mono. A previously approved
JetBrains Mono Nerd Font can replace it once those font bytes are supplied to
the generator source; an Android application cannot read Termux's private
`~/.termux/font.ttf` directly.
