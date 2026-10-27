// The standard headers squared includes, all of them, compiled once per flag
// set into a precompiled header that every squared source and test is
// compiled with (build/common.mk, "the standard headers, precompiled").
//
// Not a header to include: the build hands it to the compiler. A standard
// header missing here is still included where it is named, only not
// precompiled; tools/std_pch.sh --check says when this list is behind the
// sources (make headers runs it).
//
// Written by tools/std_pch.sh.

#pragma once

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
