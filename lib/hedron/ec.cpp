/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <hedron/cap_range.hpp>
#include <hedron/ec.hpp>
#include <hedron/qpd.hpp>
#include <hedron/syscalls.hpp>
#include <hedron/utcb.hpp>
#include <libsn.hpp>
#include <stack_base.hpp>
#include <stdio.h>
#include <trace.hpp>

execution_context::execution_context(const ec_desc& descriptor, type ec_type) : desc(descriptor)
{
    if (not desc.selector) {
        auto range = allocate_cap_range();
        PANIC_UNLESS(range.size() == 1, "invalid number of caps");
        desc.selector = range.a;
    }

    if (not desc.utcb) {
        desc.utcb = allocate_utcb();
    }

    if (not desc.evt_base) {
        auto range = allocate_cap_range(EXC_RANGE_ORDER);
        desc.evt_base = range.a;
    }

    trace(TRACE_THREAD, "CREATE {s} EC SEL:{#x} CPU:{} UTCB:{} SP:{#x} EVT:{#x}",
          ec_type == type::LOCAL ? "local" : "global", *(desc.selector), desc.cpu, *(desc.utcb), desc.stack_desc.sp,
          *(desc.evt_base));

    auto res = create_ec(desc.pd, *(desc.selector), desc.cpu, *(desc.utcb), desc.stack_desc.sp, *(desc.evt_base),
                         ec_type != type::LOCAL, desc.user_page_in_calling_pd);

    PANIC_UNLESS(res, "unable to create EC");
}

void local_ec::create_portal(hedron::mtd_bits::field mtd, uintptr_t entry_ip, hedron::cap_sel pt_sel, uint64_t id) const
{
    auto res = create_pt(pt_sel, *(desc.selector), entry_ip, static_cast<uint64_t>(mtd));
    PANIC_UNLESS(res, "unable to create portal");
    hedron::pt_ctrl(pt_sel, id);
}

static void create_sc_generic(const execution_context::ec_desc& desc, hedron::qpd qpd,
                              std::optional<hedron::cap_sel> sel)
{
    ASSERT(desc.selector, "No EC selector set");

    if (not sel) {
        auto range = allocate_cap_range();
        sel = range.a;
    }

    auto res = create_sc(*sel, *(desc.selector), qpd);
    PANIC_UNLESS(res, "unable to create SC");
}

void global_ec::create_sc(hedron::qpd qpd, std::optional<hedron::cap_sel> sel) const
{
    create_sc_generic(desc, qpd, sel);
}

hedron::cap_sel execution_context::get_evt() const
{
    PANIC_UNLESS(desc.evt_base, "no EVT base set");
    return *(desc.evt_base);
}

uintptr_t execution_context::get_sp() const
{
    PANIC_UNLESS(desc.stack_desc.sp != 0, "no stack configured");
    return desc.stack_desc.sp;
}
