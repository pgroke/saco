#pragma once

// Have to include all headers used by saco before poisoning the standard types in the global namespace.

#include <string.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace force_ambiguity {

struct dummy {};

using size_t = dummy;
using ptrdiff_t = dummy;

using max_align_t = dummy;

using intptr_t = dummy;
using intmax_t = dummy;
using uintptr_t = dummy;
using uintmax_t = dummy;

using int8_t = dummy;
using int16_t = dummy;
using int32_t = dummy;
using int64_t = dummy;
using uint8_t = dummy;
using uint16_t = dummy;
using uint32_t = dummy;
using uint64_t = dummy;

using int_fast8_t = dummy;
using int_fast16_t = dummy;
using int_fast32_t = dummy;
using int_fast64_t = dummy;
using uint_fast8_t = dummy;
using uint_fast16_t = dummy;
using uint_fast32_t = dummy;
using uint_fast64_t = dummy;

using int_least8_t = dummy;
using int_least16_t = dummy;
using int_least32_t = dummy;
using int_least64_t = dummy;
using uint_least8_t = dummy;
using uint_least16_t = dummy;
using uint_least32_t = dummy;
using uint_least64_t = dummy;

} // namespace force_ambiguity

using namespace force_ambiguity;
