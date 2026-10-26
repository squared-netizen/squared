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
| `raw/` | the images the atlas is packed from |
| `preview.png`, `preview-large.png` | what it looks like |
| `LICENSE-2.0.txt` | the Apache License, Version 2.0 |

Only `skin/` is used: it is compiled into the stage library as
`stage/src/default_skin.cpp`. After changing a file there, write that again
from the tree's root:

```sh
tools/embed_files.sh stage/skin/default/skin skins/default/skin \
    default_skin detail/default_skin.hpp stage/src/default_skin.cpp
```

`stage_test` fails while the two differ. Unmodified from libGDX's copy, as
far as squared knows; a change made here is noted here.
