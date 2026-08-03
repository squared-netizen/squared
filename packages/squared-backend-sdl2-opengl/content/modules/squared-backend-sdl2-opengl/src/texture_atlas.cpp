#include <squared/graphics2d/texture_atlas.hpp>

#include <SDL.h>
#include <SDL_image.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace squared::graphics2d {
namespace {

struct ParsedRegion {
    std::string name;
    int index{-1};
    int x{-1};
    int y{-1};
    int packed_width{-1};
    int packed_height{-1};
    int original_width{-1};
    int original_height{-1};
    int offset_x{0};
    int offset_y{0};
    bool rotated{false};
    std::optional<std::array<int, 4>> splits;
    std::optional<std::array<int, 4>> pads;
};

struct ParsedPage {
    std::string image;
    TextureFilter min_filter{TextureFilter::Nearest};
    TextureFilter mag_filter{TextureFilter::Nearest};
    TextureWrap horizontal_wrap{TextureWrap::ClampToEdge};
    TextureWrap vertical_wrap{TextureWrap::ClampToEdge};
    std::vector<ParsedRegion> regions;
};

std::string_view trim(std::string_view value) noexcept
{
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.front()))) {
        value.remove_prefix(1);
    }
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.back()))) {
        value.remove_suffix(1);
    }
    return value;
}

bool parse_integer(std::string_view value, int& result) noexcept
{
    value = trim(value);
    if (value.empty()) return false;
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto conversion = std::from_chars(begin, end, result);
    return conversion.ec == std::errc{} && conversion.ptr == end;
}

template <std::size_t Size>
bool parse_integer_list(
    std::string_view value,
    std::array<int, Size>& result
) noexcept
{
    for (std::size_t index = 0; index < Size; ++index) {
        const std::size_t comma = value.find(',');
        if (index + 1 < Size && comma == std::string_view::npos) return false;
        if (index + 1 == Size && comma != std::string_view::npos) return false;
        const std::string_view item = comma == std::string_view::npos
            ? value
            : value.substr(0, comma);
        if (!parse_integer(item, result[index])) return false;
        if (comma != std::string_view::npos) value.remove_prefix(comma + 1);
    }
    return true;
}

bool parse_filter(std::string_view value, TextureFilter& result) noexcept
{
    value = trim(value);
    if (value == "Nearest") {
        result = TextureFilter::Nearest;
        return true;
    }
    if (value == "Linear") {
        result = TextureFilter::Linear;
        return true;
    }
    return false;
}

bool parse_filters(
    std::string_view value,
    TextureFilter& minification,
    TextureFilter& magnification
) noexcept
{
    const std::size_t comma = value.find(',');
    if (comma == std::string_view::npos) return false;
    return parse_filter(value.substr(0, comma), minification) &&
        parse_filter(value.substr(comma + 1), magnification);
}

bool parse_repeat(
    std::string_view value,
    TextureWrap& horizontal,
    TextureWrap& vertical
) noexcept
{
    value = trim(value);
    horizontal = TextureWrap::ClampToEdge;
    vertical = TextureWrap::ClampToEdge;
    if (value == "none") return true;
    if (value == "x") {
        horizontal = TextureWrap::Repeat;
        return true;
    }
    if (value == "y") {
        vertical = TextureWrap::Repeat;
        return true;
    }
    if (value == "xy") {
        horizontal = TextureWrap::Repeat;
        vertical = TextureWrap::Repeat;
        return true;
    }
    return false;
}

bool safe_relative_path(std::string_view path) noexcept
{
    if (path.empty() || path.front() == '/' || path.front() == '\\') {
        return false;
    }
    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t slash = path.find_first_of("/\\", start);
        const std::string_view segment = path.substr(
            start,
            slash == std::string_view::npos
                ? path.size() - start
                : slash - start
        );
        if (segment == "..") return false;
        if (slash == std::string_view::npos) break;
        start = slash + 1;
    }
    return true;
}

std::string parent_path(std::string_view path)
{
    const std::size_t slash = path.find_last_of("/\\");
    return slash == std::string_view::npos
        ? std::string{}
        : std::string(path.substr(0, slash + 1));
}

bool read_asset(const char* path, std::string& output) noexcept
{
    SDL_RWops* stream = SDL_RWFromFile(path, "rb");
    if (!stream) {
        SDL_Log("TextureAtlas cannot open %s: %s", path, SDL_GetError());
        return false;
    }
    const Sint64 size = SDL_RWsize(stream);
    if (size < 0) {
        SDL_Log("TextureAtlas cannot size %s: %s", path, SDL_GetError());
        SDL_RWclose(stream);
        return false;
    }
    try {
        output.resize(static_cast<std::size_t>(size));
    } catch (...) {
        SDL_Log("TextureAtlas allocation failed for %s", path);
        SDL_RWclose(stream);
        return false;
    }
    const std::size_t read = SDL_RWread(
        stream,
        output.data(),
        1,
        output.size()
    );
    SDL_RWclose(stream);
    if (read != output.size()) {
        SDL_Log("TextureAtlas short read for %s", path);
        output.clear();
        return false;
    }
    return true;
}

bool report_parse_error(
    const char* path,
    std::size_t line,
    const char* message
) noexcept
{
    SDL_SetError(
        "TextureAtlas parse error at line %zu: %s",
        line,
        message
    );
    SDL_Log(
        "TextureAtlas parse error in %s at line %zu: %s",
        path,
        line,
        message
    );
    return false;
}

bool parse_atlas(
    const char* path,
    std::string_view input,
    std::vector<ParsedPage>& pages
) noexcept
{
    ParsedPage* page = nullptr;
    ParsedRegion* region = nullptr;
    std::size_t line_number = 0;
    try {
        while (!input.empty()) {
            ++line_number;
            const std::size_t newline = input.find('\n');
            std::string_view line = input.substr(0, newline);
            if (newline == std::string_view::npos) {
                input = {};
            } else {
                input.remove_prefix(newline + 1);
            }
            line = trim(line);
            if (line.empty()) {
                page = nullptr;
                region = nullptr;
                continue;
            }

            const std::size_t colon = line.find(':');
            if (colon == std::string_view::npos) {
                if (!page) {
                    if (!safe_relative_path(line)) {
                        return report_parse_error(
                            path,
                            line_number,
                            "page image must be a safe relative path"
                        );
                    }
                    pages.push_back(ParsedPage{});
                    page = &pages.back();
                    page->image.assign(line);
                    region = nullptr;
                } else {
                    page->regions.push_back(ParsedRegion{});
                    region = &page->regions.back();
                    region->name.assign(line);
                }
                continue;
            }

            if (!page) {
                return report_parse_error(
                    path,
                    line_number,
                    "property appears before a page image"
                );
            }
            const std::string_view key = trim(line.substr(0, colon));
            const std::string_view value = trim(line.substr(colon + 1));
            if (!region) {
                if (key == "filter" &&
                    !parse_filters(
                        value,
                        page->min_filter,
                        page->mag_filter
                    )) {
                    return report_parse_error(
                        path,
                        line_number,
                        "only Nearest and Linear filters are supported"
                    );
                }
                if (key == "repeat" &&
                    !parse_repeat(
                        value,
                        page->horizontal_wrap,
                        page->vertical_wrap
                    )) {
                    return report_parse_error(
                        path,
                        line_number,
                        "repeat must be none, x, y, or xy"
                    );
                }
                continue;
            }

            if (key == "rotate") {
                region->rotated =
                    value == "true" || value == "90";
                if (!region->rotated && value != "false" && value != "0") {
                    return report_parse_error(
                        path,
                        line_number,
                        "rotate must be false, true, 0, or 90"
                    );
                }
            } else if (key == "xy") {
                std::array<int, 2> values{};
                if (!parse_integer_list(value, values)) {
                    return report_parse_error(
                        path,
                        line_number,
                        "xy requires two integers"
                    );
                }
                region->x = values[0];
                region->y = values[1];
            } else if (key == "size") {
                std::array<int, 2> values{};
                if (!parse_integer_list(value, values)) {
                    return report_parse_error(
                        path,
                        line_number,
                        "size requires two integers"
                    );
                }
                region->packed_width = values[0];
                region->packed_height = values[1];
            } else if (key == "bounds") {
                std::array<int, 4> values{};
                if (!parse_integer_list(value, values)) {
                    return report_parse_error(
                        path,
                        line_number,
                        "bounds requires four integers"
                    );
                }
                region->x = values[0];
                region->y = values[1];
                region->packed_width = values[2];
                region->packed_height = values[3];
            } else if (key == "orig") {
                std::array<int, 2> values{};
                if (!parse_integer_list(value, values)) {
                    return report_parse_error(
                        path,
                        line_number,
                        "orig requires two integers"
                    );
                }
                region->original_width = values[0];
                region->original_height = values[1];
            } else if (key == "offset") {
                std::array<int, 2> values{};
                if (!parse_integer_list(value, values)) {
                    return report_parse_error(
                        path,
                        line_number,
                        "offset requires two integers"
                    );
                }
                region->offset_x = values[0];
                region->offset_y = values[1];
            } else if (key == "offsets") {
                std::array<int, 4> values{};
                if (!parse_integer_list(value, values)) {
                    return report_parse_error(
                        path,
                        line_number,
                        "offsets requires four integers"
                    );
                }
                region->offset_x = values[0];
                region->offset_y = values[1];
                region->original_width = values[2];
                region->original_height = values[3];
            } else if (key == "index") {
                if (!parse_integer(value, region->index)) {
                    return report_parse_error(
                        path,
                        line_number,
                        "index requires one integer"
                    );
                }
            } else if (key == "split" || key == "pad") {
                std::array<int, 4> values{};
                if (!parse_integer_list(value, values)) {
                    return report_parse_error(
                        path,
                        line_number,
                        "split and pad require four integers"
                    );
                }
                if (key == "split") {
                    region->splits = values;
                } else {
                    region->pads = values;
                }
            }
        }
    } catch (...) {
        SDL_Log("TextureAtlas allocation failed while parsing %s", path);
        return false;
    }

    if (pages.empty()) {
        return report_parse_error(path, line_number, "atlas has no pages");
    }
    for (const ParsedPage& parsed_page : pages) {
        if (parsed_page.regions.empty()) {
            return report_parse_error(
                path,
                line_number,
                "every page must contain at least one region"
            );
        }
        for (const ParsedRegion& parsed_region : parsed_page.regions) {
            if (parsed_region.x < 0 || parsed_region.y < 0 ||
                parsed_region.packed_width <= 0 ||
                parsed_region.packed_height <= 0) {
                return report_parse_error(
                    path,
                    line_number,
                    "every region requires non-negative bounds"
                );
            }
        }
    }
    return true;
}

}  // namespace

const std::string& AtlasRegion::name() const noexcept
{
    return name_;
}

int AtlasRegion::index() const noexcept
{
    return index_;
}

const TextureRegion& AtlasRegion::region() const noexcept
{
    return region_;
}

int AtlasRegion::packed_width() const noexcept
{
    return packed_width_;
}

int AtlasRegion::packed_height() const noexcept
{
    return packed_height_;
}

bool AtlasRegion::rotated_clockwise() const noexcept
{
    return region_.rotated_clockwise();
}

int AtlasRegion::original_width() const noexcept
{
    return original_width_;
}

int AtlasRegion::original_height() const noexcept
{
    return original_height_;
}

int AtlasRegion::offset_x() const noexcept
{
    return offset_x_;
}

int AtlasRegion::offset_y() const noexcept
{
    return offset_y_;
}

const std::optional<std::array<int, 4>>&
AtlasRegion::splits() const noexcept
{
    return splits_;
}

const std::optional<std::array<int, 4>>& AtlasRegion::pads() const noexcept
{
    return pads_;
}

TextureAtlas::~TextureAtlas()
{
    destroy();
}

bool TextureAtlas::load(const char* atlas_path) noexcept
{
    if (!atlas_path || !*atlas_path) return false;
    std::string source;
    if (!read_asset(atlas_path, source)) return false;

    std::vector<ParsedPage> parsed_pages;
    if (!parse_atlas(atlas_path, source, parsed_pages)) return false;

    std::vector<std::unique_ptr<Texture>> textures;
    std::vector<AtlasRegion> regions;
    try {
        textures.reserve(parsed_pages.size());
        std::size_t total_regions = 0;
        for (const ParsedPage& page : parsed_pages) {
            total_regions += page.regions.size();
        }
        regions.reserve(total_regions);

        const std::string base = parent_path(atlas_path);
        for (const ParsedPage& page : parsed_pages) {
            auto texture = std::make_unique<Texture>();
            const std::string image_path = base + page.image;
            if (!texture->load(image_path.c_str())) {
                SDL_SetError(
                    "TextureAtlas page load failed for %s: %s",
                    image_path.c_str(),
                    IMG_GetError()
                );
                return false;
            }
            texture->set_filter(page.min_filter, page.mag_filter);
            texture->set_wrap(
                page.horizontal_wrap,
                page.vertical_wrap
            );
            const Texture* texture_view = texture.get();
            textures.push_back(std::move(texture));

            for (const ParsedRegion& parsed : page.regions) {
                const int storage_width = parsed.rotated
                    ? parsed.packed_height
                    : parsed.packed_width;
                const int storage_height = parsed.rotated
                    ? parsed.packed_width
                    : parsed.packed_height;
                if (parsed.x + storage_width > texture_view->width() ||
                    parsed.y + storage_height >
                        texture_view->height()) {
                    SDL_Log(
                        "TextureAtlas region %s exceeds page %s",
                        parsed.name.c_str(),
                        image_path.c_str()
                    );
                    SDL_SetError(
                        "TextureAtlas region %s exceeds page %s",
                        parsed.name.c_str(),
                        image_path.c_str()
                    );
                    return false;
                }
                const auto duplicate = std::find_if(
                    regions.begin(),
                    regions.end(),
                    [&parsed](const AtlasRegion& existing) {
                        return existing.name() == parsed.name &&
                            existing.index() == parsed.index;
                    }
                );
                if (duplicate != regions.end()) {
                    SDL_Log(
                        "TextureAtlas duplicate region %s index %d",
                        parsed.name.c_str(),
                        parsed.index
                    );
                    SDL_SetError(
                        "TextureAtlas duplicate region %s index %d",
                        parsed.name.c_str(),
                        parsed.index
                    );
                    return false;
                }

                AtlasRegion result;
                result.name_ = parsed.name;
                result.index_ = parsed.index;
                result.region_ = TextureRegion(
                    *texture_view,
                    parsed.x,
                    parsed.y,
                    parsed.packed_width,
                    parsed.packed_height,
                    parsed.rotated
                );
                result.packed_width_ = parsed.packed_width;
                result.packed_height_ = parsed.packed_height;
                result.original_width_ = parsed.original_width > 0
                    ? parsed.original_width
                    : result.region_.width();
                result.original_height_ = parsed.original_height > 0
                    ? parsed.original_height
                    : result.region_.height();
                result.offset_x_ = parsed.offset_x;
                result.offset_y_ = parsed.offset_y;
                result.splits_ = parsed.splits;
                result.pads_ = parsed.pads;
                regions.push_back(std::move(result));
            }
        }
    } catch (...) {
        SDL_Log("TextureAtlas allocation failed while loading %s", atlas_path);
        return false;
    }

    textures_.swap(textures);
    regions_.swap(regions);
    return true;
}

void TextureAtlas::destroy() noexcept
{
    regions_.clear();
    textures_.clear();
}

bool TextureAtlas::valid() const noexcept
{
    return !textures_.empty() && !regions_.empty();
}

std::size_t TextureAtlas::page_count() const noexcept
{
    return textures_.size();
}

std::size_t TextureAtlas::region_count() const noexcept
{
    return regions_.size();
}

const AtlasRegion* TextureAtlas::find_region(
    const std::string& name,
    int index
) const noexcept
{
    const auto found = std::find_if(
        regions_.begin(),
        regions_.end(),
        [&name, index](const AtlasRegion& region) {
            return region.name() == name && region.index() == index;
        }
    );
    return found == regions_.end() ? nullptr : &*found;
}

}  // namespace squared::graphics2d
