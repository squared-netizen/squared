// SPDX-License-Identifier: MIT
//
// sq.cart — the reader for the Squared Cartridge Specification 3.0
// (../docs/developer/specs/sq-cartridge-3.0.md), phase 1: open, look up,
// read and verify, check signatures, validate. No writer yet.
//
//   import sq.cart;
//   auto cart = sq::cart::Archive::open("world.sq");
//   if (!cart) { std::println(stderr, "{}", cart.error().message); return 1; }
//   if (auto e = cart->find("world/regions/north.json")) {
//       auto bytes = cart->read(*e);  // verified against its digest
//   }

export module sq.cart;

export import :error;
export import :sha256;
export import :json;
export import :fold;
export import :ed25519;
export import :archive;
export import :exploded;
