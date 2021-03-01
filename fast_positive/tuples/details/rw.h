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
#include "fast_positive/tuples/1Hippeus/buffer.h"
#include "fast_positive/tuples/api.h"
#include "fast_positive/tuples/details/exceptions.h"
#include "fast_positive/tuples/details/field.h"
#include "fast_positive/tuples/details/getter.h"
#include "fast_positive/tuples/details/meta.h"
#include "fast_positive/tuples/details/ro.h"
#include "fast_positive/tuples/details/scan.h"
#include "fast_positive/tuples/essentials.h"
#include "fast_positive/tuples/schema.h"
#include "fast_positive/tuples/token.h"

#include <algorithm> // for std::min/max

#include "fast_positive/tuples/details/warnings_push_pt.h"

namespace fptu {

class tuple_ro_managed;
class tuple_rw_fixed;
class tuple_rw_managed;

namespace details {

/* Границы безопасной конвертации double->integer->double без переполнения */
template <typename F, typename I> struct roundtrip_bounds;

template <> struct roundtrip_bounds<double, uint64_t> {
  static constexpr double upper() noexcept {
    return static_cast<double>(std::numeric_limits<uint64_t>::max() -
                               INT64_C(2047));
  }
  static constexpr double lower() noexcept { return 0.0; }
};

template <> struct roundtrip_bounds<double, int64_t> {
  static constexpr double upper() noexcept {
    return static_cast<double>(std::numeric_limits<int64_t>::max() -
                               INT64_C(1023));
  }
  static constexpr double lower() noexcept {
    return static_cast<double>(std::numeric_limits<int64_t>::min() +
                               INT64_C(1024));
  }
};

template <> struct roundtrip_bounds<double, uint32_t> {
  static constexpr double upper() noexcept {
    return static_cast<double>(std::numeric_limits<uint32_t>::max());
  }
  static constexpr double lower() noexcept { return 0.0; }
};

template <> struct roundtrip_bounds<double, int32_t> {
  static constexpr double upper() noexcept {
    return static_cast<double>(std::numeric_limits<int32_t>::max());
  }
  static constexpr double lower() noexcept {
    return static_cast<double>(std::numeric_limits<int32_t>::min() +
                               INT64_C(1));
  }
};

template <> struct roundtrip_bounds<float, uint64_t> {
  static constexpr float upper() noexcept {
    return static_cast<float>(std::numeric_limits<uint64_t>::max() -
                              INT64_C(1099511627775));
  }
  static constexpr float lower() noexcept { return 0.0; }
};

template <> struct roundtrip_bounds<float, int64_t> {
  static constexpr float upper() noexcept {
    return static_cast<float>(std::numeric_limits<int64_t>::max() -
                              INT64_C(549755813887));
  }
  static constexpr float lower() noexcept {
    return static_cast<float>(std::numeric_limits<int64_t>::min() +
                              INT64_C(549755813888));
  }
};

template <> struct roundtrip_bounds<float, uint32_t> {
  static constexpr float upper() noexcept {
    return static_cast<float>(std::numeric_limits<uint32_t>::max() -
                              INT64_C(255));
  }
  static constexpr float lower() noexcept { return 0.0; }
};

template <> struct roundtrip_bounds<float, int32_t> {
  static constexpr float upper() noexcept {
    return static_cast<float>(std::numeric_limits<int32_t>::max() -
                              INT64_C(127));
  }
  static constexpr float lower() noexcept {
    return static_cast<float>(std::numeric_limits<int32_t>::min() +
                              INT64_C(128));
  }
};

template <genus GENUS, typename VALUE>
constexpr bool is_safe_roundtrip(VALUE value) noexcept {
  return value >= roundtrip_bounds<VALUE, typename meta::genus_traits<
                                              GENUS>::value_type>::lower() &&
         value <= roundtrip_bounds<VALUE, typename meta::genus_traits<
                                              GENUS>::value_type>::upper();
}

class FPTU_API_TYPE tuple_rw : public crtp_getter<tuple_rw> {
  friend class crtp_getter<tuple_rw>;
  friend class fptu::tuple_ro_managed;
  friend class fptu::tuple_rw_fixed;
  friend class fptu::tuple_rw_managed;

public:
  /* closed and noncopyable. */
  tuple_rw(const tuple_rw &) = delete;
  tuple_rw &operator=(tuple_rw const &) = delete;

  static void *operator new[](std::size_t object_size) = delete;
  static void operator delete[](void *ptr) = delete;

  struct deleter {
    void operator()(/* accepts nullptr */ tuple_rw *ptr) {
      if (ptr)
        ptr->~tuple_rw();
    }
  };

  /* TODO: сделать inline во внутреннем включаемом файле */
  static tuple_rw *create_new(std::size_t items_limit, std::size_t data_bytes,
                              const fptu::schema *schema,
                              const hippeus::buffer_tag &allot_tag);

  /* TODO: сделать inline во внутреннем включаемом файле */
  static tuple_rw *create_from_ro(const audit_holes_info &holes_info,
                                  const tuple_ro *ro, std::size_t more_items,
                                  std::size_t more_payload,
                                  const fptu::schema *schema,
                                  const hippeus::buffer_tag &allot_tag);

  /* TODO: сделать inline во внутреннем включаемом файле */
  static tuple_rw *create_from_buffer(const void *source_ptr,
                                      std::size_t source_bytes,
                                      std::size_t more_items,
                                      std::size_t more_payload,
                                      const fptu::schema *schema,
                                      const hippeus::buffer_tag &allot_tag);

  /* TODO: сделать inline во внутреннем включаемом файле */
  static tuple_rw *create_legacy(void *buffer_space, std::size_t buffer_size,
                                 std::size_t items_limit);

  /* TODO: сделать inline во внутреннем включаемом файле */
  static tuple_rw *fetch_legacy(const audit_holes_info &holes_info,
                                const tuple_ro *ro, void *buffer_space,
                                std::size_t buffer_size,
                                std::size_t more_items);

  tuple_rw *create_copy(std::size_t more_items, std::size_t more_payload,
                        const hippeus::buffer_tag &allot_tag) const {
    const audit_holes_info holes_info = {junk_.count, junk_.volume};
    return create_from_ro(holes_info, take_asis(), more_items, more_payload,
                          schema_, allot_tag);
  }

  const hippeus::buffer *get_buffer() const cxx11_noexcept {
    return const_cast<tuple_rw *>(this)->get_buffer();
  }

protected:
  /* TODO: перенести как inline во внутренний включаемый файл */
  inline tuple_rw(int buffer_offset, std::size_t buffer_size,
                  std::size_t items_limit, const fptu::schema *schema)
      : schema_(schema), buffer_offset_(buffer_offset), options_(0) {
    const auto required_space = estimate_required_space(items_limit, 0, schema);
    if (unlikely(required_space > buffer_size || buffer_size > buffer_limit))
      throw_invalid_argument();

    end_ = std::min(unsigned(max_tuple_units_netto),
                    unsigned(buffer_size - pure_tuple_size()) >>
                        fptu::fundamentals::unit_shift);
    head_ = tail_ = pivot_ = unsigned(items_limit);
    if (schema_ && schema_->number_of_preplaced()) {
      tail_ += unsigned(schema_->preplaced_units());
      std::memcpy(pivot(), schema_->preplaced_init_image(),
                  schema_->preplaced_bytes());
    }
    debug_check();
  }

  /* TODO: перенести как inline во внутренний включаемый файл */
  inline tuple_rw(const audit_holes_info &holes_info, const tuple_ro *ro,
                  int buffer_offset, std::size_t buffer_size,
                  std::size_t more_items, const fptu::schema *schema);

  const fptu::schema *schema_;

  int buffer_offset_;
  static int get_buffer_offset(const hippeus::buffer *buffer) cxx11_noexcept {
    const ptrdiff_t full_offset =
        erthink::constexpr_pointer_cast<const char *>(buffer) -
        erthink::constexpr_pointer_cast<const char *>(buffer->data());
    assert(full_offset != 0);
    assert(full_offset > INT_MIN && full_offset < INT_MAX);
    return int(full_offset);
  }
  hippeus::buffer *get_buffer() cxx11_noexcept {
    return buffer_offset_ ? erthink::constexpr_pointer_cast<hippeus::buffer *>(
                                erthink::constexpr_pointer_cast<char *>(this) +
                                buffer_offset_)
                          : nullptr;
  }
  ~tuple_rw();
  unsigned options_;

  union FPTU_API_TYPE junk_counters {
    /* Счетчики мусорных 32-битных элементов, которые образовались при
     * удалении/обновлении. */
    struct {
      uint16_t count, volume;
    };
    uint32_t both;
    cxx11_constexpr junk_counters() : both(0) {
      static_assert(sizeof(junk_counters) == 4, "WTF?");
    }
    cxx11_constexpr junk_counters(const struct audit_holes_info &holes_info)
        : count(uint16_t(holes_info.count)),
          volume(uint16_t(holes_info.volume)) {}
  };
  junk_counters junk_;

  unsigned end_ /* Индекс после конца выделенного буфера */;
  unsigned pivot_ /* Индекс опорной точки, от которой растут "голова" и
                     "хвоcт", указывает на терминатор заголовка. */
      ;
  unsigned head_ /* Индекс дозаписи дескрипторов, растет к началу буфера,
                    указывает на первый занятый элемент. */
      ;
  unsigned tail_ /* Индекс для дозаписи данных, растет к концу буфера,
                    указываент на первый не занятый элемент. */
      ;
  unit_t reserve_for_RO_header;
  unit_t area_[1];

  __pure_function void *pivot() cxx11_noexcept { return &area_[pivot_]; }
  __pure_function const void *pivot() const cxx11_noexcept {
    return const_cast<tuple_rw *>(this)->pivot();
  }

  __pure_function field_loose *begin_index() cxx11_noexcept {
    return erthink::constexpr_pointer_cast<field_loose *>(&area_[head_]);
  }
  __pure_function field_loose *end_index() cxx11_noexcept {
    return static_cast<field_loose *>(pivot());
  }

  __pure_function const field_loose *begin_index() const cxx11_noexcept {
    return const_cast<tuple_rw *>(this)->begin_index();
  }
  __pure_function const field_loose *end_index() const cxx11_noexcept {
    return const_cast<tuple_rw *>(this)->end_index();
  }

  __pure_function const unit_t *begin_data_units() const cxx11_noexcept {
    return static_cast<const unit_t *>(pivot());
  }
  __pure_function const unit_t *end_data_units() const cxx11_noexcept {
    return &area_[tail_];
  }
  __pure_function const char *begin_data_bytes() const cxx11_noexcept {
    return erthink::constexpr_pointer_cast<const char *>(begin_data_units());
  }
  __pure_function const char *end_data_bytes() const cxx11_noexcept {
    return erthink::constexpr_pointer_cast<const char *>(end_data_units());
  }

  relative_payload *alloc_data(const std::size_t units);
  void release_data(relative_payload *chunk, const std::size_t units);
  field_loose *alloc_loose(const tag_t tag, const std::size_t units);
  void release_loose(field_loose *loose, const std::size_t units);
  relative_payload *realloc_data(relative_offset &ref, const std::size_t have,
                                 const std::size_t needed);
  __pure_function static const char *audit(const tuple_rw *self) cxx11_noexcept;

  //----------------------------------------------------------------------------

  template <
      typename TOKEN, genus GENUS,
      typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique !=
                                    meta::physique_kind::stretchy> * = nullptr>
  inline void *assign_field_value(
      void *field, const typename meta::genus_traits<GENUS>::return_type value,
      const TOKEN token);

  template <
      typename TOKEN, genus GENUS,
      typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique ==
                                    meta::physique_kind::stretchy> * = nullptr>
  inline void *assign_field_value(
      void *field, const typename meta::genus_traits<GENUS>::return_type value,
      const TOKEN token);

  template <
      typename TOKEN, genus GENUS,
      typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique !=
                                    meta::physique_kind::stretchy> * = nullptr>
  inline field_loose *
  append_field(const typename meta::genus_traits<GENUS>::return_type value,
               const TOKEN token);

  template <
      typename TOKEN, genus GENUS,
      typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique ==
                                    meta::physique_kind::stretchy> * = nullptr>
  inline field_loose *
  append_field(const typename meta::genus_traits<GENUS>::return_type value,
               const TOKEN token);

  void erase(field_preplaced *preplaced, const genus type,
             const bool is_discernible_null) {
    debug_check();
    if (is_fixed_size(type)) {
      preplaced_erase(type, preplaced, is_discernible_null);
    } else if (preplaced->relative.have_payload()) {
      relative_payload *const payload = preplaced->relative.payload();
      release_data(payload, payload->stretchy.brutto_units(type));
      preplaced->relative.reset_payload();
    }
    debug_check();
  }

public:
  template <typename> class accessor_rw;
  template <typename> class collection_iterator_rw;
  template <typename> class collection_rw;

  void erase(field_loose *loose) {
    debug_check();
    assert(loose && loose != end_index());
    assert(!loose->is_hole());
    const genus type = loose->type();
    const std::size_t space =
        is_fixed_size(type) ? loose_units(type) : loose->stretchy_units();
    release_loose(loose, space);
    debug_check();
  }

  template <typename TOKEN> class accessor_rw : public accessor_ro<TOKEN> {
    using base = accessor_ro<TOKEN>;
    template <typename> friend class crtp_getter;
    friend class tuple_rw;
    friend class fptu::loose_iterator_rw;
    tuple_rw *tuple_;

    template <genus GENUS>
    inline void
    assign(const typename meta::genus_traits<GENUS>::return_type value);

    inline void remove();

    cxx11_constexpr accessor_rw() : base(), tuple_(nullptr) {
      static_assert(sizeof(*this) <= (TOKEN::is_static_token::value
                                          ? sizeof(void *) * 2
                                          : sizeof(void *) * 3),
                    "WTF?");
    }

    cxx11_constexpr accessor_rw(tuple_rw *tuple, field_preplaced *target,
                                const TOKEN token) cxx11_noexcept
        : base(target, token),
          tuple_(tuple) {}

    cxx11_constexpr accessor_rw(tuple_rw *tuple, field_loose *target,
                                const TOKEN token) cxx11_noexcept
        : base(target, token),
          tuple_(tuple) {}

    cxx11_constexpr accessor_rw(tuple_rw *tuple,
                                const base &accessor) cxx11_noexcept
        : base(accessor),
          tuple_(tuple) {}

  public:
    cxx11_constexpr accessor_rw(const accessor_rw &) cxx11_noexcept = default;
    cxx14_constexpr accessor_rw &
    operator=(const accessor_rw &) cxx11_noexcept = default;
    cxx11_constexpr accessor_rw(accessor_rw &&) cxx11_noexcept = default;
    cxx11_constexpr accessor_rw *operator->() const cxx11_noexcept {
      return this;
    }

    cxx11_constexpr tuple_rw *tuple() const cxx11_noexcept { return tuple_; }

    //--------------------------------------------------------------------------

    void set_string(const string_view &value) { return assign<text>(value); }
    void set_varbinary(const string_view &value) {
      return assign<varbin>(value);
    }
    void set_nested(const tuple_ro *value) {
      static_assert(std::is_base_of<stretchy_value_tuple, tuple_ro>::value,
                    "WTF?");
      return assign<nested>(
          erthink::constexpr_pointer_cast<const stretchy_value_tuple *>(value));
    }
    void set_property(const property_pair &value) {
      return assign<property>(value);
    }
    void set_bool(const bool value) { return assign<boolean>(value); }
    void set_enum(const short value) { return assign<enumeration>(value); }
    void set_i8(const int8_t value) { return assign<i8>(value); }
    void set_u8(const uint8_t value) { return assign<u8>(value); }
    void set_i16(const int16_t value) { return assign<i16>(value); }
    void set_u16(const uint16_t value) { return assign<u16>(value); }
    void set_i32(const int32_t value) { return assign<i32>(value); }
    void set_u32(const uint32_t value) { return assign<u32>(value); }
    void set_i64(const int64_t value) { return assign<i64>(value); }
    void set_u64(const uint64_t value) { return assign<u64>(value); }
    void set_f32(const float value) { return assign<f32>(value); }
    void set_f64(const double value) { return assign<f64>(value); }
    void set_decimal(const decimal64 value) { return assign<d64>(value); }
    void set_datetime(const datetime_t value) {
      return unlikely(base::type() == genus::t32)
                 ? assign<t32>(value.utc_seconds())
                 : assign<t64>(value);
    }
    void set_uuid(const uuid_t &value) { return assign<bin128>(value.bin128); }

    void set_bin96(const binary96_t &value) { return assign<bin96>(value); }
    void set_bin128(const binary128_t &value) { return assign<bin128>(value); }
    void set_bin160(const binary160_t &value) { return assign<bin160>(value); }
    void set_bin192(const binary192_t &value) { return assign<bin192>(value); }
    void set_bin224(const binary224_t &value) { return assign<bin224>(value); }
    void set_bin256(const binary256_t &value) { return assign<bin256>(value); }
    void set_bin320(const binary320_t &value) { return assign<bin320>(value); }
    void set_bin384(const binary384_t &value) { return assign<bin384>(value); }
    void set_bin512(const binary512_t &value) { return assign<bin512>(value); }

    void set_ip_address(const ip_address_t &value) { return assign<ip>(value); }
    void set_mac_address(const mac_address_t value) {
      return assign<mac>(value);
    }
    void set_ip_net(const ip_net_t &value) { return assign<ipnet>(value); }

    void set_float(const double value) {
      if (base::type() == f32) {
        if (!std::isinf(value) &&
            unlikely(value > std::numeric_limits<float>::max() ||
                     value < std::numeric_limits<float>::lowest()))
          throw_value_range();
        return set_f32(static_cast<float>(value));
      }
      return set_f64(value);
    }

    void set_integer(const int64_t value) {
      switch (base::type()) {
      default:
        throw_type_mismatch();
      case i8:
        if (unlikely(value != static_cast<int8_t>(value)))
          throw_value_range();
        return set_i8(static_cast<int8_t>(value));
      case i16:
        if (unlikely(value != static_cast<int16_t>(value)))
          throw_value_range();
        return set_i16(static_cast<int16_t>(value));
      case i32:
        if (unlikely(value != static_cast<int32_t>(value)))
          throw_value_range();
        return set_i32(static_cast<int32_t>(value));
      case i64:
        return set_i64(value);
      }
    }

    void set_integer(const uint64_t value) {
      if (unlikely(value > uint64_t(std::numeric_limits<int64_t>::max())))
        throw_value_range();
      return set_integer(static_cast<int64_t>(value));
    }

    void set_unsigned(const uint64_t value) {
      switch (base::type()) {
      default:
        throw_type_mismatch();
      case u8:
        if (unlikely(value != static_cast<uint8_t>(value)))
          throw_value_range();
        return set_u8(static_cast<uint8_t>(value));
      case u16:
        if (unlikely(value != static_cast<uint16_t>(value)))
          throw_value_range();
        return set_u16(static_cast<uint16_t>(value));
      case u32:
        if (unlikely(value != static_cast<uint32_t>(value)))
          throw_value_range();
        return set_u32(static_cast<uint32_t>(value));
      case u64:
        return set_u64(value);
      }
    }

    void set_unsigned(const int64_t value) {
      if (unlikely(value < 0))
        throw_value_range();
      return set_integer(static_cast<uint64_t>(value));
    }

    void set_int128(const int128_t &value) {
      if (base::type() == bin128)
        return assign<bin128>(
            *erthink::constexpr_pointer_cast<const binary128_t *>(&value));
      return set_integer(int64_t(value));
    }

    void set_uint128(const uint128_t &value) {
      if (base::type() == bin128)
        return assign<bin128>(
            *erthink::constexpr_pointer_cast<const binary128_t *>(&value));
      return set_unsigned(uint64_t(value));
    }

    void set_number(const int32_t value) {
      switch (base::type()) {
      default:
        throw_type_mismatch();
      case f32:
        if (unlikely(value < safe32_number_min || value > safe32_number_max))
          throw_value_range();
        return set_f32(static_cast<float>(value));
      case f64:
        return set_f64(static_cast<double>(value));
      case i8:
        if (unlikely(value < INT8_MIN || value > INT8_MAX))
          throw_value_range();
        return set_i8(static_cast<int8_t>(value));
      case i16:
        if (unlikely(value < INT16_MIN || value > INT16_MAX))
          throw_value_range();
        return set_i16(static_cast<int16_t>(value));
      case i32:
        return set_i32(value);
      case i64:
        return set_i64(value);
      case u8:
        if (unlikely(value < 0 || value > UINT8_MAX))
          throw_value_range();
        return set_u8(static_cast<uint8_t>(value));
      case u16:
        if (unlikely(value < 0 || value > UINT16_MAX))
          throw_value_range();
        return set_u16(static_cast<uint16_t>(value));
      case u32:
        if (unlikely(value < 0))
          throw_value_range();
        return set_u32(static_cast<uint32_t>(value));
      case u64:
        if (unlikely(value < 0))
          throw_value_range();
        return set_u64(static_cast<uint64_t>(value));
      }
    }

    void set_number(const uint32_t value) {
      switch (base::type()) {
      default:
        throw_type_mismatch();
      case f32:
        if (unlikely(value > safe32_number_max))
          throw_value_range();
        return set_f32(static_cast<float>(value));
      case f64:
        return set_f64(static_cast<double>(value));
      case i8:
        if (unlikely(value > INT8_MAX))
          throw_value_range();
        return set_i8(static_cast<int8_t>(value));
      case i16:
        if (unlikely(value > INT16_MAX))
          throw_value_range();
        return set_i16(static_cast<int16_t>(value));
      case i32:
        if (unlikely(value > INT32_MAX))
          throw_value_range();
        return set_i32(static_cast<int32_t>(value));
      case i64:
        return set_i64(static_cast<int64_t>(value));
      case u8:
        if (unlikely(value > UINT8_MAX))
          throw_value_range();
        return set_u8(static_cast<uint8_t>(value));
      case u16:
        if (unlikely(value > UINT16_MAX))
          throw_value_range();
        return set_u16(static_cast<uint16_t>(value));
      case u32:
        return set_u32(value);
      case u64:
        return set_u64(value);
      }
    }

    void set_number(const int64_t value) {
      switch (base::type()) {
      default:
        throw_type_mismatch();
      case f32:
        if (unlikely(value < safe32_number_min || value > safe32_number_max))
          throw_value_range();
        return set_f32(static_cast<float>(value));
      case f64:
        if (unlikely(value < safe64_number_min || value > safe64_number_max))
          throw_value_range();
        return set_f64(static_cast<double>(value));
      case i8:
        if (unlikely(value < INT8_MIN || value > INT8_MAX))
          throw_value_range();
        return set_i8(static_cast<int8_t>(value));
      case i16:
        if (unlikely(value < INT16_MIN || value > INT16_MAX))
          throw_value_range();
        return set_i16(static_cast<int16_t>(value));
      case i32:
        if (unlikely(value < INT32_MIN || value > INT32_MAX))
          throw_value_range();
        return set_i32(static_cast<int32_t>(value));
      case i64:
        return set_i64(value);
      case u8:
        if (unlikely(value < 0 || value > UINT8_MAX))
          throw_value_range();
        return set_u8(static_cast<uint8_t>(value));
      case u16:
        if (unlikely(value < 0 || value > UINT16_MAX))
          throw_value_range();
        return set_u16(static_cast<uint16_t>(value));
      case u32:
        if (unlikely(value < 0 || value > UINT32_MAX))
          throw_value_range();
        return set_u32(static_cast<uint32_t>(value));
      case u64:
        if (unlikely(value < 0))
          throw_value_range();
        return set_u64(static_cast<uint64_t>(value));
      }
    }

    void set_number(const uint64_t value) {
      switch (base::type()) {
      default:
        throw_type_mismatch();
      case f32:
        if (unlikely(value > safe32_number_max))
          throw_value_range();
        return set_f32(static_cast<float>(value));
      case f64:
        if (unlikely(value > safe64_number_max))
          throw_value_range();
        return set_f64(static_cast<double>(value));
      case i8:
        if (unlikely(value > INT8_MAX))
          throw_value_range();
        return set_i8(static_cast<int8_t>(value));
      case i16:
        if (unlikely(value > INT16_MAX))
          throw_value_range();
        return set_i16(static_cast<int16_t>(value));
      case i32:
        if (unlikely(value > INT32_MAX))
          throw_value_range();
        return set_i32(static_cast<int32_t>(value));
      case i64:
        if (unlikely(value > INT64_MAX))
          throw_value_range();
        return set_i64(static_cast<int64_t>(value));
      case u8:
        if (unlikely(value > UINT8_MAX))
          throw_value_range();
        return set_u8(static_cast<uint8_t>(value));
      case u16:
        if (unlikely(value > UINT16_MAX))
          throw_value_range();
        return set_u16(static_cast<uint16_t>(value));
      case u32:
        if (unlikely(value > UINT32_MAX))
          throw_value_range();
        return set_u32(static_cast<uint32_t>(value));
      case u64:
        return set_u64(value);
      }
    }

    void set_number(const float value) {
      if (unlikely(std::isnan(value)) && !base::is_float())
        throw_value_range();
      switch (base::type()) {
      default:
        throw_type_mismatch();
      case f32:
        return set_f32(value);
      case f64:
        return set_f64(value);
      case i8:
        if (unlikely(value < INT8_MIN || value > INT8_MAX))
          throw_value_range();
        return set_i8(static_cast<int8_t>(value));
      case i16:
        if (unlikely(value < INT16_MIN || value > INT16_MAX))
          throw_value_range();
        return set_i16(static_cast<int16_t>(value));
      case i32:
        if (unlikely(!(is_safe_roundtrip<i32>(value))))
          throw_value_range();
        return set_i32(static_cast<int32_t>(value));
      case i64:
        if (unlikely(!(is_safe_roundtrip<i64>(value))))
          throw_value_range();
        return set_i64(static_cast<int64_t>(value));
      case u8:
        if (unlikely(value < 0 || value > UINT8_MAX))
          throw_value_range();
        return set_u8(static_cast<uint8_t>(value));
      case u16:
        if (unlikely(value < 0 || value > UINT16_MAX))
          throw_value_range();
        return set_u16(static_cast<uint16_t>(value));
      case u32:
        if (unlikely(!(is_safe_roundtrip<u32>(value))))
          throw_value_range();
        return set_u32(static_cast<uint32_t>(value));
      case u64:
        if (unlikely(!(is_safe_roundtrip<u64>(value))))
          throw_value_range();
        return set_u64(static_cast<uint64_t>(value));
      }
    }

    void set_number(const double value) {
      if (unlikely(std::isnan(value)) && !base::is_float())
        throw_value_range();
      switch (base::type()) {
      default:
        throw_type_mismatch();
      case f32:
        if (!std::isinf(value) &&
            unlikely(value > std::numeric_limits<float>::max() ||
                     value < std::numeric_limits<float>::lowest()))
          throw_value_range();
        return set_f32(static_cast<float>(value));
      case f64:
        return set_f64(value);
      case i8:
        if (unlikely(value < INT8_MIN || value > INT8_MAX))
          throw_value_range();
        return set_i8(static_cast<int8_t>(value));
      case i16:
        if (unlikely(value < INT16_MIN || value > INT16_MAX))
          throw_value_range();
        return set_i16(static_cast<int16_t>(value));
      case i32:
        if (unlikely(!(is_safe_roundtrip<i32>(value))))
          throw_value_range();
        return set_i32(static_cast<int32_t>(value));
      case i64:
        if (unlikely(!(is_safe_roundtrip<i64>(value))))
          throw_value_range();
        return set_i64(static_cast<int64_t>(value));
      case u8:
        if (unlikely(value < 0 || value > UINT8_MAX))
          throw_value_range();
        return set_u8(static_cast<uint8_t>(value));
      case u16:
        if (unlikely(value < 0 || value > UINT16_MAX))
          throw_value_range();
        return set_u16(static_cast<uint16_t>(value));
      case u32:
        if (unlikely(!(is_safe_roundtrip<u32>(value))))
          throw_value_range();
        return set_u32(static_cast<uint32_t>(value));
      case u64:
        if (unlikely(!(is_safe_roundtrip<u64>(value))))
          throw_value_range();
        return set_u64(static_cast<uint64_t>(value));
      }
    }

    //--------------------------------------------------------------------------

#define HERE_GENUS_CASE(VALUE_TYPE, NAME)                                      \
  VALUE_TYPE operator=(VALUE_TYPE value) {                                     \
    set_##NAME(value);                                                         \
    return value;                                                              \
  }

    HERE_GENUS_CASE(const string_view &, string)
    HERE_GENUS_CASE(const tuple_ro *, nested)
    HERE_GENUS_CASE(const property_pair &, property)
    HERE_GENUS_CASE(bool, bool)
    HERE_GENUS_CASE(int8_t, number)
    HERE_GENUS_CASE(uint8_t, number)
    HERE_GENUS_CASE(int16_t, number)
    HERE_GENUS_CASE(uint16_t, number)
    HERE_GENUS_CASE(int32_t, number)
    HERE_GENUS_CASE(uint32_t, number)
    HERE_GENUS_CASE(int64_t, number)
    HERE_GENUS_CASE(uint64_t, number)
    HERE_GENUS_CASE(float, number)
    HERE_GENUS_CASE(double, number)
    HERE_GENUS_CASE(decimal64, decimal)
    HERE_GENUS_CASE(datetime_t, datetime)
    HERE_GENUS_CASE(const uuid_t &, uuid)
    HERE_GENUS_CASE(const int128_t &, int128)
    HERE_GENUS_CASE(const uint128_t &, uint128)
    HERE_GENUS_CASE(const binary96_t &, bin96)
    HERE_GENUS_CASE(const binary128_t &, bin128)
    HERE_GENUS_CASE(const binary160_t &, bin160)
    HERE_GENUS_CASE(const binary192_t &, bin192)
    HERE_GENUS_CASE(const binary224_t &, bin224)
    HERE_GENUS_CASE(const binary256_t &, bin256)
    HERE_GENUS_CASE(const binary320_t &, bin320)
    HERE_GENUS_CASE(const binary384_t &, bin384)
    HERE_GENUS_CASE(const binary512_t &, bin512)
    HERE_GENUS_CASE(const ip_address_t &, ip_address)
    HERE_GENUS_CASE(const mac_address_t, mac_address)
    HERE_GENUS_CASE(const ip_net_t &, ip_net)
#undef HERE_GENUS_CASE

    const std::string &operator=(const std::string &value) {
      set_string(value);
      return value;
    }

    const char *operator=(const char value[]) {
      set_string(string_view(value));
      return value;
    }

    template <std::size_t size>
    const char (&operator=(const char (&value)[size]))[size] {
      set_string(string_view(value));
      return value;
    }

    template <std::size_t size>
    std::array<char, size> &operator=(std::array<char, size> &value) {
      set_string(string_view(value));
      return value;
    }
  };

  //----------------------------------------------------------------------------

  template <typename TOKEN>
  class collection_iterator_rw
      : protected accessor_rw<TOKEN>
#if __cplusplus < 201703L
      ,
        public std::iterator<
            std::forward_iterator_tag, const accessor_rw<TOKEN>, std::ptrdiff_t,
            const accessor_rw<TOKEN> *, const accessor_rw<TOKEN> &>
#endif
  {
    friend class tuple_rw;
    friend class fptu::loose_iterator_ro;
    friend class fptu::loose_iterator_rw;
    using accessor = accessor_rw<TOKEN>;

    explicit cxx11_constexpr
    collection_iterator_rw(tuple_rw *tuple, field_loose *target,
                           const TOKEN token) cxx11_noexcept
        : accessor(tuple, target, token) {
      constexpr_assert(token.is_collection());
    }

  public:
#if __cplusplus >= 201703L
    using iterator_category = std::forward_iterator_tag;
    using value_type = const accessor_rw<TOKEN>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;
#endif

    cxx11_constexpr collection_iterator_rw() cxx11_noexcept : accessor() {
      static_assert(sizeof(*this) <= (TOKEN::is_static_token::value
                                          ? sizeof(void *) * 2
                                          : sizeof(void *) * 3),
                    "WTF?");
    }
    cxx11_constexpr collection_iterator_rw(const collection_iterator_rw &)
        cxx11_noexcept = default;
    cxx14_constexpr collection_iterator_rw &
    operator=(const collection_iterator_rw &) cxx11_noexcept = default;
    cxx11_constexpr const TOKEN &token() const cxx11_noexcept {
      return accessor::token();
    }

    collection_iterator_rw &operator++() cxx11_noexcept {
      assert(static_cast<const field_loose *>(accessor::field_) <
             accessor::tuple_->end_index());
      accessor::field_ =
          next(static_cast<const field_loose *>(accessor::field_),
               accessor::tuple_->end_index(), accessor::token_.tag());
      return *this;
    }
    collection_iterator_rw operator++(int) const cxx11_noexcept {
      collection_iterator_rw iterator(*this);
      ++iterator;
      return iterator;
    }
    cxx14_constexpr accessor operator*() const cxx11_noexcept {
      return accessor(*this);
    }
    cxx14_constexpr accessor &operator*() cxx11_noexcept { return *this; }

    cxx11_constexpr bool
    operator==(const accessor &other) const cxx11_noexcept {
      return accessor::field_ == other.field_;
    }
    cxx11_constexpr bool
    operator!=(const accessor &other) const cxx11_noexcept {
      return accessor::field_ != other.field_;
    }
    cxx11_constexpr bool
    operator==(const collection_iterator_rw &other) const cxx11_noexcept {
      return accessor::field_ == other.field_;
    }
    cxx11_constexpr bool
    operator!=(const collection_iterator_rw &other) const cxx11_noexcept {
      return accessor::field_ != other.field_;
    }

    operator collection_iterator_ro<TOKEN>() const cxx11_noexcept {
      return collection_iterator_ro<TOKEN>(accessor::field_.const_loose,
                                           accessor::tuple_->end_index(),
                                           accessor::token_);
    }
    cxx11_constexpr bool operator==(
        const collection_iterator_ro<TOKEN> &other) const cxx11_noexcept {
      return accessor::field_ == other.field_;
    }
    cxx11_constexpr bool operator!=(
        const collection_iterator_ro<TOKEN> &other) const cxx11_noexcept {
      return accessor::field_ != other.field_;
    }
  };

  template <typename TOKEN>
  class collection_rw : protected collection_iterator_rw<TOKEN> {
    template <typename> friend class crtp_getter;
    friend class tuple_rw;

    cxx11_constexpr collection_rw(tuple_rw *tuple, field_loose *first,
                                  const TOKEN token) cxx11_noexcept
        : collection_iterator_rw<TOKEN>(tuple, first, token) {
      constexpr_assert(token.is_collection());
    }

  public:
    using const_iterator = collection_iterator_ro<TOKEN>;
    using const_range = collection_ro<TOKEN>;
    using iterator = collection_iterator_rw<TOKEN>;
    cxx11_constexpr collection_rw(const collection_rw &) = default;
    cxx14_constexpr collection_rw &operator=(const collection_rw &) = default;
    cxx11_constexpr const TOKEN &token() const cxx11_noexcept {
      return iterator::token();
    }

    cxx11_constexpr operator const_range() const cxx11_noexcept {
      return const_range(iterator::field_.const_loose,
                         iterator::tuple_->end_index(), iterator::token_);
    }

    cxx14_constexpr const const_iterator begin() const cxx11_noexcept {
      return const_iterator(iterator::field_.const_loose,
                            iterator::tuple_->end_index(), iterator::token_);
    }
    cxx14_constexpr const_iterator end() const cxx11_noexcept {
      return const_iterator(nullptr, iterator::tuple_->end_index(),
                            iterator::token_);
    }

    cxx14_constexpr const iterator &begin() cxx11_noexcept { return *this; }
    cxx14_constexpr iterator end() cxx11_noexcept {
      return iterator(iterator::tuple_, nullptr, iterator::token_);
    }

    cxx11_constexpr bool empty() const cxx11_noexcept {
      return begin() == end();
    }
  };

private:
  template <typename TOKEN>
  __pure_function accessor_rw<TOKEN> at(const TOKEN &token) {
    return accessor_rw<TOKEN>(this, crtp_getter::at(*this, token));
  }

  template <typename TOKEN>
  __pure_function collection_rw<TOKEN> collect(const TOKEN &token) {
    if (unlikely(!token.is_collection()))
      throw_collection_required();

    field_loose *const first = const_cast<field_loose *>(
        lookup(is_sorted(), begin_index(), end_index(), token.tag()));
    return collection_rw<TOKEN>(this, first, token);
  }

  template <typename TOKEN>
  __pure_function accessor_rw<TOKEN> append(const TOKEN &token) {
    if (unlikely(!token.is_collection()))
      throw_collection_required();

    return accessor_rw<TOKEN>(this, static_cast<field_loose *>(nullptr), token);
  }

  template <typename TOKEN> __pure_function bool remove(const TOKEN &token) {
    accessor_rw<TOKEN> accessor(
        this, crtp_getter::template at<TOKEN, true>(*this, token));
    if (accessor.exist()) {
      accessor.remove();
      return true;
    }
    return false;
  }

public: //----------------------------------------------------------------------
  void ensure() const;
  void debug_check() const {
#ifndef NDEBUG
    ensure();
#endif
  }
  void reset();
  const fptu::schema *schema() const cxx11_noexcept { return schema_; }

  enum class optimize_flags {
    none = 0,
    compactify = 1,
    sort_index = 2,
    enforce_sort_index = 4,
    all = optimize_flags::compactify | optimize_flags::sort_index,
  };
  friend inline optimize_flags operator|(optimize_flags a, optimize_flags b) {
    return optimize_flags(unsigned(a) | unsigned(b));
  }
  friend inline bool operator&(optimize_flags a, optimize_flags b) {
    return (unsigned(a) & unsigned(b)) != 0;
  }

  bool optimize(const optimize_flags flags = optimize_flags::all);
  bool compactify() { return optimize(optimize_flags::compactify); }
  bool sort_index(bool force = false) {
    return optimize(force ? optimize_flags::enforce_sort_index
                          : optimize_flags::sort_index);
  }

  bool erase(const collection_iterator_rw<token> &it);
  std::size_t erase(const collection_iterator_rw<token> &begin,
                    const collection_iterator_rw<token> &end);
  std::size_t erase(const collection_rw<token> &collection);
  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  bool erase(const TOKEN &ident) {
    return likely(!ident.is_collection()) ? remove(ident)
                                          : erase(collection(ident)) > 0;
  }
  bool erase(const token &ident);
  __pure_function const char *audit() const cxx11_noexcept {
    return audit(this);
  }

  __pure_function std::size_t head_space() const cxx11_noexcept {
    return head_;
  }
  __pure_function std::size_t tail_space_units() const cxx11_noexcept {
    assert(end_ >= tail_);
    return end_ - tail_;
  }
  __pure_function std::size_t tail_space_bytes() const cxx11_noexcept {
    return units2bytes(tail_space_units());
  }
  __pure_function std::size_t junk_units() const cxx11_noexcept {
    return junk_.volume + junk_.count;
  }
  __pure_function std::size_t junk_bytes() const cxx11_noexcept {
    return units2bytes(junk_units());
  }
  __pure_function bool empty() const cxx11_noexcept {
    return head_ + junk_.count == tail_;
  }
  __pure_function std::size_t index_size() const cxx11_noexcept {
    assert(pivot_ >= head_);
    return pivot_ - head_;
  }
  __pure_function std::size_t payload_size_units() const cxx11_noexcept {
    assert(tail_ >= pivot_);
    return tail_ - pivot_;
  }
  __pure_function std::size_t payload_size_bytes() const cxx11_noexcept {
    return units2bytes(payload_size_units());
  }
  __pure_function std::size_t loose_count() const cxx11_noexcept {
    assert(index_size() >= junk_.count);
    return index_size() - junk_.count;
  }
  __pure_function std::size_t brutto_size() const cxx11_noexcept {
    return units2bytes(tail_ - head_ + /* header */ 1);
  }
  __pure_function std::size_t netto_size() const cxx11_noexcept {
    return brutto_size() - junk_units();
  }
  __pure_function std::size_t capacity() const cxx11_noexcept {
    return units2bytes(end_);
  }

  __pure_function bool have_preplaced() const cxx11_noexcept {
    return schema_ && schema_->preplaced_bytes() > 0;
  }
  bool is_sorted() const cxx11_noexcept {
    /* TODO */
    return false;
  }

  static cxx11_constexpr std::size_t pure_tuple_size() {
    return sizeof(tuple_rw) - sizeof(tuple_rw::area_);
  }

  /* TODO: перенести как inline во внутренний включаемый файл */
  __pure_function static size_t
  estimate_required_space(size_t items, std::size_t data_bytes,
                          const fptu::schema *schema,
                          bool dont_account_preplaced = false) {
    const std::size_t preplaced_bytes = schema ? schema->preplaced_bytes() : 0;
    if (unlikely(preplaced_bytes > fundamentals::max_tuple_bytes_netto))
      throw_invalid_schema();
    if (unlikely(data_bytes >
                     fundamentals::max_tuple_bytes_netto - preplaced_bytes ||
                 items > fundamentals::max_fields))
      throw_tuple_too_large();

    const std::size_t estimated =
        pure_tuple_size() + items * unit_size +
        utils::ceil(dont_account_preplaced ? data_bytes
                                           : data_bytes + preplaced_bytes,
                    unit_size);
    assert(estimated <= fundamentals::buffer_enough);
    return estimated;
  }

  /* TODO: перенести как inline во внутренний включаемый файл */
  __pure_function static std::size_t
  estimate_required_space(const tuple_ro *ro, const std::size_t more_items,
                          const std::size_t more_payload,
                          const fptu::schema *schema) {
    /* Во избежание двойной работы здесь НЕ проверяется корректность переданного
     * экземпляра tuple_ro и его соответствие схеме. Такая проверка должна
     * выполняться один раз явным вызовом валидирующих функций. Например, внутри
     * конструкторов безопасных объектов. */
    if (unlikely(more_items > fundamentals::max_fields))
      throw_invalid_argument("items > fptu::max_fields");
    if (unlikely(more_payload > fundamentals::max_tuple_bytes_netto))
      throw_invalid_argument("more_payload_bytes > fptu::buffer_limit");

    const std::size_t total_items = std::min(size_t(fundamentals::max_fields),
                                             ro->index_size() + more_items);

    const std::size_t total_payload =
        std::min(size_t(fundamentals::max_tuple_bytes_netto),
                 ro->payload_bytes() + more_payload);

    return estimate_required_space(total_items, total_payload, schema,
                                   /* не учитываем размер preplaced-полей,
                                    * ибо они уже должы быть в tuple_ro */
                                   true);
  }

  __pure_function bool operator==(const tuple_ro *ro) const cxx11_noexcept {
    return ro->flat == &area_[head_ - 1];
  }
  bool operator!=(const tuple_ro *ro) const cxx11_noexcept {
    return !(*this == ro);
  }

  const tuple_ro *take_asis() const;
  std::pair<const tuple_ro *, bool> take_optimized();

#define HERE_GENUS_CASE(VALUE_TYPE, NAME)                                      \
  FPTU_TEMPLATE_FOR_STATIC_TOKEN                                               \
  inline void set_##NAME(const TOKEN &ident, const VALUE_TYPE value) {         \
    at(ident).set_##NAME(value);                                               \
  }                                                                            \
  inline void legacy_update_##NAME(const token &ident,                         \
                                   const VALUE_TYPE value) {                   \
    accessor_rw<token> accessor(                                               \
        this, crtp_getter::template at<token, true>(*this, ident));            \
    if (unlikely(!accessor.exist()))                                           \
      throw_field_absent();                                                    \
    accessor.set_##NAME(value);                                                \
  }                                                                            \
  void set_##NAME(const token &ident, const VALUE_TYPE value);                 \
  collection_iterator_rw<token> insert_##NAME(const token &ident,              \
                                              const VALUE_TYPE value);
  HERE_GENUS_CASE(tuple_ro *, nested)
  HERE_GENUS_CASE(string_view &, string)
  HERE_GENUS_CASE(string_view &, varbinary)
  HERE_GENUS_CASE(property_pair &, property)
  HERE_GENUS_CASE(bool, bool)
  HERE_GENUS_CASE(short, enum)
  HERE_GENUS_CASE(int8_t, i8)
  HERE_GENUS_CASE(uint8_t, u8)
  HERE_GENUS_CASE(int16_t, i16)
  HERE_GENUS_CASE(uint16_t, u16)
  HERE_GENUS_CASE(int32_t, i32)
  HERE_GENUS_CASE(uint32_t, u32)
  HERE_GENUS_CASE(int64_t, i64)
  HERE_GENUS_CASE(uint64_t, u64)
  HERE_GENUS_CASE(float, f32)
  HERE_GENUS_CASE(double, f64)
  HERE_GENUS_CASE(decimal64, decimal)
  HERE_GENUS_CASE(datetime_t, datetime)
  HERE_GENUS_CASE(uuid_t &, uuid)
  HERE_GENUS_CASE(int128_t &, int128)
  HERE_GENUS_CASE(uint128_t &, uint128)
  HERE_GENUS_CASE(binary96_t &, bin96)
  HERE_GENUS_CASE(binary128_t &, bin128)
  HERE_GENUS_CASE(binary160_t &, bin160)
  HERE_GENUS_CASE(binary192_t &, bin192)
  HERE_GENUS_CASE(binary224_t &, bin224)
  HERE_GENUS_CASE(binary256_t &, bin256)
  HERE_GENUS_CASE(binary320_t &, bin320)
  HERE_GENUS_CASE(binary384_t &, bin384)
  HERE_GENUS_CASE(binary512_t &, bin512)
  HERE_GENUS_CASE(ip_address_t &, ip_address)
  HERE_GENUS_CASE(mac_address_t, mac_address)
  HERE_GENUS_CASE(ip_net_t &, ip_net)
  HERE_GENUS_CASE(int64_t, integer)
  HERE_GENUS_CASE(uint64_t, integer)
  HERE_GENUS_CASE(int64_t, unsigned)
  HERE_GENUS_CASE(uint64_t, unsigned)
  HERE_GENUS_CASE(double, float)
  HERE_GENUS_CASE(int32_t, number)
  HERE_GENUS_CASE(uint32_t, number)
  HERE_GENUS_CASE(int64_t, number)
  HERE_GENUS_CASE(uint64_t, number)
  HERE_GENUS_CASE(float, number)
  HERE_GENUS_CASE(double, number)
#undef HERE_GENUS_CASE

  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  __pure_function inline collection_rw<TOKEN> collection(const TOKEN &ident) {
    return collect(ident);
  }
  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  __pure_function inline accessor_rw<TOKEN> operator[](const TOKEN &ident) {
    return at(ident);
  }
  __pure_function collection_rw<token> collection(const token &ident);
  __pure_function accessor_rw<token> operator[](const token &ident);

  template <typename TOKEN>
  __pure_function inline bool is_present(const TOKEN &ident) const {
    return crtp_getter::is_present(ident);
  }

  __pure_function field_iterator_ro
  cbegin(const fptu::schema *schema) const cxx11_noexcept {
    return field_iterator_ro::begin(pivot(), schema);
  }
  __pure_function field_iterator_ro
  cend(const fptu::schema *schema) const cxx11_noexcept {
    return field_iterator_ro::end(begin_index(), pivot(), schema);
  }
  __pure_function field_iterator_ro
  begin(const fptu::schema *schema) const cxx11_noexcept {
    return cbegin(schema);
  }
  __pure_function field_iterator_ro
  end(const fptu::schema *schema) const cxx11_noexcept {
    return cend(schema);
  }

  __pure_function loose_iterator_ro cbegin_loose() const cxx11_noexcept {
    return loose_iterator_ro(end_index() - 1);
  }
  __pure_function loose_iterator_ro cend_loose() const cxx11_noexcept {
    return loose_iterator_ro(begin_index() - 1);
  }
  __pure_function loose_iterator_ro begin_loose() const cxx11_noexcept {
    return cbegin_loose();
  }
  __pure_function loose_iterator_ro end_loose() const cxx11_noexcept {
    return cend_loose();
  }

  friend std::ostream &operator<<(std::ostream &, const tuple_rw *);
};

//------------------------------------------------------------------------------

template <typename TOKEN, genus GENUS,
          typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique !=
                                        meta::physique_kind::stretchy> *>
inline void *tuple_rw::assign_field_value(
    void *field, const typename meta::genus_traits<GENUS>::return_type value,
    const TOKEN token) {
  if (unlikely(token.type() != GENUS))
    throw_type_mismatch();

  debug_check();
  if (likely(token.is_preplaced())) {
    // preplaced fixed-size field
    if (token.is_discernible_null() &&
        unlikely(meta::genus_traits<GENUS>::is_prohibited_nil(value)))
      throw_value_denil();
    // write value, no exceptions allowed
    meta::genus_traits<GENUS>::write(static_cast<field_preplaced *>(field),
                                     value);
  } else {
    // loose
    if (field == nullptr) {
      if (!token.is_discernible_null() &&
          meta::genus_traits<GENUS>::is_empty(value))
        return field;
      // absent, append new one (exception may be thrown)
      field = alloc_loose(token.tag(), meta::genus_traits<GENUS>::loose_units);
    } else {
      if (unlikely(static_cast<field_loose *>(field) >= end_index() ||
                   static_cast<field_loose *>(field)->type() != GENUS))
        throw_index_corrupted();
      if (!token.is_discernible_null() &&
          meta::genus_traits<GENUS>::is_empty(value)) {
        release_loose(static_cast<field_loose *>(field),
                      meta::genus_traits<GENUS>::loose_units);
        debug_check();
        return nullptr;
      }
    }
    // write value, no exceptions allowed
    meta::genus_traits<GENUS>::write(static_cast<field_loose *>(field), value);
  }
  debug_check();
  return field;
}

template <typename TOKEN, genus GENUS,
          typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique ==
                                        meta::physique_kind::stretchy> *>
inline void *tuple_rw::assign_field_value(
    void *field, const typename meta::genus_traits<GENUS>::return_type value,
    const TOKEN token) {
  if (unlikely(token.type() != GENUS))
    throw_type_mismatch();

  debug_check();
  const std::size_t needed =
      (token.is_discernible_null() ||
       !meta::genus_traits<GENUS>::is_empty(value))
          ? meta::genus_traits<GENUS>::estimate_space(value)
          : 0;
  if (token.is_loose()) {
    if (field == nullptr) {
      if (needed) {
        // absent loose, append new one (exception may be thrown)
        field = alloc_loose(token.tag(), needed);
        assert(tag2genus(static_cast<field_loose *>(field)->genus_and_id) ==
               GENUS);
        // write value, no exceptions allowed
        meta::genus_traits<GENUS>::write(
            static_cast<field_loose *>(field)->relative.payload(), value);
        debug_check();
      }
      return field;
    }
    if (unlikely(static_cast<field_loose *>(field) >= end_index() ||
                 static_cast<field_loose *>(field)->type() != GENUS))
      throw_index_corrupted();
  }

  static_assert(offsetof(field_preplaced, relative) ==
                    offsetof(field_loose, relative),
                "WTF?");
  auto &relative = static_cast<field_preplaced *>(field)->relative;
  relative_payload *payload = nullptr;
  if (relative.have_payload()) {
    payload = relative.payload();
    const std::size_t have = payload->stretchy.brutto_units(GENUS);
    if (unlikely(needed != have)) {
      if (needed == 0) {
        if (token.is_loose()) {
          release_loose(static_cast<field_loose *>(field), have);
          debug_check();
          return nullptr;
        } else {
          // exception may be thrown in case no index-space
          release_data(payload, have);
          relative.reset_payload();
          debug_check();
          return field;
        }
        __unreachable();
      }

      // exception may be thrown
      payload = realloc_data(relative, have, needed);
      assert(relative.payload() == payload);
    }
  } else {
    if (needed == 0) {
      debug_check();
      return field;
    }
    // exception may be thrown
    payload = alloc_data(needed);
    // update offset, no exceptions allowed
    relative.set_payload(payload);
  }

  assert(needed > 0 && payload);
  // write value, no exceptions allowed
  meta::genus_traits<GENUS>::write(payload, value);
  debug_check();
  return field;
}

//------------------------------------------------------------------------------

template <typename TOKEN, genus GENUS,
          typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique !=
                                        meta::physique_kind::stretchy> *>
inline field_loose *tuple_rw::append_field(
    const typename meta::genus_traits<GENUS>::return_type value,
    const TOKEN token) {
  if (unlikely(token.type() != GENUS))
    throw_type_mismatch();
  if (unlikely(!token.is_collection()))
    throw_collection_required();

  debug_check();
  if (!token.is_discernible_null() &&
      meta::genus_traits<GENUS>::is_empty(value))
    return nullptr;

  // append new one (exception may be thrown)
  field_loose *field =
      alloc_loose(token.tag(), meta::genus_traits<GENUS>::loose_units);
  assert(tag2genus(field->genus_and_id) == GENUS);

  // write value, no exceptions allowed
  meta::genus_traits<GENUS>::write(field, value);
  debug_check();
  return field;
}

template <typename TOKEN, genus GENUS,
          typename erthink::enable_if_t<meta::genus_traits<GENUS>::physique ==
                                        meta::physique_kind::stretchy> *>
inline field_loose *tuple_rw::append_field(
    const typename meta::genus_traits<GENUS>::return_type value,
    const TOKEN token) {
  if (unlikely(token.type() != GENUS))
    throw_type_mismatch();
  if (unlikely(!token.is_collection()))
    throw_collection_required();

  debug_check();
  if (!token.is_discernible_null() &&
      meta::genus_traits<GENUS>::is_empty(value))
    return nullptr;

  const std::size_t needed = meta::genus_traits<GENUS>::estimate_space(value);
  // append new one (exception may be thrown)
  field_loose *field = alloc_loose(token.tag(), needed);
  if (needed) {
    assert(tag2genus(field->genus_and_id) == GENUS);
    // write value, no exceptions allowed
    meta::genus_traits<GENUS>::write(field->relative.payload(), value);
  } else {
    assert(!field->relative.have_payload());
  }
  debug_check();
  return field;
}

//------------------------------------------------------------------------------

template <typename TOKEN>
template <genus GENUS>
inline void tuple_rw::accessor_rw<TOKEN>::assign(
    const typename meta::genus_traits<GENUS>::return_type value) {
  base::field_ = tuple_->template assign_field_value<TOKEN, GENUS>(
      const_cast<void *>(base::field_), value, base::token_);
}

template <typename TOKEN> inline void tuple_rw::accessor_rw<TOKEN>::remove() {
  if (likely(base::token_.is_preplaced())) {
    tuple_->erase(
        static_cast<field_preplaced *>(const_cast<void *>(base::field_)),
        base::token_.type(), base::token_.is_discernible_null());
  } else {
    assert(base::field_ != nullptr);
    if (unlikely(static_cast<const field_loose *>(base::field_) >=
                     tuple_->end_index() ||
                 static_cast<field_loose *>(const_cast<void *>(base::field_))
                         ->type() != base::token_.type()))
      throw_index_corrupted();
    tuple_->erase(static_cast<field_loose *>(const_cast<void *>(base::field_)));
    base::field_ = nullptr;
  }
}

FPTU_API std::ostream &operator<<(std::ostream &out, const tuple_rw *);

} // namespace details

//------------------------------------------------------------------------------

using dynamic_accessor_rw = details::tuple_rw::accessor_rw<token>;
using dynamic_collection_iterator_rw =
    details::tuple_rw::collection_iterator_rw<token>;
using dynamic_collection_rw = details::tuple_rw::collection_rw<token>;

} // namespace fptu

#include "fast_positive/tuples/details/warnings_pop.h"
