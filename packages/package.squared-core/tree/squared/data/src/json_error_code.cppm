export module sq.data:src.json_error_code;

import std;

export namespace sq::data
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
