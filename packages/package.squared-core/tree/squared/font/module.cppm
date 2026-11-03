// Every public type in sq::font: font faces read and rendered at run time,
// by FreeType (Sprint 12.3, sqterm's terminal; third_party/freetype/).

export module sq.font;

export import :face;
export import :face_error;
export import :font_metrics;
export import :glyph_bitmap;
export import :glyph_style;
