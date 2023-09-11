/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <compiler.h>

#include <cstdint>
#include <optional>
#include <string>

#pragma pack(push, 1)

namespace multiboot
{

   /**
 * Multiboot Information Structure
 *
 * This is the structure as passed by a multiboot-compliant loader.
 */
   struct multiboot_info
   {
      enum class flag : uint32_t
      {
         MEM = 1u << 0,
         DISK = 1u << 1,
         CMDLINE = 1u << 2,
      };

      uint32_t flags;  ///< Indicates the presence of fields in the structure.

      uint32_t mem_lower;  ///< KiB of lower memory available. Valid if flags[0] is set.
      uint32_t mem_upper;  ///< KiB of upper memory available. Valid if flags[0] is set.

      uint32_t boot_device;  ///< BIOS disk drive this OS was loaded from. Valid if flags[1] is set.

      uint32_t cmdline;  ///< Pointer to C-style cmdline string in physical memory. Valid if flags[2] is set.

      bool has_cmdline() const
      {
         return flags & uint32_t(flag::CMDLINE);
      }  ///< Returns true iff cmdline is valid.

      /// Returns an optional of a string. Valid iff cmdline field is valid.
      const std::optional<std::string> get_cmdline() const
      {
         if (not has_cmdline()) {
            return {};
         }
         return { reinterpret_cast<const char*>(cmdline) };
      }

      // The rest of the structure is currently unused.
   };

   struct multiboot_module
   {
      enum
      {
         MAGIC = 0x1BADB002,
         MAGIC_LDR = 0x2BADB002,
         HDR_MIN_SIZE = 12,
      };

      uint32_t magic, flags, checksum;
      uint32_t header_addr, load_addr, load_end_addr, bss_end_addr, entry_addr;

      bool __UBSAN_NO_UNSIGNED_OVERFLOW__ is_valid() const
      {
         return magic == MAGIC and (magic + flags + checksum) == 0;
      }
      bool address_header_valid() const
      {
         return flags & (1u << 16);
      }
   };

#pragma pack(pop)

}  // namespace multiboot
