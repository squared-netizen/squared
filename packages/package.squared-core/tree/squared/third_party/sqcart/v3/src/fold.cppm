// SPDX-License-Identifier: MIT
//
// sq.cart:fold — Unicode for paths (§4.1): when two paths name one file on
// a case-insensitive file system (equal after full case folding: Android's
// shared storage, exFAT, FAT, macOS and Windows all fold case, and fold more
// than ASCII), and Normalization Form C, which every path must be in.

export module sq.cart:fold;

import std;
import :json;

namespace sq::cart {

struct Fold {
    char32_t from;
    std::array<char32_t, 3> to;
};

constexpr Fold folds[] =
#include "casefold.inc"
    ;

[[nodiscard]] std::u32string folded(std::string_view path) {
    std::u32string cps, out;
    static_cast<void>(decode_utf8(path, &cps));
    for (const char32_t cp : cps) {
        const auto* it = std::ranges::lower_bound(folds, cp, {}, &Fold::from);
        if (it != std::ranges::end(folds) && it->from == cp) {
            for (const char32_t t : it->to) {
                if (t != 0) out.push_back(t);
            }
        } else {
            out.push_back(cp);
        }
    }
    return out;
}

// ---- Normalization Form C (UAX #15) ----

#include "nfc.inc"

constexpr char32_t hangul_s = 0xAC00, hangul_l = 0x1100, hangul_v = 0x1161, hangul_t = 0x11A7;
constexpr char32_t hangul_l_count = 19, hangul_v_count = 21, hangul_t_count = 28;
constexpr char32_t hangul_n = hangul_v_count * hangul_t_count, hangul_s_count = hangul_l_count * hangul_n;

[[nodiscard]] std::uint8_t combining_class(char32_t cp) {
    const auto* it = std::ranges::lower_bound(nfc_ccc, cp, {}, &std::pair<char32_t, std::uint8_t>::first);
    return it != std::ranges::end(nfc_ccc) && it->first == cp ? it->second : 0;
}

void decompose(char32_t cp, std::u32string& out) {
    if (cp >= hangul_s && cp < hangul_s + hangul_s_count) {
        const char32_t i = cp - hangul_s;
        out.push_back(hangul_l + i / hangul_n);
        out.push_back(hangul_v + (i % hangul_n) / hangul_t_count);
        if (i % hangul_t_count != 0) out.push_back(hangul_t + i % hangul_t_count);
        return;
    }
    const auto* it = std::ranges::lower_bound(nfc_decompositions, cp, {}, &std::pair<char32_t, std::array<char32_t, nfc_decomposition_width>>::first);
    if (it == std::ranges::end(nfc_decompositions) || it->first != cp) {
        out.push_back(cp);
        return;
    }
    for (const char32_t d : it->second) {
        if (d != 0) out.push_back(d);
    }
}

[[nodiscard]] std::optional<char32_t> compose_pair(char32_t a, char32_t b) {
    if (a >= hangul_l && a < hangul_l + hangul_l_count && b >= hangul_v && b < hangul_v + hangul_v_count) {
        return hangul_s + ((a - hangul_l) * hangul_v_count + (b - hangul_v)) * hangul_t_count;
    }
    if (a >= hangul_s && a < hangul_s + hangul_s_count && (a - hangul_s) % hangul_t_count == 0 && b > hangul_t &&
        b < hangul_t + hangul_t_count) {
        return a + (b - hangul_t);
    }
    const auto* it = std::ranges::lower_bound(nfc_compositions, std::array<char32_t, 3>{a, b, 0},
                                              [](const auto& x, const auto& y) { return std::pair(x[0], x[1]) < std::pair(y[0], y[1]); });
    if (it != std::ranges::end(nfc_compositions) && (*it)[0] == a && (*it)[1] == b) return (*it)[2];
    return std::nullopt;
}

void encode_utf8(std::string& out, char32_t cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xc0 | cp >> 6);
        out += static_cast<char>(0x80 | (cp & 0x3f));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xe0 | cp >> 12);
        out += static_cast<char>(0x80 | (cp >> 6 & 0x3f));
        out += static_cast<char>(0x80 | (cp & 0x3f));
    } else {
        out += static_cast<char>(0xf0 | cp >> 18);
        out += static_cast<char>(0x80 | (cp >> 12 & 0x3f));
        out += static_cast<char>(0x80 | (cp >> 6 & 0x3f));
        out += static_cast<char>(0x80 | (cp & 0x3f));
    }
}

}  // namespace sq::cart

export namespace sq::cart {

/// `text` (valid UTF-8) in Normalization Form C: canonically decomposed,
/// combining marks in canonical order, then composed again.
[[nodiscard]] std::string nfc(std::string_view text) {
    std::u32string in, d;
    static_cast<void>(decode_utf8(text, &in));
    for (const char32_t cp : in) decompose(cp, d);
    // Canonical order: a stable sort of each run of non-starters by class.
    for (std::size_t i = 0; i < d.size();) {
        if (combining_class(d[i]) == 0) {
            ++i;
            continue;
        }
        std::size_t j = i;
        while (j < d.size() && combining_class(d[j]) != 0) ++j;
        std::stable_sort(d.begin() + static_cast<std::ptrdiff_t>(i), d.begin() + static_cast<std::ptrdiff_t>(j),
                         [](char32_t a, char32_t b) { return combining_class(a) < combining_class(b); });
        i = j;
    }
    // Canonical composition.
    std::u32string out;
    std::ptrdiff_t starter = -1;
    int last_class = -1;
    for (const char32_t cp : d) {
        const int cc = combining_class(cp);
        if (starter >= 0 && (last_class < cc || last_class == 0)) {
            if (const auto composite = compose_pair(out[static_cast<std::size_t>(starter)], cp)) {
                out[static_cast<std::size_t>(starter)] = *composite;
                continue;
            }
        }
        if (cc == 0) {
            starter = static_cast<std::ptrdiff_t>(out.size());
            last_class = 0;
        } else {
            last_class = cc;
        }
        // A non-starter before any starter blocks nothing it cannot reach.
        if (starter < 0) last_class = 256;
        out.push_back(cp);
    }
    std::string utf8;
    for (const char32_t cp : out) encode_utf8(utf8, cp);
    return utf8;
}

/// Whether `text` is in Normalization Form C already.
[[nodiscard]] bool is_nfc(std::string_view text) {
    // Pure ASCII always is; most paths are.
    if (std::ranges::all_of(text, [](char c) { return static_cast<unsigned char>(c) < 0x80; })) return true;
    return nfc(text) == text;
}

}  // namespace sq::cart

export namespace sq::cart {

/// Whether two paths collide on a case-insensitive file system: equal after
/// Unicode full case folding (CaseFolding.txt, statuses C and F). Paths are
/// NFC already (§4.1).
[[nodiscard]] bool paths_collide(std::string_view a, std::string_view b) { return folded(a) == folded(b); }

/// The key two colliding paths share, for finding collisions in a set.
[[nodiscard]] std::u32string collision_key(std::string_view path) { return folded(path); }

}  // namespace sq::cart
