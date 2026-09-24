// TextureAtlas: the libGDX .atlas format, and the pages it names.

#include <squared/graphics/context.hpp>
//
// Backend-independent: every graphics call it makes goes through Texture,
// which has its own per-backend half. This file is parsing and bookkeeping.

#include <squared/graphics2d/texture_atlas.hpp>

#include <squared/files/file_handle.hpp>
#include <squared/files/file_text_result.hpp>
#include <squared/graphics2d/atlas_region.hpp>
#include <squared/graphics2d/texture.hpp>
#include <squared/graphics2d/texture_filter.hpp>
#include <squared/graphics2d/texture_recovery_options.hpp>
#include <squared/graphics2d/texture_recovery_policy.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/graphics2d/texture_wrap.hpp>

#include <array>
#include <charconv>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sq::graphics2d {

namespace {

/**
 * @brief Upper bounds on what one atlas may describe.
 *
 * A corrupt or hostile file must not be able to make this allocate without
 * limit before anything notices. These are far above any real atlas: the
 * default skin has one page and about forty regions.
 */
constexpr std::size_t k_max_pages = 64;
constexpr std::size_t k_max_regions = 65536;

/**
 * @brief One region as the file describes it, before it has a texture.
 *
 * A region's properties arrive after its name, and its TextureRegion cannot
 * exist until the page it belongs to is known and loaded. So the parse stages
 * plain numbers here and builds the real regions once, at the end.
 */
struct PendingRegion final {
    std::string name;
    std::size_t page{0};
    int index{-1};
    int x{0};
    int y{0};
    int width{0};
    int height{0};
    int original_width{0};
    int original_height{0};
    int offset_x{0};
    int offset_y{0};
    bool rotated{false};
    std::optional<std::array<int, 4>> splits;
    std::optional<std::array<int, 4>> pads;
};

/** @brief How the page header describes it. */
struct PageHeader final {
    TextureFilter minification{TextureFilter::Linear};
    TextureFilter magnification{TextureFilter::Linear};
    TextureWrap horizontal_wrap{TextureWrap::ClampToEdge};
    TextureWrap vertical_wrap{TextureWrap::ClampToEdge};
};

std::string_view trim(std::string_view text) noexcept
{
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) {
        text.remove_prefix(1);
    }
    while (!text.empty()
           && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) {
        text.remove_suffix(1);
    }
    return text;
}

bool indented(std::string_view line) noexcept
{
    return !line.empty() && (line.front() == ' ' || line.front() == '\t');
}

/** @brief Split "key: value" at the first colon. */
bool split_pair(
    std::string_view line,
    std::string_view& key,
    std::string_view& value
) noexcept
{
    const std::size_t colon = line.find(':');
    if (colon == std::string_view::npos) return false;
    key = trim(line.substr(0, colon));
    value = trim(line.substr(colon + 1));
    return !key.empty();
}

/** @brief Read up to four comma-separated integers. */
std::size_t read_integers(
    std::string_view value,
    std::array<int, 4>& out
) noexcept
{
    std::size_t count = 0;
    std::size_t start = 0;
    while (start <= value.size() && count < out.size()) {
        std::size_t end = value.find(',', start);
        if (end == std::string_view::npos) end = value.size();

        const std::string_view field = trim(value.substr(start, end - start));
        int number = 0;
        const char* first = field.data();
        const char* last = field.data() + field.size();
        if (field.empty()
            || std::from_chars(first, last, number).ptr != last) {
            return count;
        }
        out[count++] = number;

        if (end == value.size()) break;
        start = end + 1;
    }
    return count;
}

/**
 * @brief Map a libGDX filter name.
 *
 * Every MipMap variant becomes Linear: squared uploads no mipmaps, and
 * selecting a mipmapped minification filter without them makes the texture
 * sample black. Nearest is preserved exactly, because a pixel-art skin is
 * ruined by anything else - commodore64 asks for Nearest, the default skin for
 * Linear, and honouring that is the whole reason this is read from the file.
 */
TextureFilter to_filter(std::string_view name) noexcept
{
    return name == "Nearest" ? TextureFilter::Nearest : TextureFilter::Linear;
}

/** @brief Map a libGDX repeat field: none, x, y or xy. */
void apply_repeat(std::string_view value, PageHeader& page) noexcept
{
    page.horizontal_wrap = value.find('x') != std::string_view::npos
        ? TextureWrap::Repeat : TextureWrap::ClampToEdge;
    page.vertical_wrap = value.find('y') != std::string_view::npos
        ? TextureWrap::Repeat : TextureWrap::ClampToEdge;
}

}  // namespace

TextureAtlas::~TextureAtlas()
{
    destroy();
}

bool TextureAtlas::load(const files::FileHandle& atlas) noexcept
{
    return load(atlas, TextureRecoveryPolicy::ReloadFromAsset);
}

bool TextureAtlas::load(
    const files::FileHandle& atlas,
    TextureRecoveryPolicy page_recovery
) noexcept
{
    // Regenerate needs a callback, and there is nowhere here to pass one.
    // Refusing beats accepting a policy that cannot be honoured, which would
    // produce an atlas that silently fails to come back after a phone call.
    if (page_recovery == TextureRecoveryPolicy::Regenerate) return false;
    if (!atlas.valid()) return false;

    files::FileTextResult text = atlas.read_string();
    if (!text) return false;

    destroy();

    TextureRecoveryOptions recovery;
    recovery.policy = page_recovery;

    std::vector<PendingRegion> pending_regions;
    Texture* page = nullptr;
    PageHeader header;
    bool expect_page_name = true;
    bool in_page_header = false;

    // One pass. A line that is not indented either names a page or names a
    // region; an indented line is a property of the region above it. That is
    // the whole grammar.
    std::size_t start = 0;
    const std::string& body = text.text;
    while (start <= body.size()) {
        std::size_t end = body.find('\n', start);
        if (end == std::string::npos) end = body.size();
        const std::string_view raw{body.data() + start, end - start};
        start = end + 1;

        const std::string_view line = trim(raw);
        if (line.empty()) {
            // A blank line separates pages. The next unindented line names one.
            expect_page_name = true;
            in_page_header = false;
            continue;
        }

        std::string_view key;
        std::string_view value;
        const bool is_pair = split_pair(line, key, value);

        if (!indented(raw)) {
            if (expect_page_name) {
                if (textures_.size() >= k_max_pages) { destroy(); return false; }

                // Pages are named relative to the atlas, which is what sibling
                // is for: the handle carries the file system with it, so this
                // works against the APK and a directory alike.
                auto texture = std::make_unique<Texture>();
                if (!texture->load(atlas.sibling(line), recovery)) {
                    destroy();
                    return false;
                }
                textures_.push_back(std::move(texture));
                page = textures_.back().get();
                header = PageHeader{};
                expect_page_name = false;
                in_page_header = true;
                continue;
            }

            if (in_page_header && is_pair) {
                if (key == "filter") {
                    std::array<std::string_view, 2> names{};
                    const std::size_t comma = value.find(',');
                    names[0] = trim(value.substr(0, comma));
                    names[1] = comma == std::string_view::npos
                        ? names[0] : trim(value.substr(comma + 1));
                    header.minification = to_filter(names[0]);
                    header.magnification = to_filter(names[1]);
                } else if (key == "repeat") {
                    apply_repeat(value, header);
                }
                // size and format are the packer's own record. The texture
                // reports its real dimensions once loaded, and the format is
                // whatever the decoder produced, so neither is trusted here.
                continue;
            }

            // Not a page name and not a page header: a region name.
            if (page == nullptr) { destroy(); return false; }
            if (pending_regions.size() >= k_max_regions) {
                destroy();
                return false;
            }

            // The page header is finished, so its sampling can be applied.
            if (in_page_header) {
                page->set_filter(header.minification, header.magnification);
                page->set_wrap(header.horizontal_wrap, header.vertical_wrap);
                in_page_header = false;
            }

            PendingRegion pending;
            pending.name = std::string{line};
            pending.page = textures_.size() - 1;
            pending_regions.push_back(std::move(pending));
            continue;
        }

        // An indented property belongs to the region just named.
        if (pending_regions.empty() || !is_pair) continue;
        PendingRegion& region = pending_regions.back();
        std::array<int, 4> numbers{};
        const std::size_t count = read_integers(value, numbers);

        if (key == "rotate") {
            region.rotated = value == "true";
        } else if (key == "xy" && count == 2) {
            region.x = numbers[0];
            region.y = numbers[1];
        } else if (key == "size" && count == 2) {
            region.width = numbers[0];
            region.height = numbers[1];
        } else if (key == "orig" && count == 2) {
            region.original_width = numbers[0];
            region.original_height = numbers[1];
        } else if (key == "offset" && count == 2) {
            region.offset_x = numbers[0];
            region.offset_y = numbers[1];
        } else if (key == "index" && count == 1) {
            region.index = numbers[0];
        } else if (key == "split" && count == 4) {
            region.splits = numbers;
        } else if (key == "pad" && count == 4) {
            region.pads = numbers;
        }
    }

    if (in_page_header && page != nullptr) {
        page->set_filter(header.minification, header.magnification);
        page->set_wrap(header.horizontal_wrap, header.vertical_wrap);
    }

    if (textures_.empty() || pending_regions.empty()) {
        destroy();
        return false;
    }

    regions_.reserve(pending_regions.size());
    for (const PendingRegion& pending : pending_regions) {
        if (pending.width <= 0 || pending.height <= 0) { destroy(); return false; }

        AtlasRegion region;
        region.name_ = pending.name;
        region.index_ = pending.index;
        // The logical size goes in; TextureRegion swaps the storage extents
        // itself when the packer rotated the region, which is why size: is
        // passed through unchanged rather than swapped here.
        region.region_ = TextureRegion{
            *textures_[pending.page], pending.x, pending.y,
            pending.width, pending.height, pending.rotated
        };
        region.packed_width_ = pending.width;
        region.packed_height_ = pending.height;
        region.original_width_ =
            pending.original_width > 0 ? pending.original_width : pending.width;
        region.original_height_ =
            pending.original_height > 0 ? pending.original_height : pending.height;
        region.offset_x_ = pending.offset_x;
        region.offset_y_ = pending.offset_y;
        region.splits_ = pending.splits;
        region.pads_ = pending.pads;
        regions_.push_back(std::move(region));
    }
    return true;
}

void TextureAtlas::destroy() noexcept
{
    // Regions first: each holds a pointer into a page, and nothing may outlive
    // the texture it points at.
    regions_.clear();
    textures_.clear();
}

void TextureAtlas::release() noexcept
{
    // The GPU objects go; every page keeps the recipe for rebuilding itself,
    // and every region keeps its coordinates. restore() puts it all back.
    for (const std::unique_ptr<Texture>& texture : textures_) {
        texture->release();
    }
}

void TextureAtlas::invalidate() noexcept
{
    // After context loss. Not release(): those names belong to a context that
    // no longer exists, and deleting them would free unrelated objects in the
    // new one.
    for (const std::unique_ptr<Texture>& texture : textures_) {
        texture->invalidate();
    }
}

bool TextureAtlas::restore(const graphics::Context& graphics) noexcept
{
    // The context knows whether anything was lost; asking it removes the one
    // decision a caller could get backwards.
    return restore(graphics.resources_preserved());
}

bool TextureAtlas::restore(bool context_preserved) noexcept
{
    bool restored = true;
    for (const std::unique_ptr<Texture>& texture : textures_) {
        if (!texture->restore(context_preserved)) restored = false;
    }

    // Every page that came back did so from the same file, with the same
    // layout, so each region's coordinates still describe its own pixels - but
    // its recorded generation is now stale. This is precisely what refresh()
    // is for: the atlas asserting that the layout matches, on its regions'
    // behalf. A page that failed leaves its regions invalid, which is correct.
    for (AtlasRegion& region : regions_) {
        static_cast<void>(region.region_.refresh());
    }
    return restored;
}

bool TextureAtlas::valid() const noexcept
{
    if (textures_.empty()) return false;
    for (const std::unique_ptr<Texture>& texture : textures_) {
        if (!texture->has_content()) return false;
    }
    return true;
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
    for (const AtlasRegion& region : regions_) {
        if (region.name() != name) continue;
        // A negative index means "any", which is how a caller asks for a
        // region that was packed without one.
        if (index < 0 || region.index() == index) return &region;
    }
    return nullptr;
}

}  // namespace sq::graphics2d
