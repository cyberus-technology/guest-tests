/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "toyos/util/math.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

namespace multiboot2
{

   static constexpr uint32_t MB2_MAGIC{ 0x36D76289 };  ///< Passed to entry point by compliant loader

   /*
 * Multiboot2 information structure
 */
   struct mbi2_fixed
   {
      uint32_t total_size{ 0 };
      uint32_t reserved{ 0 };
   };
   static_assert(sizeof(mbi2_fixed) == 8, "Wrong structure size!");

   struct mbi2_tag
   {
      static constexpr size_t ALIGNMENT{ 8 };

      explicit mbi2_tag(uint32_t t)
         : type(t) {}
      mbi2_tag(uint32_t t, uint32_t s)
         : type(t), size(s) {}

      uint32_t type;
      uint32_t size{ 0 };
   };
   static_assert(sizeof(mbi2_tag) == 8, "Wrong structure size!");

   struct mbi2_cmdline : mbi2_tag
   {
      static constexpr uint32_t TYPE{ 1 };
      mbi2_cmdline()
         : mbi2_tag(TYPE) {}

      // u8[n] string
   };
   static_assert(sizeof(mbi2_cmdline) == 8, "Wrong structure size!");

   struct mbi2_boot_module : mbi2_tag
   {
      static constexpr uint32_t TYPE{ 3 };
      mbi2_boot_module(uint32_t s, uint32_t e)
         : mbi2_tag(TYPE), start(s), end(e) {}
      mbi2_boot_module()
         : mbi2_boot_module(0, 0) {}

      uint32_t start{ 0 };
      uint32_t end{ 0 };

      // u8[n] string
   };
   static_assert(sizeof(mbi2_boot_module) == 16, "Wrong structure size!");

   struct mmap_entry
   {
      enum
      {
         MMAP_AVAILABLE = 1,
      };

      uint64_t base{ 0 };
      uint64_t length{ 0 };
      uint32_t type{ 0 };
      uint32_t reserved{ 0 };
   };
   static_assert(sizeof(mmap_entry) == 24, "Wrong structure size!");

   struct mbi2_mmap : mbi2_tag
   {
      static constexpr uint32_t TYPE{ 6 };
      mbi2_mmap()
         : mbi2_tag(TYPE) {}

      uint32_t entry_size{ sizeof(mmap_entry) };
      uint32_t entry_version{ 0 };

      // varies entries
   };
   static_assert(sizeof(mbi2_mmap) == 16, "Wrong structure size!");

   struct mbi2_rsdp2 : mbi2_tag
   {
      static constexpr uint32_t TYPE{ 15 };
      mbi2_rsdp2()
         : mbi2_tag(TYPE) {}

      // copy of RSDPv2
   };
   static_assert(sizeof(mbi2_rsdp2) == 8, "Wrong structure size!");

   struct mbi2_efi_system_table : mbi2_tag
   {
      static constexpr uint32_t TYPE{ 12 };
      explicit mbi2_efi_system_table(uint64_t system_table_)
         : mbi2_tag(TYPE), system_table(system_table_) {}

      mbi2_efi_system_table()
         : mbi2_efi_system_table(0) {}

      uint64_t system_table{ 0 };
   };
   static_assert(sizeof(mbi2_efi_system_table) == 16, "Wrong structure size!");

   struct mbi2_efi_image_handle : mbi2_tag
   {
      static constexpr uint32_t TYPE{ 20 };
      explicit mbi2_efi_image_handle(uint64_t image_handle_)
         : mbi2_tag(TYPE), image_handle(image_handle_) {}

      mbi2_efi_image_handle()
         : mbi2_efi_image_handle(0) {}

      uint64_t image_handle{ 0 };
   };
   static_assert(sizeof(mbi2_efi_image_handle) == 16, "Wrong structure size!");

   struct mbi2_efi_boot_services : mbi2_tag
   {
      static constexpr uint32_t TYPE{ 18 };
      mbi2_efi_boot_services()
         : mbi2_tag(TYPE) {}
   };
   static_assert(sizeof(mbi2_efi_boot_services) == 8, "Wrong structure size!");

   struct mbi2_image_load_base : mbi2_tag
   {
      static constexpr uint32_t TYPE{ 21 };
      mbi2_image_load_base()
         : mbi2_tag(TYPE) {}

      explicit mbi2_image_load_base(uint32_t load_base_addr_)
         : mbi2_tag(TYPE), load_base_addr(load_base_addr_) {}

      uint32_t load_base_addr{ 0 };
   };
   static_assert(sizeof(mbi2_image_load_base) == 12, "Wrong structure size!");

   // For some reason, this is not according to the spec online... Pfui GRUB!
   struct mbi2_elf_symbols : mbi2_tag
   {
      static constexpr uint32_t TYPE{ 9 };

      mbi2_elf_symbols()
         : mbi2_tag(TYPE) {}

      uint32_t num;
      uint32_t entsize;
      uint32_t shndx;
   };
   static_assert(sizeof(mbi2_elf_symbols) == 20, "Wrong structure size!");

   struct mbi2_nulltag : mbi2_tag
   {
      mbi2_nulltag()
         : mbi2_tag(0, 8) {}
   };
   static_assert(sizeof(mbi2_nulltag) == 8, "Wrong structure size!");

   /*
 * Multiboot2 header
 */

   struct mb2_header_fixed
   {
      uint32_t magic;
      uint32_t architecture;
      uint32_t total_size;
      uint32_t checksum;
   };
   static_assert(sizeof(mb2_header_fixed) == 16, "Wrong structure size!");

   struct mb2_header_tag
   {
      static constexpr size_t ALIGNMENT{ 8 };

      explicit mb2_header_tag(uint32_t t)
         : type(t) {}
      mb2_header_tag(uint32_t t, uint32_t s)
         : type(t), size(s) {}

      uint16_t type;
      uint16_t flags{ 0 };
      uint32_t size{ 0 };
   };
   static_assert(sizeof(mb2_header_tag) == 8, "Wrong structure size!");

   struct mb2_header_keep_bs : mb2_header_tag
   {
      static constexpr uint16_t TYPE{ 7 };

      mb2_header_keep_bs()
         : mb2_header_tag(TYPE) {}
   };
   static_assert(sizeof(mb2_header_keep_bs) == 8, "Wrong structure size!");

   struct mb2_header_efi_64bit_entry : mb2_header_tag
   {
      static constexpr uint16_t TYPE{ 9 };

      mb2_header_efi_64bit_entry()
         : mb2_header_tag(TYPE) {}

      uint32_t entry_addr;
   };
   static_assert(sizeof(mb2_header_efi_64bit_entry) == 12, "Wrong structure size!");

   struct mb2_header_relocatable : mb2_header_tag
   {
      static constexpr uint16_t TYPE{ 10 };

      mb2_header_relocatable()
         : mb2_header_tag(TYPE) {}

      enum
      {
         PREFERENCE_NONE = 0,
         PREFERENCE_LOWEST = 1,
         PREFERENCE_HIGHEST = 2,
      };

      uint32_t min_addr;
      uint32_t max_addr;
      uint32_t align;
      uint32_t preference;
   };
   static_assert(sizeof(mb2_header_relocatable) == 24, "Wrong structure size!");

   /**
 * Simple MB2 header / MBI2 structure reader
 */
   template<typename FIXED_PART, typename TAG_TYPE>
   struct mb2_reader_generic
   {
      explicit mb2_reader_generic(const uint8_t* raw_pointer)
         : raw(raw_pointer)
      {
         FIXED_PART fix;
         memcpy(&fix, raw, sizeof(fix));
         size = fix.total_size;
      }

      struct mb2_iterator_tag
      {
         TAG_TYPE generic{ 0 };
         const uint8_t* addr;

         explicit mb2_iterator_tag(const uint8_t* addr_)
            : addr(addr_) {}

         template<typename T>
         T get_full_tag() const
         {
            assert(sizeof(T) <= generic.size);

            T ret;
            memcpy(&ret, addr, sizeof(T));
            return ret;
         }
      };

      class iterator
      {
      public:
         using iterator_category = std::input_iterator_tag;
         using value_type = mb2_iterator_tag;
         using difference_type = void;
         using pointer = void;
         using reference = void;

         explicit iterator(const uint8_t* addr)
            : current_tag(addr) {}

         value_type operator*() const
         {
            value_type ret{ current_tag };
            memcpy(&ret.generic, current_tag, sizeof(ret.generic));
            return ret;
         }

         bool operator!=(const iterator& o) const
         {
            return o.current_tag != current_tag;
         }
         bool operator==(const iterator& o) const
         {
            return o.current_tag == current_tag;
         }

         iterator& operator++()
         {
            current_tag += math::align_up((this->operator*()).generic.size, math::order_max(mbi2_tag::ALIGNMENT));
            return *this;
         }

      private:
         const uint8_t* current_tag{ nullptr };
      };

      iterator begin() const
      {
         return iterator(raw + sizeof(FIXED_PART));
      }
      iterator end() const
      {
         return iterator(raw + size);
      }

      std::optional<typename iterator::value_type> find_tag(const uint32_t tag_type) const
      {
         auto matches_tag_type = [tag_type](const typename iterator::value_type& tag) {
            return tag.generic.type == tag_type;
         };

         if (const auto it{ std::find_if(begin(), end(), matches_tag_type) }; it != end()) {
            return { *it };
         }

         return std::nullopt;
      }

      template<typename TAG>
      std::optional<TAG> find_and_extract_tag() const
      {
         if (const auto tag{ find_tag(TAG::TYPE) }; tag) {
            return tag->template get_full_tag<TAG>();
         }
         return std::nullopt;
      }

      const uint8_t* raw;
      size_t size{ 0 };
   };

   using mbi2_reader = mb2_reader_generic<mbi2_fixed, mbi2_tag>;
   using mb2_header_reader = mb2_reader_generic<mb2_header_fixed, mb2_header_tag>;

   inline std::optional<mb2_header_reader> locate_header(const std::vector<unsigned char>& binary)
   {
      // We need the binary to hold at least the size of the fixed part.
      if (binary.size() < sizeof(mb2_header_fixed)) {
         return std::nullopt;
      }

      // The specification says the header must be located within the first 32 KiB
      static constexpr size_t MB2_HDR_MAX{ 32 * 1024 };

      /// Magic value in the binary indicating Multiboot2
      static constexpr std::array<uint8_t, 4> MB2_HDR_MAGIC{ 0xD6, 0x50, 0x52, 0xE8 };

      // Subtract 8 bytes for the rest of the fixed part. Otherwise, there isn't
      // even enough room for the header length and we would end up interpreting
      // undefined memory as length.
      // Because of the early check, we can be sure that the binary size is at
      // least 16 bytes and subtracting 8 won't cause trouble.
      const auto search_end{ binary.begin() + std::min(MB2_HDR_MAX, binary.size()) - 8 };

      auto search_begin{ binary.begin() };
      while (search_begin < search_end) {
         const auto found{ std::search(search_begin, search_end, MB2_HDR_MAGIC.begin(), MB2_HDR_MAGIC.end()) };

         if (found == search_end) {
            return std::nullopt;
         }

         // The specification also mandates a 64-bit alignment. We keep looking
         // until that requirement is fulfilled.
         if (const auto offset{ std::distance(binary.begin(), found) }; offset % 8 == 0) {
            return mb2_header_reader{ binary.data() + offset };
         }
         else {
            search_begin = found + 1;
         }
      }

      return std::nullopt;
   }

}  // namespace multiboot2
