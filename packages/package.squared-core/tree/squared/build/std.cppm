// sq.std: the standard library as one module, for squared and the apps on it.
// Every squared file writes `import sq.std;` where it wrote standard includes:
// the headers below are parsed once per flag set, into one BMI, and a file
// reads from it only what it uses (7b.4). The `import std` that Termux's libc++
// lacks and the NDK's std.cppm cannot yet build.
//
// Written by tools/std_module.sh, from the names squared writes as std::<name>;
// `make headers` says when it is behind them. Not here: the <cctype> functions
// (bionic gives them internal linkage) and every C macro (assert, errno,
// va_start, UINT32_MAX, SIGSEGV): a file that uses one includes its C header.

// clang-format off: this file is written, in the generator's order.
module;

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <compare>
#include <concepts>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <numeric>
#include <optional>
#include <random>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

export module sq.std;

export namespace std
{

using std::FILE;
using std::abort;
using std::abs;
using std::align_val_t;
using std::all_of;
using std::array;
using std::as_bytes;
using std::atoi;
using std::atomic;
using std::atomic_thread_fence;
using std::bad_alloc;
using std::begin;
using std::byte;
using std::ceil;
using std::clamp;
using std::construct_at;
using std::convertible_to;
using std::copy;
using std::copy_n;
using std::cos;
using std::count_if;
using std::default_initializable;
using std::deque;
using std::destroy_at;
using std::distance;
using std::end;
using std::equal;
using std::errc;
using std::error_code;
using std::exception;
using std::exchange;
using std::exit;
using std::fabs;
using std::fclose;
using std::fflush;
using std::fgets;
using std::fill;
using std::find;
using std::find_if;
using std::fopen;
using std::forward;
using std::forward_iterator_tag;
using std::fprintf;
using std::fread;
using std::free;
using std::from_chars;
using std::from_chars_result;
using std::function;
using std::fwrite;
using std::generate_n;
using std::get_if;
using std::get_new_handler;
using std::getenv;
using std::getline;
using std::holds_alternative;
using std::ifstream;
using std::initializer_list;
using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::invalid_argument;
using std::ios;
using std::iota;
using std::is_arithmetic_v;
using std::is_convertible_v;
using std::is_integral_v;
using std::is_nothrow_move_assignable_v;
using std::is_nothrow_move_constructible_v;
using std::is_same_v;
using std::is_trivially_copyable_v;
using std::isfinite;
using std::isnan;
using std::istreambuf_iterator;
using std::istringstream;
using std::less;
using std::lower_bound;
using std::lround;
using std::make_heap;
using std::make_shared;
using std::make_unique;
using std::malloc;
using std::map;
using std::max;
using std::memcmp;
using std::memcpy;
using std::memmove;
using std::memory_order_acq_rel;
using std::memory_order_acquire;
using std::memory_order_relaxed;
using std::memory_order_release;
using std::memory_order_seq_cst;
using std::memset;
using std::min;
using std::move;
using std::mt19937;
using std::mt19937_64;
using std::new_handler;
using std::none_of;
using std::nothrow_t;
using std::nullopt;
using std::nullptr_t;
using std::numeric_limits;
using std::ofstream;
using std::optional;
using std::ostringstream;
using std::out_of_range;
using std::pair;
using std::pop_heap;
using std::printf;
using std::ptrdiff_t;
using std::push_heap;
using std::puts;
using std::ref;
using std::remove;
using std::remove_cvref_t;
using std::rename;
using std::replace;
using std::reverse;
using std::round;
using std::shared_ptr;
using std::shuffle;
using std::sin;
using std::size_t;
using std::snprintf;
using std::sort;
using std::span;
using std::static_pointer_cast;
using std::stoi;
using std::stoll;
using std::stoull;
using std::strcmp;
using std::strerror;
using std::string;
using std::string_view;
using std::stringstream;
using std::strlen;
using std::strpbrk;
using std::strrchr;
using std::strtod;
using std::strtoul;
using std::strtoull;
using std::swap;
using std::system;
using std::thread;
using std::to_chars;
using std::to_chars_result;
using std::to_string;
using std::tuple;
using std::tuple_size_v;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;
using std::uintptr_t;
using std::unique;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::va_list;
using std::variant;
using std::vector;
using std::vsnprintf;

// Found by argument-dependent lookup, so exported by name.
using std::operator==;
using std::operator!=;
using std::operator<;
using std::operator>;
using std::operator<=;
using std::operator>=;
using std::operator<=>;
using std::operator+;
using std::operator-;
using std::operator<<;
using std::operator>>;
using std::operator|;
using std::operator&;
using std::operator^;
using std::operator~;
using std::operator|=;
using std::operator&=;
using std::operator^=;

namespace chrono
{
using std::chrono::ceil;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using std::chrono::seconds;
using std::chrono::steady_clock;
using std::chrono::operator+;
using std::chrono::operator-;
using std::chrono::operator*;
using std::chrono::operator/;
using std::chrono::operator%;
using std::chrono::operator==;
using std::chrono::operator<=>;
using std::chrono::operator<;
using std::chrono::operator>;
using std::chrono::operator<=;
using std::chrono::operator>=;
} // namespace chrono

namespace filesystem
{
using std::filesystem::create_directories;
using std::filesystem::create_directory;
using std::filesystem::directory_entry;
using std::filesystem::directory_iterator;
using std::filesystem::exists;
using std::filesystem::is_directory;
using std::filesystem::path;
using std::filesystem::remove;
using std::filesystem::remove_all;
using std::filesystem::rename;
using std::filesystem::begin;
using std::filesystem::end;
} // namespace filesystem

namespace this_thread
{
using std::this_thread::get_id;
using std::this_thread::sleep_for;
using std::this_thread::yield;
} // namespace this_thread

namespace ranges
{
using std::ranges::any_of;
using std::ranges::sort;
} // namespace ranges

inline namespace literals
{
inline namespace chrono_literals
{
using std::literals::chrono_literals::operator""h;
using std::literals::chrono_literals::operator""min;
using std::literals::chrono_literals::operator""s;
using std::literals::chrono_literals::operator""ms;
using std::literals::chrono_literals::operator""us;
using std::literals::chrono_literals::operator""ns;
} // namespace chrono_literals
} // namespace literals

} // namespace std

// libstdc++ keeps its iterators' operators in __gnu_cxx, where argument-
// dependent lookup in an importer finds them only when they are exported.
#if defined( __GLIBCXX__ )
export namespace __gnu_cxx
{
using __gnu_cxx::operator==;
using __gnu_cxx::operator<=>;
using __gnu_cxx::operator-;
using __gnu_cxx::operator+;
} // namespace __gnu_cxx
#endif
// clang-format on
