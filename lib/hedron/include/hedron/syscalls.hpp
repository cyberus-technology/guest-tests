/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <cbl/cast_helpers.hpp>
#include <cbl/traits.hpp>
#include <compiler.h>
#include <hedron/cap_sel.hpp>
#include <hedron/crd.hpp>
#include <math.hpp>
#include <trace.hpp>

namespace hedron
{

class utcb;

enum
{
    MAX_ORDER = 0x1f,
    HYPERCALL_FLAG0 = 1u << 0,
    HYPERCALL_FLAG1 = 1u << 1,
    HYPERCALL_FLAG2 = 1u << 2,
    HYPERCALL_FLAG3 = 1u << 3,
};

enum class pd_ctrl_op : uint8_t
{
    LOOKUP,
    MAP_APIC_ACCESS_PAGE,
    DELEGATE,
    MSR_ACCESS,
};

enum class irq_ctrl_op : uint8_t
{
    CONFIGURE_VECTOR,
    ASSIGN_IOAPIC_PIN,
    MASK_IOAPIC_PIN,
    ASSIGN_MSI,
    ASSIGN_LVT,
    MASK_LVT,
};

enum class irq_ctrl_lvt_entry : uint8_t
{
    THERM = 0,
};

enum class machine_ctrl_op : uint8_t
{
    SUSPEND,
    UPDATE_MICROCODE,
};

enum class kp_ctrl_op : uint8_t
{
    MAP,
    UNMAP
};

enum class vcpu_ctrl_op : uint8_t
{
    RUN = 0,
    POKE = 1,
};

enum class ec_ctrl_op : uint8_t
{
    YIELD = 1,
};

static constexpr size_t PARAM0_ID_SIZE {8};
static constexpr size_t PARAM0_FLAGS_SIZE {4};

static constexpr size_t PARAM0_FLAGS_SHIFT {PARAM0_ID_SIZE};
static constexpr size_t PARAM0_SEL_SHIFT {PARAM0_ID_SIZE + PARAM0_FLAGS_SIZE};

struct hypercall_identifier
{
    constexpr hypercall_identifier() {}

    struct hypercall_id_param
    {
        uint8_t no;
        uint8_t flags;
    };

    hypercall_identifier(hypercall_id_param param)
    {
        no_ = param.no;
        flags_ = param.flags;
    }

    hypercall_identifier(uint8_t no, bool flag0 = false, bool flag1 = false, bool flag2 = false, bool flag3 = false)
    {
        no_ = no;
        flags_ = (flag0 * HYPERCALL_FLAG0) | (flag1 * HYPERCALL_FLAG1) | (flag2 * HYPERCALL_FLAG2)
                 | (flag3 * HYPERCALL_FLAG3);
    }

    uint16_t value() { return ((flags_ << PARAM0_FLAGS_SHIFT) | no_) & math::mask(PARAM0_ID_SIZE + PARAM0_FLAGS_SIZE); }

    uint8_t no_ {0};
    uint8_t flags_ {0};
};

struct hypercall_params
{
    uint64_t rdi, rsi, rdx, rax, r8;
    uint8_t result() { return rdi; }

    hypercall_params(hypercall_identifier id, uint64_t par0 = 0, uint64_t par1 = 0, uint64_t par2 = 0,
                     uint64_t par3 = 0, uint64_t par4 = 0)
        : rdi((par0 << PARAM0_SEL_SHIFT) | id.value()), rsi(par1), rdx(par2), rax(par3), r8(par4)
    {}

    // SC_CTRL returns the execution time in RSI:RDX
    uint64_t sc_ctrl_time() { return (rsi << cbl::bit_width<uint32_t>()) | rdx; }

    // LOOKUP returns the resulting CRD in RSI
    hedron::crd lookup_crd() { return hedron::crd(rsi); }

    // IRQ_CTRL.ASSIGN_MSI returns MSI address/data in RSI/RDX
    uint64_t msi_addr() { return rsi; }
    uint64_t msi_data() { return rdx; }

    // MSR_ACCESS returns value in RSI
    uint64_t msr_value() { return rsi; }

    // SUSPEND returns wake vector and mode in RSI
    uint64_t waking_mode_and_vector() { return rsi; }
};

[[nodiscard]] inline uint64_t do_hypercall(hypercall_params& params)
{
    register uint64_t r8 asm("r8") = params.r8;
    asm volatile("syscall"
                 : "+D"(params.rdi), "+S"(params.rsi), "+d"(params.rdx), "+a"(params.rax), "+r"(r8)::"rcx", "r11",
                   "memory");
    params.r8 = r8;
    return params.result();
}

enum space : uint64_t
{
    HOST = 1u << 0,
    GUEST = 1u << 1,
    DMA = 1u << 2,

    GUEST_DMA = GUEST | DMA,
    HOST_DMA = HOST | DMA,
    HOST_GUEST = HOST | GUEST,
    HOST_GUEST_DMA = HOST | GUEST | DMA,
};

enum class ec_type
{
    LOCAL,
    GLOBAL,
};

enum hypercalls
{
    CALL = 0x0,
    REPLY = 0x1,
    CREATE_PD = 0x2,
    CREATE_EC = 0x3,
    CREATE_SC = 0x4,
    CREATE_PT = 0x5,
    CREATE_SM = 0x6,
    REVOKE = 0x7,
    PD_CTRL = 0x8,
    EC_CTRL = 0x9,
    SC_CTRL = 0xa,
    PT_CTRL = 0xb,
    SM_CTRL = 0xc,
    MACHINE_CTRL = 0xf,
    CREATE_KP = 0x10,
    KP_CTRL = 0x11,
    IRQ_CTRL = 0x12,
    CREATE_VCPU = 0x13,
    VCPU_CTRL = 0x14,
};

enum status
{
    SUCCESS = 0x0,
    COM_TIM = 0x1,
    COM_ABT = 0x2,
    BAD_HYP = 0x3,
    BAD_CAP = 0x4,
    BAD_PAR = 0x5,
    BAD_FTR = 0x6,
    BAD_CPU = 0x7,
    BAD_DEV = 0x8,
    OOM = 0x9,
    BUSY = 0xa,
};

inline uint8_t call(uint64_t sel_obj_pt)
{
    hypercall_params params({CALL}, sel_obj_pt);
    return do_hypercall(params);
}

[[noreturn]] inline uint8_t reply(uint64_t sp)
{
    asm volatile("mov %[sp], %%rsp; syscall" ::[sp] "S"(sp), "D"(REPLY));
    __UNREACHED__;
}

inline uint8_t create_pd(uint64_t sel_obj_0, uint64_t sel_obj_pd, bool is_passthrough = false)
{
    hypercall_params params({CREATE_PD, is_passthrough}, sel_obj_0, sel_obj_pd);
    return do_hypercall(params);
}

inline uint8_t create_ec(uint64_t sel_obj_0, uint64_t sel_obj_pd, uint64_t cpu, hedron::utcb* utcb, uint64_t sp,
                         uint64_t sel_evt, ec_type type, bool user_page_in_calling_pd = false)
{
    bool is_global {type != ec_type::LOCAL};

    hypercall_params params {{CREATE_EC, is_global, false, false, user_page_in_calling_pd},
                             sel_obj_0,
                             sel_obj_pd,
                             ptr_to_num(utcb) | cpu,
                             sp,
                             sel_evt};

    return do_hypercall(params);
}

inline uint8_t create_sc(uint64_t sel_obj_0, uint64_t sel_obj_pd, uint64_t sel_obj_ec, uint64_t qpd)
{
    hypercall_params params({CREATE_SC}, sel_obj_0, sel_obj_pd, sel_obj_ec, qpd);
    return do_hypercall(params);
}

inline uint8_t create_pt(uint64_t sel_obj_0, uint64_t sel_obj_pd, uint64_t sel_obj_ec, uint64_t mtd, uint64_t ip)
{
    hypercall_params params({CREATE_PT}, sel_obj_0, sel_obj_pd, sel_obj_ec, mtd, ip);
    return do_hypercall(params);
}

inline uint8_t create_sm(uint64_t sel_obj_0, uint64_t sel_obj_pd, uint64_t counter)
{
    hypercall_params params({CREATE_SM}, sel_obj_0, sel_obj_pd, counter);
    return do_hypercall(params);
}

inline uint8_t create_kp(uint64_t sel_obj_0, uint64_t sel_obj_pd)
{
    hypercall_params params({CREATE_KP}, sel_obj_0, sel_obj_pd);
    return do_hypercall(params);
}

inline uint8_t revoke(crd range, bool self = false)
{
    const bool remote {false};
    hypercall_params params({REVOKE, self, remote}, 0, range.value());
    return do_hypercall(params);
}

inline uint8_t revoke_remote(hedron::cap_sel dst_pd, crd range, bool self = true)
{
    const bool remote {true};
    hypercall_params params({REVOKE, self, remote}, 0, range.value(), dst_pd);
    return do_hypercall(params);
}

inline uint8_t pd_ctrl(pd_ctrl_op op, crd& range)
{
    hypercall_params params({{.no = PD_CTRL, .flags = to_underlying(op)}}, 0, range.value());

    auto res = do_hypercall(params);
    if (op == pd_ctrl_op::LOOKUP) {
        range = params.lookup_crd();
    }
    return res;
}

inline uint8_t lookup(crd& range)
{
    return pd_ctrl(pd_ctrl_op::LOOKUP, range);
}

inline uint8_t map_apic_access_page(crd range)
{
    return pd_ctrl(pd_ctrl_op::MAP_APIC_ACCESS_PAGE, range);
}

inline uint8_t delegate(uint64_t src_pd, uint64_t dst_pd, crd src_crd, uint64_t flags, crd dst_crd)
{
    hypercall_params params({{.no = PD_CTRL, .flags = to_underlying(pd_ctrl_op::DELEGATE)}}, src_pd, dst_pd,
                            src_crd.value(), flags, dst_crd.value());

    return do_hypercall(params);
}

inline uint8_t read_msr(uint32_t msr_index, uint64_t& msr_value)
{
    hypercall_params params({{.no = PD_CTRL, .flags = to_underlying(pd_ctrl_op::MSR_ACCESS)}}, msr_index, 0);

    const auto rc {do_hypercall(params)};

    msr_value = params.msr_value();

    return rc;
}

inline uint8_t write_msr(uint32_t msr_index, uint64_t msr_value)
{
    const uint8_t IS_WRITE {0x4};
    hypercall_params params({{.no = PD_CTRL, .flags = IS_WRITE | to_underlying(pd_ctrl_op::MSR_ACCESS)}}, msr_index,
                            msr_value);

    return do_hypercall(params);
}

inline uint8_t sc_ctrl(uint64_t sel_obj_sc, uint64_t& time)
{
    hypercall_params params({SC_CTRL}, sel_obj_sc);
    auto res = do_hypercall(params);
    time = params.sc_ctrl_time();
    return res;
}

inline uint8_t pt_ctrl(uint64_t sel_obj_pt, uint64_t pid)
{
    hypercall_params params({PT_CTRL}, sel_obj_pt, pid);
    return do_hypercall(params);
}

inline uint8_t sm_ctrl(uint64_t sel_obj_sm, bool down, bool zero = false, uint64_t timeout = 0)
{
    hypercall_params params({SM_CTRL, down, zero}, sel_obj_sm, timeout >> 32, timeout);
    return do_hypercall(params);
}

inline uint8_t kp_ctrl_map(uint64_t sel_obj_kp, uint64_t dst_pd, uint64_t dst_addr)
{
    hypercall_params params({{.no = KP_CTRL, .flags = to_underlying(kp_ctrl_op::MAP)}}, sel_obj_kp, dst_pd, dst_addr);
    return do_hypercall(params);
}

inline uint8_t kp_ctrl_unmap(uint64_t sel_obj_kp)
{
    hypercall_params params({{.no = KP_CTRL, .flags = to_underlying(kp_ctrl_op::UNMAP)}}, sel_obj_kp);
    return do_hypercall(params);
}

enum class waking_mode : uint8_t
{
    REAL_MODE = 0,
};

/**
 * @brief Enter sleep mode.
 *
 * After resume, the function reports the ACPI waking vector from when this
 * function was called.
 *
 * @param slp_typa ACPI sleeping state type of power management control register a
 * @param slp_typb ACPI sleeping state type of power management control register b
 * @param waking_vector ACPI firmware waking vector for
 * @param mode mode to which the waking vector refers to
 *
 * @return
 */
inline uint8_t suspend(uint8_t slp_typa, uint8_t slp_typb, uint64_t& waking_vector, waking_mode& mode)
{
    constexpr static uint64_t WAKING_MODE_MASK = 0x3;
    constexpr static uint64_t WAKING_MODE_SHIFT = 62;

    hypercall_params params({{.no = MACHINE_CTRL, .flags = to_underlying(machine_ctrl_op::SUSPEND)}},
                            (slp_typb << 8 | slp_typa), waking_vector);

    auto result {do_hypercall(params)};
    auto mode_and_vector {params.waking_mode_and_vector()};
    mode = static_cast<waking_mode>((mode_and_vector >> WAKING_MODE_SHIFT) & WAKING_MODE_MASK);
    waking_vector = mode_and_vector & ~(WAKING_MODE_MASK << WAKING_MODE_SHIFT);
    return result;
}

inline uint8_t update_microcode(uint64_t update_address, unsigned update_size)
{
    hypercall_params params({{.no = MACHINE_CTRL, .flags = to_underlying(machine_ctrl_op::UPDATE_MICROCODE)}},
                            update_size, update_address);
    return do_hypercall(params);
}

inline uint8_t irq_ctrl_configure_vector(uint8_t vector, uint16_t cpu, uint64_t sel_obj_sm, uint64_t sel_obj_kp,
                                         uint16_t kp_bit)
{
    PANIC_ON(kp_bit & ~0x7FFF, "KP bit out-of-range: {}", kp_bit);

    hypercall_params params({{.no = IRQ_CTRL, .flags = to_underlying(irq_ctrl_op::CONFIGURE_VECTOR)}},
                            vector | (cpu << 8), sel_obj_sm, sel_obj_kp, kp_bit);

    return do_hypercall(params);
}

inline uint8_t irq_ctrl_assign_ioapic_pin(uint8_t vector, uint16_t cpu, uint8_t ioapic_id, uint8_t ioapic_pin,
                                          bool level, bool active_low)
{
    PANIC_ON(ioapic_id & ~0xF, "invalid IO-APIC ID: {}", ioapic_id);

    hypercall_params params({{.no = IRQ_CTRL, .flags = to_underlying(irq_ctrl_op::ASSIGN_IOAPIC_PIN)}},
                            vector | (cpu << 8) | (level << 24) | (active_low << 25), ioapic_id | ioapic_pin << 4);

    return do_hypercall(params);
}

inline uint8_t irq_ctrl_mask_ioapic_pin(uint8_t ioapic_id, uint8_t ioapic_pin, bool mask)
{
    PANIC_ON(ioapic_id & ~0xF, "invalid IO-APIC ID: {}", ioapic_id);

    hypercall_params params({{.no = IRQ_CTRL, .flags = to_underlying(irq_ctrl_op::MASK_IOAPIC_PIN)}}, mask,
                            (ioapic_id & 0xF) | ioapic_pin << 4);

    return do_hypercall(params);
}

inline uint8_t irq_ctrl_assign_msi(uint8_t vector, uint16_t cpu, uint64_t dev, uint64_t& msi_addr, uint64_t& msi_data)
{
    PANIC_ON(dev & 0xFFF, "dev is not page aligned: {#x}", dev);

    hypercall_params params({{.no = IRQ_CTRL, .flags = to_underlying(irq_ctrl_op::ASSIGN_MSI)}}, vector | (cpu << 8),
                            dev);

    auto res {do_hypercall(params)};

    msi_addr = params.msi_addr();
    msi_data = params.msi_data();

    return res;
}

inline uint8_t irq_ctrl_assign_lvt(uint8_t vector, irq_ctrl_lvt_entry lvt_entry)
{
    hypercall_params params({{.no = IRQ_CTRL, .flags = to_underlying(irq_ctrl_op::ASSIGN_LVT)}}, vector,
                            to_underlying(lvt_entry));

    return do_hypercall(params);
}

inline uint8_t irq_ctrl_mask_lvt(irq_ctrl_lvt_entry lvt_entry, bool mask)
{
    hypercall_params params({{.no = IRQ_CTRL, .flags = to_underlying(irq_ctrl_op::MASK_LVT)}}, mask,
                            to_underlying(lvt_entry));

    return do_hypercall(params);
}

inline uint8_t create_vcpu(uint64_t sel_obj_0, uint64_t sel_obj_pd, uint64_t sel_obj_kp_vcpu_state,
                           uint64_t sel_obj_kp_vlapic, uint64_t sel_obj_kp_fpu, uint16_t cpu)
{
    PANIC_ON(cpu & ~0xfff, "CPU number is too large: {}", cpu);

    hypercall_params params({CREATE_VCPU}, sel_obj_0, sel_obj_pd << 12 | cpu, sel_obj_kp_vcpu_state, sel_obj_kp_vlapic,
                            sel_obj_kp_fpu);
    auto res {do_hypercall(params)};

    return res;
}

inline uint8_t vcpu_ctrl_run(uint64_t sel_obj_vcpu, uint64_t mtd)
{
    hypercall_params params({{.no = VCPU_CTRL, .flags = to_underlying(vcpu_ctrl_op::RUN)}}, sel_obj_vcpu, mtd);
    return do_hypercall(params);
}

inline uint8_t vcpu_ctrl_poke(uint64_t sel_obj_vcpu)
{
    hypercall_params params({{.no = VCPU_CTRL, .flags = to_underlying(vcpu_ctrl_op::POKE)}}, sel_obj_vcpu);
    return do_hypercall(params);
}

inline uint8_t ec_ctrl_yield()
{
    const hypercall_identifier id {EC_CTRL, to_underlying(ec_ctrl_op::YIELD)};
    hypercall_params params(id);
    return do_hypercall(params);
}

} // namespace hedron
