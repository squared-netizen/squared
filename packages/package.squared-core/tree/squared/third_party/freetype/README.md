# FreeType, pinned

The font module (`font/`, Sprint 12.3) renders glyphs with FreeType, from
these files: the public headers and the sources of the modules squared
builds, copied from a FreeType release by `tools/pin_freetype.sh` and
committed, so a checkout builds for the host and for Android with nothing
installed.

- **Which release.** FreeType's latest stable release, by the rule sqterm's
  tools follow: 2.15.0 (2026-10-12) when first pinned. `PINNED` says
  where from (the git tag and commit), the version, and a sha256 for each
  file.
- **What is taken.** `include/`, and `src/` of base, truetype, sfnt,
  smooth, autofit, cff, psaux, pshinter, psnames: TrueType and
  OpenType/CFF outlines, the anti-aliased rasterizer, the auto-hinter. The
  modules compiled and the options they are compiled with are squared's own
  files, `font/src/freetype/freetype/config/` (`ftmodule.h`, `ftoption.h`),
  first on the include path; nothing here is edited.
- **Checked.** `make headers` checks the sums.
- **Pinning again**, for a new release:

      git clone --depth 1 --branch VER-2-16-0 https://github.com/freetype/freetype ~/src/freetype
      tools/pin_freetype.sh ~/src/freetype

  then build, test, and commit `third_party/freetype/` with `PINNED`.

License: the FreeType License (`FTL.TXT`; FreeType is dual-licensed, see
`LICENSE.TXT`, and squared takes the FTL). The FTL asks that a program built
with it says so in its documentation: "Portions of this software are
copyright © The FreeType Project (https://freetype.org). All rights
reserved." An app that links `sq.font` (sqterm) carries that line.
