// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <toyos/x86/segmentation.hpp>

namespace x86
{
    gdt_entry* get_gdt_entry(descriptor_ptr gdtr, segment_selector s)
    {
        assert(s.index() < gdtr.limit);
        return reinterpret_cast<gdt_entry*>(num_to_ptr<uint64_t*>(gdtr.base) + s.index());
    }

}  // namespace x86
