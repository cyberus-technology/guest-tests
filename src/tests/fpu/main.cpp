/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/baretest/baretest.hpp>
#include <toyos/testhelper/idt.hpp>
#include <toyos/testhelper/irq_handler.hpp>
#include <toyos/testhelper/irqinfo.hpp>
#include <toyos/util/cast_helpers.hpp>
#include <toyos/util/cpuid.hpp>
#include <toyos/util/trace.hpp>
#include <toyos/x86/x86asm.hpp>
#include <toyos/x86/x86fpu.hpp>

using namespace x86;

static constexpr uint64_t TEST_VAL{ 0x42 };
static constexpr xmm_t TEST_VAL_128{ 0x23, 0x42 };
static constexpr ymm_t TEST_VAL_256{ 0x23, 0x42, 0x2342, 0x23424223 };
static constexpr zmm_t TEST_VAL_512{ 0x23, 0x42, 0x2342, 0x23424223, 0x4223, 0x1337, 0xc4f3, 0xc0ff33 };

static constexpr uint64_t DESTROY_VAL{ ~0ull };
static constexpr xmm_t DESTROY_VAL_128{ DESTROY_VAL, DESTROY_VAL };
static constexpr ymm_t DESTROY_VAL_256{ DESTROY_VAL, DESTROY_VAL, DESTROY_VAL, DESTROY_VAL };
static constexpr zmm_t DESTROY_VAL_512{ DESTROY_VAL, DESTROY_VAL, DESTROY_VAL, DESTROY_VAL, DESTROY_VAL, DESTROY_VAL, DESTROY_VAL, DESTROY_VAL };

#define CHECK_FEATURE(features, f) info(#f ": %u", !!(features & f));

TEST_CASE(vector_support)
{
    auto features1{ cpuid(CPUID_LEAF_FAMILY_FEATURES) };

    CHECK_FEATURE(features1.ecx, LVL_0000_0001_ECX_SSE3);
    CHECK_FEATURE(features1.ecx, LVL_0000_0001_ECX_SSSE3);
    CHECK_FEATURE(features1.ecx, LVL_0000_0001_ECX_FMA);
    CHECK_FEATURE(features1.ecx, LVL_0000_0001_ECX_SSE41);
    CHECK_FEATURE(features1.ecx, LVL_0000_0001_ECX_SSE42);
    CHECK_FEATURE(features1.ecx, LVL_0000_0001_ECX_XSAVE);
    CHECK_FEATURE(features1.ecx, LVL_0000_0001_ECX_OSXSAVE);
    CHECK_FEATURE(features1.ecx, LVL_0000_0001_ECX_AVX);
    CHECK_FEATURE(features1.ecx, LVL_0000_0001_ECX_F16C);

    info("------");

    CHECK_FEATURE(features1.edx, LVL_0000_0001_EDX_MMX);
    CHECK_FEATURE(features1.edx, LVL_0000_0001_EDX_FXSR);
    CHECK_FEATURE(features1.edx, LVL_0000_0001_EDX_SSE);
    CHECK_FEATURE(features1.edx, LVL_0000_0001_EDX_SSE2);

    auto features7{ cpuid(CPUID_LEAF_EXTENDED_FEATURES) };

    info("------");

    CHECK_FEATURE(features7.ebx, LVL_0000_0007_EBX_AVX2);
    CHECK_FEATURE(features7.ebx, LVL_0000_0007_EBX_AVX512F);
    CHECK_FEATURE(features7.ebx, LVL_0000_0007_EBX_AVX512DQ);
    CHECK_FEATURE(features7.ebx, LVL_0000_0007_EBX_AVX512IFMA);
    CHECK_FEATURE(features7.ebx, LVL_0000_0007_EBX_AVX512PF);
    CHECK_FEATURE(features7.ebx, LVL_0000_0007_EBX_AVX512ER);
    CHECK_FEATURE(features7.ebx, LVL_0000_0007_EBX_AVX512CD);
    CHECK_FEATURE(features7.ebx, LVL_0000_0007_EBX_AVX512BW);
    CHECK_FEATURE(features7.ebx, LVL_0000_0007_EBX_AVX512VL);

    CHECK_FEATURE(features7.ecx, LVL_0000_0007_ECX_AVX512VBMI);
    CHECK_FEATURE(features7.ecx, LVL_0000_0007_ECX_AVX512VPDQ);

    CHECK_FEATURE(features7.edx, LVL_0000_0007_EDX_AVX512QVNNIW);
    CHECK_FEATURE(features7.edx, LVL_0000_0007_EDX_AVX512QFMA);
}

static uint64_t get_supported_xstate()
{
    auto res{ cpuid(CPUID_LEAF_EXTENDED_STATE, CPUID_EXTENDED_STATE_MAIN) };

    return static_cast<uint64_t>(res.edx) << 32 | res.eax;
}

static void print_cpuid(uint32_t leaf, uint32_t subleaf)
{
    const auto res{ cpuid(leaf, subleaf) };

    info("{#08x} {#08x}: eax={#08x} ebx={#08x} ecx={#08x} edx={#08x}", leaf, subleaf, res.eax, res.ebx, res.ecx, res.edx);
}

TEST_CASE_CONDITIONAL(xstate_features, xsave_supported())
{
    uint64_t supported_xstate{ get_supported_xstate() };

    print_cpuid(CPUID_LEAF_EXTENDED_STATE, CPUID_EXTENDED_STATE_MAIN);
    print_cpuid(CPUID_LEAF_EXTENDED_STATE, CPUID_EXTENDED_STATE_SUB);

    info("xstate {x}", supported_xstate);
    for (unsigned bit = 2; bit <= 62; bit++) {
        if (supported_xstate & (static_cast<uint64_t>(1) << bit)) {
            print_cpuid(CPUID_LEAF_EXTENDED_STATE, bit);
        }
    }
}

static uint64_t xsave_mask{ 0 };

alignas(CPU_CACHE_LINE_SIZE) static uint8_t xsave_area[PAGE_SIZE];

void prologue()
{
    // Everything from MMX onwards is only supported if FPU emulation is disabled.
    set_cr0(get_cr0() & ~math::mask_from(cr0::EM));

    if (xsave_supported()) {
        xsave_mask = get_supported_xstate() & XCR0_MASK;

        set_cr4(get_cr4() | math::mask_from(cr4::OSXSAVE, cr4::OSFXSR));
        set_xcr(xsave_mask);

        auto xsave_size{ cpuid(CPUID_LEAF_EXTENDED_STATE, CPUID_EXTENDED_STATE_MAIN).ecx };
        if (xsave_size > sizeof(xsave_area)) {
            baretest::hello(1);
            baretest::fail("Size of XSAVE area greater than allocated space!\n");
            baretest::goodbye();
            disable_interrupts_and_halt();
        }
    }

    asm volatile("fninit");
}

static irqinfo irq_info;
static jmp_buf jump_buffer;

void irq_handler_fn(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
    longjmp(jump_buffer, 1);
}

TEST_CASE(fxsave_fxrstor_default)
{
    set_mm0(TEST_VAL);

    fxsave(xsave_area);

    set_mm0(DESTROY_VAL);

    fxrstor(xsave_area);

    BARETEST_ASSERT(get_mm0() == TEST_VAL);
}

TEST_CASE_CONDITIONAL(xstate_size_checks, xsave_supported())
{
    // This is the test as seen in the Linux kernel. We basically sum up all the
    // state components and compare it to the maximum size reported by
    // CPUID(0xD).ebx. See arch/x86/kernel/fpu/xstate.c

    auto xsave_size{ cpuid(CPUID_LEAF_EXTENDED_STATE).ebx };
    auto calculated_size{ x86::FXSAVE_AREA_SIZE + x86::XSAVE_HEADER_SIZE };

    for (unsigned feat{ 2 }; feat <= 62; feat++) {
        if (not(xsave_mask & (static_cast<uint64_t>(1) << feat))) {
            continue;
        }

        auto feature_info{ cpuid(CPUID_LEAF_EXTENDED_STATE, feat) };

        // Round the current size up to the offset of this feature.
        calculated_size = feature_info.ebx;

        calculated_size += feature_info.eax;
    }

    info("Reported: {#x} vs. calculated {#x}", xsave_size, calculated_size);
    BARETEST_ASSERT(calculated_size == xsave_size);
}

static bool check_xcr_exception(uint64_t val, uint32_t xcrN = 0)
{
    irq_handler::guard _(irq_handler_fn);
    irq_info.reset();

    if (setjmp(jump_buffer) == 0) {
        set_xcr(val, xcrN);
    }

    return irq_info.valid and irq_info.vec == to_underlying(exception::GP);
}

// We need AVX for this test, so we have one feature to turn off and see the
// state area size shrink.
TEST_CASE_CONDITIONAL(cpuid_reports_xstate_size, xsave_supported() and avx_supported())
{
    uint32_t reported_max_size{ cpuid(CPUID_LEAF_EXTENDED_STATE, CPUID_EXTENDED_STATE_MAIN).ecx };
    auto current_size = []() { return cpuid(CPUID_LEAF_EXTENDED_STATE, CPUID_EXTENDED_STATE_MAIN).ebx; };

    set_xcr(XCR0_FPU | XCR0_SSE);
    uint32_t fpu_size{ current_size() };

    set_xcr(xsave_mask);
    uint32_t all_enabled_size{ current_size() };

    BARETEST_ASSERT(fpu_size < all_enabled_size);
    BARETEST_ASSERT(all_enabled_size <= reported_max_size);
}

TEST_CASE_CONDITIONAL(setting_invalid_xcr0_causes_gp, xsave_supported())
{
    std::vector<uint64_t> values;

    // Clearing bit 0 (Legacy FPU state) is always invalid
    values.push_back(0);

    // If AVX is supported, clearing SSE while enabling AVX is invalid
    if (xsave_mask & XCR0_AVX) {
        values.push_back(XCR0_FPU | XCR0_AVX);
    }

    // If AVX-512 is supported, setting any of those bits while not setting AVX is invalid,
    // as is not setting all of the AVX-512 bits together
    if (xsave_mask & XCR0_OPMASK) {
        values.push_back(XCR0_FPU | XCR0_AVX);
        values.push_back(XCR0_FPU | XCR0_OPMASK | XCR0_ZMM_Hi256 | XCR0_Hi16_ZMM);
        values.push_back(XCR0_FPU | XCR0_OPMASK | XCR0_Hi16_ZMM);
        values.push_back(XCR0_FPU | XCR0_OPMASK | XCR0_ZMM_Hi256);
        values.push_back(XCR0_FPU | XCR0_ZMM_Hi256 | XCR0_Hi16_ZMM);
        values.push_back(XCR0_FPU | XCR0_AVX | XCR0_ZMM_Hi256 | XCR0_Hi16_ZMM);
        values.push_back(XCR0_FPU | XCR0_AVX | XCR0_OPMASK | XCR0_Hi16_ZMM);
        values.push_back(XCR0_FPU | XCR0_AVX | XCR0_OPMASK | XCR0_ZMM_Hi256);
    }

    for (const auto v : values) {
        info("Setting XCR0 to {x}", v);
        BARETEST_ASSERT(check_xcr_exception(v));
    }
}

TEST_CASE_CONDITIONAL(setting_valid_xcr0_works, xsave_supported())
{
    std::vector<uint64_t> values;

    values.push_back(XCR0_FPU);
    values.push_back(XCR0_FPU | XCR0_SSE);

    if (avx_supported()) {
        values.push_back(XCR0_FPU | XCR0_SSE | XCR0_AVX);
    }
    if (avx512_supported()) {
        values.push_back(XCR0_FPU | XCR0_SSE | XCR0_AVX | XCR0_AVX512);
    }

    for (const auto v : values) {
        set_xcr(v);
        BARETEST_ASSERT(get_xcr() == v);
    }

    set_xcr(xsave_mask);
}

TEST_CASE_CONDITIONAL(invalid_xcrN_causes_gp, xsave_supported())
{
    BARETEST_ASSERT(check_xcr_exception(XCR0_FPU, 1));
}

TEST_CASE_CONDITIONAL(xsave_xrstor_full, xsave_supported())
{
    set_mm0(TEST_VAL);
    set_xmm0(TEST_VAL_128);

    xsave(xsave_area, xsave_mask);

    set_mm0(DESTROY_VAL);
    set_xmm0(DESTROY_VAL_128);

    xrstor(xsave_area, xsave_mask);

    BARETEST_ASSERT(get_mm0() == TEST_VAL);
    BARETEST_ASSERT(get_xmm0() == TEST_VAL_128);
}

TEST_CASE_CONDITIONAL(xsave_xrstor_full_avx, avx_supported())
{
    set_ymm0(TEST_VAL_256);

    xsave(xsave_area, xsave_mask);

    set_ymm0(DESTROY_VAL_256);

    xrstor(xsave_area, xsave_mask);

    BARETEST_ASSERT(get_ymm0() == TEST_VAL_256);
}

TEST_CASE_CONDITIONAL(xsave_xrstor_full_avx512, avx512_supported())
{
    set_k0(TEST_VAL);
    set_zmm0(TEST_VAL_512);
    set_zmm23(TEST_VAL_512);

    xsave(xsave_area, xsave_mask);

    set_k0(DESTROY_VAL);
    set_zmm0(DESTROY_VAL_512);
    set_zmm23(DESTROY_VAL_512);

    xrstor(xsave_area, xsave_mask);

    BARETEST_ASSERT(get_k0() == TEST_VAL);
    BARETEST_ASSERT(get_zmm0() == TEST_VAL_512);
    BARETEST_ASSERT(get_zmm23() == TEST_VAL_512);
}

TEST_CASE_CONDITIONAL(xsavec, xsavec_supported())
{
    set_mm0(TEST_VAL);

    xsavec(xsave_area, xsave_mask);

    set_mm0(DESTROY_VAL);

    xrstor(xsave_area, xsave_mask);

    BARETEST_ASSERT(get_mm0() == TEST_VAL);
}

TEST_CASE_CONDITIONAL(xsaveopt, xsaveopt_supported())
{
    set_mm0(TEST_VAL);

    xsaveopt(xsave_area, xsave_mask);

    set_mm0(DESTROY_VAL);

    xrstor(xsave_area, xsave_mask);

    BARETEST_ASSERT(get_mm0() == TEST_VAL);
}

TEST_CASE_CONDITIONAL(xsaves, xsaves_supported())
{
    set_mm0(TEST_VAL);

    xsaves(xsave_area, xsave_mask);

    set_mm0(DESTROY_VAL);

    xrstors(xsave_area, xsave_mask);

    BARETEST_ASSERT(get_mm0() == TEST_VAL);
}

// AMD does not allow to automatically inject a #UD for XSAVES
// in case the VMM does not want to expose this feature.
TEST_CASE_CONDITIONAL(xsaves_raises_ud, not xsaves_supported() and not(util::cpuid::is_amd_cpu() and util::cpuid::hv_bit_present()))
{
    irq_handler::guard irq_guard{ irq_handler_fn };
    irq_info.reset();

    if (setjmp(jump_buffer) == 0) {
        xsaves(xsave_area, XCR0_FPU);
    }

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == to_underlying(exception::UD));
}

static bool fma_supported()
{
    return cpuid(CPUID_LEAF_FAMILY_FEATURES).ecx & LVL_0000_0001_ECX_FMA;
}

TEST_CASE_CONDITIONAL(fused_multiply_add, fma_supported() and xsave_supported())
{
    const uint64_t float_one{ 0x3f800000 };
    xmm_t a{}, b{}, c{ float_one };

    // Compute as double precision float: a := a*b + c
    asm("movdqu %[a], %%xmm0\n"
        "movdqu %[b], %%xmm1\n"
        "movdqu %[c], %%xmm2\n"
        "vfmadd132pd %%xmm1, %%xmm2, %%xmm0\n"
        "movdqu %%xmm0, %[a]\n"
        : [a] "+m"(a)
        : [b] "m"(b), [c] "m"(c));

    BARETEST_ASSERT(a[0] == float_one);
}

TEST_CASE_CONDITIONAL(cpuid_reflects_correct_osxsave_value, xsave_supported())
{
    set_cr4(get_cr4() & ~math::mask_from(cr4::OSXSAVE));
    BARETEST_ASSERT((cpuid(CPUID_LEAF_FAMILY_FEATURES).ecx & LVL_0000_0001_ECX_OSXSAVE) == 0);

    set_cr4(get_cr4() | math::mask_from(cr4::OSXSAVE));
    BARETEST_ASSERT((cpuid(CPUID_LEAF_FAMILY_FEATURES).ecx & LVL_0000_0001_ECX_OSXSAVE) != 0);
}

BARETEST_RUN;
