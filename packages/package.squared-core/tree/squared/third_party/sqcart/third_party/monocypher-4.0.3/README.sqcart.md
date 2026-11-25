# Monocypher 4.0.3

From the release archive
(https://github.com/LoupVaillant/Monocypher/releases/download/4.0.3/monocypher-4.0.3.tar.gz,
sha256 8cc9bc341a66249016db9bd70e9142d8d0aef9945973744b1ac05dbc55d8ee66):
`src/monocypher.c`, `src/monocypher.h`, `src/optional/monocypher-ed25519.c`
and `.h`, unchanged. CC0 or BSD-2-Clause, at your choice (`LICENCE.md`).

sqcart verifies 3.0 signatures (format 3.0 §9.3, Appendix B.1) with
`crypto_ed25519_check` from the optional file: Ed25519 as RFC 8032 defines
it, with SHA-512. Monocypher's own `crypto_eddsa_check` uses BLAKE2b and is
not Ed25519. Monocypher rejects S >= L but accepts a public key or R whose
encoding is not canonical; §9.3 refuses those, and sqcart checks them
itself before calling it (src/v3/signature.cpp).
