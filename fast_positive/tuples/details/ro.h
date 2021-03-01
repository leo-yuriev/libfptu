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
#include "fast_positive/tuples/details/audit.h"
#include "fast_positive/tuples/details/exceptions.h"
#include "fast_positive/tuples/details/field.h"
#include "fast_positive/tuples/details/getter.h"
#include "fast_positive/tuples/details/meta.h"
#include "fast_positive/tuples/details/scan.h"
#include "fast_positive/tuples/essentials.h"
#include "fast_positive/tuples/schema.h"
#include "fast_positive/tuples/token.h"

namespace fptu {
class tuple_ro_managed;

class FPTU_API_TYPE
    loose_iterator_ro /* Вспомогательный легковестный итератор по loose-полям.
  В прямом направлении двигается от loose-полей добавленных первыми к последним
  loose-полям (в направлении противоположном росту индексов).

  Актуальный порядок перебора loose-полей определяется физическим расположением
  их дескрипторов в индексе. При последовательном добавлении экземпляры
  loose-полей добавленные последними, также будут проитерированы последними. */
#if __cplusplus < 201703L
    : public std::iterator<std::random_access_iterator_tag,
                           const dynamic_accessor_ro, std::ptrdiff_t,
                           const dynamic_accessor_ro *,
                           const dynamic_accessor_ro &>
#endif
{
  friend class details::tuple_ro;
  friend class details::tuple_rw;
  friend class field_iterator_ro;

protected:
  const details::field_loose
      *field_ /* При итерировании изменяется в обратном направлении */;

  explicit cxx11_constexpr
  loose_iterator_ro(const details::field_loose *field) cxx11_noexcept
      : field_(field) {}

public:
#if __cplusplus >= 201703L
  using iterator_category = std::random_access_iterator_tag;
  using value_type = const dynamic_accessor_ro;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *;
  using reference = value_type &;
#endif

  cxx11_constexpr loose_iterator_ro() cxx11_noexcept : field_(nullptr) {}
  cxx11_constexpr
  loose_iterator_ro(const dynamic_collection_iterator_ro &iter) cxx11_noexcept
      : field_(static_cast<const details::field_loose *>(iter.field_)) {}
  explicit operator bool() const cxx11_noexcept { return field_ != nullptr; }

  cxx11_constexpr
  loose_iterator_ro(const loose_iterator_ro &) cxx11_noexcept = default;
  cxx14_constexpr loose_iterator_ro &
  operator=(const loose_iterator_ro &) cxx11_noexcept = default;

  cxx14_constexpr dynamic_accessor_ro
  operator[](const difference_type index) const cxx11_noexcept {
    return dynamic_accessor_ro(
        field_ - index,
        /* При чтении можно обойтись без всех token-флажков:
           - is_collection: не имеет значения при чтении одного текущего
             экземпляра поля.
           - is_discernible_null: не имеет значения, ибо точно знаем что
             loose-поле есть.
           - is_rangechecking/is_saturated: не нужен при чтении. */
        field_token(false, false, false));
  }
  cxx14_constexpr dynamic_accessor_ro operator*() const cxx11_noexcept {
    return operator[](0);
  }
  cxx14_constexpr dynamic_accessor_ro operator->() const cxx11_noexcept {
    return operator*();
  }

  __pure_function token field_token(const fptu::schema *schema) const;
  __pure_function token
  field_token_nothow(const fptu::schema *schema) const cxx11_noexcept {
    return schema->by_loose(field_);
  }
  cxx14_constexpr token field_token(const bool collection,
                                    const bool discernible_null,
                                    const bool saturated = false) const {
    return token(field_->genus_and_id, collection, discernible_null, saturated);
  }
  cxx14_constexpr genus field_type() const {
    return details::descriptor2genus(field_->genus_and_id);
  }
  cxx14_constexpr unsigned field_id() const {
    return details::descriptor2id(field_->genus_and_id);
  }

  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  cxx11_constexpr bool operator==(const TOKEN &ident) const cxx11_noexcept {
    return ident.is_loose() && field_->genus_and_id == uint16_t(ident.tag());
  }
  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  cxx11_constexpr bool operator!=(const TOKEN &ident) const cxx11_noexcept {
    return !operator==(ident);
  }
  cxx11_constexpr bool operator==(const token &ident) const cxx11_noexcept {
    return ident.is_loose() && field_->genus_and_id == uint16_t(ident.tag());
  }
  cxx11_constexpr bool operator!=(const token &ident) const cxx11_noexcept {
    return !operator==(ident);
  }

  cxx11_constexpr bool
  operator==(const dynamic_collection_iterator_ro &iter) const cxx11_noexcept {
    return field_ == iter.field_;
  }
  cxx11_constexpr bool
  operator!=(const dynamic_collection_iterator_ro &iter) const cxx11_noexcept {
    return field_ != iter.field_;
  }
  friend cxx11_constexpr bool
  operator==(const dynamic_collection_iterator_ro &a,
             const loose_iterator_ro &b) cxx11_noexcept {
    return b == a;
  }
  friend cxx11_constexpr bool
  operator!=(const dynamic_collection_iterator_ro &a,
             const loose_iterator_ro &b) cxx11_noexcept {
    return b != a;
  }

  friend cxx11_constexpr bool
  operator<(const loose_iterator_ro &a,
            const loose_iterator_ro &b) cxx11_noexcept {
    return a.field_ > b.field_;
  }
  friend cxx11_constexpr bool
  operator<=(const loose_iterator_ro &a,
             const loose_iterator_ro &b) cxx11_noexcept {
    return a.field_ >= b.field_;
  }
  friend cxx11_constexpr bool
  operator==(const loose_iterator_ro &a,
             const loose_iterator_ro &b) cxx11_noexcept {
    return a.field_ == b.field_;
  }
  friend cxx11_constexpr bool
  operator>=(const loose_iterator_ro &a,
             const loose_iterator_ro &b) cxx11_noexcept {
    return a.field_ <= b.field_;
  }
  friend cxx11_constexpr bool
  operator>(const loose_iterator_ro &a,
            const loose_iterator_ro &b) cxx11_noexcept {
    return a.field_ < b.field_;
  }
  friend cxx11_constexpr bool
  operator!=(const loose_iterator_ro &a,
             const loose_iterator_ro &b) cxx11_noexcept {
    return a.field_ != b.field_;
  }

  loose_iterator_ro &operator++() cxx11_noexcept {
    --field_;
    return *this;
  }
  loose_iterator_ro operator++(int) const cxx11_noexcept {
    loose_iterator_ro iterator(*this);
    return --iterator;
  }
  loose_iterator_ro &operator--() cxx11_noexcept {
    ++field_;
    return *this;
  }
  loose_iterator_ro operator--(int) const cxx11_noexcept {
    loose_iterator_ro iterator(*this);
    return ++iterator;
  }
  loose_iterator_ro &operator+=(const difference_type shift) cxx11_noexcept {
    field_ -= shift;
    return *this;
  }
  loose_iterator_ro &operator-=(const difference_type shift) cxx11_noexcept {
    field_ += shift;
    return *this;
  }
  friend difference_type operator-(const loose_iterator_ro &a,
                                   const loose_iterator_ro &b) cxx11_noexcept {
    return b.field_ - a.field_;
  }

  friend loose_iterator_ro
  operator+(const loose_iterator_ro &iter,
            const difference_type shift) cxx11_noexcept {
    loose_iterator_ro copy(iter);
    copy += shift;
    return copy;
  }
  friend loose_iterator_ro
  operator+(const difference_type shift,
            const loose_iterator_ro &iter) cxx11_noexcept {
    loose_iterator_ro copy(iter);
    copy += shift;
    return copy;
  }
  friend loose_iterator_ro
  operator-(const loose_iterator_ro &iter,
            const difference_type shift) cxx11_noexcept {
    loose_iterator_ro copy(iter);
    copy -= shift;
    return copy;
  }
  friend loose_iterator_ro
  operator-(const difference_type shift,
            const loose_iterator_ro &iter) cxx11_noexcept {
    loose_iterator_ro copy(iter);
    copy -= shift;
    return copy;
  }
};

class FPTU_API_TYPE
    field_iterator_ro /* Реализует универсальный (но относительно
  тяжелый) итератор по полям кортежа. В прямом направлении двигается
  от первых preplaced-полей к последним loose-полям.

  Актуальный порядок перебора loose-полей определяется физическим расположением
  их дескрипторов в индексе. При последовательном добавлении экземпляры
  loose-полей добавленные последними, также будут проитерированы последними.

  Тяжесть итератора определяется двумя дополнительными указателями в его
  составе, необходимыми для итерации preplaced-полей. */
#if __cplusplus < 201703L
    : public std::iterator<std::bidirectional_iterator_tag,
                           const dynamic_accessor_ro, std::ptrdiff_t,
                           const dynamic_accessor_ro *,
                           const dynamic_accessor_ro &>
#endif
{
  friend class details::tuple_ro;
  friend class details::tuple_rw;

  /* Механизм итерирования:
     1. ЦЕЛЬ = Пользователю нужен итератор с ожидаемым "знакомым" поведением,
        т.е. обеспечивающий итерование полей нормализованного кортежа (с
        отсортированным индексом) в порядке их определения в схеме: начиная
        с preplaced-полей в их физическом порядке, затем loose-поля в порядке
        увеличения идентификаторов/тегов. Условно это порядок итерирования
        требуемых для экспорта в JSON.
     2. Индекс дескрипторов loose-полей растет в порядке уменьшения адресов.
        Поэтому, следуя принципу роста атрибутного дерева (последовательное
        обогащение и уточнение информации путем добавления атрибутов),
        в нормализованном кортеже с отсортированном индексе дескрипторов поля
        должны располагаться по-убыванию относительно их физических адресов.
        Соответственно, итерация loose-полей должна быть в порядке УМЕНЬШЕНИЯ
        адресов (сонаправлено росту индекса), а итерация preplaced-полей в
        порядке УВЕЛИЧЕНИЯ адресов (в порядке появления новых полей при их
        добавлении в схему).
     3. При итерации изменяется только указатель field_ из базового класса,
        при этом он указывает на текущее поле кортежа. Упрощенно:
         - Если field_ < pivot_, то field_ указывает на дескриптор текущего
           loose-поля и при инкременте итератора field_ УМЕНЬШАЕТСЯ;
         - Иначе, когда field_ >= pivot_, то field_ указывает на данные текущего
           preplaced-поля и при инкременте итератора field_ УВЕЛИЧИВАЕТСЯ;
         = таким образом, field_ всегда содержит адрес текущего поля.

         - При инкременте итератора на позиции последнего preplaced-поля,
           field_ устанавливается в значение loose_field_ptr(pivot_) - 1;
         - При декременте итератора на позиции первого loose-поля, field_
           изменяется только при наличии preplaced и устанавливается
           в значение byte_ptr(pivot_) + last_preplaced_offset, с последующим
           пропуском пустых stretchy-preplaced полей;
         - Значению begin(tuple) соответствует
           field_ = have_preplaced ? pivot_ : loose_field_ptr(pivot_) - 1,
           с последующим пропуском пустых stretchy-preplaced полей;
         - Значению end(tuple) соответствует
           field_ = loose_field_ptr(pivot_) - number_of_loose - 1;

         - Для итератора определяются только операции == и !=, которые работают
           вне зависимости от pivot_, т.е. в том числе пр смешивании итераторов
           от разных кортежей.
   */
protected:
  const void *field_;
  const void *pivot_ /* Конец индекса loose-дескрипторов и начало данных,
                        включая значения preplaced-полей */
      ;
  const schema *schema_ /* Схема, требуется для итерирования preplaced-полей */;

  explicit cxx11_constexpr
  field_iterator_ro(const details::field_loose *field, const void *pivot,
                    const fptu::schema *schema) cxx11_noexcept
      : field_(field),
        pivot_(pivot),
        schema_(schema) {
    constexpr_assert(!on_preplaced_field());
  }

  explicit cxx11_constexpr
  field_iterator_ro(const details::field_preplaced *field, const void *pivot,
                    const fptu::schema *schema) cxx11_noexcept
      : field_(field),
        pivot_(pivot),
        schema_(schema) {
    constexpr_assert(schema != nullptr);
    constexpr_assert(on_preplaced_field());
  }

  __pure_function static field_iterator_ro
  begin(const void *pivot, const fptu::schema *schema) cxx11_noexcept;
  cxx11_constexpr static field_iterator_ro
  end(const details::field_loose *index, const void *pivot,
      const fptu::schema *schema) cxx11_noexcept {
    return field_iterator_ro(index - 1, pivot, schema);
  }

  using compare_result_t = ptrdiff_t /* c++20 std::strong_ordering */;
  friend cxx14_constexpr compare_result_t compare(
      const field_iterator_ro &a, const field_iterator_ro &b) cxx11_noexcept {
    constexpr_assert(a.pivot_ == b.pivot_);
    const char *const pivot = static_cast<const char *>(a.pivot_);
    const ptrdiff_t diff_a = static_cast<const char *>(a.field_) - pivot;
    const ptrdiff_t diff_b = static_cast<const char *>(b.field_) - pivot;
    const ptrdiff_t pos_a =
        diff_a >= 0 ? diff_a : max_tuple_bytes_netto - diff_a;
    const ptrdiff_t pos_b =
        diff_b >= 0 ? diff_b : max_tuple_bytes_netto - diff_b;
    return pos_a - pos_b;
  }

  cxx11_constexpr bool on_preplaced_field() const cxx11_noexcept {
    return static_cast<const char *>(field_) >=
           static_cast<const char *>(pivot_);
  }

  cxx11_constexpr const details::field_loose *loose() const cxx11_noexcept {
    constexpr_assert(!on_preplaced_field());
    return static_cast<const details::field_loose *>(field_);
  }

  cxx11_constexpr const details::field_preplaced *
  preplaced() const cxx11_noexcept {
    constexpr_assert(on_preplaced_field());
    return static_cast<const details::field_preplaced *>(field_);
  }

  cxx11_constexpr ptrdiff_t preplaced_offset() const cxx11_noexcept {
    return static_cast<const char *>(field_) -
           static_cast<const char *>(pivot_);
  }

  __pure_function const token preplaced_token() const;

public:
#if __cplusplus >= 201703L
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = const dynamic_accessor_ro;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *;
  using reference = value_type &;
#endif

  field_iterator_ro() cxx11_noexcept : field_(nullptr),
                                       pivot_(nullptr),
                                       schema_(nullptr) {}
  explicit operator bool() const cxx11_noexcept { return field_ != nullptr; }
  cxx11_constexpr
  field_iterator_ro(const field_iterator_ro &) cxx11_noexcept = default;
  cxx14_constexpr field_iterator_ro &
  operator=(const field_iterator_ro &) cxx11_noexcept = default;

  __pure_function dynamic_accessor_ro operator*() const {
    if (on_preplaced_field()) {
      const auto ident = preplaced_token();
      return dynamic_accessor_ro(
          static_cast<const details::field_preplaced *>(field_), ident);
    }

    return dynamic_accessor_ro(
        loose(),
        token(loose()->genus_and_id,
              /* При чтении можно обойтись без всех token-флажков:
                 - is_collection: не имеет значения при чтении одного
                 текущего экземпляра поля.
                 - is_discernible_null: не имеет значения, ибо точно
                 знаем что loose-поле есть.
                 - is_rangechecking/is_saturated: не нужен при чтении. */
              false, false, false));
  }

  __pure_function dynamic_accessor_ro operator->() const { return operator*(); }

  __pure_function token field_token_nothrow() const cxx11_noexcept {
    return on_preplaced_field() ? schema_->by_offset(preplaced_offset())
                                : schema_->by_loose(loose());
  }
  __pure_function token field_token() const;
  __pure_function genus field_type() const {
    if (on_preplaced_field())
      return preplaced_token().type();
    return loose()->type();
  }

  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  __pure_function bool operator==(const TOKEN &ident) const {
    if (on_preplaced_field())
      return ident.is_preplaced() && preplaced_token() == ident;
    return ident.is_loose() &&
           loose()->genus_and_id == details::tag2genus_and_id(ident.tag());
  }
  FPTU_TEMPLATE_FOR_STATIC_TOKEN
  __pure_function bool operator!=(const TOKEN &ident) const {
    return !operator==(ident);
  }
  __pure_function bool operator==(const token &ident) const;
  __pure_function bool operator!=(const token &ident) const {
    return !operator==(ident);
  }

  cxx11_constexpr bool
  operator==(const field_iterator_ro &other) const cxx11_noexcept {
    return field_ == other.field_;
  }
  cxx11_constexpr bool
  operator!=(const field_iterator_ro &other) const cxx11_noexcept {
    return !operator==(other);
  }

  friend cxx14_constexpr bool
  operator<(const field_iterator_ro &a,
            const field_iterator_ro &b) cxx11_noexcept {
    return compare(a, b) < 0;
  }
  friend cxx14_constexpr bool
  operator<=(const field_iterator_ro &a,
             const field_iterator_ro &b) cxx11_noexcept {
    return compare(a, b) <= 0;
  }
  friend cxx14_constexpr bool
  operator>=(const field_iterator_ro &a,
             const field_iterator_ro &b) cxx11_noexcept {
    return compare(a, b) >= 0;
  }
  friend cxx14_constexpr bool
  operator>(const field_iterator_ro &a,
            const field_iterator_ro &b) cxx11_noexcept {
    return compare(a, b) > 0;
  }

  cxx11_constexpr bool
  operator==(const dynamic_collection_iterator_ro &iter) const cxx11_noexcept {
    return field_ == iter.field_;
  }
  cxx11_constexpr bool
  operator!=(const dynamic_collection_iterator_ro &iter) const cxx11_noexcept {
    return field_ != iter.field_;
  }
  friend cxx11_constexpr bool
  operator==(const dynamic_collection_iterator_ro &a,
             const field_iterator_ro &b) cxx11_noexcept {
    return b == a;
  }
  friend cxx11_constexpr bool
  operator!=(const dynamic_collection_iterator_ro &a,
             const field_iterator_ro &b) cxx11_noexcept {
    return b != a;
  }

  cxx11_constexpr bool
  operator==(const loose_iterator_ro &iter) const cxx11_noexcept {
    return field_ == iter.field_;
  }
  cxx11_constexpr bool
  operator!=(const loose_iterator_ro &iter) const cxx11_noexcept {
    return field_ != iter.field_;
  }
  friend cxx11_constexpr bool
  operator==(const loose_iterator_ro &a,
             const field_iterator_ro &b) cxx11_noexcept {
    return b == a;
  }
  friend cxx11_constexpr bool
  operator!=(const loose_iterator_ro &a,
             const field_iterator_ro &b) cxx11_noexcept {
    return b != a;
  }

  field_iterator_ro &operator++();
  field_iterator_ro operator++(int) const {
    field_iterator_ro iterator(*this);
    return ++iterator;
  }
  field_iterator_ro &operator--();
  field_iterator_ro operator--(int) const {
    field_iterator_ro iterator(*this);
    return --iterator;
  }
};

namespace details {

class FPTU_API_TYPE tuple_ro : private stretchy_value_tuple,
                               public crtp_getter<tuple_ro> {
  friend class crtp_getter<tuple_ro>;
  template <typename> friend class crtp_setter;
  friend class tuple_rw;
  friend class fptu::tuple_ro_managed /* mostly for assertions */;
  friend class fptu::loose_iterator_ro;
  friend class fptu::field_iterator_ro;
  static inline const char *
  inline_lite_checkup(const void *ptr, std::size_t bytes) cxx11_noexcept;

  tuple_ro() = delete;
  ~tuple_ro() = delete;
  tuple_ro &operator=(const tuple_ro &) = delete;

public:
  static const char *lite_checkup(const void *ptr,
                                  std::size_t bytes) cxx11_noexcept;
  static const char *audit(const void *ptr, std::size_t bytes,
                           const fptu::schema *schema, audit_holes_info &);
  static const char *audit(const void *ptr, std::size_t bytes,
                           const fptu::schema *schema,
                           bool holes_are_not_allowed) {
    audit_holes_info holes_info;
    const char *trouble = audit(ptr, bytes, schema, holes_info);
    if (unlikely(trouble))
      return trouble;
    if (holes_are_not_allowed) {
      if (unlikely(holes_info.count))
        return "tuple have holes";
      assert(holes_info.volume == 0);
    }
    return nullptr;
  }
  static const char *audit(const tuple_ro *self, const fptu::schema *schema,
                           bool holes_are_not_allowed) {
    return audit(self, self ? self->size() : 0, schema, holes_are_not_allowed);
  }
  static const tuple_ro *make_from_buffer(const void *ptr, std::size_t bytes,
                                          const fptu::schema *schema,
                                          bool skip_validation);

  const char *audit(const fptu::schema *schema,
                    bool holes_are_not_allowed) const {
    return audit(this, schema, holes_are_not_allowed);
  }
  cxx11_constexpr bool empty() const cxx11_noexcept {
    return stretchy_value_tuple::brutto_units < 2;
  }
  cxx11_constexpr std::size_t size() const cxx11_noexcept {
    return stretchy_value_tuple::length();
  }
  cxx11_constexpr const void *data() const cxx11_noexcept { return this; }
  cxx14_constexpr std::size_t payload_size() const cxx11_noexcept {
    return stretchy_value_tuple::payload_bytes();
  }
  cxx11_constexpr const void *payload() const cxx11_noexcept {
    return stretchy_value_tuple::begin_data_bytes();
  }
  cxx11_constexpr std::size_t index_size() const cxx11_noexcept {
    return stretchy_value_tuple::index_size();
  }
  cxx11_constexpr bool is_sorted() const cxx11_noexcept {
    return stretchy_value_tuple::is_sorted();
  }
  cxx11_constexpr bool have_preplaced() const cxx11_noexcept {
    return stretchy_value_tuple::have_preplaced();
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

  cxx11_constexpr loose_iterator_ro cbegin_loose() const cxx11_noexcept {
    return loose_iterator_ro(end_index() - 1);
  }
  cxx11_constexpr loose_iterator_ro cend_loose() const cxx11_noexcept {
    return loose_iterator_ro(begin_index() - 1);
  }
  cxx11_constexpr loose_iterator_ro begin_loose() const cxx11_noexcept {
    return cbegin_loose();
  }
  cxx11_constexpr loose_iterator_ro end_loose() const cxx11_noexcept {
    return cend_loose();
  }

  friend std::ostream &operator<<(std::ostream &, const tuple_ro *);
};

inline iovec_thunk::iovec_thunk(const tuple_ro *ro)
    : iovec(ro, ro ? ro->size() : 0) {}

FPTU_API std::ostream &operator<<(std::ostream &out, const tuple_ro *);

} // namespace details
} // namespace fptu
