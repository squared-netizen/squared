/*
 * The FreeType modules squared compiles (font/Makefile), registered for
 * FT_Init_FreeType: font/Makefile puts this directory first on the include
 * path, so <freetype/config/ftmodule.h> is this file, not FreeType's. What a terminal's
 * fonts need: TrueType (JetBrains Mono, Roboto), OpenType/CFF (Noto CJK),
 * the SFNT tables both share, PostScript hinting and names for CFF, the
 * auto-hinter, and the anti-aliased rasterizer. Squared's own file, not
 * FreeType's (third_party/freetype/ is pinned and never edited).
 */

FT_USE_MODULE( FT_Module_Class, autofit_module_class )
FT_USE_MODULE( FT_Driver_ClassRec, tt_driver_class )
FT_USE_MODULE( FT_Driver_ClassRec, cff_driver_class )
FT_USE_MODULE( FT_Module_Class, psaux_module_class )
FT_USE_MODULE( FT_Module_Class, psnames_module_class )
FT_USE_MODULE( FT_Module_Class, pshinter_module_class )
FT_USE_MODULE( FT_Module_Class, sfnt_module_class )
FT_USE_MODULE( FT_Renderer_Class, ft_smooth_renderer_class )
