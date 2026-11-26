# zstd 1.5.7

From the release archive
(https://github.com/facebook/zstd/releases/download/v1.5.7/zstd-1.5.7.tar.gz,
sha256 eb33e51f49a15e023950cd7825ca74a4a2b43db8354825ac24fc1b7ee09e6fa3),
built into single files with its own `build/single_file_libs` scripts:

- `zstddeclib.c`: the decoder alone (`create_single_file_decoder.sh`), the
  same copy sqterm carries (`sq_app/third_party/zstd`). What a program that
  only reads cartridges links (sq.cart's reader, codecs 1 and 2).
- `zstd.c`: compressor, decoder and dictionary builder
  (`create_single_file_library.sh`). What the 3.0 writer links (sq.cart.write)
  instead of `zstddeclib.c`: the two define the same decoder, so a program
  links one or the other, never both.
- `zstd.h`, `zstd_errors.h`, `zdict.h`: the release's headers, unchanged.

BSD licence (`LICENSE`).
