# The default skin

The files here are libGDX's test skin, "uiskin", from
[github.com/libgdx/libgdx](https://github.com/libgdx/libgdx) (its tests'
`data/` assets), under the Apache License, Version 2.0: a copy is
[`LICENSE-2.0.txt`](LICENSE-2.0.txt), beside this file. Its font,
`default.fnt` and its glyphs in `uiskin.png`, is Droid Sans, also under the
Apache License 2.0.

| Path | What |
|---|---|
| `skin/` | the skin as libGDX loads it: `uiskin.atlas`, its page `uiskin.png`, `uiskin.json`, `default.fnt` |
| `LICENSE-2.0.txt` | the Apache License, Version 2.0 |

Only the skin itself is here, not the images its atlas is packed from: those
are in libGDX's repository, beside it. `skin/` is compiled into the stage
library as `stage/src/default_skin.cpp`. After changing a file there, write that again
from the tree's root:

```sh
tools/embed_files.sh stage/skin/default/skin skins/default/skin \
    default_skin detail/default_skin.hpp stage/src/default_skin.cpp
```

`stage_test` fails while the two differ. Changes made here, from libGDX's
copy:

| File | Change |
|---|---|
| `skin/uiskin.json` | squared, 2026-10-27: a third window style, `pane`, the frame the Stage draws a pane in: `default-pane` (the skin's own pane background, as its scroll panes and tooltips use) and no title bar, since `default-window` has one painted in |
