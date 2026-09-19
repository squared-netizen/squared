#include "detail/image_decode.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include <stb_image_write.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

static std::vector<unsigned char> encoded;
static void collect(void*, void* data, int size)
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    encoded.insert(encoded.end(), bytes, bytes + size);
}

int main()
{
    using namespace sq::graphics2d::detail;

    // a 3x2 RGBA image: width 3 is the misaligned case
    unsigned char source[3 * 2 * 4];
    for (int i = 0; i < 3 * 2; ++i) {
        source[i * 4 + 0] = static_cast<unsigned char>(i * 40);
        source[i * 4 + 1] = 10;
        source[i * 4 + 2] = 20;
        source[i * 4 + 3] = 255;
    }
    assert(stbi_write_png_to_func(collect, nullptr, 3, 2, 4, source, 3 * 4));
    assert(!encoded.empty());

    const auto bytes = std::as_bytes(std::span{encoded});

    // round trip: the decoded pixels must equal what was encoded
    DecodedImage image = decode_image(bytes);
    assert(image);
    assert(image.width == 3 && image.height == 2);
    assert(image.pixels.size() == 3 * 2 * 4);
    assert(std::memcmp(image.pixels.data(), source, sizeof(source)) == 0);
    assert(image.message.empty());

    // garbage is rejected with the decoder's own reason
    const unsigned char junk[] = {1, 2, 3, 4, 5, 6, 7, 8};
    DecodedImage bad = decode_image(std::as_bytes(std::span{junk}));
    assert(!bad);
    assert(!bad.message.empty());

    // empty input
    assert(!decode_image({}));

    // the pixel limit rejects before allocating
    DecodedImage capped = decode_image(bytes, 4);   // 3x2 = 6 > 4
    assert(!capped);
    assert(capped.message.find("limit") != std::string::npos);
    assert(capped.pixels.empty());

    // a limit that fits still decodes
    assert(decode_image(bytes, 6));

    std::printf("image_decode: round trip exact, %dx%d, limits enforced\n",
                image.width, image.height);
    return 0;
}
