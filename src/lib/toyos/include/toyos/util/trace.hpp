/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <compiler.h>
#include <cstdint>
#include <pprintpp/pprintpp.hpp>
#include <stdio.h>

enum : uint64_t
{
   TRACE_VERBOSE = 1ul << 0,
   TRACE_BUDDY = 1ul << 1,
   TRACE_BOOT = 1ul << 2,
   TRACE_MEMMAP = 1ul << 3,
   TRACE_THREAD = 1ul << 4,
   TRACE_GSI = 1ul << 5,
   TRACE_LOADER = 1ul << 6,
   TRACE_MSR = 1ul << 7,
   TRACE_BIOS = 1ul << 8,
   TRACE_MMU = 1ul << 9,
   TRACE_LAPIC = 1ul << 10,
   TRACE_IOAPIC = 1ul << 11,
   TRACE_PIC = 1ul << 12,
   TRACE_PCI = 1ul << 13,
   TRACE_HPET = 1ul << 14,
   TRACE_XHCI = 1ul << 15,
   TRACE_QUIRK = 1ul << 16,
   TRACE_ROOTTASK = 1ul << 17,
   TRACE_HOTPLUG = 1ul << 18,
   TRACE_SRIOV = 1ul << 19,

   TRACE_MASK = TRACE_BOOT | TRACE_MEMMAP | TRACE_LOADER | TRACE_BIOS | TRACE_PIC | TRACE_IOAPIC | TRACE_XHCI
                | TRACE_QUIRK | TRACE_HOTPLUG | TRACE_SRIOV
};

/// Strip all directories from a file path.
///
/// Example: strip_file_path("../foo/bar.cpp") -> "bar.cpp"
inline constexpr const char* strip_file_path(const char* file_name)
{
   const char* shortened_name{ file_name };

   // Walk to the last forward slash. We could achieve this more elegantly with the STL, but as this file is included
   // everywhere and we can't hide the implementation of this function, let's not pull in `<algorithm>` for everyone.
   for (; file_name[0]; file_name++) {
      if (file_name[0] == '/') {
         shortened_name = file_name + 1;
      }
   }

   return shortened_name;
}

#define print_macro(level, fmtstr, ...)                                                     \
   do {                                                                                     \
      constexpr const char* __short_path__{ strip_file_path(__FILE__) };                    \
      pprintf("[" #level " {s}:{}] " fmtstr "\n", __short_path__, __LINE__, ##__VA_ARGS__); \
   } while (0);

#define info(fmtstr, ...) print_macro(INF, fmtstr, ##__VA_ARGS__)
#define warning(fmtstr, ...) print_macro(WRN, fmtstr, ##__VA_ARGS__)

#define trace(context, fmtstr, ...)                \
   do {                                            \
      if ((TRACE_MASK & (context)) == (context)) { \
         info(fmtstr, ##__VA_ARGS__);              \
      }                                            \
   } while (0);

#ifdef __cpp_exceptions
#include <exception>
#define INTERNAL_TRAP(...) throw std::exception();
#else
#define INTERNAL_TRAP(...) __builtin_trap();
#endif

#define ASSERT(cond, fmtstr, ...)                                  \
   do {                                                            \
      if (UNLIKELY(!(cond))) {                                     \
         pprintf("[%s:%d]  ", __FILE__, __LINE__);                 \
         pprintf("Assertion failed: " fmtstr "\n", ##__VA_ARGS__); \
         INTERNAL_TRAP();                                          \
      }                                                            \
   } while (0);

#define PANIC_ON(cond, ...) ASSERT(!(cond), ##__VA_ARGS__)
#define PANIC_UNLESS(cond, ...) ASSERT(cond, ##__VA_ARGS__)

#define PANIC(...)                   \
   do {                              \
      PANIC_ON(true, ##__VA_ARGS__); \
      INTERNAL_TRAP();               \
   } while (0)

#define DEFAULT_TO_PANIC(...)               \
   default:                                 \
      pprintf("%s: ", __PRETTY_FUNCTION__); \
      PANIC_ON(true, ##__VA_ARGS__);        \
      break

#define WARN_ONCE(...)              \
   do {                             \
      static bool do_print{ true }; \
      if (do_print) {               \
         do_print = false;          \
         warning(__VA_ARGS__);      \
      }                             \
   } while (0);
