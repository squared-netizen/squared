# zstd 1.5.7

squared's one copy of zstd (`squared.zstd`, linked by `sq.data`): the same
files as sqcart's (`third_party/sqcart/third_party/zstd-1.5.7`, where they
came from, and its README.sqcart.md for how they were made from the 1.5.7
release archive).

- `zstd.c`: compressor, decoder and dictionary builder, one file.
- `zstd.h`, `zstd_errors.h`, `zdict.h`: the release's headers, unchanged.

An app or tool that wants zstd links `squared.zstd` (or `squared.data`,
which brings it) and includes `<zstd.h>`, or calls `sq::data::compress` and
`sq::data::decompress`; it carries no copy of its own, so one is linked
whoever else uses it (sq.cart, given it as `sqcart_settings.zstd`).

BSD licence (`LICENSE`).
