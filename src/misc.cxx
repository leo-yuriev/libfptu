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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "fast_positive/tuples/internal.h"

#ifdef _MSC_VER
#pragma warning(disable : 4996) /* 'getenv' : This function or variable may be \
                       unsafe. Consider using _dupenv_s instead. To disable    \
                       deprecation, use _CRT_SECURE_NO_WARNINGS. */
#endif                          /* _MSC_VER (warnings) */

bool fptu_is_under_valgrind(void) cxx11_noexcept {
#ifdef RUNNING_ON_VALGRIND
  if (RUNNING_ON_VALGRIND)
    return true;
#endif
  const char *str = getenv("RUNNING_ON_VALGRIND");
  if (str)
    return strcmp(str, "0") != 0;
  return false;
}

namespace fptu {

string_view::const_reference string_view::at(string_view::size_type pos) const {
  if (unlikely(pos >= size()))
    throw std::out_of_range("fptu::string_view::at(): pos >= size()");
  return str[pos];
}

__cold std::string format_va(const char *fmt, va_list ap) {
  va_list ones;
  va_copy(ones, ap);
#ifdef _MSC_VER
  int needed = _vscprintf(fmt, ap);
#else
  int needed = vsnprintf(nullptr, 0, fmt, ap);
#endif
  assert(needed >= 0);
  std::string result;
  result.reserve((size_t)needed + 1);
  result.resize((size_t)needed, '\0');
  assert((int)result.capacity() > needed);
  int actual = vsnprintf((char *)result.data(), result.capacity(), fmt, ones);
  assert(actual == needed);
  (void)actual;
  va_end(ones);
  return result;
}

__cold std::string format(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  std::string result = format_va(fmt, ap);
  va_end(ap);
  return result;
}

__cold std::string hexadecimal(const void *data, std::size_t bytes) {
  std::string result;
  if (bytes > 0) {
    result.reserve(bytes * 2 + 1);
    const uint8_t *ptr = (const uint8_t *)data;
    const uint8_t *const end = ptr + bytes;
    do {
      char high = *ptr >> 4;
      char low = *ptr & 15;
      result.push_back((high < 10) ? high + '0' : high - 10 + 'a');
      result.push_back((low < 10) ? low + '0' : low - 10 + 'a');
    } while (++ptr < end);
  }
  return result;
}

namespace details {

std::ostream &operator<<(std::ostream &out, const tuple_ro *) {
  return out << "tuple_ro{FIXME(" __FILE__ ":" STRINGIFY(__LINE__) ")}";
}

std::ostream &operator<<(std::ostream &out, const tuple_rw *) {
  return out << "tuple_rw{FIXME(" __FILE__ ":" STRINGIFY(__LINE__) ")}";
}

} // namespace details
} /* namespace fptu */

//----------------------------------------------------------------------------

/* LY: temporary workaround for Elbrus's memcmp() bug. */
#if defined(__e2k__) && !__GLIBC_PREREQ(2, 24)
FPTU_API int __hot __attribute__((weak))
mdbx_e2k_memcmp_bug_workaround(const void *s1, const void *s2, std::size_t n) {
  if (unlikely(n > 42
               /* LY: align followed access if reasonable possible */
               && (((uintptr_t)s1) & 7) != 0 &&
               (((uintptr_t)s1) & 7) == (((uintptr_t)s2) & 7))) {
    if (((uintptr_t)s1) & 1) {
      const int diff = *(uint8_t *)s1 - *(uint8_t *)s2;
      if (diff)
        return diff;
      s1 = (char *)s1 + 1;
      s2 = (char *)s2 + 1;
      n -= 1;
    }

    if (((uintptr_t)s1) & 2) {
      const uint16_t a = *(uint16_t *)s1;
      const uint16_t b = *(uint16_t *)s2;
      if (likely(a != b))
        return (__builtin_bswap16(a) > __builtin_bswap16(b)) ? 1 : -1;
      s1 = (char *)s1 + 2;
      s2 = (char *)s2 + 2;
      n -= 2;
    }

    if (((uintptr_t)s1) & 4) {
      const uint32_t a = *(uint32_t *)s1;
      const uint32_t b = *(uint32_t *)s2;
      if (likely(a != b))
        return (__builtin_bswap32(a) > __builtin_bswap32(b)) ? 1 : -1;
      s1 = (char *)s1 + 4;
      s2 = (char *)s2 + 4;
      n -= 4;
    }
  }

  while (n >= 8) {
    const uint64_t a = *(uint64_t *)s1;
    const uint64_t b = *(uint64_t *)s2;
    if (likely(a != b))
      return (__builtin_bswap64(a) > __builtin_bswap64(b)) ? 1 : -1;
    s1 = (char *)s1 + 8;
    s2 = (char *)s2 + 8;
    n -= 8;
  }

  if (n & 4) {
    const uint32_t a = *(uint32_t *)s1;
    const uint32_t b = *(uint32_t *)s2;
    if (likely(a != b))
      return (__builtin_bswap32(a) > __builtin_bswap32(b)) ? 1 : -1;
    s1 = (char *)s1 + 4;
    s2 = (char *)s2 + 4;
  }

  if (n & 2) {
    const uint16_t a = *(uint16_t *)s1;
    const uint16_t b = *(uint16_t *)s2;
    if (likely(a != b))
      return (__builtin_bswap16(a) > __builtin_bswap16(b)) ? 1 : -1;
    s1 = (char *)s1 + 2;
    s2 = (char *)s2 + 2;
  }

  return (n & 1) ? *(uint8_t *)s1 - *(uint8_t *)s2 : 0;
}

FPTU_API int __hot __attribute__((weak))
mdbx_e2k_strcmp_bug_workaround(const char *s1, const char *s2) {
  while (true) {
    int diff = *(uint8_t *)s1 - *(uint8_t *)s2;
    if (likely(diff != 0) || *s1 == '\0')
      return diff;
    s1 += 1;
    s2 += 1;
  }
}

FPTU_API int __hot __attribute__((weak))
mdbx_e2k_strncmp_bug_workaround(const char *s1, const char *s2, std::size_t n) {
  while (n > 0) {
    int diff = *(uint8_t *)s1 - *(uint8_t *)s2;
    if (likely(diff != 0) || *s1 == '\0')
      return diff;
    s1 += 1;
    s2 += 1;
    n -= 1;
  }
  return 0;
}

FPTU_API std::size_t __hot __attribute__((weak))
mdbx_e2k_strlen_bug_workaround(const char *s) {
  std::size_t n = 0;
  while (*s) {
    s += 1;
    n += 1;
  }
  return n;
}

FPTU_API std::size_t __hot __attribute__((weak))
mdbx_e2k_strnlen_bug_workaround(const char *s, std::size_t maxlen) {
  std::size_t n = 0;
  while (maxlen > n && *s) {
    s += 1;
    n += 1;
  }
  return n;
}
#endif /* Elbrus's memcmp() bug. */

FPTU_API fptu::string_view std::to_string(const fptu::genus type) {
  static const char *typenames[32] = {"text",
                                      "varbin",
                                      "nested",
                                      "property",
                                      "i8",
                                      "u8",
                                      "i16",
                                      "u16",
                                      "i32",
                                      "u32",
                                      "f32",
                                      "t32",
                                      "i64",
                                      "u64",
                                      "f64",
                                      "d64",
                                      "t64",
                                      "bin96",
                                      "bin128",
                                      "bin160",
                                      "bin192",
                                      "bin224",
                                      "bin256",
                                      "bin320",
                                      "bin384",
                                      "bin512",
                                      "app_reserved_64",
                                      "app_reserved_128",
                                      "mac",
                                      "ip",
                                      "ipnet",
                                      "hole"};
  assert(type >= 0 && type <= 31);
  return typenames[type];
}
