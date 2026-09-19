#include <squared/graphics2d/bitmap_font.hpp>

#include "detail/bitmap_font_detail.hpp"
#include <squared/graphics2d/bitmap_font_error.hpp>
#include <squared/graphics2d/bitmap_font_error_code.hpp>
#include <squared/graphics2d/bitmap_font_page.hpp>
#include <squared/graphics2d/bitmap_font_parse_limits.hpp>
#include <squared/graphics2d/bitmap_glyph.hpp>
#include <squared/graphics2d/bitmap_kerning.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace sq::graphics2d {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

std::uint64_t BitmapFont::kerning_key(char32_t first, char32_t second) noexcept
{
    return (static_cast<std::uint64_t>(first) << 32U) |
           static_cast<std::uint32_t>(second);
}

bool BitmapFont::load(
    std::string_view descriptor,
    BitmapFontError& error,
    const BitmapFontParseLimits& limits
) noexcept
{
    error = {};
    if (limits.maximum_bytes == 0 || limits.maximum_line_bytes == 0 ||
        limits.maximum_pages == 0 || limits.maximum_glyphs == 0 ||
        limits.maximum_kernings == 0 || limits.maximum_name_bytes == 0) {
        fail(error, BitmapFontErrorCode::invalid_argument, 0,
             "all parse limits must be positive");
        return false;
    }
    if (descriptor.size() > limits.maximum_bytes) {
        fail(error, BitmapFontErrorCode::limit_exceeded, 0,
             "descriptor exceeds maximum_bytes");
        return false;
    }

    try {
        BitmapFont candidate;
        bool saw_info = false;
        bool saw_common = false;
        bool saw_pages = false;
        bool saw_chars = false;
        bool saw_kernings = false;
        std::size_t declared_pages = 0;
        std::size_t declared_glyphs = 0;
        std::size_t declared_kernings = 0;

        std::size_t line_number = 0;
        std::size_t begin = 0;
        while (begin <= descriptor.size()) {
            const auto newline = descriptor.find('\n', begin);
            auto line = descriptor.substr(
                begin,
                newline == descriptor.npos ? descriptor.size() - begin
                                           : newline - begin
            );
            ++line_number;
            if (!line.empty() && line.back() == '\r') {
                line.remove_suffix(1);
            }
            if (line.size() > limits.maximum_line_bytes) {
                fail(error, BitmapFontErrorCode::limit_exceeded, line_number,
                     "line exceeds maximum_line_bytes");
                return false;
            }

            Fields fields;
            std::string_view tag;
            if (!parse_record(line, tag, fields, error, line_number)) {
                return false;
            }

            if (tag == "info") {
                if (saw_info) {
                    fail(error, BitmapFontErrorCode::duplicate, line_number,
                         "duplicate info record");
                    return false;
                }
                saw_info = true;
                const auto face = fields.find("face");
                if (face == fields.end() || face->second.empty()) {
                    fail(error, BitmapFontErrorCode::missing_required, line_number,
                         "missing font face");
                    return false;
                }
                if (face->second.size() > limits.maximum_name_bytes) {
                    fail(error, BitmapFontErrorCode::limit_exceeded, line_number,
                         "font face exceeds maximum_name_bytes");
                    return false;
                }
                candidate.info_.face = face->second;
                int bold = 0;
                int italic = 0;
                int unicode = 0;
                if (!integer_field(fields, "size", candidate.info_.size, error,
                                   line_number) ||
                    !integer_field(fields, "bold", bold, error, line_number) ||
                    !integer_field(fields, "italic", italic, error, line_number) ||
                    !integer_field(fields, "unicode", unicode, error,
                                   line_number)) {
                    return false;
                }
                candidate.info_.bold = bold != 0;
                candidate.info_.italic = italic != 0;
                candidate.info_.unicode = unicode != 0;
            } else if (tag == "common") {
                if (saw_common) {
                    fail(error, BitmapFontErrorCode::duplicate, line_number,
                         "duplicate common record");
                    return false;
                }
                saw_common = true;
                int pages = 0;
                if (!integer_field(fields, "lineHeight", candidate.line_height_,
                                   error, line_number) ||
                    !integer_field(fields, "base", candidate.baseline_, error,
                                   line_number) ||
                    !integer_field(fields, "scaleW", candidate.page_width_, error,
                                   line_number) ||
                    !integer_field(fields, "scaleH", candidate.page_height_, error,
                                   line_number) ||
                    !integer_field(fields, "pages", pages, error, line_number)) {
                    return false;
                }
                if (candidate.line_height_ <= 0 || candidate.baseline_ < 0 ||
                    candidate.page_width_ <= 0 || candidate.page_height_ <= 0 ||
                    pages <= 0) {
                    fail(error, BitmapFontErrorCode::malformed, line_number,
                         "common metrics must be positive");
                    return false;
                }
                declared_pages = static_cast<std::size_t>(pages);
                if (declared_pages > limits.maximum_pages) {
                    fail(error, BitmapFontErrorCode::limit_exceeded, line_number,
                         "page count exceeds maximum_pages");
                    return false;
                }
                candidate.pages_.reserve(declared_pages);
            } else if (tag == "page") {
                saw_pages = true;
                int id = 0;
                if (!integer_field(fields, "id", id, error, line_number)) {
                    return false;
                }
                const auto file = fields.find("file");
                if (id < 0 || file == fields.end() ||
                    file->second.size() > limits.maximum_name_bytes ||
                    !safe_page_path(file == fields.end() ? std::string_view{}
                                                         : file->second)) {
                    fail(error, BitmapFontErrorCode::malformed, line_number,
                         "page requires a nonnegative id and safe relative file");
                    return false;
                }
                if (candidate.pages_.size() == limits.maximum_pages) {
                    fail(error, BitmapFontErrorCode::limit_exceeded, line_number,
                         "page count exceeds maximum_pages");
                    return false;
                }
                const auto page_id = static_cast<std::size_t>(id);
                if (std::ranges::any_of(candidate.pages_, [page_id](const auto& page) {
                        return page.id == page_id;
                    })) {
                    fail(error, BitmapFontErrorCode::duplicate, line_number,
                         "duplicate page id");
                    return false;
                }
                candidate.pages_.push_back({page_id, std::string{file->second}});
            } else if (tag == "chars") {
                if (saw_chars) {
                    fail(error, BitmapFontErrorCode::duplicate, line_number,
                         "duplicate chars record");
                    return false;
                }
                saw_chars = true;
                if (!integer_field(fields, "count", declared_glyphs, error,
                                   line_number)) {
                    return false;
                }
                if (declared_glyphs == 0 ||
                    declared_glyphs > limits.maximum_glyphs) {
                    fail(error, BitmapFontErrorCode::limit_exceeded, line_number,
                         "glyph count is zero or exceeds maximum_glyphs");
                    return false;
                }
                candidate.glyphs_.reserve(declared_glyphs);
            } else if (tag == "char") {
                if (candidate.glyphs_.size() == limits.maximum_glyphs) {
                    fail(error, BitmapFontErrorCode::limit_exceeded, line_number,
                         "glyph count exceeds maximum_glyphs");
                    return false;
                }
                int id = 0;
                int page = 0;
                BitmapGlyph glyph;
                if (!integer_field(fields, "id", id, error, line_number) ||
                    !integer_field(fields, "x", glyph.x, error, line_number) ||
                    !integer_field(fields, "y", glyph.y, error, line_number) ||
                    !integer_field(fields, "width", glyph.width, error,
                                   line_number) ||
                    !integer_field(fields, "height", glyph.height, error,
                                   line_number) ||
                    !integer_field(fields, "xoffset", glyph.x_offset, error,
                                   line_number) ||
                    !integer_field(fields, "yoffset", glyph.y_offset, error,
                                   line_number) ||
                    !integer_field(fields, "xadvance", glyph.x_advance, error,
                                   line_number) ||
                    !integer_field(fields, "page", page, error, line_number)) {
                    return false;
                }
                glyph.codepoint = static_cast<char32_t>(id);
                if (id < 0 || !valid_scalar(glyph.codepoint) || page < 0 ||
                    glyph.x < 0 || glyph.y < 0 || glyph.width < 0 ||
                    glyph.height < 0 || glyph.x_advance < 0 ||
                    glyph.x > candidate.page_width_ - glyph.width ||
                    glyph.y > candidate.page_height_ - glyph.height) {
                    fail(error, BitmapFontErrorCode::malformed, line_number,
                         "invalid glyph scalar, page, or metrics");
                    return false;
                }
                glyph.page = static_cast<std::size_t>(page);
                if (std::ranges::any_of(candidate.glyphs_, [&glyph](const auto& item) {
                        return item.codepoint == glyph.codepoint;
                    })) {
                    fail(error, BitmapFontErrorCode::duplicate, line_number,
                         "duplicate glyph id");
                    return false;
                }
                candidate.glyphs_.push_back(glyph);
            } else if (tag == "kernings") {
                if (saw_kernings) {
                    fail(error, BitmapFontErrorCode::duplicate, line_number,
                         "duplicate kernings record");
                    return false;
                }
                saw_kernings = true;
                int count = 0;
                if (!integer_field(fields, "count", count, error, line_number)) {
                    return false;
                }
                // libGDX's Hiero exporter uses -1 as a no-kerning sentinel.
                if (count < -1) {
                    fail(error, BitmapFontErrorCode::malformed, line_number,
                         "kerning count must be nonnegative or libGDX -1");
                    return false;
                }
                declared_kernings = count < 0 ? 0 : static_cast<std::size_t>(count);
                if (declared_kernings > limits.maximum_kernings) {
                    fail(error, BitmapFontErrorCode::limit_exceeded, line_number,
                         "kerning count exceeds maximum_kernings");
                    return false;
                }
                candidate.kernings_.reserve(declared_kernings);
            } else if (tag == "kerning") {
                if (candidate.kernings_.size() == limits.maximum_kernings) {
                    fail(error, BitmapFontErrorCode::limit_exceeded, line_number,
                         "kerning count exceeds maximum_kernings");
                    return false;
                }
                int first = 0;
                int second = 0;
                BitmapKerning kerning;
                if (!integer_field(fields, "first", first, error, line_number) ||
                    !integer_field(fields, "second", second, error, line_number) ||
                    !integer_field(fields, "amount", kerning.amount, error,
                                   line_number)) {
                    return false;
                }
                kerning.first = static_cast<char32_t>(first);
                kerning.second = static_cast<char32_t>(second);
                if (first < 0 || second < 0 || !valid_scalar(kerning.first) ||
                    !valid_scalar(kerning.second)) {
                    fail(error, BitmapFontErrorCode::malformed, line_number,
                         "kerning pair contains an invalid scalar");
                    return false;
                }
                const auto key = kerning_key(kerning.first, kerning.second);
                if (std::ranges::any_of(candidate.kernings_, [key](const auto& item) {
                        return BitmapFont::kerning_key(item.first, item.second) == key;
                    })) {
                    fail(error, BitmapFontErrorCode::duplicate, line_number,
                         "duplicate kerning pair");
                    return false;
                }
                candidate.kernings_.push_back(kerning);
            } else if (!tag.empty()) {
                fail(error, BitmapFontErrorCode::unsupported, line_number,
                     "unsupported BMFont record: " + std::string{tag});
                return false;
            }

            if (newline == descriptor.npos) {
                break;
            }
            begin = newline + 1;
        }

        if (!saw_info || !saw_common || !saw_pages || !saw_chars) {
            fail(error, BitmapFontErrorCode::missing_required, 0,
                 "descriptor is missing required records");
            return false;
        }
        if (candidate.pages_.size() != declared_pages) {
            fail(error, BitmapFontErrorCode::malformed, 0,
                 "declared page count does not match descriptor contents");
            return false;
        }

        std::ranges::sort(candidate.pages_, {}, &BitmapFontPage::id);
        for (std::size_t index = 0; index < candidate.pages_.size(); ++index) {
            if (candidate.pages_[index].id != index) {
                fail(error, BitmapFontErrorCode::malformed, 0,
                     "page ids must be contiguous from zero");
                return false;
            }
        }
        for (const auto& glyph : candidate.glyphs_) {
            if (glyph.page >= candidate.pages_.size()) {
                fail(error, BitmapFontErrorCode::malformed, 0,
                     "glyph references an undeclared page");
                return false;
            }
        }

        std::ranges::sort(candidate.glyphs_, {}, &BitmapGlyph::codepoint);
        std::ranges::sort(candidate.kernings_, [](const auto& left, const auto& right) {
            return BitmapFont::kerning_key(left.first, left.second) <
                   BitmapFont::kerning_key(right.first, right.second);
        });
        for (std::size_t index = 0; index < candidate.glyphs_.size(); ++index) {
            candidate.glyph_indices_.emplace(candidate.glyphs_[index].codepoint,
                                             index);
        }
        for (const auto& kerning : candidate.kernings_) {
            candidate.kerning_amounts_.emplace(
                kerning_key(kerning.first, kerning.second), kerning.amount
            );
        }
        *this = std::move(candidate);
        return true;
    } catch (const std::bad_alloc&) {
        error.code = BitmapFontErrorCode::limit_exceeded;
        error.line = 0;
        error.message.clear();
        return false;
    } catch (...) {
        error.code = BitmapFontErrorCode::malformed;
        error.line = 0;
        error.message.clear();
        return false;
    }
}

void BitmapFont::clear() noexcept
{
    *this = BitmapFont{};
}

bool BitmapFont::valid() const noexcept
{
    return line_height_ > 0 && page_width_ > 0 && page_height_ > 0 &&
           !pages_.empty() && !glyphs_.empty();
}

const BitmapGlyph* BitmapFont::glyph(char32_t codepoint) const noexcept
{
    const auto found = glyph_indices_.find(codepoint);
    return found == glyph_indices_.end() ? nullptr : &glyphs_[found->second];
}

int BitmapFont::kerning(char32_t first, char32_t second) const noexcept
{
    const auto found = kerning_amounts_.find(kerning_key(first, second));
    return found == kerning_amounts_.end() ? 0 : found->second;
}

} // namespace sq::graphics2d
