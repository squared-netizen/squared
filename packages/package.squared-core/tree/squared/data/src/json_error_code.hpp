#pragma once

#include <cstdint>

namespace sq::data
{

/** @brief Stable error categories produced by JSON parsing and writing. */
enum class JsonErrorCode : std::uint8_t {
    None,
    InputTooLarge,
    Syntax,
    DuplicateKey,
    NestingTooDeep,
    NonFiniteNumber,
    AllocationFailure,
    InvalidValue
};

} // namespace sq::data
