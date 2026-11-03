/*
 * FreeType's build options as squared compiles it: FreeType's own defaults
 * (its ftoption.h, pinned), then what squared turns off. font/Makefile puts
 * this directory on the include path before third_party/freetype/include,
 * so <freetype/config/ftoption.h> is this file, and FreeType's own is read
 * from here by its path in the tree. Squared's own file.
 *
 * Off, each for a reason:
 *   - compressed fonts (zlib, LZW: .pcf.gz and the like) and WOFF: a
 *     terminal's fonts are plain files, and zlib would be one more library
 *     to vendor;
 *   - Mac resource-fork fonts, OpenType-SVG (it needs an SVG renderer of
 *     the program's own), BDF tables inside TrueType;
 *   - incremental loading (for printers' font downloads);
 *   - FREETYPE_PROPERTIES from the environment: a program's look does not
 *     change with a variable set in a shell.
 */

#ifndef SQUARED_FTOPTION_H_
#define SQUARED_FTOPTION_H_

/* font/src/freetype/freetype/config/ -> squared/third_party/freetype/... */
#include "../../../../../third_party/freetype/include/freetype/config/ftoption.h"

#undef FT_CONFIG_OPTION_USE_LZW
#undef FT_CONFIG_OPTION_USE_ZLIB
#undef FT_CONFIG_OPTION_MAC_FONTS
#undef FT_CONFIG_OPTION_GUESSING_EMBEDDED_RFORK
#undef FT_CONFIG_OPTION_INCREMENTAL
#undef FT_CONFIG_OPTION_SVG
#undef FT_CONFIG_OPTION_ENVIRONMENT_PROPERTIES
#undef TT_CONFIG_OPTION_BDF

#endif /* SQUARED_FTOPTION_H_ */
