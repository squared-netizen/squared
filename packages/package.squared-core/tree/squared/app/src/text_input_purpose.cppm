export module sq.app:src.text_input_purpose;

import std;

export namespace sq::app
{

/**
 * @brief Desired soft-keyboard content mode.
 *
 * @sa TextInputRequest::purpose
 */
enum class TextInputPurpose : std::uint8_t { normal, number, email, password };

} // namespace sq::app
