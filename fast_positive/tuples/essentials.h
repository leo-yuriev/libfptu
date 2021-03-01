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
#include "fast_positive/tuples/api.h"

#include "fast_positive/tuples/details/utils.h"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fptu {

enum fundamentals : std::ptrdiff_t {
  ident_bitness = 11,
  genus_bitness = 5,
  unit_size = 4,
  unit_shift = 2,
  tuple_flags_bits = 3,
  max_tuple_units_brutto = 65535
  /* С размером кортежа связаны такие ограничения и компромиссы:
   *  1) Желательно хранить все смещения внутри кортежа как 16-битные числа,
   *     т.е. между заголовком поля и его данными может быть до 65535
   *     4-х байтовых юнитов. Это позволяет создать кортеж размером почти
   *     до 64K + 256К * 2, например если последнее поле в индексе будет
   *     максимального размера (16-битного смещение будет достаточно для
   *     ссылки на его начало из конца индекса). Однако, возникает масса
   *     сопутствующих проблем и особых случаев.
   *  2) В заголовке кортежа необходимо хранить кол-во loose-полей и полный
   *     размер кортежа. Если оставлять заголовок 4-байтным, а не увеличивать
   *     вдвое, то с учетом места под флажки/признаки для кодирования длины
   *     остается примерно 16 бит. Можно выкроить 17, но тогда возникают
   *     трудности при проверке и компактификации.
   *  3) Проверка кортежа и компактификация требует создания "карты полей".
   *     Если ограничить полный размер кортежа 65535 юнитами, то карта может
   *     быть построена в 16-ти битных координатах. Иначе требуется использовать
   *     32-е числа, что удваивает размер и трафик по памяти.
   *  4) Отказ от лимита в 65535 юнитов на полный размер кортежа также требует
   *     дополнительных проверок при управлениями данными полей и смещениями
   *     "дырок". Кроме проверок, при этом неизбежно могут возникать ситуации
   *     когда смещение не может быть сгенерировано/сохранено без перемещения
   *     полей внутри кортежа.
   *
   * По совокупности решено НЕ "натягивать сову на глобус" и принять ограничение
   * максимального полного/brutto размера кортежа в 65535 4-байтных юнитов:
   *  - максимальный брутто-размер кортежа 262140 байт = 256K - 4 байта.
   *  - максимальный нетто-размер кортежа 2621436 байт = 256K - 8 байт.
   *  - максимальный размер данных одного поля 2621432 байта = 256K - 12 байт,
   *    т.е. если имеется кортеж максимального размера с одним полем, то:
   *      - полный размер сериализованной формы кортежа = 262140 байт;
   *      - в эти 262140 байт входит 4 байта заголовка кортежа и 4 байта
   *        дескриптора/заголовка поля;
   *      - размер непосредстенно полезных данных самого поля = 262132 байта.
   */
  ,
  max_tuple_bytes_brutto = unit_size * max_tuple_units_brutto,
  max_tuple_units_netto = max_tuple_units_brutto - 1,
  max_tuple_bytes_netto = unit_size * max_tuple_units_netto,
  max_field_units = max_tuple_units_netto - 1,
  max_field_bytes = unit_size * max_field_units,
  max_fields = (1 << (16 - tuple_flags_bits)) - 1,
  buffer_enough =
      sizeof(size_t) * 16 + max_tuple_bytes_netto + max_fields * unit_size,
  buffer_limit = max_tuple_bytes_netto * 2,
  max_preplaced_size = (1u << ident_bitness) - 1,
};

/* The min/max "safe" values which could be converted
 * to IEEE-754 double (64 bit) without loss precision */
static constexpr int64_t safe64_number_max = INT64_C(0x001FFFFFFFFFFFFF);
static constexpr int64_t safe64_number_min = -safe64_number_max;

/* The min/max "safe" values which could be converted
 * to IEEE-754 single (32 bit) without loss precision */
static constexpr int32_t safe32_number_max = INT32_C(0x001FFFFF);
static constexpr int32_t safe32_number_min = -safe32_number_max;

enum configure {
  onstask_allocation_threshold = 2048,
  sort_index_threshold = 256
};

enum genus : unsigned /* тип данных, 5 бит */ {
  // variable length type
  text = 0,
  varbin = 1 /* variable length binary (up to 256K) & arrays */,
  nested = 2 /* nested tuple */,
  property = 3 /* { id:8, len:8, data[0 <= len <= 253] } */,

  i8 = 4,
  boolean = i8,
  u8 = 5,
  bin8 = u8,

  i16 = 6,
  enumeration = i16,
  u16 = 7,
  bin16 = u16,

  i32 = 8,
  u32 = 9,
  bin32 = u32,
  f32 = 10 /* Single precision IEEE-754 */,
  t32 = 11 /* 32-bit UTC time_t */,
  datetime_utc = t32,

  i64 = 12,
  u64 = 13,
  bin64 = u64,
  f64 = 14 /* Double precision IEEE-754 */,
  d64 = 15 /* https://en.wikipedia.org/wiki/Decimal_floating_point */,
  decimal = d64,
  t64 = 16 /* 64-bit fixed-point UTC datetime & timestamp */,
  datetime_h100 = t64,
  timestamp = t64,

  // fixed binary
  bin96 = 17,
  bin128 = 18,
  bin160 = 19,
  bin192 = 20,
  bin224 = 21,
  bin256 = 22,
  bin320 = 23,
  bin384 = 24,
  bin512 = 25,

  // application-specific with predefined size and DENIL=0
  app_reserved_64 = 26,
  app_reserved_128 = 27,
  mac = 28,
  ip = 29,
  ipnet = 30,

  // auxiliary internal (don't use it!)
  hole = 31
};

namespace details {

using loose_genus_and_id_t = uint16_t;
using tag_t = uint32_t;
using genus_mask_t = uint32_t;
using unit_t = uint32_t;

constexpr std::size_t bytes2units(const std::size_t bytes) cxx11_noexcept {
  return (bytes + unit_size - 1) >> unit_shift;
}
constexpr std::size_t units2bytes(const std::size_t units) cxx11_noexcept {
  return units << unit_shift;
}

/* FIELD's TOKEN & TAG ---------------------------------------------------------

        1                 1 0                 0
   MSB> FEDC'BA98'7654'3210 FEDC'BA98'7654'3210 <LSB
        oooo'oooo'oooo'ooSD GGGG'Gsss'ssss'ssss <<== preplaced
        1111'1111'1111'1CSD GGGG'GIii'iiii'iiii <<== loose/inlay
                            1111'1sss'ssss'ssss <<== hole

        Такое распределение бит позволяет:
         - сортировкой прижать дескрипторы "дырок" к одному
           краю, а поля переменной длины к другому.
         - сортировка тэгов по возрастанию разместит
           preplaced в начале и в порядке физического
           расположения в кортеже.

        o - Offset of preplaced fields
        i - id for loose fields
        I - inlay flag
        G - Genus
        s - the size in bytes ONLY for holes and preplaced fields

        C - Collection/repeated (for loose)
        D - Discernible NIL/NULL
        S - Saturation instead of range checking
*/

enum tag_bits : uint32_t {
  id_shift = 0,
  id_mask = (1u << ident_bitness) - 1u,
  inlay_flag = 1u << (ident_bitness - 1u),

  genus_shift = ident_bitness,
  genus_mask = (1u << genus_bitness) - 1u,

  discernible_null_flag = 1u << 16,
  saturation_flag = 1u << 17,
  loose_collection_flag = 1u << 18,

  offset_shift = 18,
  offset_bits = 14,
  max_preplaced_offset = (1u << offset_bits) - 3u,
  max_ident = (1u << ident_bitness) - 1u,
  loose_threshold = (max_preplaced_offset + 1u) << offset_shift,
  inlay_pattern = loose_threshold + inlay_flag,
  collection_threshold = loose_threshold + loose_collection_flag,

  loose_begin = 0,
  loose_end = 1024 /* (1u << fundamentals::ident_bitness) / 2 */,
  inlay_begin = loose_end,
  inlay_end = 2048 /* 1u << fundamentals::ident_bitness */,

  loose_first = loose_begin,
  loose_last = loose_end - 1,
  inlay_first = inlay_begin,
  inlay_last = inlay_end - 1
};

//------------------------------------------------------------------------------

static constexpr bool is_fixed_size(const genus type) cxx11_noexcept {
  constexpr_assert(type != hole);
  return type > property;
}

static constexpr bool is_fixed_size(const tag_t tag) cxx11_noexcept {
  return (tag & 0xE000) != 0;
}

static cxx11_constexpr bool is_inplaced(const genus type) cxx11_noexcept {
  return utils::test_bit(
      utils::bitset_mask<i16, u16, i8, u8, boolean, enumeration>::value, type);
}

static constexpr loose_genus_and_id_t
tag2genus_and_id(const tag_t tag) cxx11_noexcept {
  return loose_genus_and_id_t(tag >> tag_bits::id_shift);
}

static constexpr genus tag2genus(const tag_t tag) cxx11_noexcept {
  return genus(loose_genus_and_id_t(tag) >> tag_bits::genus_shift);
}

static cxx11_constexpr bool is_inplaced(const tag_t tag) cxx11_noexcept {
  return is_inplaced(tag2genus(tag));
}

static constexpr bool is_loose(const tag_t tag) cxx11_noexcept {
  return tag >= tag_bits::loose_threshold;
}

static constexpr bool is_preplaced(const tag_t tag) cxx11_noexcept {
  return tag < tag_bits::loose_threshold;
}

static constexpr bool is_saturated(const tag_t tag) cxx11_noexcept {
  return (tag & tag_bits::saturation_flag) != 0;
}

static constexpr bool is_inlay(const tag_t tag) cxx11_noexcept {
  constexpr_assert(is_loose(tag));
  return (tag & tag_bits::inlay_flag) != 0;
}

static constexpr bool is_loose_inlay(const tag_t tag) cxx11_noexcept {
  return (tag & tag_bits::inlay_pattern) == tag_bits::inlay_pattern;
}

static constexpr bool is_loose_collection(const tag_t tag) cxx11_noexcept {
  return tag >= tag_bits::collection_threshold;
}

static constexpr bool is_discernible_null(const tag_t tag) cxx11_noexcept {
  return (tag & tag_bits::discernible_null_flag) != 0;
}

static constexpr std::size_t tag2offset(const tag_t tag) cxx11_noexcept {
  constexpr_assert(is_preplaced(tag));
  return tag >> tag_bits::offset_shift;
}

static constexpr std::size_t tag2indysize(const tag_t tag) cxx11_noexcept {
  constexpr_assert(is_preplaced(tag));
  return (tag >> tag_bits::id_shift) & tag_bits::id_mask;
}

static constexpr unsigned tag2id(const tag_t tag) cxx11_noexcept {
  constexpr_assert(is_loose(tag));
  return (tag >> tag_bits::id_shift) & tag_bits::id_mask;
}

static constexpr unsigned
descriptor2id(const loose_genus_and_id_t loose_descriptor) cxx11_noexcept {
  return (loose_descriptor >> tag_bits::id_shift) & tag_bits::id_mask;
}

static constexpr genus
descriptor2genus(const loose_genus_and_id_t loose_descriptor) cxx11_noexcept {
  return genus(loose_descriptor >> tag_bits::genus_shift);
}

static constexpr tag_t make_tag(const genus type, const unsigned id,
                                const bool collection,
                                const bool discernible_null,
                                const bool saturated) cxx11_noexcept {
  constexpr_assert(type <= hole && id <= tag_bits::max_ident);
  return tag_t(tag_bits::loose_threshold + (type << tag_bits::genus_shift) +
               (id << tag_bits::id_shift) +
               (collection ? tag_bits::loose_collection_flag : 0u) +
               (discernible_null ? tag_bits::discernible_null_flag : 0u) +
               (saturated ? tag_bits::saturation_flag : 0u));
}

static constexpr tag_t make_tag(const loose_genus_and_id_t loose_descriptor,
                                const bool collection,
                                const bool discernible_null,
                                const bool saturated) cxx11_noexcept {
  constexpr_assert(descriptor2genus(loose_descriptor) != hole);
  return tag_t(tag_bits::loose_threshold + loose_descriptor +
               (collection ? tag_bits::loose_collection_flag : 0u) +
               (discernible_null ? tag_bits::discernible_null_flag : 0u) +
               (saturated ? tag_bits::saturation_flag : 0u));
}

static constexpr tag_t make_hole(const std::size_t units) cxx11_noexcept {
  constexpr_assert(units <= tag_bits::max_ident);
  return tag_t(tag_bits::collection_threshold +
               (genus::hole << tag_bits::genus_shift) +
               (units << tag_bits::id_shift));
}

static constexpr tag_t tag_from_offset(const std::size_t offset,
                                       const genus type,
                                       const std::size_t indysize,
                                       const bool discernible_null,
                                       const bool saturated) cxx11_noexcept {
  constexpr_assert(type <= hole && offset <= tag_bits::max_preplaced_offset);
  constexpr_assert(indysize > 0 && indysize <= tag_bits::max_ident);
  return tag_t((type << tag_bits::genus_shift) +
               (offset << tag_bits::offset_shift) +
               (indysize << tag_bits::id_shift) +
               (discernible_null ? tag_bits::discernible_null_flag : 0u) +
               (saturated ? tag_bits::saturation_flag : 0u));
}

static constexpr tag_t normalize_tag(const tag_t tag,
                                     const bool as_preplaced) cxx11_noexcept {
  constexpr_assert(is_preplaced(tag) == as_preplaced);
  return tag |
         (as_preplaced
              ? tag_bits::discernible_null_flag | tag_bits::saturation_flag
              : tag_bits::discernible_null_flag | tag_bits::saturation_flag |
                    tag_bits::loose_collection_flag);
}

static constexpr tag_t normalize_tag(const tag_t tag) cxx11_noexcept {
  return normalize_tag(tag, is_preplaced(tag));
}

static constexpr bool tag_less(const tag_t a, const tag_t b) cxx11_noexcept {
  return normalize_tag(a) < normalize_tag(b);
}

static constexpr bool tag_same(const tag_t a, const tag_t b) cxx11_noexcept {
  return normalize_tag(a) == normalize_tag(b);
}

//------------------------------------------------------------------------------

static constexpr genus_mask_t mask_all_types =
    UINT32_MAX - utils::bitset_mask<hole>::value;

static constexpr genus_mask_t mask_integer =
    utils::bitset_mask<i8, u8, i16, u16, i32, u32, i64, u64>::value;

static constexpr genus_mask_t mask_float =
    utils::bitset_mask<f32, f64, d64>::value;

static constexpr genus_mask_t mask_signed =
    utils::bitset_mask<i8, i16, i32, i64>::value | mask_float;

static constexpr genus_mask_t mask_unsigned = mask_integer & ~mask_signed;

static constexpr genus_mask_t mask_number = mask_integer | mask_float;

} // namespace details
} // namespace fptu
