/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <arch.h>
#include <stdint.h>

#include <cbl/interval.hpp>
#include <compiler.hpp>
#include <math/math.hpp>
#include <segmentation.hpp>

namespace x86
{

enum class memory_access_type_t
{
    READ,
    WRITE,
    EXECUTE,
};

enum : uint32_t
{
    FLAGS_CF = 1u << 0,
    FLAGS_MBS = 1u << 1,
    FLAGS_PF = 1u << 2,
    FLAGS_AF = 1u << 4,
    FLAGS_ZF = 1u << 6,
    FLAGS_SF = 1u << 7,
    FLAGS_TF = 1u << 8,
    FLAGS_IF = 1u << 9,
    FLAGS_DF = 1u << 10,
    FLAGS_OF = 1u << 11,
    FLAGS_IOPL = 3u << 12,
    FLAGS_NT = 1u << 14,
    FLAGS_RF = 1u << 16,
    FLAGS_VM = 1u << 17,
    FLAGS_AC = 1u << 18,
    FLAGS_ID = 1u << 21,

    EFER_SCE = 1u << 0,
    EFER_LME = 1u << 8,
    EFER_LMA = 1u << 10,

    XCR0_FPU = 1u << 0,       // Legacy FPU/MMX state (must be set)
    XCR0_SSE = 1u << 1,       // MXCSR, XMM registers
    XCR0_AVX = 1u << 2,       // YMM registers
    XCR0_BNDREG = 1u << 3,    // BND0-BND3 registers (MPX)
    XCR0_BNDCSR = 1u << 4,    // BNDCFGU/BNDSTATUS registers (MPX)
    XCR0_OPMASK = 1u << 5,    // AVX-512: k0-k7 registers
    XCR0_ZMM_Hi256 = 1u << 6, // AVX-512: ZMM0-ZMM15 registers
    XCR0_Hi16_ZMM = 1u << 7,  // AVX-512: ZMM16-ZMM31 registers
    XCR0_PKRU = 1u << 9,      // PKRU register (protection keys)

    XCR0_AVX512 = XCR0_OPMASK | XCR0_ZMM_Hi256 | XCR0_Hi16_ZMM,

    XCR0_MASK = XCR0_FPU | XCR0_SSE | XCR0_AVX | XCR0_AVX512,

    SPEC_CTRL_IBRS = 1u << 0,
    SPEC_CTRL_STIBP = 1u << 1,
    SPEC_CTRL_SSBD = 1u << 2,
};

enum class cr0 : uint32_t
{
    PE = 1u << 0,
    MP = 1u << 1,
    EM = 1u << 2,
    TS = 1u << 3,
    ET = 1u << 4,
    NE = 1u << 5,
    WP = 1u << 16,
    AM = 1u << 18,
    NW = 1u << 29,
    CD = 1u << 30,
    PG = 1u << 31,
};

enum class cr4 : uint32_t
{
    VME = 1u << 0,
    PVI = 1u << 1,
    TSD = 1u << 2,
    DE = 1u << 3,
    PSE = 1u << 4,
    PAE = 1u << 5,
    MCE = 1u << 6,
    PGE = 1u << 7,
    PCE = 1u << 8,
    OSFXSR = 1u << 9,
    OSXMMEXCEPT = 1u << 10,
    VMXE = 1u << 13,
    SMXE = 1u << 14,
    FSGSBASE = 1u << 16,
    PCIDE = 1u << 17,
    OSXSAVE = 1u << 18,
    SMEP = 1u << 20,
};

enum class exception : uint8_t
{
    DE,
    DB,
    NMI,
    BP,
    OF,
    BR,
    UD,
    NM,
    DF,
    TS = 10,
    NP,
    SS,
    GP,
    PF,
    MF = 16,
    AC,
    MC,
    XM,
    VE,
};

constexpr bool is_exception(uint8_t vector)
{
    return vector <= uint8_t(exception::VE);
}

constexpr bool is_user_interrupt(uint8_t vector)
{
    return vector >= 0x20;
}

enum msr : uint32_t
{
    EFER = 0xC0000080,
    STAR = 0xC0000081,
    LSTAR = 0xC0000082,
    CSTAR = 0xC0000083,
    FMASK = 0xC0000084,
    FS_BASE = 0xC0000100,
    GS_BASE = 0xC0000101,
    KERNEL_GS_BASE = 0xC0000102,
    SYSENTER_CS = 0x00000174,
    SYSENTER_SP = 0x00000175,
    SYSENTER_IP = 0x00000176,
    MISC_ENABLE = 0x000001A0,

    PAT = 0x00000277,
    MTRR_CAP = 0x000000FE,
    MTRR_PHYS_BASE_0 = 0x00000200,
    MTRR_DEF_TYPE = 0x000002FF,
    MTRR_FIX_64K_00000 = 0x00000250,
    MTRR_FIX_16K_80000 = 0x00000258,
    MTRR_FIX_16K_A0000 = 0x00000259,
    MTRR_FIX_4K_C0000 = 0x00000268,
    MTRR_FIX_4K_C8000 = 0x00000269,
    MTRR_FIX_4K_D0000 = 0x0000026A,
    MTRR_FIX_4K_D8000 = 0x0000026B,
    MTRR_FIX_4K_E0000 = 0x0000026C,
    MTRR_FIX_4K_E8000 = 0x0000026D,
    MTRR_FIX_4K_F0000 = 0x0000026E,
    MTRR_FIX_4K_F8000 = 0x0000026F,

    IA32_TIME_STAMP_COUNTER = 0x00000010,
    IA32_FEATURE_CONTROL = 0x0000003A,
    IA32_TSC_ADJUST = 0x0000003B,
    IA32_TSC_AUX = 0xC0000103,

    MWAIT_FILTER_SIZE = 0x00000006,

    MSR_PLATFORM_INFO = 0x000000CE,

    IA32_TSC_DEADLINE = 0x000006E0,
    IA32_APIC_BASE = 0x0000001B,

    X2APIC_LAPIC_ID = 0x00000802,
    X2APIC_LAPIC_VERSION = 0x00000803,

    X2APIC_TPR = 0x00000808,

    X2APIC_PPR = 0x0000080A,
    X2APIC_EOI = 0x0000080B,

    X2APIC_LDR = 0x0000080D,

    X2APIC_SVR = 0x0000080F,

    X2APIC_ISR_31_0 = 0x00000810,
    X2APIC_ISR_63_32 = 0x00000811,
    X2APIC_ISR_95_64 = 0x00000812,
    X2APIC_ISR_127_96 = 0x00000813,
    X2APIC_ISR_159_128 = 0x00000814,
    X2APIC_ISR_191_160 = 0x00000815,
    X2APIC_ISR_223_192 = 0x00000816,
    X2APIC_ISR_255_224 = 0x00000817,

    X2APIC_TMR_31_0 = 0x00000818,
    X2APIC_TMR_63_32 = 0x00000819,
    X2APIC_TMR_95_64 = 0x0000081A,
    X2APIC_TMR_127_96 = 0x0000081B,
    X2APIC_TMR_159_128 = 0x0000081C,
    X2APIC_TMR_191_160 = 0x0000081D,
    X2APIC_TMR_223_192 = 0x0000081E,
    X2APIC_TMR_255_224 = 0x0000081F,

    X2APIC_IRR_31_0 = 0x00000820,
    X2APIC_IRR_63_32 = 0x00000821,
    X2APIC_IRR_95_64 = 0x00000822,
    X2APIC_IRR_127_96 = 0x00000823,
    X2APIC_IRR_159_128 = 0x00000824,
    X2APIC_IRR_191_160 = 0x00000825,
    X2APIC_IRR_223_192 = 0x00000826,
    X2APIC_IRR_255_224 = 0x00000827,

    X2APIC_ERR_STS = 0x00000828,
    X2APIC_LVT_CMCI = 0x0000082f,
    X2APIC_ICR = 0x00000830,

    X2APIC_LVT_TIMER = 0x00000832,
    X2APIC_LVT_THERMAL = 0x00000833,
    X2APIC_LVT_PERF_MON = 0x00000834,
    X2APIC_LVT_LINT0 = 0x00000835,
    X2APIC_LVT_LINT1 = 0x00000836,
    X2APIC_LVT_ERR = 0x00000837,
    X2APIC_INIT_CNT = 0x00000838,
    X2APIC_CURR_CNT = 0x00000839,

    X2APIC_DIV_CONF = 0x0000083E,
    X2APIC_X2_SELF_IPI = 0x0000083F,

    IA32_VMX_BASIC = 0x00000480,
    IA32_VMX_PINBASED_CTLS = 0x00000481,
    IA32_VMX_PROCBASED_CTLS = 0x00000482,
    IA32_VMX_EXIT_CTLS = 0x00000483,
    IA32_VMX_ENTRY_CTLS = 0x00000484,
    IA32_VMX_MISC = 0x00000485,
    IA32_VMX_CR0_FIXED0 = 0x00000486,
    IA32_VMX_CR0_FIXED1 = 0x00000487,
    IA32_VMX_CR4_FIXED0 = 0x00000488,
    IA32_VMX_CR4_FIXED1 = 0x00000489,
    IA32_VMX_VMCS_ENUM = 0x0000048A,
    IA32_VMX_PROCBASED_CTLS2 = 0x0000048B,
    IA32_VMX_EPT_VPID_CAP = 0x0000048C,
    IA32_VMX_TRUE_PINBASED_CTLS = 0x0000048D,

    // Microcode updates
    IA32_PLATFORM_ID = 0x00000017,
    IA32_BIOS_UPDT_TRIG = 0x00000079,
    IA32_BIOS_SIGN_ID = 0x0000008b,

    // Processor Vulnerability Mitigations
    IA32_SPEC_CTRL = 0x00000048,
    IA32_PRED_CMD = 0x00000049,
    IA32_ARCH_CAPABILITIES = 0x0000010a,

    IA32_SGXLEPUBKEYHASH0 = 0x0000008c,
    IA32_SGXLEPUBKEYHASH1 = 0x0000008d,
    IA32_SGXLEPUBKEYHASH2 = 0x0000008e,
    IA32_SGXLEPUBKEYHASH3 = 0x0000008f,

    // Thermal Management
    IA32_PACKAGE_THERM_STATUS = 0x000001b1,
    IA32_PACKAGE_THERM_INTERRUPT = 0x000001b2,

    // Hardware Feedback Interface
    IA32_HW_FEEDBACK_PTR = 0x000017d0,
    IA32_HW_FEEDBACK_CONFIG = 0x000017d1,
    IA32_THREAD_FEEDBACK_CHAR = 0x000017d2,
    IA32_THREAD_FEEDBACK_CONFIG = 0x000017d4,
    IA32_HRESET_ENABLE = 0x000017da,
};

constexpr const uint64_t MTRR_CAP_VARIABLE_RANGE_COUNT_MASK = 0xff;

constexpr const uint64_t IA32_APIC_BASE_BSP_MASK = 1u << 8;
constexpr const uint64_t IA32_APIC_BASE_EXTD_MASK = 1u << 10;
constexpr const uint64_t IA32_APIC_BASE_EN_MASK = 1u << 11;
constexpr const uint64_t IA32_APIC_BASE_ADDR_MASK = 0xfffff000;

constexpr const uint64_t IA32_FEATURE_CONTROL_LOCK = 1u;
constexpr const uint64_t IA32_FEATURE_CONTROL_ENABLE_VMX_OUTSIDE_SMX = 1u << 2;
constexpr const uint64_t IA32_FEATURE_CONTROL_SGX_LAUNCH_CONTROL_ENABLE = 1u << 17;
constexpr const uint64_t IA32_FEATURE_CONTROL_SGX = 1u << 18;

/// A default value for PAT that ensures legacy compatible behavior of PCD/PWT in page tables.
constexpr const uint64_t IA32_PAT_DEFAULT_VALUE = 0x7040600070406ULL;

constexpr const uint64_t IA32_PACKAGE_THERM_STATUS_HFI_CHANGE = 1u << 26;
constexpr const uint64_t IA32_PACKAGE_THERM_INTERRUPT_HFI_ENABLE = 1u << 25;

constexpr const uint64_t IA32_HW_FEEDBACK_PTR_VALID = 1U << 0;
constexpr const uint64_t IA32_HW_FEEDBACK_PTR_ADDR_MASK = ~0xFFFULL;

constexpr const uint64_t IA32_HW_FEEDBACK_CONFIG_HFI_ENABLE = 1U << 0;
constexpr const uint64_t IA32_HW_FEEDBACK_CONFIG_TD_ENABLE = 1U << 1;

enum class intr_type : uint8_t
{
    EXTINT = 0,
    NMI = 2,
    HW_EXC = 3,
    SW_INT = 4,
    SW_INT_PRIV = 5,
    SW_EXC = 6,
    OTHER = 7,
};

enum class gpr
{
    // 8-bit GPRs
    AL,
    BL,
    CL,
    DL,
    AH,
    BH,
    CH,
    DH,
    SIL,
    DIL,
    SPL,
    BPL,
    R8L,
    R9L,
    R10L,
    R11L,
    R12L,
    R13L,
    R14L,
    R15L,

    // 16-bit GPRs
    AX,
    BX,
    CX,
    DX,
    SP,
    BP,
    SI,
    DI,
    R8W,
    R9W,
    R10W,
    R11W,
    R12W,
    R13W,
    R14W,
    R15W,

    // 32-bit GPRs
    EAX,
    EBX,
    ECX,
    EDX,
    ESP,
    EBP,
    ESI,
    EDI,
    R8D,
    R9D,
    R10D,
    R11D,
    R12D,
    R13D,
    R14D,
    R15D,

    // 64-bit GPRs
    RAX,
    RBX,
    RCX,
    RDX,
    RSP,
    RBP,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,

    // Special
    IP,
    CUR_IP,
    FLAGS,
};

enum class mmreg
{
    IDTR,
    GDTR,
    LDTR,
    TR,
};

enum class control_register
{
    CR0,
    CR2,
    CR3,
    CR4,
    CR8,
    XCR0,
};

enum class cpu_vendor
{
    GENERIC,
    INTEL,
    AMD,
};

enum class rep_prefix
{
    REP,
    REPE,
    REPNE,
};

static constexpr cbl::interval lapic_range {cbl::interval::from_size(0xfee00000, PAGE_SIZE)};

enum cpuid_constants
{
    LARGEST_EXTENDED_FUNCTION_CODE = 0x80000000,
    ADDR_SIZE_INFORMATION = 0x80000008,
    PHY_ADDR_BITS_MASK = 0xFF,
    LIN_ADDR_BITS_MASK = 0xFF00,
    PHY_ADDR_BITS_SHIFT = 0x0,
    LIN_ADDR_BITS_SHIFT = 0x8,
};

enum class vmx_memory_types : uint8_t
{
    UNCACHEABLE = 0,
    WRITE_BACK = 6,
};

/**
 * Definitions related to the IA32_VMX_BASIC MSR.
 * Intel SDM Vol 3 Appendix A.1
 */
enum class vmx_basic_constants : uint64_t
{
    MAX_VMXON_REGION_BYTES = 0x1000,

    /// The values of bits 47:45 and bits 63:56 are reserved and are read as 0.
    RESERVED_ZERO_MASK = 0xF00E000000000000,
    ALWAYS_ZERO = 1ul << 31,

    VMCS_REVISION_ID_MASK = math::mask(30),
    VMCS_REVISION_ID_SHIFT = 0,
    BYTES_VMXON_REGION_MASK = math::mask(13),
    BYTES_VMXON_REGION_SHIFT = 32,
    MEMORY_TYPE_VMCS_MASK = math::mask(4),
    MEMORY_TYPE_VMCS_SHIFT = 50,

    LIMIT_WIDTH_PHY_ADDR_LEN_VMXON = 1ul << 48,
    DUAL_MONITOR_TREATMENT = 1ul << 49,
    INS_OUTS_VMEXIT_INFO = 1ul << 54,
    OVERRIDE_DEFAULT_ONE_CLASS = 1ul << 55,
};

/**
 * Definitions related to the IA32_VMX_PINBASED_CTLS MSR.
 * See Intel SDM Vol 3 Chapter 24.6.1
 */
enum class vmx_pin_based_constants : uint32_t
{
    /**
     * On the first processors without IA32_VMX_TRUE_PINBASED_CTLS MSR, these bits are always
     * reported as 1. If zero setting of these bits is supported, the IA32_VMX_TRUE_PINBASED_CTLS
     * MSR describes which bits can bet set to zero.
     *
     * If the IA32_VMX_TRUE_PINBASED_CTLS MSR is not emulated these bits should be reported
     * as 1 by the IA32_VMX_PINBASED_CTLS.
     */
    DEFAULT_ONE = 0x16,

    EXT_INT_EXITING = 1ul << 0,
    NMI_EXITING = 1ul << 3,
    VIRTUAL_NMI = 1ul << 5,
    ACTIVE_VMX_PREEMPTION_TIMER = 1ul << 6,
    PROCESS_POSTED_INTERRUPTS = 1ul << 7,

};

/**
 * Definitions related to the IA32_VMX_PROCBASED_CTLS MSR.
 * See Intel SDM Vol 3 Chapter 24.6.2 Table 24-6
 */
enum class vmx_primary_exc_ctls_constants : uint32_t
{
    /**
     * On the first processors without IA32_VMX_TRUE_PROCBASED_CTLS MSR, these bits are always
     * reported as 1. If zero setting of these bits is supported, the IA32_VMX_TRUE_PROCBASED_CTLS
     * MSR describes which bits can bet set to zero.
     *
     * If the IA32_VMX_TRUE_PROCBASED_CTLS MSR is not emulated these bits should be reported
     * as 1 by the IA32_VMX_PROCBASED_CTLS.
     */
    DEFAULT_ONE = 0x401E172,

    INTERRUPT_WINDOW_EXITING = 1ul << 2,
    TSC_OFFSETTING = 1ul << 3,
    HLT_EXITING = 1ul << 7,
    INVLPG_EXITING = 1ul << 9,
    MWAIT_EXITING = 1ul << 10,
    RDPMC_EXITING = 1ul << 11,
    RDTSC_EXITING = 1ul << 12,

    CR3_LOAD_EXITING = 1ul << 15,
    CR3_STORE_EXITING = 1ul << 16,
    CR8_LOAD_EXITING = 1ul << 19,
    CR8_STORE_EXITING = 1ul << 20,
    USE_TPR_SHADOW = 1ul << 21,
    NMI_WINDOW_EXITING = 1ul << 22,
    MOV_DR_EXITING = 1ul << 23,
    UNCONDITIONAL_IO_EXITING = 1ul << 24,
    USE_IO_BITMAPS = 1ul << 25,
    MONITOR_TRAP_FLAG = 1ul << 27,
    USE_MSR_BITMAPS = 1ul << 28,
    MONITOR_EXITING = 1ul << 29,
    PAUSE_EXITING = 1ul << 30,
    ACTIVATE_SEC_EXEC_CTRLS = 1ul << 31,
};

/**
 * Definitions related to the IA32_VMX_PROCBASED_CTLS2 MSR.
 * See Intel SDM Vol 3 Chapter 24.6.2 Table 24-7
 */
enum class vmx_sec_exc_ctls_constants : uint32_t
{
    VIRTUALIZE_APIC_ACCESS = 1ul << 0,
    ENABLE_EPT = 1ul << 1,
    DESCRIPTOR_TABLE_EXITING = 1ul << 2,
    ENABLE_RDTSCP = 1ul << 3,
    VIRTUALIZE_X2APIC_MODE = 1ul << 4,
    ENABLE_VPID = 1ul << 5,
    WBINVD_EXITING = 1ul << 6,
    UNRESTRICTED_GUEST = 1ul << 7,
    APIC_REGISTER_VIRTUALIZATION = 1ul << 8,
    VIRTUAL_INTERRUPT_DELIVERY = 1ul << 9,
    PAUSE_LOOP_EXITING = 1ul << 10,
    RDRAND_EXITING = 1ul << 11,
    ENABLE_INVPCID = 1ul << 12,
    ENABLE_VM_FUNCTIONS = 1ul << 13,
    VMCS_SHADOWING = 1ul << 14,
    ENABLE_ENCLS_EXITING = 1ul << 15,
    RDSEED_EXITING = 1ul << 16,
    ENABLE_PML = 1ul << 17,
    EPT_VIOLATION_CAUSES_VE = 1ul << 18,
    CONCEAL_VMX_NON_ROOT_FROM_PT = 1ul << 19,
    ENABLE_XSAVE_XRESTORE = 1ul << 20,
    MODE_BASED_EXECUTE_CTRL_FOR_EPT = 1ul << 22,
    SUBPAGE_WRITE_PERM_FOR_EPT = 1ul << 23,
    USE_TSC_SCALING = 1ul << 25,
    ENABLE_ENCLAVE_EXITING = 1ul << 28,
};

/**
 * Definitions related to the IA32_VMX_EXIT_CTLS MSR.
 * See Intel SDM Vol 3 Chapter 24.7.1
 */
enum class vmx_exit_ctrls_constants : uint32_t
{
    /**
     * On the first processors without IA32_VMX_TRUE_EXIT_CTLS MSR, these bits are always
     * reported as 1. If zero setting of these bits is supported, the IA32_VMX_TRUE_EXIT_CTLS
     * MSR describes which bits can bet set to zero.
     *
     * If the IA32_VMX_TRUE_EXIT_CTLS MSR is not emulated these bits should be reported
     * as 1 by the IA32_VMX_EXIT_CTLS.
     */
    DEFAULT_ONE = 0x36DFF,

    SAVE_DBG_CONTROLS = 1ul << 2,
    HOST_ADDR_SPACE_SIZE = 1ul << 9,
    LOAD_IA32_PERF_GLOBAL_CTRL = 1ul << 12,
    ACK_INTERRUPT_ON_EXIT = 1ul << 15,
    SAVE_IA32_PAT = 1ul << 18,
    LOAD_IA32_PAT = 1ul << 19,
    SAVE_IA32_EFER = 1ul << 20,
    LOAD_IA32_EFER = 1ul << 21,
    SAVE_VMX_PREEMPT_TIMER = 1ul << 22,
    CLEAR_IA32_BNDCFGS = 1ul << 23,
    CONCEAL_VM_EXIT_FROM_PT = 1ul << 24,
};

/**
 * Definitions related to the IA32_VMX_ENTRY_CTLS MSR.
 * See Intel SDM Vol 3 Chapter 24.8.1
 */
enum class vmx_entry_ctrls_constants : uint32_t
{
    /**
     * On the first processors without IA32_VMX_TRUE_ENTRY_CTLS MSR, these bits are always
     * reported as 1. If zero setting of these bits is supported, the IA32_VMX_TRUE_ENTRY_CTLS
     * MSR describes which bits can bet set to zero.
     *
     * If the IA32_VMX_TRUE_ENTRY_CTLS MSR is not emulated these bits should be reported
     * as 1 by the IA32_VMX_ENTRY_CTLS.
     */
    DEFAULT_ONE = 0x11FF,

    LOAD_DBG_CONTROLS = 1ul << 2,
    IA32_MODE_GUEST = 1ul << 9,
    ENTRY_TO_SMM = 1ul << 10,
    DEACT_DUAL_MON_TREATM = 1ul << 11,
    LOAD_IA32_PERF_GLOBAL_CTRL = 1ul << 13,
    LOAD_IA32_PAT = 1ul << 14,
    LOAD_IA32_EFER = 1ul << 15,
    LOAD_IA32_BNDCFGS = 1ul << 16,
    CONCEAL_VMX_FROM_PT = 1ul << 17,

};

/**
 * Definitions related to the IA32_VMX_MISC MSR.
 * See Intel SDM Vol 3 Appendix A.6
 */
enum class vmx_misc_constants : uint64_t
{
    RESERVED_ZERO = 0x80003E00,

    RATIO_PREEMPT_TIMER_TSC_MASK = math::mask(5),
    RATIO_PREEMPT_TIMER_TSC_SHIFT = 0,
    ACTIVITY_STATES_MASK = math::mask(3),
    ACTIVITY_STATES_SHIFT = 6,
    SUPPORTED_CR3_TARGET_VALUES_MASK = math::mask(9),
    SUPPORTED_CR3_TARGET_VALUES_SHIFT = 16,
    MAX_NUM_MSR_STORE_LIST_MASK = math::mask(3),
    MAX_NUM_MSR_STORE_LIST_SHIFT = 25,
    MSEG_REVISION_ID_MASK = math::mask(32),
    MSEG_REVISION_ID_SHIFT = 32,

    VM_EXIT_STORE_EFER_LMA_INTO_MODE_GUEST = 1ul << 5,
    INTEL_PROCESSOR_TRACE = 1ul << 14,
    RDMSR_IN_SMM_CAN_READ_SMBASE_MSR = 1ul << 15,
    IA32_SMM_MONITOR_CTL = 1ul << 28,
    VMWRITE_TO_VMCS_ALLOWED = 1ul << 29,
    VMENTRY_SFTWR_EXCEPTION_LENGTH_ZERO = 1ul << 30,
};

/**
 * Definitions related to the IA32_VMX_EPT_VPID_CAP MSR.
 * See Intel SDM Vol 3 Appendix A.10
 */
enum class vmx_ept_vpid_cap_constants : uint64_t
{
    RESERVED_ZERO = 0xFFFFF0FEF98CBEBE,

    EPT_EXECUTE_ONLY_TRANSLATION = 1ul << 0,
    PAGE_WALK_LENGTH_OF_4 = 1ul << 6,
    EPT_ALLOW_MT_UNCACHEABLE = 1ul << 8,
    EPT_ALLOW_MT_WRITE_BACK = 1ul << 14,
    EPT_ALLOW_PDE_2_MB = 1ul << 16,
    EPT_ALLOW_PDPTE_1_GB = 1ul << 17,
    EPT_ACCESSED_DIRTY_FLAGS = 1ul << 21,
    REPORT_ADVANCED_VMEXIT_INFO_ON_EPT_VIO = 1ul << 22,

    SUPPORT_INVEPT = 1ul << 20,
    SUPPORT_INVEPT_SINGLE_CONTEXT = 1ul << 25,
    SUPPORT_INVEPT_ALL_CONTEXT = 1ul << 26,

    SUPPORT_INVVPID = 1ul << 32,
    SUPPORT_INVVPID_INDIVIDUAL_ADDRESS = 1ul << 40,
    SUPPORT_INVVPID_SINGLE_CONTEXT = 1ul << 41,
    SUPPORT_INVVPID_ALL_CONTEXT = 1ul << 42,
    SUPPORT_INVVPID_SINGLE_CONTEXT_RETAINING_GLOBALS = 1ul << 43,
};

static constexpr unsigned NUM_PDPTE {4};
static constexpr unsigned PDPTE_SIZE {sizeof(uint64_t)};

static constexpr uint64_t ALLOWED_ZERO_AREA_SIZE {32};
static constexpr uint32_t FXSAVE_AREA_SIZE {512};
static constexpr uint32_t XSAVE_HEADER_SIZE {64};

enum vmcs_encoding : uint32_t
{
    /* 16-bit control fields */
    VPID = 0x00000000,
    POSTED_INTERRUPT_NOTI_VEC = 0x00000002,
    EPTP_INDEX = 0x00000004,

    /* 16-bit guest state fields */
    GUEST_SEL_ES = 0x00000800,
    GUEST_SEL_CS = 0x00000802,
    GUEST_SEL_SS = 0x00000804,
    GUEST_SEL_DS = 0x00000806,
    GUEST_SEL_FS = 0x00000808,
    GUEST_SEL_GS = 0x0000080a,
    GUEST_SEL_LDTR = 0x0000080c,
    GUEST_SEL_TR = 0x0000080e,
    GUEST_INTERRUPT_STS = 0x00000810,
    PML_INDEX = 0x00000812,

    /* 16-bit host state fields */
    HOST_SEL_ES = 0x00000c00,
    HOST_SEL_CS = 0x00000c02,
    HOST_SEL_SS = 0x00000c04,
    HOST_SEL_DS = 0x00000c06,
    HOST_SEL_FS = 0x00000c08,
    HOST_SEL_GS = 0x00000c0a,
    HOST_SEL_TR = 0x00000c0c,

    /* 64-bit control fields */
    IO_BITMAP_A = 0x00002000,
    IO_BITMAP_B = 0x00002002,
    MSR_BITMAP_A = 0x00002004,
    EXI_MSR_STORE_ADDR = 0x00002006,
    EXI_MSR_LOAD_ADDR = 0x00002008,
    ENT_MSR_LOAD_ADDR = 0x0000200a,
    EXEC_VMCS_PTR = 0x0000200c,
    PML_ADDR = 0x0000200e,
    TSC_OFFSET = 0x00002010,
    VAPIC_ADDR = 0x00002012,
    APIC_ACCESS_ADDR = 0x00002014,
    POSTED_INTERRUPT_DESC = 0x00002016,
    VM_FUNC_CTRL = 0x00002018,
    EPT_PTR = 0x0000201a,
    EOI_EXI_BITMAP_0 = 0x0000201c,
    EOI_EXI_BITMAP_1 = 0x0000201e,
    EOI_EXI_BITMAP_2 = 0x00002020,
    EOI_EXI_BITMAP_3 = 0x00002022,
    EPTP_LIST_ADDR = 0x00002024,
    VMREAD_BITMAP_ADDR = 0x00002026,
    WMWRITE_BITMAP_ADDR = 0x00002028,
    VIRT_EXC_INFO_ADDR = 0x0000202a,
    XSS_EXI_BITMAP = 0x0000202c,
    ENCLS_EXI_BITMAP = 0x0000202e,
    TSC_MULTIPLIER = 0x00002032,

    /* 64-bit read only data fields */
    GUEST_PHYS_ADDR = 0x00002400,

    /* 64-bit guest state fields */
    VMCS_LINK_PTR = 0x00002800,
    GUEST_IA32_DBG_CTRL = 0x00002802,
    GUEST_IA32_PAT = 0x00002804,
    GUEST_IA32_EFER = 0x00002806,
    GUEST_IA32_PERF_GLB_CTRL = 0x00002808,
    GUEST_PDPTE0 = 0x0000280a,
    GUEST_PDPTE1 = 0x0000280c,
    GUEST_PDPTE2 = 0x0000280e,
    GUEST_PDPTE3 = 0x00002810,
    GUEST_IA32_BNFCFGS = 0x00002812,

    /* 64-bit host state fields */
    HOST_IA32_PAT = 0x00002c00,
    HOST_IA32_EFER = 0x00002c02,
    HOST_IA32_PERF_GLB_CTRL = 0x00002c04,

    /* 32-bit control fields */
    PIN_BASED_EXEC_CTRL = 0x00004000,
    PRIMARY_EXEC_CTRL = 0x00004002,
    EXC_BITMAP = 0x00004004,
    PF_ERR_CODE_MASK = 0x00004006,
    PF_ERR_CODE_MATCH = 0x00004008,
    CR3_TARGET_COUNT = 0x0000400a,
    VM_EXI_CTRL = 0x0000400c,
    VM_EXI_MSR_STORE_CNT = 0x0000400e,
    VM_EXI_MSR_LOAD_CNT = 0x00004010,
    VM_ENT_CTRL = 0x00004012,
    VM_ENT_MSR_LOAD_CNT = 0x00004014,
    VM_ENT_INT_INFO = 0x00004016,
    VM_ENT_EXC_ERR_CODE = 0x00004018,
    VM_ENT_INS_LEN = 0x0000401a,
    TPR_THRESHOLD = 0x0000401c,
    SECONDARY_EXEC_CTRL = 0x0000401e,
    PLE_GAP = 0x00004020,
    PLE_WIN = 0x00004022,

    /* 32-bit read only fields */
    VM_INS_ERR = 0x00004400,
    EXI_REASON = 0x00004402,
    VM_EXI_INT_INFO = 0x00004404,
    VM_EXI_INT_ERR = 0x00004406,
    IDT_VEC_INFO = 0x00004408,
    IDT_VEC_ERR = 0x0000440a,
    VM_EXI_INS_LEN = 0x0000440c,
    VM_EXI_INS_INFO = 0x0000440e,

    /* 32-bit guest state fields */
    GUEST_LIMIT_ES = 0x00004800,
    GUEST_LIMIT_CS = 0x00004802,
    GUEST_LIMIT_SS = 0x00004804,
    GUEST_LIMIT_DS = 0x00004806,
    GUEST_LIMIT_FS = 0x00004808,
    GUEST_LIMIT_GS = 0x0000480a,
    GUEST_LIMIT_LDTR = 0x0000480c,
    GUEST_LIMIT_TR = 0x0000480e,
    GUEST_LIMIT_GDTR = 0x00004810,
    GUEST_LIMIT_IDTR = 0x00004812,
    GUEST_AR_ES = 0x00004814,
    GUEST_AR_CS = 0x00004816,
    GUEST_AR_SS = 0x00004818,
    GUEST_AR_DS = 0x0000481a,
    GUEST_AR_FS = 0x0000481c,
    GUEST_AR_GS = 0x0000481e,
    GUEST_AR_LDTR = 0x00004820,
    GUEST_AR_TR = 0x00004822,
    GUEST_INT_STATE = 0x00004824,
    GUEST_ACT_STATE = 0x00004826,
    GUEST_SMBASE = 0x00004828,
    GUEST_IA32_SYSENTER_CS = 0x0000482a,
    VMX_PREEPMTION_TIMER = 0x0000482e,

    /* 32-bit host state fields */
    HOST_IA32_SYSENTER_CS = 0x00004c00,

    /* natural-width control fields */
    CR0_GUEST_HOST_MASK = 0x00006000,
    CR4_GUEST_HOST_MASK = 0x00006002,
    CR0_READ_SHADOW = 0x00006004,
    CR4_READ_SHADOW = 0x00006006,
    CR3_TARGET_VAL0 = 0x00006008,
    CR3_TARGET_VAL1 = 0x0000600a,
    CR3_TARGET_VAL2 = 0x0000600c,
    CR3_TARGET_VAL3 = 0x0000600e,

    /* natural-width read only data fields */
    EXI_QUAL = 0x00006400,
    IO_RCX = 0x00006402,
    IO_RSI = 0x00006404,
    IO_RDI = 0x00006406,
    IO_RIP = 0x00006408,
    GUEST_LIN_ADDR = 0x0000640a,

    /* natural-width guest state fields */
    GUEST_CR0 = 0x00006800,
    GUEST_CR3 = 0x00006802,
    GUEST_CR4 = 0x00006804,
    GUEST_BASE_ES = 0x00006806,
    GUEST_BASE_CS = 0x00006808,
    GUEST_BASE_SS = 0x0000680a,
    GUEST_BASE_DS = 0x0000680c,
    GUEST_BASE_FS = 0x0000680e,
    GUEST_BASE_GS = 0x00006810,
    GUEST_BASE_LDTR = 0x00006812,
    GUEST_BASE_TR = 0x00006814,
    GUEST_BASE_GDTR = 0x00006816,
    GUEST_BASE_IDTR = 0x00006818,
    GUEST_DR7 = 0x0000681a,
    GUEST_RSP = 0x0000681c,
    GUEST_RIP = 0x0000681e,
    GUEST_RFLAGS = 0x00006820,
    GUEST_PENDING_DBG_EXC = 0x00006822,
    GUEST_IA32_SYSENTER_ESP = 0x00006824,
    GUEST_IA32_SYSENTER_EIP = 0x00006826,

    /* natural-width host state fields */
    HOST_CR0 = 0x00006c00,
    HOST_CR3 = 0x00006c02,
    HOST_CR4 = 0x00006c04,
    HOST_BASE_FS = 0x00006c06,
    HOST_BASE_GS = 0x00006c08,
    HOST_BASE_TR = 0x00006c0a,
    HOST_BASE_GDTR = 0x00006c0c,
    HOST_BASE_IDTR = 0x00006c0e,
    HOST_IA32_SYSENTER_ESP = 0x00006c10,
    HOST_IA32_SYSENTER_EIP = 0x00006c12,
    HOST_RSP = 0x00006c14,
    HOST_RIP = 0x00006c16,
};

enum class vmx_exit_reason : uint32_t
{
    EXCEPTION = 0,
    INTR,
    TRIPLE_FAULT,
    INIT,
    SIPI,
    SMI_IO,
    SMI_OTHER,
    INT_WIN,
    NMI_WIN,
    TASK_SWITCH,
    CPUID,
    GETSEC,
    HLT,
    INVD,
    INVLPG,
    RDPMC,
    RDTSC,
    RSM,
    VMCALL,
    VMCLEAR,
    VMLAUNCH,
    VMPTRLD,
    VMPTRST,
    VMREAD,
    VMRESUME,
    VMWRITE,
    VMXOFF,
    VMXON,
    CR_ACCESS,
    DR_ACCESS,
    IO_ACCESS,
    RDMSR,
    WRMSR,
    INVALID_GUEST_STATE,
    MSR_LOAD_FAIL,
    RESERVED1,
    MWAIT,
    MTF,
    RESERVED2,
    MONITOR,
    PAUSE,
    MCA,
    RESERVED3,
    TPR_THRESHOLD,
    APIC_ACCESS,
    VEOI,
    GDTR_IDTR,
    LDTR_TR,
    EPT_VIOLATION,
    EPT_MISCONF,
    INVEPT,
    RDTSCP,
    PREEMPTION_TIMER,
    INVVPID,
    WBINVD,
    XSETBV,
    APIC_WRITE,
    RDRAND,
    INVPCID,
    VMFUNC,
    ENCLS,
    RDSEED,
    PAGE_MOD_LOG_FULL,
    XSAVES,
    XRSTORS,
    NUM_EXITS,

    // Hedron reports this exit reason if a call to vcpu_ctrl_run returns due to a vcpu_ctrl_poke.
    POKED = 255,
};

enum class cpu_mode : uint32_t
{
    RM16,
    CM16,
    PM16,

    RM32,
    PM32,
    CM32,

    PM64,
};

inline cpu_mode determine_cpu_mode(gdt_segment cs, uint64_t cr0, uint64_t efer, [[maybe_unused]] uint64_t rflags)
{
    using ar_mask = gdt_segment::ar_flag_mask;

    // Are we in IA32 mode?
    if (efer & EFER_LMA) {
        if (cs.ar_set(ar_mask::IA32_CODE_LONG)) {
            return cpu_mode::PM64;
        } else if (cs.ar_set(ar_mask::CODE_DEFAULT)) {
            return cpu_mode::CM32;
        } else {
            return cpu_mode::CM16;
        }
    }

    // Are we in protected mode?
    if (cr0 & math::mask_from(cr0::PE)) {
        assert(not(rflags & FLAGS_VM)); // VM8086 mode not supported
        return cs.ar_set(ar_mask::CODE_DEFAULT) ? cpu_mode::PM32 : cpu_mode::PM16;
    }

    // So Real mode then. Very well.
    return cs.ar_set(ar_mask::CODE_DEFAULT) ? x86::cpu_mode::RM32 : cpu_mode::RM16;
}

inline unsigned determine_cpl(uint64_t ss_sel)
{
    return ss_sel & 0x3;
}

struct cpu_info
{
    size_t family, model, stepping;

    bool operator==(const cpu_info& o) const
    {
        return family == o.family and model == o.model and stepping == o.stepping;
    }
};

} // namespace x86
