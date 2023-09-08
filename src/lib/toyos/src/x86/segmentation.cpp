/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/x86/segmentation.hpp>

namespace x86
{
   gdt_entry* get_gdt_entry(descriptor_ptr gdtr, segment_selector s)
   {
      assert(s.index() < gdtr.limit);
      return reinterpret_cast<gdt_entry*>(num_to_ptr<uint64_t*>(gdtr.base) + s.index());
   }

}  // namespace x86
