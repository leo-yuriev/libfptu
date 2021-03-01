/*
 *  Fast Positive Tuples (libfptu), aka Позитивные Кортежи
 *  Copyright 2016-2020 Leonid Yuriev <leo@yuriev.ru>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once
#include "fast_positive/erthink/erthink_dynamic_constexpr.h"
#include "fast_positive/tuples/api.h"
#include "fast_positive/tuples/details/field.h"
#include "fast_positive/tuples/details/utils.h"
#include "fast_positive/tuples/essentials.h"
#include "fast_positive/tuples/types.h"

#include <climits>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

#include "fast_positive/tuples/details/warnings_push_pt.h"

namespace fptu {

namespace details {
class tuple_ro;
class tuple_rw;

FPTU_API extern /* alignas(64) */ const char zeroed_cacheline[64];

} // namespace details

namespace meta {

// Primary template for genus_traits.
template <genus GENUS> struct genus_traits;

enum class physique_kind { inplaced, fixed, stretchy };

template <typename TYPE, TYPE DENIL = 0> struct inplaced {
  using value_type = TYPE;
  using return_type = TYPE;
  static constexpr TYPE optional_denil = DENIL;
  static constexpr physique_kind physique = physique_kind::inplaced;
  static constexpr unsigned preplaced_bytes = sizeof(value_type);
  static constexpr unsigned loose_units = 0;

  static cxx11_constexpr bool
  is_denil(const details::field_preplaced *preplaced) cxx11_noexcept {
    return DENIL == *erthink::constexpr_pointer_cast<const TYPE *>(preplaced);
  }
  static cxx11_constexpr return_type
  read(const details::field_preplaced *preplaced) cxx11_noexcept {
    return *erthink::constexpr_pointer_cast<const value_type *>(preplaced);
  }
  static cxx11_constexpr return_type read(const details::field_loose *loose)
      cxx11_noexcept {
    static_assert(sizeof(TYPE) <=
                      sizeof(details::field_loose::inplace_storage_t),
                  "WTF?");
    return *erthink::constexpr_pointer_cast<const TYPE *>(&loose->inplaced);
  }
  static cxx11_constexpr return_type empty() cxx11_noexcept {
    return return_type(0);
  }

  static cxx14_constexpr bool is_empty(const return_type value) cxx11_noexcept {
    return value == return_type(0);
  }
  static cxx14_constexpr bool
  is_prohibited_nil(return_type value) cxx11_noexcept {
    (void)value;
    return false;
  }
  static void write(details::field_preplaced *preplaced,
                    const return_type value) cxx11_noexcept {
    *erthink::constexpr_pointer_cast<value_type *>(preplaced) = value;
  }
  static void write(details::field_loose *loose,
                    const return_type value) cxx11_noexcept {
    static_assert(sizeof(TYPE) <=
                      sizeof(details::field_loose::inplace_storage_t),
                  "WTF?");
    loose->inplaced =
        static_cast<details::field_loose::inplace_storage_t>(value);
  }
  static void erase(details::field_preplaced *preplaced,
                    const bool distinct_null) {
    *erthink::constexpr_pointer_cast<TYPE *>(preplaced) =
        (distinct_null && DENIL != 0) ? DENIL : 0;
  }
};

template <typename TYPE, int32_t DENIL = 0> struct unit_1 {
  using value_type = TYPE;
  using return_type = TYPE;
  static constexpr unsigned preplaced_bytes = 4;
  static constexpr unsigned loose_units = 1;
  static constexpr physique_kind physique = physique_kind::fixed;

  static cxx11_constexpr bool
  is_denil(const details::field_preplaced *preplaced) cxx11_noexcept {
    return DENIL ==
           *erthink::constexpr_pointer_cast<const int32_t *>(preplaced);
  }
  static cxx11_constexpr return_type
  read(const details::field_preplaced *preplaced) cxx11_noexcept {
    return *erthink::constexpr_pointer_cast<const value_type *>(preplaced);
  }
  static cxx11_constexpr return_type read(const details::field_loose *loose)
      cxx11_noexcept {
    return *erthink::constexpr_pointer_cast<const value_type *>(
        loose->relative.payload());
  }
  static cxx11_constexpr return_type empty() cxx11_noexcept {
    return return_type(0);
  }

  static cxx14_constexpr bool is_empty(const return_type value) cxx11_noexcept {
    return value == return_type(0);
  }
  static cxx14_constexpr bool
  is_prohibited_nil(return_type value) cxx11_noexcept {
    (void)value;
    return false;
  }
  static void write(details::field_preplaced *preplaced,
                    const return_type value) cxx11_noexcept {
    static_assert(sizeof(value_type) == 4, "WTF?");
    *erthink::constexpr_pointer_cast<value_type *>(preplaced) = value;
  }
  static void write(details::field_loose *loose,
                    const return_type value) cxx11_noexcept {
    static_assert(sizeof(value_type) == 4, "WTF?");
    *erthink::constexpr_pointer_cast<value_type *>(loose->relative.payload()) =
        value;
  }
  static void erase(details::field_preplaced *preplaced,
                    const bool distinct_null) {
    *erthink::constexpr_pointer_cast<int32_t *>(preplaced) =
        (distinct_null && DENIL != 0) ? DENIL : 0;
  }
};

template <typename TYPE, int64_t DENIL = 0> struct unit_2 {
  using value_type = TYPE;
  using return_type = TYPE;
  static constexpr unsigned preplaced_bytes = 8;
  static constexpr unsigned loose_units = 2;
  static constexpr physique_kind physique = physique_kind::fixed;

  static cxx11_constexpr bool
  is_denil(const details::field_preplaced *preplaced) cxx11_noexcept {
    return DENIL ==
           *erthink::constexpr_pointer_cast<const int64_t *>(preplaced);
  }
  static cxx11_constexpr return_type
  read(const details::field_preplaced *preplaced) cxx11_noexcept {
    return *erthink::constexpr_pointer_cast<const value_type *>(preplaced);
  }
  static cxx11_constexpr return_type read(const details::field_loose *loose)
      cxx11_noexcept {
    return *erthink::constexpr_pointer_cast<const value_type *>(
        loose->relative.payload());
  }
  static cxx11_constexpr return_type empty() cxx11_noexcept {
    return return_type(0);
  }

  static cxx14_constexpr bool is_empty(const return_type value) cxx11_noexcept {
    return value == return_type(0);
  }
  static cxx14_constexpr bool
  is_prohibited_nil(return_type value) cxx11_noexcept {
    (void)value;
    return false;
  }
  static void write(details::field_preplaced *preplaced,
                    const return_type value) cxx11_noexcept {
    static_assert(sizeof(value_type) == 8, "WTF?");
    *erthink::constexpr_pointer_cast<value_type *>(preplaced) = value;
  }
  static void write(details::field_loose *loose,
                    const return_type value) cxx11_noexcept {
    static_assert(sizeof(value_type) == 8, "WTF?");
    *erthink::constexpr_pointer_cast<value_type *>(loose->relative.payload()) =
        value;
  }
  static void erase(details::field_preplaced *preplaced,
                    const bool distinct_null) {
    *erthink::constexpr_pointer_cast<int64_t *>(preplaced) =
        (distinct_null && DENIL != 0) ? DENIL : 0;
  }
};

template <unsigned N, typename TYPE = uint32_t[N]> struct unit_n {
  using value_type = TYPE;
  using return_type = const TYPE &;
  static constexpr unsigned preplaced_bytes = 4 * N;
  static constexpr unsigned loose_units = N;
  static constexpr physique_kind physique = physique_kind::fixed;

  static cxx11_constexpr bool
  is_denil(const details::field_preplaced *preplaced) cxx11_noexcept {
    static_assert(sizeof(value_type) <= preplaced_bytes, "WTF?");
    return utils::is_zero<sizeof(value_type)>(preplaced);
  }
  static cxx11_constexpr return_type
  read(const details::field_preplaced *preplaced) cxx11_noexcept {
    return *erthink::constexpr_pointer_cast<const value_type *>(preplaced);
  }
  static cxx11_constexpr return_type read(const details::field_loose *loose)
      cxx11_noexcept {
    return *erthink::constexpr_pointer_cast<const value_type *>(
        loose->relative.payload());
  }
  static cxx11_constexpr return_type empty() cxx11_noexcept {
    return *erthink::constexpr_pointer_cast<const value_type *>(
        &details::zeroed_cacheline);
  }

  static cxx14_constexpr bool is_empty(return_type value) cxx11_noexcept {
    return utils::is_zero<sizeof(return_type)>(&value);
  }
  static cxx14_constexpr bool
  is_prohibited_nil(return_type value) cxx11_noexcept {
    (void)value;
    return false;
  }
  static void write(details::field_preplaced *preplaced,
                    return_type value) cxx11_noexcept {
    static_assert(sizeof(value_type) <= preplaced_bytes, "WTF?");
    std::memcpy(preplaced->bytes, &value, preplaced_bytes);
    if (preplaced_bytes != sizeof(value_type))
      std::memset(preplaced->bytes + sizeof(value_type), 0,
                  preplaced_bytes - sizeof(value_type));
  }

#if __GNUC_PREREQ(10, 0) /* || __has_warning("-Wstringop-overflow") */
  /* This is workaround for GCC 10 false-positive warnings
   * like "writing 16 bytes into a region of size 2 [-Wstringop-overflow=]".
   * Unfortunately this crutch has no effect if LTO enabled. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif /* GNU C >= 10.x */
  static void write(details::field_loose *loose,
                    return_type value) cxx11_noexcept {
    static_assert(sizeof(value_type) <= preplaced_bytes, "WTF?");
    details::relative_payload *ptr = loose->relative.payload();

#if __GNUC_PREREQ(10, 0) && defined(LTO_ENABLED)
    /* This is workaround for GCC 10 false-positive warnings
     * like "writing 16 bytes into a region of size 2 [-Wstringop-overflow=]".
     * This crutch is for the case when LTO is enabled. */
    __asm("#workaround $1" : "=r"(ptr) : "r"(ptr));
#endif /* GNU C >= 10.x */

    static_assert(sizeof(ptr->fixed) >= preplaced_bytes, "WTF?");
    std::memcpy(&ptr->fixed.words, &value, preplaced_bytes);
    if (preplaced_bytes != sizeof(value_type))
      std::memset(ptr->fixed.bytes + sizeof(value_type), 0,
                  preplaced_bytes - sizeof(value_type));
  }
#if __GNUC_PREREQ(10, 0) /* || __has_warning("-Wstringop-overflow") */
#pragma GCC diagnostic pop
#endif /* GNU C >= 10.x */

  static void erase(details::field_preplaced *preplaced,
                    const bool distinct_null) {
    (void)distinct_null;
    std::memset(preplaced->bytes, 0, preplaced_bytes);
  }
};

// physique inplace, 8/16 bits -------------------------------------------------

template <> struct genus_traits<i8> : public inplaced<int8_t, -128> {
  static constexpr value_type max = 127;
  static constexpr value_type min = -max;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<i8>::value;
};

template <> struct genus_traits<u8> : public inplaced<uint8_t, 0> {
  static constexpr value_type max = 255;
  static constexpr value_type min = 0;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<i8>::value;
};

template <> struct genus_traits<i16> : public inplaced<int16_t, -32768> {
  static constexpr value_type max = 32767;
  static constexpr value_type min = -max;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<i16>::value |
      genus_traits<i8>::trivially_convertible_from |
      genus_traits<u8>::trivially_convertible_from;
};

template <> struct genus_traits<u16> : public inplaced<uint16_t, 0> {
  static constexpr value_type max = 65535;
  static constexpr value_type min = 0;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<u16>::value |
      genus_traits<u8>::trivially_convertible_from;
};

// physique unit×1, 32 bits ----------------------------------------------------

template <> struct genus_traits<i32> : public unit_1<int32_t, INT32_MIN> {
  static constexpr value_type max = std::numeric_limits<value_type>::max();
  static constexpr value_type min = -max;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<i32>::value |
      genus_traits<i16>::trivially_convertible_from |
      genus_traits<u16>::trivially_convertible_from;
};

template <> struct genus_traits<u32> : public unit_1<uint32_t, 0> {
  static constexpr value_type max = std::numeric_limits<value_type>::max();
  static constexpr value_type min = 0;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<u32>::value |
      genus_traits<u16>::trivially_convertible_from;
};

template <>
struct genus_traits<f32> : public unit_1<float, -1 /* quied negative nan */> {
  static constexpr value_type max = std::numeric_limits<value_type>::max();
  static constexpr value_type min = std::numeric_limits<value_type>::lowest();
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask</*f16,*/ f32>::value |
      genus_traits<i16>::trivially_convertible_from |
      genus_traits<u16>::trivially_convertible_from;
};

template <> struct genus_traits<t32> : public unit_1<unsigned, 0> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<t32, t64>::value;
};

// physique unit×2, 64 bits ----------------------------------------------------

template <> struct genus_traits<i64> : public unit_2<int64_t, INT64_MIN> {
  static constexpr value_type max = std::numeric_limits<value_type>::max();
  static constexpr value_type min = -max;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<i64>::value |
      genus_traits<i32>::trivially_convertible_from |
      genus_traits<u32>::trivially_convertible_from;
  static cxx14_constexpr bool is_empty(const return_type value) cxx11_noexcept {
    return value == return_type(0);
  }
};

template <> struct genus_traits<u64> : public unit_2<uint64_t, 0> {
  static constexpr value_type max = std::numeric_limits<value_type>::max();
  static constexpr value_type min = 0;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<u64>::value |
      genus_traits<u32>::trivially_convertible_from;
  static cxx14_constexpr bool is_empty(const return_type value) cxx11_noexcept {
    return value == return_type(0);
  }
};

template <>
struct genus_traits<f64> : public unit_2<double, -1 /* quied negative nan */> {
  static constexpr value_type max = std::numeric_limits<value_type>::max();
  static constexpr value_type min = std::numeric_limits<value_type>::lowest();
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<i32, u32, f64>::value |
      genus_traits<f32>::trivially_convertible_from;
  static cxx14_constexpr bool is_empty(const return_type value) cxx11_noexcept {
    return value == return_type(0);
  }
};

template <>
struct genus_traits<d64>
    : public unit_2<decimal64, -1 /* quied negative nan */> {
  //  static constexpr value_type min = ;
  //  static constexpr value_type max = ;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<d64>::value;
  static bool is_empty(const return_type &value) cxx11_noexcept {
    return value == return_type(0);
  }
};

template <> struct genus_traits<t64> : public unit_2<datetime_t, 0> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<t32, t64>::value;
  static cxx11_constexpr return_type empty() cxx11_noexcept {
    return datetime_t::from_fixedpoint_32dot32(0);
  }
  static cxx14_constexpr bool is_empty(const return_type value) cxx11_noexcept {
    return value.fixedpoint_32dot32() == 0;
  }
};

// physique unit×N, 96/128/160/192/224/256/384/512 bits ------------------------

template <> struct genus_traits<bin96> : public unit_n<3, binary96_t> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<bin96>::value;
};

template <> struct genus_traits<bin128> : public unit_n<4, binary128_t> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<bin128>::value;
};

template <> struct genus_traits<bin160> : public unit_n<5, binary160_t> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<bin160>::value;
};

template <> struct genus_traits<bin192> : public unit_n<6, binary192_t> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<bin192>::value;
};

template <> struct genus_traits<bin224> : public unit_n<7, binary224_t> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<bin224>::value;
};

template <> struct genus_traits<bin256> : public unit_n<8, binary256_t> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<bin256>::value;
};

template <> struct genus_traits<bin320> : public unit_n<10, binary320_t> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<bin320>::value;
};

template <> struct genus_traits<bin384> : public unit_n<12, binary384_t> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<bin384>::value;
};

template <> struct genus_traits<bin512> : public unit_n<16, binary512_t> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<bin512>::value;
};

//------------------------------------------------------------------------------

template <> struct genus_traits<text> : public unit_1<uint32_t, 0> {
  using value_type = string_view;
  using return_type = value_type;
  static constexpr unsigned loose_units = 0;
  static constexpr physique_kind physique = physique_kind::stretchy;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<text>::value;

  static cxx11_constexpr return_type empty() cxx11_noexcept {
    return string_view();
  }
  static cxx14_constexpr return_type
  read(const details::relative_payload *payload) cxx11_noexcept {
    if (unlikely(payload->stretchy.string.is_pool_tag()))
      return string_view("TODO: call external string-pool helper");
    return string_view(payload->stretchy.string.begin(),
                       payload->stretchy.string.length());
  }

  static bool is_empty(const return_type &value) cxx11_noexcept {
    return value.empty();
  }
  static std::size_t estimate_space(const return_type &value) {
    return details::stretchy_value_string::estimate_space(value);
  }
  static void write(details::relative_payload *payload,
                    const return_type &value) cxx11_noexcept {
    return payload->stretchy.string.store(value);
  }
};

template <> struct genus_traits<varbin> : public unit_1<uint32_t, 0> {
  using value_type = string_view;
  using return_type = value_type;
  static constexpr unsigned loose_units = 0;
  static constexpr physique_kind physique = physique_kind::stretchy;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<varbin>::value;

  static cxx11_constexpr return_type empty() cxx11_noexcept {
    return string_view();
  }
  static cxx14_constexpr return_type
  read(const details::relative_payload *relative) cxx11_noexcept {
    return string_view(
        static_cast<const char *>(relative->stretchy.varbin.begin()),
        relative->stretchy.varbin.length());
  }

  static bool is_empty(const return_type &value) cxx11_noexcept {
    return value.empty();
  }
  static std::size_t estimate_space(const return_type &value) {
    return details::stretchy_value_varbin::estimate_space(value);
  }
  static void write(details::relative_payload *payload,
                    const return_type &value) cxx11_noexcept {
    return payload->stretchy.varbin.store(value);
  }
};

template <> struct genus_traits<nested> : public unit_1<uint32_t, 0> {
  using value_type = const details::stretchy_value_tuple *;
  using return_type = value_type;
  static constexpr physique_kind physique = physique_kind::stretchy;
  static constexpr unsigned loose_units = 0;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<nested>::value;

  static cxx11_constexpr return_type empty() cxx11_noexcept { return nullptr; }
  static cxx11_constexpr return_type
  read(const details::relative_payload *payload) cxx11_noexcept {
    return erthink::constexpr_pointer_cast<return_type>(
        &payload->stretchy.tuple);
  }

  static bool is_empty(const return_type &value) cxx11_noexcept {
    return !value || value->brutto_units < 2;
  }
  static std::size_t estimate_space(const return_type value) {
    return details::stretchy_value_tuple::estimate_space(value);
  }
  static void write(details::relative_payload *payload,
                    const return_type value) cxx11_noexcept {
    return payload->stretchy.tuple.store(value);
  }
};

template <> struct genus_traits<property> : public unit_1<uint32_t, 0> {
  using value_type = property_pair;
  using return_type = value_type;
  static constexpr physique_kind physique = physique_kind::stretchy;
  static constexpr unsigned loose_units = 0;
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<property>::value;

  static cxx11_constexpr return_type empty() cxx11_noexcept {
    return property_pair(string_view(), 0);
  }
  static cxx11_constexpr return_type
  read(const details::relative_payload *payload) cxx11_noexcept {
    return property_pair(
        string_view(erthink::constexpr_pointer_cast<const char *>(
                        payload->stretchy.property.bytes),
                    payload->stretchy.property.data_length),
        payload->stretchy.property.id);
  }

  static bool is_empty(const return_type &value) cxx11_noexcept {
    return value.first.empty() && !value.second;
  }
  static std::size_t estimate_space(const return_type &value) {
    return details::stretchy_value_property::estimate_space(value);
  }
  static void write(details::relative_payload *payload,
                    const return_type &value) cxx11_noexcept {
    return payload->stretchy.property.store(value);
  }
};

//------------------------------------------------------------------------------

template <> struct genus_traits<ip> : public unit_n<4, ip_address_t> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<ip>::value;
};

template <> struct genus_traits<ipnet> : public unit_n<5, ip_net_t> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<ip, ipnet>::value;
};

template <> struct genus_traits<mac> : public unit_2<mac_address_t, 0> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<mac>::value;
  static cxx14_constexpr bool is_empty(const return_type value) cxx11_noexcept {
    return value.raw64 == 0;
  }
};

template <> struct genus_traits<app_reserved_64> : public unit_2<int64_t, 0> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<app_reserved_64>::value;
  static cxx14_constexpr bool is_empty(const return_type value) cxx11_noexcept {
    return value == return_type(0);
  }
};

template <> struct genus_traits<app_reserved_128> : public unit_n<4> {
  static constexpr details::genus_mask_t trivially_convertible_from =
      utils::bitset_mask<app_reserved_128>::value;
};
} // namespace meta

namespace details {

extern FPTU_API const uint8_t genus2looseunits_array[];
static inline std::size_t loose_units_dynamic(const genus type) cxx11_noexcept {
  return genus2looseunits_array[type];
}

extern FPTU_API const uint8_t genus2preplacedbytes_array[];
static inline std::size_t
preplaced_bytes_dynamic(const genus type) cxx11_noexcept {
  return genus2preplacedbytes_array[type];
}

extern FPTU_API const genus_mask_t trivially_convertible_from_array[];
static inline bool
is_trivially_convertible_dynamic(const genus from,
                                 const genus to) cxx11_noexcept {
  return utils::test_bit(trivially_convertible_from_array[to], from);
}

static cxx14_constexpr size_t loose_units_constexpr(const genus type)
    cxx11_noexcept {
  switch (type) {
  default:
    constexpr_assert(false);
    __unreachable();
#define HERE_CASE_ITEM(N)                                                      \
  case genus(N):                                                               \
    return meta::genus_traits<genus(N)>::loose_units
    HERE_CASE_ITEM(0);
    HERE_CASE_ITEM(1);
    HERE_CASE_ITEM(2);
    HERE_CASE_ITEM(3);
    HERE_CASE_ITEM(4);
    HERE_CASE_ITEM(5);
    HERE_CASE_ITEM(6);
    HERE_CASE_ITEM(7);
    HERE_CASE_ITEM(8);
    HERE_CASE_ITEM(9);
    HERE_CASE_ITEM(10);
    HERE_CASE_ITEM(11);
    HERE_CASE_ITEM(12);
    HERE_CASE_ITEM(13);
    HERE_CASE_ITEM(14);
    HERE_CASE_ITEM(15);
    HERE_CASE_ITEM(16);
    HERE_CASE_ITEM(17);
    HERE_CASE_ITEM(18);
    HERE_CASE_ITEM(19);
    HERE_CASE_ITEM(20);
    HERE_CASE_ITEM(21);
    HERE_CASE_ITEM(22);
    HERE_CASE_ITEM(23);
    HERE_CASE_ITEM(24);
    HERE_CASE_ITEM(25);
    HERE_CASE_ITEM(26);
    HERE_CASE_ITEM(27);
    HERE_CASE_ITEM(28);
    HERE_CASE_ITEM(29);
    HERE_CASE_ITEM(30);
#undef HERE_CASE_ITEM
  case hole:
    return 0;
  }
}

static cxx14_constexpr size_t preplaced_bytes_constexpr(const genus type)
    cxx11_noexcept {
  switch (type) {
  default:
    constexpr_assert(false);
    __unreachable();
#define HERE_CASE_ITEM(N)                                                      \
  case genus(N):                                                               \
    return meta::genus_traits<genus(N)>::preplaced_bytes
    HERE_CASE_ITEM(0);
    HERE_CASE_ITEM(1);
    HERE_CASE_ITEM(2);
    HERE_CASE_ITEM(3);
    HERE_CASE_ITEM(4);
    HERE_CASE_ITEM(5);
    HERE_CASE_ITEM(6);
    HERE_CASE_ITEM(7);
    HERE_CASE_ITEM(8);
    HERE_CASE_ITEM(9);
    HERE_CASE_ITEM(10);
    HERE_CASE_ITEM(11);
    HERE_CASE_ITEM(12);
    HERE_CASE_ITEM(13);
    HERE_CASE_ITEM(14);
    HERE_CASE_ITEM(15);
    HERE_CASE_ITEM(16);
    HERE_CASE_ITEM(17);
    HERE_CASE_ITEM(18);
    HERE_CASE_ITEM(19);
    HERE_CASE_ITEM(20);
    HERE_CASE_ITEM(21);
    HERE_CASE_ITEM(22);
    HERE_CASE_ITEM(23);
    HERE_CASE_ITEM(24);
    HERE_CASE_ITEM(25);
    HERE_CASE_ITEM(26);
    HERE_CASE_ITEM(27);
    HERE_CASE_ITEM(28);
    HERE_CASE_ITEM(29);
    HERE_CASE_ITEM(30);
#undef HERE_CASE_ITEM
  case hole:
    return 0;
  }
}

static cxx14_constexpr bool
is_trivially_convertible_constexpr(const genus from,
                                   const genus to) cxx11_noexcept {
  switch (to) {
  default:
    constexpr_assert(false);
    __unreachable();
#define HERE_CASE_ITEM(N)                                                      \
  case genus(N):                                                               \
    return utils::test_bit(                                                    \
        meta::genus_traits<genus(N)>::trivially_convertible_from, from)
    HERE_CASE_ITEM(0);
    HERE_CASE_ITEM(1);
    HERE_CASE_ITEM(2);
    HERE_CASE_ITEM(3);
    HERE_CASE_ITEM(4);
    HERE_CASE_ITEM(5);
    HERE_CASE_ITEM(6);
    HERE_CASE_ITEM(7);
    HERE_CASE_ITEM(8);
    HERE_CASE_ITEM(9);
    HERE_CASE_ITEM(10);
    HERE_CASE_ITEM(11);
    HERE_CASE_ITEM(12);
    HERE_CASE_ITEM(13);
    HERE_CASE_ITEM(14);
    HERE_CASE_ITEM(15);
    HERE_CASE_ITEM(16);
    HERE_CASE_ITEM(17);
    HERE_CASE_ITEM(18);
    HERE_CASE_ITEM(19);
    HERE_CASE_ITEM(20);
    HERE_CASE_ITEM(21);
    HERE_CASE_ITEM(22);
    HERE_CASE_ITEM(23);
    HERE_CASE_ITEM(24);
    HERE_CASE_ITEM(25);
    HERE_CASE_ITEM(26);
    HERE_CASE_ITEM(27);
    HERE_CASE_ITEM(28);
    HERE_CASE_ITEM(29);
    HERE_CASE_ITEM(30);
#undef HERE_CASE_ITEM
  case hole:
    return from == hole;
  }
}

ERTHINK_DYNAMIC_CONSTEXPR(size_t, loose_units, (const genus type), (type), type)
ERTHINK_DYNAMIC_CONSTEXPR(size_t, preplaced_bytes, (const genus type), (type),
                          type)
ERTHINK_DYNAMIC_CONSTEXPR(bool, is_trivially_convertible,
                          (const genus from, const genus to), (from, to), to)

template <genus FROM, genus TO>
struct is_trivially_convertible_static
    : public std::integral_constant<
          bool, utils::test_bit(
                    meta::genus_traits<TO>::trivially_convertible_from::value,
                    FROM)> {};

static inline void preplaced_erase(const genus type,
                                   details::field_preplaced *preplaced,
                                   const bool distinct_null) cxx11_noexcept {
  switch (type) {
  default:
    assert(false);
    __unreachable();
#define HERE_CASE_ITEM(N)                                                      \
  case genus(N):                                                               \
    meta::genus_traits<genus(N)>::erase(preplaced, distinct_null);             \
    return
    HERE_CASE_ITEM(0);
    HERE_CASE_ITEM(1);
    HERE_CASE_ITEM(2);
    HERE_CASE_ITEM(3);
    HERE_CASE_ITEM(4);
    HERE_CASE_ITEM(5);
    HERE_CASE_ITEM(6);
    HERE_CASE_ITEM(7);
    HERE_CASE_ITEM(8);
    HERE_CASE_ITEM(9);
    HERE_CASE_ITEM(10);
    HERE_CASE_ITEM(11);
    HERE_CASE_ITEM(12);
    HERE_CASE_ITEM(13);
    HERE_CASE_ITEM(14);
    HERE_CASE_ITEM(15);
    HERE_CASE_ITEM(16);
    HERE_CASE_ITEM(17);
    HERE_CASE_ITEM(18);
    HERE_CASE_ITEM(19);
    HERE_CASE_ITEM(20);
    HERE_CASE_ITEM(21);
    HERE_CASE_ITEM(22);
    HERE_CASE_ITEM(23);
    HERE_CASE_ITEM(24);
    HERE_CASE_ITEM(25);
    HERE_CASE_ITEM(26);
    HERE_CASE_ITEM(27);
    HERE_CASE_ITEM(28);
    HERE_CASE_ITEM(29);
    HERE_CASE_ITEM(30);
#undef HERE_CASE_ITEM
  }
}

} // namespace details

//------------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4275) /* non dll-interface class 'FOO' used as base  \
                                   \ for dll-interface class */
#endif

class FPTU_API_TYPE preplaced_string
    : public details::preplaced_stretchy_value {
public:
  using traits = meta::genus_traits<genus::text>;
  using value_type = typename traits::value_type;
  value_type value_nothrow() const cxx11_noexcept {
    return unlikely(nil()) ? traits::empty() : traits::read(payload());
  }
  value_type value() const;
};

class FPTU_API_TYPE preplaced_varbin
    : public details::preplaced_stretchy_value {
public:
  using traits = meta::genus_traits<genus::varbin>;
  using value_type = typename traits::value_type;
  value_type value_nothrow() const cxx11_noexcept {
    return unlikely(nil()) ? traits::empty() : traits::read(payload());
  }
  value_type value() const;
};

class tuple_ro_weak;
class FPTU_API_TYPE preplaced_nested
    : public details::preplaced_stretchy_value {
public:
  using traits = meta::genus_traits<genus::nested>;
  using value_type = tuple_ro_weak;
  inline value_type value_nothrow() const cxx11_noexcept;
  value_type value() const;
};

class FPTU_API_TYPE preplaced_property
    : public details::preplaced_stretchy_value {
public:
  using traits = meta::genus_traits<genus::property>;
  using value_type = typename traits::value_type;
  value_type value_nothrow() const cxx11_noexcept {
    return unlikely(nil()) ? traits::empty() : traits::read(payload());
  }
  value_type value() const;
};

//------------------------------------------------------------------------------

} // namespace fptu

#include "fast_positive/tuples/details/warnings_pop.h"
