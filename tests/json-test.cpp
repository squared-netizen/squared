#include <squared/data/json.hpp>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>

namespace {

int failures = 0;

void check(bool condition, const char* message)
{
    if (condition) return;
    std::fprintf(stderr, "FAILED: %s\n", message);
    ++failures;
}

void strict_parsing()
{
    const auto parsed = squared::data::parse_json(
        R"({"text":"λ","signed":-7,"unsigned":18446744073709551615,)"
        R"("real":1.25,"array":[true,null]})"
    );
    check(static_cast<bool>(parsed), "strict document parses");
    const auto* text = parsed.value.find("text");
    check(text && text->string_if() && *text->string_if() == "λ",
          "UTF-8 string survives parsing");
    const auto* signed_value = parsed.value.find("signed");
    check(
        signed_value &&
            signed_value->signed_integer_if() &&
            *signed_value->signed_integer_if() == -7,
        "signed integer remains signed"
    );
    const auto* unsigned_value = parsed.value.find("unsigned");
    check(
        unsigned_value &&
            unsigned_value->unsigned_integer_if() &&
            *unsigned_value->unsigned_integer_if() ==
                std::numeric_limits<std::uint64_t>::max(),
        "full uint64 remains unsigned"
    );

    for (std::string_view invalid : {
        std::string_view{R"({"a":1,})"},
        std::string_view{R"({"a":/* comment */1})"},
        std::string_view{R"({'a':1})"},
        std::string_view{},
        std::string_view{"\xEF\xBB\xBF{}"},
        std::string_view{"{\"bad\":\"\xFF\"}"}
    }) {
        check(
            !squared::data::parse_json(invalid),
            "non-standard or invalid UTF-8 input is rejected"
        );
    }
}

void duplicate_and_limits()
{
    const auto duplicate =
        squared::data::parse_json(R"({"same":1,"same":2})");
    check(!duplicate, "duplicate keys are rejected by default");
    check(
        duplicate.error.code ==
            squared::data::JsonErrorCode::DuplicateKey,
        "duplicate key has a structured error"
    );

    squared::data::JsonParseOptions compatibility;
    compatibility.reject_duplicate_keys = false;
    const auto last_wins = squared::data::parse_json(
        R"({"same":1,"same":2})",
        compatibility
    );
    const auto* same = last_wins.value.find("same");
    check(
        last_wins &&
            same &&
            same->unsigned_integer_if() &&
            *same->unsigned_integer_if() == 2,
        "explicit duplicate compatibility uses last value"
    );

    squared::data::JsonParseOptions size_limit;
    size_limit.maximum_bytes = 2;
    const auto oversized = squared::data::parse_json("null", size_limit);
    check(
        !oversized &&
            oversized.error.code ==
                squared::data::JsonErrorCode::InputTooLarge,
        "input size limit is enforced"
    );

    squared::data::JsonParseOptions depth_limit;
    depth_limit.maximum_depth = 2;
    const auto too_deep =
        squared::data::parse_json(R"({"outer":[0]})", depth_limit);
    check(
        !too_deep &&
            too_deep.error.code ==
                squared::data::JsonErrorCode::NestingTooDeep,
        "nesting depth limit is enforced"
    );
}

void deterministic_writing()
{
    squared::data::JsonValue::Object object;
    object.emplace("z", squared::data::JsonValue{std::uint64_t{2}});
    object.emplace("a", squared::data::JsonValue{"first"});
    object.emplace("m", squared::data::JsonValue{
        squared::data::JsonValue::Array{
            squared::data::JsonValue{true},
            squared::data::JsonValue{nullptr},
            squared::data::JsonValue{std::int64_t{-3}}
        }
    });

    const squared::data::JsonValue value{std::move(object)};
    const auto compact = squared::data::write_json(value);
    check(static_cast<bool>(compact), "compact JSON writes");
    check(
        compact.text == R"({"a":"first","m":[true,null,-3],"z":2})",
        "object keys have deterministic ordering"
    );

    squared::data::JsonWriteOptions pretty_options;
    pretty_options.pretty = true;
    pretty_options.newline_at_end = true;
    const auto pretty =
        squared::data::write_json(value, pretty_options);
    check(
        pretty &&
            pretty.text.find("\n  \"a\": \"first\"") !=
                std::string::npos &&
            pretty.text.ends_with('\n'),
        "pretty JSON uses two spaces and optional final newline"
    );

    const auto round_trip = squared::data::parse_json(compact.text);
    const auto repeated = squared::data::write_json(round_trip.value);
    check(
        round_trip && repeated && repeated.text == compact.text,
        "parse and write round trip is stable"
    );

    const auto not_finite = squared::data::write_json(
        squared::data::JsonValue{
            std::numeric_limits<double>::infinity()
        }
    );
    check(
        !not_finite &&
            not_finite.error.code ==
                squared::data::JsonErrorCode::NonFiniteNumber,
        "non-finite values cannot be written"
    );

    const auto invalid_utf8 = squared::data::write_json(
        squared::data::JsonValue{std::string{"\xFF"}}
    );
    check(!invalid_utf8, "invalid UTF-8 values cannot be written");
}

}  // namespace

int main()
{
    strict_parsing();
    duplicate_and_limits();
    deterministic_writing();

    if (failures != 0) {
        std::fprintf(stderr, "%d JSON test(s) failed\n", failures);
        return 1;
    }

    std::puts("Squared strict JSON: OK");
    return 0;
}
