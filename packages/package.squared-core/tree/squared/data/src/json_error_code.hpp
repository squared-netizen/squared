#pragma once

namespace sq::data {

/** @brief Stable error categories produced by JSON parsing and writing. */
enum class JsonErrorCode {
    None,
    InputTooLarge,
    Syntax,
    DuplicateKey,
    NestingTooDeep,
    NonFiniteNumber,
    AllocationFailure,
    InvalidValue
};

}  // namespace sq::data
