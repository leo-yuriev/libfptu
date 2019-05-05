﻿/*
 * Copyright 2016-2019 libfptu authors: please see AUTHORS file.
 *
 * This file is part of libfptu, aka "Fast Positive Tuples".
 *
 * libfptu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libfptu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libfptu.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../fptu_test.h"
#include "fast_positive/details/cpu_features.h"

#include <array>
#include <vector>

#include "fast_positive/details/warnings_push_pt.h"

//------------------------------------------------------------------------------

#pragma pack(push, 1)
struct Foo {
  char x;
  int Bar;
};
#pragma pack(pop)

struct MyToken_FooBar_int : public FPTU_TOKEN(Foo, Bar) {
  MyToken_FooBar_int() noexcept {
    static_assert(static_offset == 1, "WTF?");
    static_assert(std::is_base_of<::fptu::details::token_static_tag,
                                  MyToken_FooBar_int>::value,
                  "WTF?");
    static_assert(MyToken_FooBar_int::is_static_token::value, "WTF?");
  }
};

FPTU_TEMPLATE_FOR_STATIC_TOKEN
bool probe2static(const TOKEN &token) {
  (void)token;
  return true;
}

bool probe2static(const fptu::token &token) {
  (void)token;
  return false;
}

TEST(Token, StaticPreplaced) {
  MyToken_FooBar_int token;

  EXPECT_TRUE(token.is_preplaced());
  EXPECT_FALSE(token.is_loose());
  EXPECT_FALSE(token.is_inlay());
  EXPECT_FALSE(token.is_collection());
  EXPECT_EQ(fptu::genus::i32, token.type());
  EXPECT_TRUE(probe2static(token));
  EXPECT_TRUE(MyToken_FooBar_int::is_static_preplaced());
  EXPECT_TRUE(MyToken_FooBar_int::is_static_token::value);

#ifndef __clang__
  const fptu::tuple_ro_weak tuple_ro;
  /* FIXME: CLANG ?-6-7-8 WTF? */
  EXPECT_THROW(tuple_ro.collection(token).empty(), ::fptu::collection_required);
#endif
}

#ifdef __clang__
TEST(clang_WTF, DISABLED_ExceptionHandling) {
#else
TEST(clang_WTF, ExceptionHandling) {
#endif
  try {
    bool got_collection_required_exception = false;
    try {
      // fptu::throw_collection_required();
      const fptu::tuple_ro_weak tuple_ro;
      MyToken_FooBar_int token;
      tuple_ro.collection(token).empty();
    } catch (const ::fptu::collection_required &) {
      got_collection_required_exception = true;
    }
    EXPECT_TRUE(got_collection_required_exception);
  } catch (const ::std::exception &e) {
    std::string msg = fptu::format("Unexpected exception type '%s': %s",
                                   typeid(e).name(), e.what());
    GTEST_FATAL_FAILURE_(msg.c_str());
  } catch (...) {
    GTEST_FATAL_FAILURE_("Unknown NOT std::exception");
  }
}

//------------------------------------------------------------------------------

TEST(Smoke, trivia_set) {
  fptu::tuple_rw_managed rw;
  fptu::token token(fptu::u16, 0);
  rw.set_u16(token, 42);
  auto value = rw.get_u16(token);
  EXPECT_EQ(42, value);
}

TEST(Smoke, trivia_autogrowth) {
  fptu::tuple_rw_managed rw;
  fptu::token token(fptu::text, 0, true);
  EXPECT_GT(fptu::max_tuple_bytes_netto, rw.capacity());
  for (int i = 1; i < 555; ++i)
    rw.insert_string(token,
                     fptu::format("This is the string #%*d.", i - 555, i));

  EXPECT_EQ(fptu::max_tuple_bytes_netto, rw.capacity());
}

TEST(Smoke, trivia_managing) {
  cxx14_constexpr fptu::token token_utc32(fptu::genus::datetime_utc, 0);
  cxx14_constexpr fptu::token token_datetime64(fptu::genus::datetime_utc, 0);
  cxx14_constexpr fptu::token token_i64(fptu::i64, 0);
  fptu::tuple_rw_fixed rw_fixed;
  rw_fixed.set_datetime(token_utc32, fptu::datetime_t::now());
  rw_fixed.set_datetime(token_utc32, fptu::datetime_t::now_coarse());
  rw_fixed.set_datetime(token_datetime64, fptu::datetime_t::now_fine());
  rw_fixed.set_integer(token_i64, INT64_MIN);
  rw_fixed.set_integer(token_i64, INT64_MAX);

  fptu::tuple_ro_weak ro_weak = rw_fixed.take_weak().first;
  EXPECT_FALSE(ro_weak.empty());
  EXPECT_TRUE(ro_weak.is_present(token_utc32));
  EXPECT_TRUE(ro_weak.is_present(token_datetime64));
  EXPECT_TRUE(ro_weak.is_present(token_i64));

  fptu::tuple_ro_managed ro_managed = rw_fixed.take_managed_clone().first;
  EXPECT_EQ(ro_weak.size(), ro_managed.size());
  EXPECT_EQ(0,
            std::memcmp(ro_weak.data(), ro_managed.data(), ro_managed.size()));

  ro_managed = rw_fixed.move_to_ro();
  EXPECT_EQ(ro_weak.size(), ro_managed.size());
  EXPECT_EQ(0,
            std::memcmp(ro_weak.data(), ro_managed.data(), ro_managed.size()));

  rw_fixed = std::move(ro_managed);
  ro_managed = rw_fixed.take_managed_clone().first;
  EXPECT_EQ(ro_weak.size(), ro_managed.size());
  EXPECT_EQ(0,
            std::memcmp(ro_weak.data(), ro_managed.data(), ro_managed.size()));

  rw_fixed = fptu::tuple_rw_fixed::clone(ro_managed);
  ro_weak = rw_fixed.take_weak().first;
  EXPECT_EQ(ro_weak.size(), ro_managed.size());
  EXPECT_EQ(0,
            std::memcmp(ro_weak.data(), ro_managed.data(), ro_managed.size()));

  rw_fixed = fptu::tuple_rw_fixed::clone(ro_weak);
  ro_weak = rw_fixed.take_weak().first;
  EXPECT_EQ(ro_weak.size(), ro_managed.size());
  EXPECT_EQ(0,
            std::memcmp(ro_weak.data(), ro_managed.data(), ro_managed.size()));

  EXPECT_EQ(1, ro_managed.get_buffer()->ref_counter);
  auto ro_managed2 = ro_managed;
  EXPECT_EQ(2, ro_managed.get_buffer()->ref_counter);
  ro_managed.purge();
  EXPECT_FALSE(ro_managed);
  EXPECT_EQ(1, ro_managed2.get_buffer()->ref_counter);
}

//------------------------------------------------------------------------------

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
