/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <hedron/utcb.hpp>

#include <stdio.h>
#include <utility>

namespace hedron
{

void utcb::dump() const
{
    pprintf("MTD: {#x} INS_LEN: {#x}\n", static_cast<uint64_t>(mtd), ins_len);
    pprintf("IP: {#016x} FLAGS: {#016x}\n", ip, rflags);
    pprintf("INT:  {#08x} STA: {#08x}\n", static_cast<uint32_t>(int_state), act_state);
    pprintf("INTR: {#08x} ERR: {#08x}\n", static_cast<uint32_t>(inj_info), inj_error);
    pprintf("VECT: {#08x} ERR: {#08x}\n", static_cast<uint32_t>(vect_info), vect_error);
    pprintf("AX:  {#016x} BX:  {#016x} CX:  {#016x} DX:  {#016x}\n", ax, bx, cx, dx);
    pprintf("SP:  {#016x} BP:  {#016x} SI:  {#016x} DI:  {#016x}\n", sp, bp, si, di);
    pprintf("R8:  {#016x} R9:  {#016x} R10: {#016x} R11: {#016x}\n", r8, r9, r10, r11);
    pprintf("R12: {#016x} R13: {#016x} R14: {#016x} R15: {#016x}\n", r12, r13, r14, r15);
    pprintf("QUAL {#016x} {#016x}\n", qual[0], qual[1]);
    pprintf("CTRL {#08x} {#08x}\n", ctrl[0], ctrl[1]);
    pprintf("CR0: {#016x} CR2:  {#016x} CR3:  {#016x} CR4: {#016x}\n", cr0, cr2, cr3, cr4);
    pprintf("CR8: {#016x} EFER: {#016x} XCR0: {#016x}\n", cr8, efer, xcr0);
    unsigned i = 0;
    for (const auto& entry : pdpte) {
        pprintf("PDPTE[%u]: {#x}\n", i++, entry);
    }
    pprintf(" STAR:          {#x}\n", star);

    pprintf("LSTAR:          {#x}\n", lstar);
    pprintf("FMASK:          {#x}\n", fmask);
    pprintf("KERNEL_GS_BASE: {#x}\n", kernel_gs_base);

    using segment_name = std::pair<const char*, hedron::segment_descriptor>;
    const segment_name segments[] {{"CS", cs}, {"DS", ds},     {"ES", es}, {"FS", fs},     {"GS", gs},
                                   {"SS", ss}, {"LDTR", ldtr}, {"TR", tr}, {"GDTR", gdtr}, {"IDTR", idtr}};

    for (const auto& s : segments) {
        pprintf("{4s}:   sel {#04x} ar {#04x} limit {#04x} base {#016x}\n", s.first, s.second.sel, s.second.ar,
                s.second.limit, s.second.base);
    }
}

} // namespace hedron
