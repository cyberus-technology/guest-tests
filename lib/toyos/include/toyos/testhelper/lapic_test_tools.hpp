/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */
#pragma once

#include <array>
#include <map>

#include <toyos/util/interval.hpp>
#include "int_guard.hpp"
#include "../x86/x86asm.hpp"

struct intr_regs;

namespace lapic_test_tools
{
constexpr uintptr_t LAPIC_START_ADDR {0xfee00000};
constexpr uintptr_t LAPIC_ID {0xfee00020};
constexpr uintptr_t LAPIC_TPR {0xfee00080};
constexpr uintptr_t LAPIC_PPR {0xfee000a0};
constexpr uintptr_t LAPIC_EOI {0xfee000b0};
constexpr uintptr_t LAPIC_SVR {0xfee000f0};
constexpr uintptr_t ISR_0_31 {0xfee00100};
constexpr uintptr_t ISR_32_63 {0xfee00110};
constexpr uintptr_t ISR_64_95 {0xfee00120};
constexpr uintptr_t ISR_96_127 {0xfee00130};
constexpr uintptr_t ISR_128_159 {0xfee00140};
constexpr uintptr_t ISR_160_191 {0xfee00150};
constexpr uintptr_t ISR_192_223 {0xfee00160};
constexpr uintptr_t ISR_224_255 {0xfee00170};
constexpr uintptr_t IRR_0_31 {0xfee00200};
constexpr uintptr_t IRR_32_63 {0xfee00210};
constexpr uintptr_t IRR_64_95 {0xfee00220};
constexpr uintptr_t IRR_96_127 {0xfee00230};
constexpr uintptr_t IRR_128_159 {0xfee00240};
constexpr uintptr_t IRR_160_191 {0xfee00250};
constexpr uintptr_t IRR_192_223 {0xfee00260};
constexpr uintptr_t IRR_224_255 {0xfee00270};
constexpr uintptr_t LAPIC_LVT_CMCI {0xfee002f0};
constexpr uintptr_t LAPIC_ICR_LOW {0xfee00300};
constexpr uintptr_t LAPIC_ICR_HIGH {0xfee00310};
constexpr uintptr_t LAPIC_LVT_TIMER {0xfee00320};
constexpr uintptr_t LAPIC_LVT_THERMAL {0xfee00330};
constexpr uintptr_t LAPIC_LVT_PERF_MON {0xfee00340};
constexpr uintptr_t LAPIC_LVT_LINT0 {0xfee00350};
constexpr uintptr_t LAPIC_LVT_LINT1 {0xfee00360};
constexpr uintptr_t LAPIC_LVT_ERROR {0xfee00370};
constexpr uintptr_t LAPIC_INIT_COUNT {0xfee00380};
constexpr uintptr_t LAPIC_CURR_COUNT {0xfee00390};
constexpr uintptr_t LAPIC_DIVIDE_CONF {0xfee003e0};

static constexpr std::array<uintptr_t, 7> lvt_regs = {{
    LAPIC_LVT_CMCI,
    LAPIC_LVT_TIMER,
    LAPIC_LVT_THERMAL,
    LAPIC_LVT_PERF_MON,
    LAPIC_LVT_LINT0,
    LAPIC_LVT_LINT1,
    LAPIC_LVT_ERROR,
}};

constexpr uint8_t PIC0_DATA {0x21};
constexpr uint8_t PIC1_DATA {0xa1};

constexpr uint32_t LAPIC_REG_STRIDE {0x010};

constexpr uint32_t LAPIC_DLV_STS_MASK {1u << 12};
constexpr uint32_t SVR_VECTOR_MASK {0xff};
constexpr uint32_t LVT_VECTOR_MASK {0xff};
constexpr uint32_t LVT_MODE_SHIFT {17};
constexpr uint32_t LVT_MODE_MASK {0b11};
constexpr uint32_t LVT_MASK_SHIFT {16};
constexpr uint32_t LVT_MASK_MASK {0b1};
constexpr uint32_t LAPIC_TPR_CLASS_SHIFT {4};
constexpr uint32_t LAPIC_TPR_CLASS_MASK {0xff};
constexpr uint32_t ICR_DEST_MASK {0x3};
constexpr uint32_t ICR_DEST_SHIFT {56 - 32};
constexpr uint32_t ICR_DEST_SH_MASK {0x3};
constexpr uint32_t ICR_DEST_SH_SHIFT {18};
constexpr uint32_t ICR_LEVEL_MASK {0x1};
constexpr uint32_t ICR_LEVEL_SHIFT {14};
constexpr uint32_t ICR_DEST_MODE_MASK {0x1};
constexpr uint32_t ICR_DEST_MODE_SHIFT {11};
constexpr uint32_t ICR_DLV_MODE_MASK {0x7};
constexpr uint32_t ICR_DLV_MODE_SHIFT {8};
constexpr uint32_t LAPIC_ID_SHIFT {24};
constexpr uint32_t LAPIC_ID_MASK {0xff};
constexpr uint32_t MAX_VECTOR {255};
constexpr uint32_t MIN_VECTOR {33};
constexpr uint32_t VECTORS_PER_CLASS {16};
constexpr uint32_t SPURIOUS_TEST_VECTOR {32};
constexpr uint32_t LAPIC_MAX_COUNT {0xffffffff};

constexpr uint32_t SVR_ENABLED_MASK {1u};
constexpr uint32_t SVR_ENABLED_SHIFT {8u};
constexpr uint32_t APIC_GLOBAL_ENABLED_MASK {1u};
constexpr uint32_t APIC_GLOBAL_ENABLED_SHIFT {11u};

constexpr cbl::interval fixed_valid_vectors(MIN_VECTOR, MAX_VECTOR + 1);

enum class lvt_entry
{
    TIMER = 0x320,
    CMCI = 0x2f0,
    LINT0 = 0x350,
    LINT1 = 0x360,
    ERROR = 0x370,
    PERFORMANCE_MON = 0x340,
    THERMAL_SENSOR = 0x330,
};

struct lvt_entry_t
{
    uint32_t vector;
    uint32_t mode;
    uint32_t mask;
};

enum dest_sh
{
    NO_SH = 0,
    SELF = 1,
    ALL_INC_SELF = 2,
    ALL_EXC_SELF = 3,
};

enum level
{
    DE_ASSERT = 0,
    ASSERT = 1,
};

enum dest_mode
{
    PHYSICAL = 0,
    LOGICAL = 1,
};

enum dlv_mode
{
    FIXED = 0,
    LOWEST = 1,
    SMI = 2,
    NMI = 4,
    INIT = 5,
    START_UP = 6,
    EXTINT = 7,
};

enum lvt_modes
{
    ONESHOT = 0,
    PERIODIC = 1,
    DEADLINE = 2,
};

void mask_pic();

void write_to_register(uintptr_t address, uint32_t value);

uint32_t read_from_register(uintptr_t address);

void drain_irq(intr_regs* regs);

bool lapic_send_pending();

void wait_until_ready_for_ipi();

void write_reg_generic(uintptr_t address, uint32_t mask, uint32_t shift, uint32_t value);

void write_spurious_vector(uint8_t value);

void lapic_set_task_priority(uint8_t priority, bool use_mmio = false);

void write_lvt_generic(lvt_entry entry, uint32_t mask, uint32_t shift, uint32_t value);

void write_lvt_vector(lvt_entry entry, uint32_t vector);

void write_lvt_mask(lvt_entry entry, uint32_t mask);

void write_lvt_mode(lvt_entry entry, uint32_t mode);

void write_lvt_entry(lvt_entry entry, lvt_entry_t data);

void write_divide_conf(uint32_t conf);

void stop_lapic_timer();

void send_eoi();

uint32_t ticks_per_second(uint32_t divisor);

void enable_periodic_timer(uint32_t interrupt_vector, uint32_t time_in_ms);

/**
 * \brief disables APIC via APIC_BASE_MSR
 *
 * Caller has to assure that no interrupt is sent when calling this function
 * and while the APIC is disabled.
 */
void global_apic_disable();

/**
 * \brief enabled APIC via APIC_BASE_MSR
 *
 * Caller has to assure that no interrupt is sent when calling this function.
 * Note that the APIC may remain software disabled.
 */
void global_apic_enable();
bool global_apic_enabled();

/**
 * \brief disable APIC via SVR
 */
void software_apic_disable();

/**
 * \brief enable APIC via SVR
 */
void software_apic_enable();

/**
 * @return True if the APIC is enabled via SVR, false otherwise.
 */
[[nodiscard]] bool software_apic_enabled();

void send_self_ipi(uint8_t vector, dest_sh sh = dest_sh::SELF, dest_mode dest = dest_mode::PHYSICAL,
                   dlv_mode dlv = dlv_mode::FIXED);

bool check_irr(uint8_t vector);
} // namespace lapic_test_tools
