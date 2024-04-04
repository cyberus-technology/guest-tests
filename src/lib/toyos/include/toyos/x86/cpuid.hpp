/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

enum
{
    CPUID_LEAF_MAX_LEVEL_VENDOR_ID = 0x00000000,
    CPUID_LEAF_FAMILY_FEATURES = 0x00000001,
    CPUID_LEAF_POWER_MANAGEMENT = 0x00000006,
    CPUID_LEAF_EXTENDED_FEATURES = 0x00000007,
    CPUID_LEAF_EXTENDED_STATE = 0x0000000D,
    CPUID_LEAF_SGX_CAPABILITY = 0x00000012,
    /**
     * Base leaf for the extended CPU brand string. The full name is in this
     * leaf and the two subsequent leaves.
     */
    CPUID_LEAF_EXTENDED_BRAND_STRING_BASE = 0x80000002,

    CPUID_EXTENDED_STATE_MAIN = 0,
    CPUID_EXTENDED_STATE_SUB = 1,

    /* How to read these constants:
     *                the feature described by this constant
     *                    |
     *                 vvvvvvv
     * LVL_xxx_xxx_reg_feature
     *     ^^^^^^^ ^^^
     *        |     |
     *        |     \------\
     *    value in eax     |
     *    when executing   |
     *    cpuid            |
     *                  register in which
     *                  the feature is announced
     */

    LVL_0000_0001_ECX_SSE3 = 1u << 0,
    LVL_0000_0001_ECX_PCLMUL = 1u << 1,
    LVL_0000_0001_ECX_DTES64 = 1u << 2,
    LVL_0000_0001_ECX_MON = 1u << 3,
    LVL_0000_0001_ECX_DSCPL = 1u << 4,
    LVL_0000_0001_ECX_VMX = 1u << 5,
    LVL_0000_0001_ECX_SMX = 1u << 6,
    LVL_0000_0001_ECX_EST = 1u << 7,
    LVL_0000_0001_ECX_TM2 = 1u << 8,
    LVL_0000_0001_ECX_SSSE3 = 1u << 9,
    LVL_0000_0001_ECX_CID = 1u << 10,
    LVL_0000_0001_ECX_SDBG = 1u << 11,
    LVL_0000_0001_ECX_FMA = 1u << 12,
    LVL_0000_0001_ECX_CX16 = 1u << 13,
    LVL_0000_0001_ECX_ETPRD = 1u << 14,
    LVL_0000_0001_ECX_PDCM = 1u << 15,
    LVL_0000_0001_ECX_PCID = 1u << 17,
    LVL_0000_0001_ECX_DCA = 1u << 18,
    LVL_0000_0001_ECX_SSE41 = 1u << 19,
    LVL_0000_0001_ECX_SSE42 = 1u << 20,
    LVL_0000_0001_ECX_X2APIC = 1u << 21,
    LVL_0000_0001_ECX_MOVBE = 1u << 22,
    LVL_0000_0001_ECX_POPCNT = 1u << 23,
    LVL_0000_0001_ECX_TSCD = 1u << 24,
    LVL_0000_0001_ECX_AES = 1u << 25,
    LVL_0000_0001_ECX_XSAVE = 1u << 26,
    LVL_0000_0001_ECX_OSXSAVE = 1u << 27,
    LVL_0000_0001_ECX_AVX = 1u << 28,
    LVL_0000_0001_ECX_F16C = 1u << 29,
    LVL_0000_0001_ECX_RDRAND = 1u << 30,
    LVL_0000_0001_ECX_HV = 1u << 31,

    LVL_0000_0001_EDX_FPU = 1u << 0,
    LVL_0000_0001_EDX_VME = 1u << 1,
    LVL_0000_0001_EDX_DE = 1u << 2,
    LVL_0000_0001_EDX_PSE = 1u << 3,
    LVL_0000_0001_EDX_TSC = 1u << 4,
    LVL_0000_0001_EDX_MSR = 1u << 5,
    LVL_0000_0001_EDX_PAE = 1u << 6,
    LVL_0000_0001_EDX_MCE = 1u << 7,
    LVL_0000_0001_EDX_CX8 = 1u << 8,
    LVL_0000_0001_EDX_APIC = 1u << 9,
    LVL_0000_0001_EDX_SEP = 1u << 11,
    LVL_0000_0001_EDX_MTRR = 1u << 12,
    LVL_0000_0001_EDX_PGE = 1u << 13,
    LVL_0000_0001_EDX_MCA = 1u << 14,
    LVL_0000_0001_EDX_CMOV = 1u << 15,
    LVL_0000_0001_EDX_PAT = 1u << 16,
    LVL_0000_0001_EDX_PSE36 = 1u << 17,
    LVL_0000_0001_EDX_PSN = 1u << 18,
    LVL_0000_0001_EDX_CLFL = 1u << 19,
    LVL_0000_0001_EDX_DTES = 1u << 21,
    LVL_0000_0001_EDX_ACPI = 1u << 22,
    LVL_0000_0001_EDX_MMX = 1u << 23,
    LVL_0000_0001_EDX_FXSR = 1u << 24,
    LVL_0000_0001_EDX_SSE = 1u << 25,
    LVL_0000_0001_EDX_SSE2 = 1u << 26,
    LVL_0000_0001_EDX_SS = 1u << 27,
    LVL_0000_0001_EDX_HTT = 1u << 28,
    LVL_0000_0001_EDX_TM1 = 1u << 29,
    LVL_0000_0001_EDX_IA64 = 1u << 30,
    LVL_0000_0001_EDX_PBE = 1u << 31,

    LVL_0000_0006_EAX_HW_FEEDBACK = 1u << 19,

    LVL_0000_0007_EBX_FSGSBASE = 1u << 0,
    LVL_0000_0007_EBX_TSCADJUST = 1u << 1,
    LVL_0000_0007_EBX_SGX = 1u << 2,
    LVL_0000_0007_EBX_BMI1 = 1u << 3,
    LVL_0000_0007_EBX_HLE = 1u << 4,
    LVL_0000_0007_EBX_AVX2 = 1u << 5,
    LVL_0000_0007_EBX_FPDP = 1u << 6,
    LVL_0000_0007_EBX_SMEP = 1u << 7,
    LVL_0000_0007_EBX_BMI2 = 1u << 8,
    LVL_0000_0007_EBX_ERMS = 1u << 9,
    LVL_0000_0007_EBX_INVPCID = 1u << 10,
    LVL_0000_0007_EBX_RTM = 1u << 11,
    LVL_0000_0007_EBX_PQM = 1u << 12,
    LVL_0000_0007_EBX_FPCSDS = 1u << 13,
    LVL_0000_0007_EBX_MPX = 1u << 14,
    LVL_0000_0007_EBX_RDTA = 1u << 15,  // Intel SDM Vol 3, Chapter 17.19, Intel Resource Director Allocation Technology
    LVL_0000_0007_EBX_AVX512F = 1u << 16,
    LVL_0000_0007_EBX_AVX512DQ = 1u << 17,
    LVL_0000_0007_EBX_RDSEED = 1u << 18,
    LVL_0000_0007_EBX_ADX = 1u << 19,
    LVL_0000_0007_EBX_SMAP = 1u << 20,
    LVL_0000_0007_EBX_AVX512IFMA = 1u << 21,
    LVL_0000_0007_EBX_PCOMMIT = 1u << 22,
    LVL_0000_0007_EBX_CLFLUSHOPT = 1u << 23,
    LVL_0000_0007_EBX_CLWB = 1u << 24,
    LVL_0000_0007_EBX_PT = 1u << 25,
    LVL_0000_0007_EBX_AVX512PF = 1u << 26,
    LVL_0000_0007_EBX_AVX512ER = 1u << 27,
    LVL_0000_0007_EBX_AVX512CD = 1u << 28,
    LVL_0000_0007_EBX_SHA = 1u << 29,
    LVL_0000_0007_EBX_AVX512BW = 1u << 30,
    LVL_0000_0007_EBX_AVX512VL = 1u << 31,

    LVL_0000_0007_ECX_PREFETCHWT1 = 1u << 0,
    LVL_0000_0007_ECX_AVX512VBMI = 1u << 1,
    LVL_0000_0007_ECX_UMIP = 1u << 2,
    LVL_0000_0007_ECX_PKU = 1u << 3,
    LVL_0000_0007_ECX_OSPKE = 1u << 4,
    LVL_0000_0007_ECX_CET = 1u << 7,
    LVL_0000_0007_ECX_GFNI = 1u << 8,         // Intel Galois Field New Instructions Technology Guide
    LVL_0000_0007_ECX_VAES = 1u << 9,         // Intel SDM Vol 1, 14.3.1 Detection of VEX-Encoded AES and VPCLMULQDQ
    LVL_0000_0007_ECX_VPCLMULQDQ = 1u << 10,  // Intel SDM Vol 1, 14.3.1 Detection of VEX-Encoded AES and VPCLMULQDQ
    LVL_0000_0007_ECX_AVX512VPDQ = 1u << 14,
    LVL_0000_0007_ECX_VA57 = 1u << 16,
    LVL_0000_0007_ECX_MAWAU = 0x1f << 17,
    LVL_0000_0007_ECX_RDPID = 1u << 22,
    LVL_0000_0007_ECX_MOVDIRI = 1u << 27,    // Intel SDM Vol 2, Chapter 4.3, MOVDIRI
    LVL_0000_0007_ECX_MOVDIRI64 = 1u << 28,  // Intel SDM VOl 2, Chapter 4.3, MOVDIRI64
    LVL_0000_0007_ECX_SGX_LC = 1u << 30,

    LVL_0000_0007_EDX_AVX512QVNNIW = 1u << 2,
    LVL_0000_0007_EDX_AVX512QFMA = 1u << 3,
    LVL_0000_0007_EDX_FSRM = 1u << 4,
    LVL_0000_0007_EDX_MD_CLEAR = 1u << 10,
    LVL_0000_0007_EDX_SERIALIZE = 1u << 14,
    LVL_0000_0007_EDX_IBRS_IBPB = 1u << 26,
    LVL_0000_0007_EDX_STIBP = 1u << 27,
    LVL_0000_0007_EDX_L1D_FLUSH = 1u << 28,
    LVL_0000_0007_EDX_ARCH_CAP = 1u << 29,
    LVL_0000_0007_SSBD = 1u << 31,

    LVL_0000_000D_EAX_XSAVEOPT = 1u << 0,
    LVL_0000_000D_EAX_XSAVEC = 1u << 1,
    LVL_0000_000D_EAX_XCR1 = 1u << 2,
    LVL_0000_000D_EAX_XSAVES = 1u << 3,

    LVL_8000_0001_ECX_AHF64 = 1u << 0,
    LVL_8000_0001_ECX_CMP = 1u << 1,
    LVL_8000_0001_ECX_SVM = 1u << 2,
    LVL_8000_0001_ECX_EAS = 1u << 3,
    LVL_8000_0001_ECX_CR8D = 1u << 4,
    LVL_8000_0001_ECX_LZCNT = 1u << 5,
    LVL_8000_0001_ECX_SSE4A = 1u << 6,
    LVL_8000_0001_ECX_MSSE = 1u << 7,
    LVL_8000_0001_ECX_3DNOWP = 1u << 8,  // Intel SDM Vol 2, Chapter 4.3, PREFETCH and PREFETCHW
    LVL_8000_0001_ECX_OSVW = 1u << 9,
    LVL_8000_0001_ECX_IBS = 1u << 10,
    LVL_8000_0001_ECX_XOP = 1u << 11,
    LVL_8000_0001_ECX_SKINIT = 1u << 12,
    LVL_8000_0001_ECX_WDT = 1u << 13,
    LVL_8000_0001_ECX_LWP = 1u << 15,
    LVL_8000_0001_ECX_FMA4 = 1u << 16,
    LVL_8000_0001_ECX_TCE = 1u << 17,
    LVL_8000_0001_ECX_NODEID = 1u << 19,
    LVL_8000_0001_ECX_TBM = 1u << 21,
    LVL_8000_0001_ECX_TOPX = 1u << 22,
    LVL_8000_0001_ECX_PCXCORE = 1u << 23,
    LVL_8000_0001_ECX_PCXNB = 1u << 24,
    LVL_8000_0001_ECX_DBX = 1u << 26,
    LVL_8000_0001_ECX_PERFTSC = 1u << 27,
    LVL_8000_0001_ECX_PCXL2I = 1u << 28,
    LVL_8000_0001_ECX_MONX = 1u << 29,

    LVL_8000_0001_EDX_FPU = 1u << 0,
    LVL_8000_0001_EDX_VME = 1u << 1,
    LVL_8000_0001_EDX_DE = 1u << 2,
    LVL_8000_0001_EDX_PSE = 1u << 3,
    LVL_8000_0001_EDX_TSC = 1u << 4,
    LVL_8000_0001_EDX_PAE = 1u << 5,
    LVL_8000_0001_EDX_MSR = 1u << 6,
    LVL_8000_0001_EDX_MCE = 1u << 7,
    LVL_8000_0001_EDX_CX8 = 1u << 8,
    LVL_8000_0001_EDX_APIC = 1u << 9,
    LVL_8000_0001_EDX_SEP = 1u << 11,
    LVL_8000_0001_EDX_MTRR = 1u << 12,
    LVL_8000_0001_EDX_PGE = 1u << 13,
    LVL_8000_0001_EDX_MCA = 1u << 14,
    LVL_8000_0001_EDX_CMOV = 1u << 15,
    LVL_8000_0001_EDX_PAT = 1u << 16,
    LVL_8000_0001_EDX_PSE36 = 1u << 17,
    LVL_8000_0001_EDX_MP = 1u << 19,
    LVL_8000_0001_EDX_NX = 1u << 20,
    LVL_8000_0001_EDX_MMXP = 1u << 22,
    LVL_8000_0001_EDX_MMX = 1u << 23,
    LVL_8000_0001_EDX_FXSR = 1u << 24,
    LVL_8000_0001_EDX_FFXSR = 1u << 25,
    LVL_8000_0001_EDX_PG1G = 1u << 26,
    LVL_8000_0001_EDX_TSCP = 1u << 27,
    LVL_8000_0001_EDX_LM = 1u << 29,
    LVL_8000_0001_EDX_3DNOWP = 1u << 30,
    LVL_8000_0001_EDX_3DNOW = 1u << 31,
};
