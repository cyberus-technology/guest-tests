// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

namespace x86
{

    /// Local APIC register offsets
    struct lapic
    {
        enum reg
        {
            APIC_ID = 0x020,
            APIC_VERSION = 0x030,

            TPR = 0x080,
            APR = 0x090,
            PPR = 0x0a0,
            EOI = 0x0b0,
            RRD = 0x0c0,
            LOGICAL_DEST = 0x0d0,
            DEST_FORMAT = 0x0e0,
            SVR = 0x0f0,

            ISR_31_0 = 0x100,
            ISR_63_32 = 0x110,
            ISR_95_64 = 0x120,
            ISR_127_96 = 0x130,
            ISR_159_128 = 0x140,
            ISR_191_160 = 0x150,
            ISR_223_192 = 0x160,
            ISR_255_224 = 0x170,

            TMR_31_0 = 0x180,
            TMR_63_32 = 0x190,
            TMR_95_64 = 0x1a0,
            TMR_127_96 = 0x1b0,
            TMR_159_128 = 0x1c0,
            TMR_191_160 = 0x1d0,
            TMR_223_192 = 0x1e0,
            TMR_255_224 = 0x1f0,

            IRR_31_0 = 0x200,
            IRR_63_32 = 0x210,
            IRR_95_64 = 0x220,
            IRR_127_96 = 0x230,
            IRR_159_128 = 0x240,
            IRR_191_160 = 0x250,
            IRR_223_192 = 0x260,
            IRR_255_224 = 0x270,

            ERR_STS = 0x280,
            LVT_CMCI = 0x2f0,
            ICR_LOW = 0x300,
            ICR_HIGH = 0x310,
            LVT_TIMER = 0x320,
            LVT_THERMAL = 0x330,
            LVT_PERF_MON = 0x340,
            LVT_LINT0 = 0x350,
            LVT_LINT1 = 0x360,
            LVT_ERR = 0x370,
            INIT_CNT = 0x380,
            CURR_CNT = 0x390,

            DIV_COUNT = 0x3e0,

            X2_SELF_IPI = 0x3f0,
        };
    };

}  // namespace x86
