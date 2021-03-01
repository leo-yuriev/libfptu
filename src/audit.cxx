/*
 *  Fast Positive Tuples (libfptu), aka Позитивные Кортежи
 *  Copyright 2016-2019 Leonid Yuriev <leo@yuriev.ru>
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

#include "fast_positive/tuples/internal.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "fast_positive/tuples/details/warnings_push_pt.h"

namespace fptu {
namespace details {
namespace {

#ifdef NDEBUG
#define AUDIT_FAILURE(x) (x)
#else
#define AUDIT_FAILURE(x) (abort(), (x))
#endif

class auditor {
  using offset = uint16_t;
  using interval = std::pair<offset, offset>;
  using map_vector = std::vector<interval, onstack_short_allocator<interval>>;
  map_vector map_;
  std::size_t holes_count_, holes_volume_;

  struct map_less {
    /* bool operator()(const interval &a, const interval &b) const
    cxx11_noexcept { return a.first < b.first;
    } */
    bool operator()(const offset &a, const interval &b) const cxx11_noexcept {
      return a < b.first;
    }
    /* bool operator()(const interval &a, const offset &b) const cxx11_noexcept
    { return a.first < b;
    }
    bool operator()(const offset &a, const offset &b) const cxx11_noexcept {
      return a < b;
    } */
  };

  bool check_map() const;
  bool map_insert(const offset begin, const offset end);

public:
  std::size_t holes_count() const { return holes_count_; }
  std::size_t holes_volume() const { return holes_volume_; }
  bool map_empty() const { return map_.empty(); }
  void map_reset(unsigned limit) {
    map_.clear();
    map_.reserve(limit);
  }
  bool map_insert(const ptrdiff_t begin, const ptrdiff_t end) {
    FPTU_ENSURE(begin < end && size_t(begin) <= UINT16_MAX &&
                size_t(end) <= UINT16_MAX);
    return map_insert(uint16_t(begin), uint16_t(end));
  }

  unsigned map_begin() const { return map_.empty() ? 0 : map_.front().first; }
  unsigned map_end() const { return map_.empty() ? 0 : map_.back().second; }

  bool map_have_holes() const {
    assert(check_map());
    return map_.size() > 1;
  }

  std::size_t map_items() const { return map_.size(); }

  auditor(onstack_allocation_arena &area, std::size_t limit)
      : map_(area), holes_count_(0), holes_volume_(0) {
    static_assert(sizeof(interval) == 2 * sizeof(offset), "WTF?");
    FPTU_ENSURE(limit <= fptu::max_fields);
    map_.reserve(limit);
  }

  const char *check_chunk(const unit_t *chunk_begin,
                          const std::size_t chunk_units,
                          const unit_t *begin_payload,
                          const unit_t *end_payload);

  const char *check_loose(const field_loose *loose, const unit_t *begin_payload,
                          const unit_t *end_payload);
};

bool __maybe_unused auditor::check_map() const {
  for (size_t i = 0; i < map_.size(); ++i) {
    if (i > 0) {
      assert(map_[i - 1].first < map_[i].first);
      assert(map_[i - 1].second < map_[i].first);
      if (map_[i - 1].first >= map_[i].first ||
          map_[i - 1].second >= map_[i].first)
        return AUDIT_FAILURE(false);
    }
    assert(map_[i].first < map_[i].second);
    if (map_[i].first >= map_[i].second)
      return AUDIT_FAILURE(false);
  }
  return true;
}

bool auditor::map_insert(const offset begin, const offset end) {
  assert(begin < end);
  assert(check_map());

  auto const next = std::upper_bound(map_.begin(), map_.end(), end, map_less());
  assert(next == map_.end() || end < next->first) /* paranoia */;

  if (next == map_.begin()) {
    map_.insert(next, interval(begin, end));
    assert(check_map());
    return true;
  }

  auto const here = std::prev(next);
  if (here->second < begin) {
    map_.insert(next, interval(begin, end));
    assert(check_map());
    return true;
  }

  assert(begin <= here->second && here->first <= end) /* paranoia */;
  if (here->first == end) {
    /* should merge */
    if (here != map_.begin()) {
      auto const prev = std::prev(here);
      if (prev->second > begin)
        return false /* overlapped */;
      if (prev->second == begin) {
        /* merge more */
        prev->second = here->second;
        map_.erase(here);
        assert(check_map());
        return true;
      }
    }
    here->first = begin;
    assert(check_map());
    return true;
  }

  if (likely(begin == here->second)) {
    here->second = end;
    assert(check_map());
    return true;
  }

  assert(begin < here->second && here->first < end) /* paranoia */;
  return AUDIT_FAILURE(false) /* overlapped */;
}

const char *auditor::check_chunk(const unit_t *chunk_begin,
                                 const std::size_t chunk_units,
                                 const unit_t *begin_payload,
                                 const unit_t *end_payload) {
  assert(chunk_units > 0);
  if (unlikely(chunk_begin < begin_payload))
    return AUDIT_FAILURE("chunk_begin < begin_payload");
  if (unlikely(chunk_units > fptu::max_field_bytes))
    return AUDIT_FAILURE("chunk_units > fptu::max_field_bytes");
  if (unlikely(chunk_begin + chunk_units > end_payload))
    return AUDIT_FAILURE("chunk_end > end_payload");

  const ptrdiff_t offset_begin = chunk_begin - begin_payload;
  const ptrdiff_t offset_end = offset_begin + chunk_units;
  assert(offset_begin >= 0 && offset_begin <= UINT16_MAX);
  assert(offset_end > offset_begin && offset_end <= UINT16_MAX + 1);
  if (unlikely(!map_insert(uint16_t(offset_begin), uint16_t(offset_end))))
    return AUDIT_FAILURE("chunk overlaps with field's payload or hole");
  return nullptr;
}

const char *auditor::check_loose(const field_loose *loose,
                                 const unit_t *begin_payload,
                                 const unit_t *end_payload) {
  const genus type = tag2genus(loose->genus_and_id);
  if (is_inplaced(type))
    return nullptr;

  if (type == genus::hole) {
    const std::size_t units = loose->hole_get_units();
    holes_count_ += 1;
    if (units == 0) {
      if (unlikely(loose->relative.have_payload()))
        return AUDIT_FAILURE("tuple.pure_hole.payload != null");
      return nullptr;
    }
    if (!loose->relative.have_payload())
      return AUDIT_FAILURE("tuple.non_pure_hole.payload == null");
    holes_volume_ += units;
    return check_chunk(loose->hole_begin(), units, begin_payload, end_payload);
  }

  if (is_fixed_size(type)) {
    if (!loose->relative.have_payload())
      return AUDIT_FAILURE("tuple.fixed_size_loose_field.payload == null");
    return check_chunk(loose->relative.payload()->flat,
                       loose_units_dynamic(type), begin_payload, end_payload);
  }

  /* stretchy */
  if (loose->relative.have_payload())
    return check_chunk(loose->relative.payload()->flat,
                       loose->relative.payload()->stretchy.brutto_units(type),
                       begin_payload, end_payload);
  return nullptr;
}
} // namespace

__hot const char *audit_tuple(const fptu::schema *const schema,
                              const field_loose *const index_begin,
                              const void *const pivot, const void *const end,
                              const audit_flags flags,
                              audit_holes_info &holes_info) {
  const field_loose *const index_end = static_cast<const field_loose *>(pivot);
  const unit_t *const payload_begin = static_cast<const unit_t *>(pivot);
  const unit_t *const payload_end = static_cast<const unit_t *>(end);
  assert(index_begin <= index_end && payload_begin <= payload_end);

  const ptrdiff_t payload_bytes =
      static_cast<const char *>(end) - static_cast<const char *>(pivot);
  const std::size_t preplaced_bytes = schema ? schema->preplaced_bytes() : 0;
  assert(payload_bytes >= 0 && ptrdiff_t(preplaced_bytes) >= 0);

  const char *trouble;
  onstack_allocation_arena area;
  auditor validator(area,
                    index_end - index_begin +
                        (schema ? schema->number_of_stretchy_preplaced() : 0));
  for (const field_loose *loose = index_end; --loose >= index_begin;) {
    trouble = validator.check_loose(loose, payload_begin, payload_end);
    if (unlikely(trouble))
      return AUDIT_FAILURE(trouble);
  }

  if (unlikely(validator.holes_count() > UINT16_MAX / 2))
    return AUDIT_FAILURE("tool many holes");
  if (unlikely(validator.holes_volume() > UINT16_MAX))
    return AUDIT_FAILURE("tool large holes volume");
  holes_info.count = uint16_t(validator.holes_count());
  holes_info.volume = uint16_t(validator.holes_volume());

  const bool tuple_have_preplaced =
      (flags & audit_flags::audit_tuple_have_preplaced) != 0;
  if (tuple_have_preplaced && !schema)
    return AUDIT_FAILURE("schema is required for tuples with preplaced fields");
  if (tuple_have_preplaced != (preplaced_bytes != 0))
    return AUDIT_FAILURE("preplaced-fields presence mismatch with schema");

  if (preplaced_bytes) {
    if (!validator.map_empty() &&
        unlikely(bytes2units(preplaced_bytes) > validator.map_begin()))
      return AUDIT_FAILURE("schema.preplaced > tuple.loose_payload");
    if (unlikely(ptrdiff_t(preplaced_bytes) > payload_bytes))
      return AUDIT_FAILURE("schema.preplaced > tuple.whole_payload");
    for (const auto &field_token : schema->tokens()) {
      if (!field_token.is_preplaced())
        break;
      const std::size_t preplaced_offset = field_token.preplaced_offset();
      assert(preplaced_offset + preplaced_bytes_dynamic(field_token.type()) <=
             preplaced_bytes);
      if (is_fixed_size(field_token.type()))
        continue;

      const field_preplaced *const target =
          erthink::constexpr_pointer_cast<const field_preplaced *>(
              static_cast<const char *>(pivot) + preplaced_offset);

      if (!target->relative.have_payload())
        continue;
      const auto payload = target->relative.payload();
      trouble = validator.check_chunk(
          payload->flat, payload->stretchy.brutto_units(field_token.type()),
          payload_begin, payload_end);
      if (unlikely(trouble))
        return AUDIT_FAILURE(trouble);
    }
  }

  if (!validator.map_empty()) {
    const auto map_begin = validator.map_begin();
    if (unlikely(tuple_have_preplaced != (map_begin > 0)))
      return AUDIT_FAILURE(
          tuple_have_preplaced
              ? "preplaced-flag is ON but corresponding fields absent"
              : "preplaced-flag is OFF but tuple have hole for ones");

    if (unlikely(map_begin != bytes2units(preplaced_bytes)))
      return AUDIT_FAILURE(
          tuple_have_preplaced
              ? "preplaced-flag is ON but corresponding fields absent"
              : "preplaced-flag is OFF but tuple have hole for ones");

    const unit_t *const allocated_end = payload_begin + validator.map_end();
    if (unlikely(allocated_end != payload_end))
      return AUDIT_FAILURE((allocated_end > payload_end)
                               ? "allocated beyond end of tuple"
                               : "lose space at the end of tuple");

    if (unlikely(validator.map_have_holes()))
      return AUDIT_FAILURE("tuple have unaccounted holes");

    if ((flags & audit_flags::audit_adjacent_holes) != 0 &&
        holes_info.count > 1) {
      validator.map_reset(holes_info.count);
      std::size_t count = 0;
      for (const field_loose *loose = index_end; --loose >= index_begin;) {
        if (!loose->is_hole() || loose->hole_get_units() == 0)
          continue;
        const bool inserted =
            validator.map_insert(loose->hole_begin() - payload_begin,
                                 loose->hole_end() - payload_begin);
        assert(loose->hole_begin() >= payload_begin &&
               loose->hole_begin() - payload_begin < UINT16_MAX);
        assert(loose->hole_end() >= payload_begin &&
               loose->hole_end() - payload_begin < UINT16_MAX);
        assert(inserted);
        (void)inserted;
        if (unlikely(++count != validator.map_items()))
          return AUDIT_FAILURE("tuple have unmerged adjacent holes");
      }
    }
  }

  if ((flags & audit_flags::audit_tuple_sorted_loose) != 0 &&
      !std::is_sorted(index_begin, index_end,
                      [](const field_loose &a, const field_loose &b) {
                        return a.genus_and_id < b.genus_and_id;
                      }))
    return AUDIT_FAILURE("loose fields mis-sorted");

  return nullptr;
}

__always_inline const char *
tuple_ro::inline_lite_checkup(const void *ptr,
                              std::size_t bytes) cxx11_noexcept {
  if (unlikely(ptr == nullptr))
    return AUDIT_FAILURE("hollow tuple (nullptr)");

  if (unlikely(bytes < sizeof(unit_t)))
    return AUDIT_FAILURE("hollow tuple (too short)");
  if (unlikely(bytes > fptu::max_tuple_bytes_netto))
    return AUDIT_FAILURE("tuple too large");
  if (unlikely(bytes % sizeof(unit_t)))
    return AUDIT_FAILURE("odd tuple size");

  const tuple_ro *const self = static_cast<const tuple_ro *>(ptr);
  if (unlikely(bytes != self->size()))
    return AUDIT_FAILURE("tuple size mismatch");

  const std::size_t index_size = self->index_size();
  if (unlikely(index_size > fptu::max_fields))
    return AUDIT_FAILURE("index too large (many loose fields)");

  if (self->pivot() > self->end_data_bytes())
    return AUDIT_FAILURE("tuple.pivot > tuple.end");

  return nullptr;
}

__hot const char *tuple_ro::lite_checkup(const void *ptr,
                                         std::size_t bytes) cxx11_noexcept {
  return inline_lite_checkup(ptr, bytes);
}

__hot const char *tuple_ro::audit(const void *ptr, std::size_t bytes,
                                  const fptu::schema *schema,
                                  audit_holes_info &holes_info) {

  const char *trouble = inline_lite_checkup(ptr, bytes);
  if (unlikely(trouble != nullptr))
    return trouble;

  const tuple_ro *const self = static_cast<const tuple_ro *>(ptr);
  audit_flags flags = audit_flags::audit_default;
  if (self->is_sorted())
    flags |= audit_flags::audit_tuple_sorted_loose;
  if (self->have_preplaced())
    flags |= audit_flags::audit_tuple_have_preplaced;

  return audit_tuple(schema, self->begin_index(), self->pivot(),
                     self->end_data_bytes(), flags, holes_info);
}

__hot const char *tuple_rw::audit(const tuple_rw *self) cxx11_noexcept {
  if (!self)
    return AUDIT_FAILURE("hollow tuple (nullptr)");

  if (unlikely(self->head_ > self->pivot_))
    return AUDIT_FAILURE("tuple.head > tuple.pivot");

  if (unlikely(self->pivot_ > self->tail_))
    return AUDIT_FAILURE("tuple.pivot > tuple.tail");

  if (unlikely(self->tail_ > self->end_))
    return AUDIT_FAILURE("tuple.tail > tuple.end");

  if (unlikely(self->end_ > max_tuple_units_netto))
    return AUDIT_FAILURE("tuple.end > fptu::max_tuple_bytes");

  if (unlikely(self->pivot_ - self->head_ > fptu::max_fields))
    return AUDIT_FAILURE("tuple.loose_fields > fptu::max_fields");

  if (unlikely(self->junk_.count > self->pivot_ - self->head_))
    return AUDIT_FAILURE("tuple.junk.holes_count > tuple.index_size");

  if (unlikely(self->junk_.volume > self->tail_ - self->pivot_))
    return AUDIT_FAILURE("tuple.junk.data_units > tuple.payload_size");

  audit_flags flags = audit_flags::audit_adjacent_holes;
  if (self->is_sorted())
    flags |= audit_flags::audit_tuple_sorted_loose;
  if (self->have_preplaced())
    flags |= audit_flags::audit_tuple_have_preplaced;

  audit_holes_info holes_info;
  const char *trouble =
      audit_tuple(self->schema_, self->begin_index(), self->pivot(),
                  self->end_data_bytes(), flags, holes_info);
  if (unlikely(trouble))
    return trouble;

  if (unlikely(self->junk_.count != holes_info.count))
    return AUDIT_FAILURE(self->junk_.count ? "tuple.holes_count mismatch"
                                           : "tuple have holes");
  if (unlikely(self->junk_.volume != holes_info.volume))
    return AUDIT_FAILURE(self->junk_.volume ? "tuple.junk_volume mismatch"
                                            : "tuple have unaccounted holes");
  return nullptr;
}

} // namespace details
} // namespace fptu

#include "fast_positive/tuples/details/warnings_pop.h"
