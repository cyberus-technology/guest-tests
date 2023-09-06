/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <array>
#include <memory>
#include <string>

#include <arch.hpp>
#include <cbl/traits.hpp>
#include <compiler.hpp>
#include <decoder.hpp>
#include <decoder/gdt.hpp>
#include <decoder/instructions.hpp>
#include <decoder/operand.hpp>
#include <emulator.hpp>
#include <emulator/syscall.hpp>
#include <logger/trace.hpp>
#include <x86defs.hpp>

using namespace x86;

using func_t = void (*)();
#define GENERATE_EXTERNS
#include <emulator/macros.hpp>
#undef GENERATE_EXTERNS

EXTERN_C func_t bt_r16_r16 asm("bt_r16_r16");
EXTERN_C func_t bt_r32_r32 asm("bt_r32_r32");
EXTERN_C func_t bt_r64_r64 asm("bt_r64_r64");

EXTERN_C func_t bt_r16_m16 asm("bt_r16_m16");
EXTERN_C func_t bt_r32_m32 asm("bt_r32_m32");
EXTERN_C func_t bt_r64_m64 asm("bt_r64_m64");

EXTERN_C func_t movbe_r16_m16 asm("movbe_r16_m16");
EXTERN_C func_t movbe_m16_r16 asm("movbe_m16_r16");
EXTERN_C func_t movbe_r32_m32 asm("movbe_r32_m32");

EXTERN_C func_t movbe_m32_r32 asm("movbe_m32_r32");
EXTERN_C func_t movbe_r64_m64 asm("movbe_r64_m64");
EXTERN_C func_t movbe_m64_r64 asm("movbe_m64_r64");

using namespace decoder;
namespace emulator
{

// XXX: Emulation functions are experimental and untested

template <typename OP>
static bool emulate_mov_generic(OP src, OP dst)
{
    if (not src->read()) {
        return false;
    }

    dst->set(src->template get<uint64_t>());

    return dst->write();
}

template <typename DECODER>
static bool emulate_mov(emulator_vcpu&, DECODER& decoder_)
{
    return emulate_mov_generic(decoder_.get_operand(1), decoder_.get_operand(0));
}

template <typename DECODER>
static bool emulate_movs(emulator_vcpu& vcpu, DECODER& decoder_, size_t width)
{
    gpr src_base {gpr::RSI};
    gpr dst_base {gpr::RDI};
    switch (decoder_.get_address_size()) {
    case 16:
        src_base = gpr::SI;
        dst_base = gpr::DI;
        break;
    case 32:
        src_base = gpr::ESI;
        dst_base = gpr::EDI;
        break;
    case 64:
        src_base = gpr::RSI;
        dst_base = gpr::RDI;
        break;
    default: PANIC("Unsupported address size for MOVS!");
    }

    auto segment_override = decoder_.get_segment_override();

    auto src_segment = segment_override.value_or(segment_register::DS);
    auto dst_segment = segment_register::ES;

    auto src = decoder_.make_memory_operand(width, src_segment, src_base, {}, 0, {});
    auto dst = decoder_.make_memory_operand(width, dst_segment, dst_base, {}, 0, {});

    bool success = emulate_mov_generic(std::move(src), std::move(dst));
    if (success) {
        // Update address registers according to access width and direction flag
        auto si = decoder_.make_register_operand(decoder_.get_address_size() / 8, src_base);
        auto di = decoder_.make_register_operand(decoder_.get_address_size() / 8, dst_base);
        si->read();
        di->read();
        uint64_t flags {vcpu.read(gpr::FLAGS)};
        int64_t adjustment = (flags & FLAGS_DF) ? -width : width;
        si->set(si->template get<uint64_t>() + adjustment);
        di->set(di->template get<uint64_t>() + adjustment);
        si->write();
        di->write();
    }

    return success;
}

template <typename DECODER>
static bool emulate_stos(emulator_vcpu& vcpu, DECODER& decoder_, size_t width)
{
    gpr src_base {gpr::RAX};
    switch (width) {
    case sizeof(uint8_t): src_base = gpr::AL; break;
    case sizeof(uint16_t): src_base = gpr::AX; break;
    case sizeof(uint32_t): src_base = gpr::EAX; break;
    case sizeof(uint64_t): src_base = gpr::RAX; break;
    default: PANIC("Unsupported operand size for STOS!");
    }

    gpr dst_base {gpr::RDI};
    switch (decoder_.get_address_size()) {
    case 8: dst_base = gpr::DL; break;
    case 16: dst_base = gpr::DI; break;
    case 32: dst_base = gpr::EDI; break;
    case 64: dst_base = gpr::RDI; break;
    default: PANIC("Unsupported address size for STOS!");
    }

    auto dst_segment = segment_register::ES;

    auto src = decoder_.make_register_operand(width, src_base);
    auto dst = decoder_.make_memory_operand(width, dst_segment, dst_base, {}, 0, {});

    bool success = emulate_mov_generic(std::move(src), std::move(dst));
    if (success) {
        // Update address register according to access width and direction flag
        auto di = decoder_.make_register_operand(decoder_.get_address_size() / 8, dst_base);
        di->read();
        uint64_t flags {vcpu.read(gpr::FLAGS)};
        int64_t adjustment = (flags & FLAGS_DF) ? -width : width;
        di->set(di->template get<uint64_t>() + adjustment);
        di->write();
    }

    return success;
}

template <typename DECODER, typename FUNC, typename OP>
static bool helper_1op(emulator_vcpu& vcpu, [[maybe_unused]] DECODER& dec, FUNC fn, OP& op, bool readonly = false)
{
    constexpr uint64_t flags_mask {FLAGS_CF | FLAGS_PF | FLAGS_AF | FLAGS_ZF | FLAGS_SF | FLAGS_OF};
    uint64_t flags_guest {vcpu.read(gpr::FLAGS)};
    uint64_t flags_guest_post;

    std::optional<uint64_t> op_dummy;
    uint64_t op_val = 0;
    bool unresolved_access {false};
    if (op->get_type() == common_operand::type::REG or op->get_type() == common_operand::type::IMM) {
        op->read();
        op_val = op->template get<uint64_t>();
    } else {
        // We need to provide the guest-physical address as register content
        auto memop = static_cast<memory_operand*>(op.get());
        auto gpa = memop->physical_address(x86::memory_access_type_t::READ);
        if (not gpa) {
            return false;
        }

        if (not memop->is_guest_accessible()) {
            memop->read();
            op_dummy = memop->get<uint64_t>();
            op_val = uint64_t(&(*op_dummy));
            unresolved_access = true;
        } else {
            op_val = static_cast<uintptr_t>(*gpa);
        }
    }

    asm volatile("pushf;"
                 "push %[flags_guest_pre];"
                 "popf;"
                 "call *%[func];"
                 "pushf;"
                 "pop %[flags_guest_post];"
                 "popf;"
                 : "+b"(op_val), [flags_guest_post] "=r"(flags_guest_post)
                 : [flags_guest_pre] "r"(flags_guest & flags_mask), [func] "r"(fn)
                 : "memory");

    flags_guest &= ~flags_mask;
    flags_guest |= flags_guest_post & flags_mask;
    vcpu.write(gpr::FLAGS, flags_guest);

    if (op->get_type() == common_operand::type::REG) {
        op->set(op_val);
        op->write();
    }

    if (unresolved_access and not readonly) {
        ASSERT(op_dummy, "Replacement destination operand not initialized!");
        op->set(*op_dummy);
        op->write();
    }

    return true;
}

static constexpr bool equal(const char* lhs, const char* rhs)
{
    for (; *lhs or *rhs; lhs++, rhs++) {
        if (*lhs != *rhs) {
            return false;
        }
    }
    return true;
}
static constexpr bool has_read_only_dst(const char* insn)
{
    return equal(insn, "CMP");
}

template <typename DECODER, typename FUNC, typename OP>
static bool helper_2op(emulator_vcpu& vcpu, [[maybe_unused]] DECODER& dec, FUNC fn, OP& src, OP& dst,
                       bool dst_readonly = false)
{
    constexpr uint64_t flags_mask {FLAGS_CF | FLAGS_PF | FLAGS_AF | FLAGS_ZF | FLAGS_SF | FLAGS_OF};
    uint64_t flags_guest {vcpu.read(gpr::FLAGS)};
    uint64_t flags_guest_post;

    uint64_t src_dummy;
    uint64_t src_val = 0;
    if (src->get_type() == common_operand::type::REG or src->get_type() == common_operand::type::IMM) {
        src->read();
        src_val = src->template get<uint64_t>();
    } else {
        // We need to provide the guest-physical address as register content
        auto memop = static_cast<memory_operand*>(src.get());
        auto gpa = memop->physical_address(x86::memory_access_type_t::READ);
        if (not gpa) {
            return false;
        }

        if (not memop->is_guest_accessible()) {
            memop->read();
            src_dummy = memop->get<uint64_t>();
            src_val = uint64_t(&src_dummy);
        } else {
            src_val = static_cast<uintptr_t>(*gpa);
        }
    }

    std::optional<uint64_t> dst_dummy;
    uint64_t dst_val = 0;
    bool unresolved_access {false};
    if (dst->get_type() == common_operand::type::REG) {
        dst->read();
        dst_val = dst->template get<uint64_t>();
    } else {
        // We need to provide the guest-physical address as register content
        auto memop = static_cast<memory_operand*>(dst.get());
        auto gpa = memop->physical_address(x86::memory_access_type_t::WRITE);
        if (not gpa) {
            return false;
        }

        if (not memop->is_guest_accessible()) {
            memop->read();
            dst_dummy = memop->get<uint64_t>();
            dst_val = uint64_t(&(*dst_dummy));
            unresolved_access = true;
        } else {
            dst_val = static_cast<uintptr_t>(*gpa);
        }
    }

    asm volatile("pushf;"
                 "push %[flags_guest_pre];"
                 "popf;"
                 "call *%[func];"
                 "pushf;"
                 "pop %[flags_guest_post];"
                 "popf;"
                 : "+c"(src_val), "+b"(dst_val), [flags_guest_post] "=r"(flags_guest_post)
                 : [flags_guest_pre] "r"(flags_guest & flags_mask), [func] "r"(fn)
                 : "memory");

    flags_guest &= ~flags_mask;
    flags_guest |= flags_guest_post & flags_mask;
    vcpu.write(gpr::FLAGS, flags_guest);

    if (src->get_type() == common_operand::type::REG) {
        src->set(src_val);
        src->write();
    }

    if (dst->get_type() == common_operand::type::REG) {
        dst->set(dst_val);
        dst->write();
    }

    if (unresolved_access and not dst_readonly) {
        ASSERT(dst_dummy, "Replacement destination operand not initialized!");
        dst->set(*dst_dummy);
        dst->write();
    }

    return true;
}

template <typename DECODER>
static bool emulate_bt(emulator_vcpu& vcpu, DECODER& decoder_)
{
    auto src = decoder_.get_operand(1);
    auto dst = decoder_.get_operand(0);

    PANIC_UNLESS(src->read(), "BT: bit offset has to be readable!");

    PANIC_ON(src->template get<uint64_t>() >= dst->get_width() * 8, "Address mangling not yet supported!");

    using optype = common_operand::type;

    auto determine_asm_block = [](optype dst_type, size_t width) -> func_t* {
        if (dst_type == optype::MEM) {
            switch (width) {
            case 16: return &bt_r16_m16;
            case 32: return &bt_r32_m32;
            case 64: return &bt_r64_m64;
            default: PANIC("Unsupported instruction width for BT!");
            };
        } else {
            switch (width) {
            case 16: return &bt_r16_r16;
            case 32: return &bt_r32_r32;
            case 64: return &bt_r64_r64;
            default: PANIC("Unsupported instruction width for BT!");
            };
        }
    };
    auto fn = determine_asm_block(dst->get_type(), dst->get_width() * 8);

    return helper_2op(vcpu, decoder_, fn, src, dst);
}

template <typename DECODER>
static bool emulate_movbe(emulator_vcpu& vcpu, DECODER& decoder_)
{
    auto src = decoder_.get_operand(1);
    auto dst = decoder_.get_operand(0);

    PANIC_UNLESS(src->read(), "MOVBE: source can not be read!");

    using optype = common_operand::type;

    auto determine_asm_block = [](optype dst_type, size_t width) -> func_t* {
        if (dst_type == optype::MEM) {
            switch (width) {
            case 16: return &movbe_r16_m16;
            case 32: return &movbe_r32_m32;
            case 64: return &movbe_r64_m64;
            default: PANIC("Unsupported instruction width for MOVBE!");
            };
        } else {
            switch (width) {
            case 16: return &movbe_m16_r16;
            case 32: return &movbe_m32_r32;
            case 64: return &movbe_m64_r64;
            default: PANIC("Unsupported instruction width for MOVBE!");
            };
        }
    };
    auto fn = determine_asm_block(dst->get_type(), dst->get_width() * 8);

    return helper_2op(vcpu, decoder_, fn, src, dst);
}

template <typename DECODER>
static bool emulate_syscall(emulator_vcpu& vcpu, [[maybe_unused]] DECODER& decoder_)
{
    // TODO: Check for #UD

    // Save RIP+insn_len in RCX
    vcpu.write(gpr::RCX, vcpu.read(gpr::IP));

    // Fetch RIP from LSTAR
    vcpu.write(gpr::IP, vcpu.handle_rdmsr(msr::LSTAR).value());

    // Save flags in R11
    auto flags = vcpu.read(gpr::FLAGS);
    vcpu.write(gpr::R11, flags);

    // Mask flags with FMASK
    flags &= ~(vcpu.handle_rdmsr(msr::FMASK).value());
    vcpu.write(gpr::FLAGS, flags);

    msr_star star {vcpu.handle_rdmsr(msr::STAR).value()};

    vcpu.write(segment_register::CS, star.cs(msr_star::mode::KERNEL));
    vcpu.write(segment_register::SS, star.ss(msr_star::mode::KERNEL));

    return true;
}

template <typename DECODER>
static bool emulate_sysret(emulator_vcpu& vcpu, DECODER& decoder_)
{
    // TODO: Check for #UD
    // Determine operand size
    bool is_64bit {(decoder_.get_operand_size() == 64)};

    // Set RIP to (R/E)CX
    // TODO: Check for non-canonical IP
    vcpu.write(gpr::IP, vcpu.read(gpr::RCX));

    // Set flags to (R11 & 0x3c7fd7) | 2
    uint64_t flags {vcpu.read(gpr::R11)};
    flags = (flags & 0x3c7fd7) | FLAGS_MBS;
    vcpu.write(gpr::FLAGS, flags);

    msr_star star {vcpu.handle_rdmsr(msr::STAR).value()};

    vcpu.write(segment_register::CS, star.cs(msr_star::mode::USER, is_64bit));
    vcpu.write(segment_register::SS, star.ss(msr_star::mode::USER));

    return true;
}

template <typename DECODER>
static std::pair<gpr, size_t> determine_stack_parameters(DECODER& decoder_)
{
    gpr sp_reg;
    size_t increment {decoder_.get_operand_size() / 8};
    switch (decoder_.get_stack_address_size()) {
    case 64: sp_reg = gpr::RSP; break;
    case 32: sp_reg = gpr::ESP; break;
    case 16: sp_reg = gpr::SP; break;
    default: PANIC("Unknown stack address size {}", decoder_.get_stack_address_size());
    };

    return {sp_reg, increment};
}

template <typename DECODER>
static ::decoder::decoder::operand_ptr_t get_stack_operand(DECODER& decoder_, gpr sp_reg)
{
    return decoder_.make_memory_operand(decoder_.get_operand_size() / 8, segment_register::SS, sp_reg, {}, 0, {});
}

template <typename DECODER>
static void adjust_stack_pointer(emulator_vcpu& vcpu, [[maybe_unused]] DECODER& decoder_, gpr sp_reg, int amount)
{
    vcpu.write(sp_reg, vcpu.read(sp_reg) + amount);
}

template <typename DECODER>
static uint64_t pop_helper(emulator_vcpu& vcpu, DECODER& decoder_)
{
    auto params = determine_stack_parameters(decoder_);

    auto op = get_stack_operand(decoder_, params.first);
    PANIC_UNLESS(op->read(), "Reading guest stack failed");

    adjust_stack_pointer(vcpu, decoder_, params.first, params.second);

    return op->template get<uint64_t>();
}

template <typename DECODER>
static void push_helper(emulator_vcpu& vcpu, DECODER& decoder_, uint64_t val)
{
    auto params = determine_stack_parameters(decoder_);

    adjust_stack_pointer(vcpu, decoder_, params.first, -params.second);

    auto op = get_stack_operand(decoder_, params.first);
    op->set(val);
    PANIC_UNLESS(op->write(), "Writing guest stack failed");
}

template <typename DECODER>
static bool emulate_push(emulator_vcpu& vcpu, DECODER& decoder_)
{
    auto src = decoder_.get_operand(0);
    src->read();
    push_helper(vcpu, decoder_, src->template get<uint64_t>());

    return true;
}

template <typename DECODER>
static bool emulate_pop(emulator_vcpu& vcpu, DECODER& decoder_)
{
    auto dst = decoder_.get_operand(0);
    dst->set(pop_helper(vcpu, decoder_));
    PANIC_UNLESS(dst->write(), "Could not write operand");

    return true;
}

template <typename DECODER>
static bool emulate_iret(emulator_vcpu& vcpu, DECODER& decoder_)
{
    // TODO: Check for exceptions
    PANIC_ON(vcpu.read(gpr::FLAGS) & FLAGS_NT, "Task switch not supported");
    PANIC_ON(vcpu.read(gpr::FLAGS) & FLAGS_VM, "VM8086 mode not supported");

    PANIC_UNLESS(decoder_.get_operand_size() == 64, "Only 64-bit operand size supported!");
    PANIC_UNLESS(decoder_.get_address_size() == 64, "Only 64-bit address size supported!");
    PANIC_UNLESS(vcpu.get_cpu_mode() == x86::cpu_mode::PM64, "Only 64-bit mode supported!");

    uint64_t stack_frame[5];
    for (auto& e : stack_frame) {
        e = pop_helper(vcpu, decoder_);
    }

    vcpu.write(gpr::IP, stack_frame[0]);

    unsigned cpl {vcpu.current_cpl()};
    unsigned iopl {vcpu.current_iopl()};
    unsigned rpl {unsigned(stack_frame[1] & SELECTOR_PL_MASK)};

    auto new_cs {parse_gdt(vcpu, stack_frame[1] & ~SELECTOR_PL_MASK)};
    vcpu.write(segment_register::CS, {uint16_t(stack_frame[1]), new_cs.ar(), new_cs.limit(), new_cs.base()});

    uint64_t flags_mask {FLAGS_CF | FLAGS_PF | FLAGS_AF | FLAGS_ZF | FLAGS_SF | FLAGS_TF | FLAGS_DF | FLAGS_OF
                         | FLAGS_NT};

    if (decoder_.get_operand_size() >= 32) {
        flags_mask |= FLAGS_RF | FLAGS_AC | FLAGS_ID;
    }

    if (cpl <= iopl) {
        flags_mask |= FLAGS_IF;
    }
    if (cpl == 0) {
        flags_mask |= FLAGS_IOPL;
    }

    auto flags = vcpu.read(gpr::FLAGS);
    flags &= ~flags_mask;
    flags |= (stack_frame[2] & flags_mask);
    PANIC_UNLESS(flags & FLAGS_MBS, "MBS must be set!");
    vcpu.write(gpr::FLAGS, flags);

    PANIC_ON(rpl < cpl, "Exception injection not yet supported!");

    // In 64-bit mode, IRET *always* restores SS:RSP from the stack.
    vcpu.write(gpr::RSP, stack_frame[3]);
    auto new_ss {parse_gdt(vcpu, stack_frame[4] & ~SELECTOR_PL_MASK)};
    vcpu.write(segment_register::SS, {uint16_t(stack_frame[4]), new_ss.ar(), new_ss.limit(), new_ss.base()});

    DEBUG_ONLY(
        // Segments that are no longer accessible with the new privilege level have to be invalidated
        for (const auto reg
             : {segment_register::DS, segment_register::ES, segment_register::FS, segment_register::GS}) {
            auto seg {vcpu.read(reg)};
            if (rpl > unsigned(((seg.ar & x86::gdt_entry::AR_DPL) >> tzcnt(x86::gdt_entry::AR_DPL)))) {
                WARN_ONCE("Invalidate segment!");
            }
        });

    return true;
}

// See SDM 2, LGDT/LIDT description. These instructions always fetch 6 or 10 bytes, depending on the operand size.
static constexpr size_t xdt_memop_size(size_t operand_size)
{
    return operand_size == 64 ? 10 : 6;
}

template <typename DECODER>
static bool emulate_sgdt_sidt(emulator_vcpu& vcpu, DECODER& decoder_, mmreg reg)
{
    auto dst {decoder_.get_operand(0)};
    auto dst_op {static_cast<memory_operand*>(dst.get())};
    auto addr {dst_op->physical_address(x86::memory_access_type_t::WRITE)};
    PANIC_UNLESS(addr, "Need a valid translation");

    auto src {vcpu.read(reg)};

    uint64_t val[2] = {0};
    val[0] = (src.base << cbl::bit_width<uint16_t>()) | src.limit;
    val[1] = src.base >> 48;

    return vcpu.write(*addr, xdt_memop_size(decoder_.get_address_size()), val);
}

template <typename DECODER>
static bool emulate_lgdt_lidt(emulator_vcpu& vcpu, DECODER& decoder_, mmreg reg)
{
    // TODO: Exception checks
    auto src {decoder_.get_operand(0)};
    auto src_op {static_cast<memory_operand*>(src.get())};
    auto addr {src_op->physical_address(x86::memory_access_type_t::READ)};
    PANIC_UNLESS(addr, "Need a valid translation");

    struct mem_op
    {
        mem_op(size_t operand_size_) : operand_size(operand_size_), raw {} {}

        uint16_t limit() const { return raw[0]; }

        uint64_t base() const
        {
            switch (operand_size) {
            case 16: return (raw[0] >> 16) & math::mask(24);
            case 32: return (raw[0] >> 16) & math::mask(32);
            case 64: return (raw[0] >> 16) | ((raw[1] & math::mask(16)) << 48);
            default: PANIC("Invalid operand size {#x}", operand_size);
            }
        }

        size_t bytes() const { return xdt_memop_size(operand_size); }

        size_t operand_size;
        std::array<uint64_t, 2> raw;
    };
    // Unfortunately, the size that determines the operand size of the LGDT/LIDT instruction is
    // actually the address size *sigh*
    mem_op new_reg {decoder_.get_address_size()};
    PANIC_UNLESS(vcpu.read(*addr, new_reg.bytes(), new_reg.raw.data()), "Failed to read GDTR base/limit");
    vcpu.write(reg, {uint16_t(0), uint16_t(0), new_reg.limit(), new_reg.base()});

    return true;
}

template <typename DECODER>
static bool emulate_str_sldt(emulator_vcpu& vcpu, DECODER& decoder_, mmreg reg)
{
    auto dst {decoder_.get_operand(0)};
    auto src {vcpu.read(reg)};
    dst->set(src.sel);
    return dst->write();
}

template <typename DECODER>
static bool emulate_ltr_lldt(emulator_vcpu& vcpu, DECODER& decoder_, mmreg reg)
{
    // TODO: Check for exceptions

    auto src {decoder_.get_operand(0)};
    PANIC_UNLESS(src->read(), "Unable to read LDT/TR selector");
    auto sel {src->template get<uint16_t>()};

    if (reg == mmreg::LDTR and (sel & ~SELECTOR_PL_MASK) == 0) {
        // NULL selector means LDTR is unusable
        vcpu.write(reg, {uint16_t(0), uint16_t(1u << 12), 0u, 0u});
        return true;
    }

    PANIC_ON((sel & ~SELECTOR_PL_MASK) == 0, "#GP injection not yet supported!");

    auto entry {parse_gdt(vcpu, sel, decoder_.get_address_size() == 64)};

    uint16_t ar {entry.ar()};
    if (reg == mmreg::TR) {
        // XXX: Check for not busy
        // XXX: Write busy bit in GDT (atomically...)
        WARN_ONCE("FIXME: LTR is experimental and does not write the Busy bit!");
        ar = 0x8b;
    }

    vcpu.write(reg, {sel, ar, entry.limit(), entry.base()});

    return true;
}

template <typename DECODER>
static bool emulate_sti(emulator_vcpu& vcpu, [[maybe_unused]] DECODER& decoder_)
{
    vcpu.write(gpr::FLAGS, vcpu.read(gpr::FLAGS) | FLAGS_IF);
    vcpu.set_sti_blocking();
    return true;
}

template <typename DECODER>
static bool emulate_cli(emulator_vcpu& vcpu, [[maybe_unused]] DECODER& decoder_)
{
    vcpu.write(gpr::FLAGS, vcpu.read(gpr::FLAGS) & ~FLAGS_IF);
    return true;
}

template <typename DECODER>
static bool emulate_out(emulator_vcpu& vcpu, DECODER& decoder_)
{
    auto dst {decoder_.get_operand(0)};
    auto src {decoder_.get_operand(1)};

    PANIC_UNLESS(dst->read(), "Could not read I/O port number");
    PANIC_UNLESS(src->read(), "Could not read I/O value");

    uint16_t port =
        dst->get_type() == common_operand::type::IMM ? dst->template get<uint8_t>() : dst->template get<uint16_t>();
    size_t bytes {src->get_width()};
    uint32_t val {src->template get<uint32_t>()};

    PANIC_UNLESS(vcpu.write(port, bytes, val), "Unhandled emulated I/O access");

    return true;
}

template <typename DECODER>
static bool emulate_jmp(emulator_vcpu& vcpu, DECODER& decoder_, bool force_near = false)
{
    uint16_t cs {0};
    uint64_t ip {vcpu.read(gpr::IP)};

    auto target {decoder_.get_operand(0)};

    if (not force_near and decoder_.is_far_branch()) {
        switch (target->get_type()) {
        case common_operand::type::MEM: {
            auto target_op {static_cast<memory_operand*>(target.get())};
            auto addr {target_op->physical_address(x86::memory_access_type_t::READ)};
            PANIC_UNLESS(addr, "Need a valid translation");

            size_t opsize {decoder_.get_operand_size() / 8};

            PANIC_UNLESS(vcpu.read(*addr, opsize, &ip), "Failed to read IP");
            *addr += opsize;
            PANIC_UNLESS(vcpu.read(*addr, sizeof(cs), &cs), "Failed to read CS");
            break;
        }
        case common_operand::type::PTR: {
            auto target_op {static_cast<pointer_operand*>(target.get())};
            cs = target_op->get_selector();
            ip = target_op->get_offset();
            break;
        }
        default: PANIC("Unknown operand type!");
        }

        auto current_cpl {vcpu.current_cpl()};
        auto new_cs {parse_gdt(vcpu, cs & ~SELECTOR_PL_MASK)};
        PANIC_UNLESS(new_cs.is_code(), "Attempted far branch to non-code descriptor");
        if (new_cs.dpl() != current_cpl) {
            PANIC_UNLESS(new_cs.is_conforming(), "Exception handling not implemented yet");
            cs |= current_cpl;
        }
        vcpu.write(segment_register::CS, {cs, new_cs.ar(), new_cs.limit(), new_cs.base()});
    } else {
        switch (target->get_type()) {
        case common_operand::type::IMM:
            // Set IP to next IP+displacement
            ip += target->template get<uint64_t>();
            break;
        case common_operand::type::MEM:
        case common_operand::type::REG:
            // Set IP to target given by operand
            PANIC_UNLESS(target->read(), "Unable to fetch target operand");
            ip = target->template get<uint64_t>();
            break;
        default: PANIC("Wrong operand type for near branch");
        }
    }

    vcpu.write(gpr::IP, ip);

    return true;
}

#define GENERATE_EMULATOR_FUNCTIONS
#include <emulator/macros.hpp>
#undef GENERATE_EMULATOR_FUNCTIONS

template <typename DECODER>
static bool emulate_call(emulator_vcpu& vcpu, DECODER& decoder_)
{
    auto target {decoder_.get_operand(0)};

    // Near call, relative displacement to next instruction
    auto ret_ip {vcpu.read(gpr::IP)};
    std::optional<uint16_t> ret_cs;

    if (decoder_.is_far_branch()) {
        ret_cs = vcpu.read(segment_register::CS).sel;
    }
    PANIC_UNLESS(emulate_jmp(vcpu, decoder_), "Emulating JMP part failed!");

    // Push current CS, if far branch
    if (ret_cs) {
        push_helper(vcpu, decoder_, *ret_cs);
    }

    // Push next IP
    push_helper(vcpu, decoder_, ret_ip);

    return true;
}

template <typename DECODER>
static bool emulate_ret(emulator_vcpu& vcpu, DECODER& decoder_)
{
    auto op {decoder_.get_operand(0)};
    if (op) {
        PANIC_UNLESS(op->get_type() == common_operand::type::IMM, "Wrong operand type for RET");

        auto params {determine_stack_parameters(decoder_)};
        vcpu.write(params.first, vcpu.read(params.first) + op->template get<uint64_t>());
    }

    auto ret_ip {pop_helper(vcpu, decoder_)};
    vcpu.write(gpr::IP, ret_ip);

    return true;
}

template <typename DECODER>
static bool emulate_lea(emulator_vcpu&, DECODER& decoder_)
{
    auto src {decoder_.get_operand(1)};
    auto dst {decoder_.get_operand(0)};

    PANIC_UNLESS(src->get_type() == common_operand::type::MEM, "Invalid operand type");

    auto src_op {static_cast<memory_operand*>(src.get())};
    dst->set(src_op->effective_address());
    dst->write();

    return true;
}

emulator::status_t emulator::emulate(uintptr_t ip, std::optional<uint8_t> replace_byte)
{
    decoder_.decode(ip, {}, replace_byte);

    *insn_len_ = decoder_.get_instruction_length();

    switch (decoder_.get_instruction()) {
    case instruction::MOV:
    case instruction::MOVZX: return emulate_mov(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::BT: return emulate_bt(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::MOVBE: return emulate_movbe(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::MOVSB: return emulate_movs(vcpu_, decoder_, sizeof(uint8_t)) ? status_t::OK : status_t::FAULT;
    case instruction::MOVSW: return emulate_movs(vcpu_, decoder_, sizeof(uint16_t)) ? status_t::OK : status_t::FAULT;
    case instruction::MOVSD: return emulate_movs(vcpu_, decoder_, sizeof(uint32_t)) ? status_t::OK : status_t::FAULT;
    case instruction::MOVSQ: return emulate_movs(vcpu_, decoder_, sizeof(uint64_t)) ? status_t::OK : status_t::FAULT;
    case instruction::STOSB: return emulate_stos(vcpu_, decoder_, sizeof(uint8_t)) ? status_t::OK : status_t::FAULT;
    case instruction::STOSW: return emulate_stos(vcpu_, decoder_, sizeof(uint16_t)) ? status_t::OK : status_t::FAULT;
    case instruction::STOSD: return emulate_stos(vcpu_, decoder_, sizeof(uint32_t)) ? status_t::OK : status_t::FAULT;
    case instruction::STOSQ: return emulate_stos(vcpu_, decoder_, sizeof(uint64_t)) ? status_t::OK : status_t::FAULT;
    case instruction::SYSCALL: return emulate_syscall(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::SYSRET: return emulate_sysret(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::IRET: return emulate_iret(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::SGDT: return emulate_sgdt_sidt(vcpu_, decoder_, mmreg::GDTR) ? status_t::OK : status_t::FAULT;
    case instruction::SIDT: return emulate_sgdt_sidt(vcpu_, decoder_, mmreg::IDTR) ? status_t::OK : status_t::FAULT;
    case instruction::LGDT: return emulate_lgdt_lidt(vcpu_, decoder_, mmreg::GDTR) ? status_t::OK : status_t::FAULT;
    case instruction::LIDT: return emulate_lgdt_lidt(vcpu_, decoder_, mmreg::IDTR) ? status_t::OK : status_t::FAULT;
    case instruction::STR: return emulate_str_sldt(vcpu_, decoder_, mmreg::TR) ? status_t::OK : status_t::FAULT;
    case instruction::SLDT: return emulate_str_sldt(vcpu_, decoder_, mmreg::LDTR) ? status_t::OK : status_t::FAULT;
    case instruction::LLDT: return emulate_ltr_lldt(vcpu_, decoder_, mmreg::LDTR) ? status_t::OK : status_t::FAULT;
    case instruction::LTR: return emulate_ltr_lldt(vcpu_, decoder_, mmreg::TR) ? status_t::OK : status_t::FAULT;
    case instruction::STI: return emulate_sti(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::CLI: return emulate_cli(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::OUT: return emulate_out(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::JMP: return emulate_jmp(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::CALL: return emulate_call(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::RET: return emulate_ret(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::PUSH: return emulate_push(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::POP: return emulate_pop(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::LEA: return emulate_lea(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;
    case instruction::CPUID: vcpu_.handle_cpuid(); return status_t::OK;
#define GENERATE_FUNCTION_SELECTION
#include <emulator/macros.hpp>
#undef GENERATE_FUNCTION_SELECTION
    case instruction::INVALID:
    case instruction::UNKNOWN: decoder_.print_instruction(); return status_t::UNIMPLEMENTED;
    };

    __UNREACHED__;
}

bool emulator::is_fully_implemented(instruction i)
{
    switch (i) {
    case instruction::MOV:
    case instruction::MOVZX:
    case instruction::MOVSB:
    case instruction::MOVSW:
    case instruction::MOVSD:
    case instruction::MOVSQ:
    case instruction::STOSB:
    case instruction::STOSW:
    case instruction::STOSD:
    case instruction::STOSQ:
    case instruction::SYSCALL:
    case instruction::SYSRET:
    case instruction::STI:
    case instruction::CLI:
    case instruction::JMP:
    case instruction::CALL:
    case instruction::RET:
    case instruction::PUSH:
    case instruction::POP:
    case instruction::LEA:
    case instruction::CPUID:
#define GENERATE_INSN_IMPLEMENTED_LIST
#include <emulator/macros.hpp>
#undef GENERATE_INSN_IMPLEMENTED_LIST
        return true;
    default: return false;
    }
}

} // namespace emulator
