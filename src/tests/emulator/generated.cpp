/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <initializer_list>

#include <compiler.hpp>
#include <toyos/baretest/baretest.hpp>
#include <toyos/testhelper/debugport_interface.h>

using namespace x86;

struct oneop_result
{
    uint64_t op;
    bool operator==(const oneop_result& o) const
    {
        return op == o.op;
    }
};

static oneop_result test_inc(uint64_t op, bool emul)
{
    oneop_result ret{ op };
    asm volatile("cmp $0, %[emul];"
                 "je 2f;"
                 "outb %[dbgport];"
                 "2: inc %[op];"
                 : [op] "+r"(ret.op)
                 : [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE), [emul] "r"(emul));
    return ret;
}

struct twoop_result
{
    uint64_t src, dst;
    bool operator==(const twoop_result& o) const
    {
        return src == o.src and dst == o.dst;
    }
};

static twoop_result test_and(uint64_t src, uint64_t dst, bool emul)
{
    twoop_result ret{ src, dst };
    asm volatile("cmp $0, %[emul];"
                 "je 2f;"
                 "outb %[dbgport];"
                 "2: and %[src], %[dst];"
                 : [src] "+r"(ret.src), [dst] "+r"(ret.dst)
                 : [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE), [emul] "r"(emul));
    return ret;
}

template<int OFF>
static twoop_result test_or(uint64_t dst, bool emul)
{
    twoop_result ret{ static_cast<uint64_t>(OFF), dst };
    asm volatile("cmp $0, %[emul];"
                 "je 2f;"
                 "outb %[dbgport];"
                 "2: orq %[src], %[dst];"
                 : [dst] "+r"(ret.dst)
                 : [src] "i"(OFF), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE), [emul] "r"(emul));
    return ret;
}

TEST_CASE(manual_1op)
{
    uint64_t op{ 0xcafe };

    auto res_native = test_inc(op, false);
    auto res_emul = test_inc(op, true);

    BARETEST_VERIFY(res_native == res_emul);
}

TEST_CASE(manual_2op_and)
{
    uint64_t op1{ 0xcafe };
    uint64_t op2{ 0xbabe };

    auto res_native = test_and(op1, op2, false);
    auto res_emul = test_and(op1, op2, true);

    BARETEST_VERIFY(res_native == res_emul);
}

TEST_CASE(manual_2op_or)
{
    uint64_t op1{ 0xcafe };

    static constexpr int offset{ -2 };
    auto res_native = test_or<offset>(op1, false);
    auto res_emul = test_or<offset>(op1, true);

    BARETEST_VERIFY(res_native == res_emul);
}

TEST_CASE(manual_test_instruction)
{
    static constexpr const uint64_t FLAGS_MASK{ FLAGS_OF | FLAGS_CF | FLAGS_SF | FLAGS_ZF | FLAGS_PF };

    struct result
    {
        uint16_t op1_16, op2_16;
        uint64_t op1_64, op2_64;
        uint64_t flags_16{ 0 }, flags_64{ 0 };

        result(uint64_t v1, uint64_t v2)
            : op1_16(v1), op2_16(v2), op1_64(v1), op2_64(v2) {}

        bool operator==(const result& o) const
        {
            return op1_16 == o.op1_16 and op2_16 == o.op2_16 and op1_64 == o.op1_64 and op2_64 == o.op2_64
                   and (flags_16 & FLAGS_MASK) == (o.flags_16 & FLAGS_MASK)
                   and (flags_64 & FLAGS_MASK) == (o.flags_64 & FLAGS_MASK);
        }
    };
    auto values = { 0ul, 0xfful, 0xcafeul, 0xcafed00dul, 0xacafed00dul };
    for (const auto v1 : values) {
        for (const auto v2 : values) {
            result result_native(v1, v2), result_emul(v1, v2);
            asm volatile(
                "test %[op1_native_16], %[op2_native_16];"
                "pushf;"
                "pop %[flags_native_16];"
                "test %[op1_native_64], %[op2_native_64];"
                "pushf;"
                "pop %[flags_native_64];"
                "mov %[dbgport_val], %%rax;"
                "outb %[dbgport];"
                "test %[op1_emul_16], %[op2_emul_16];"
                "pushf;"
                "pop %[flags_emul_16];"
                "outb %[dbgport];"
                "test %[op1_emul_64], %[op2_emul_64];"
                "pushf;"
                "pop %[flags_emul_64];"
                : [op1_native_16] "+r"(result_native.op1_16), [op2_native_16] "+r"(result_native.op2_16), [op1_emul_16] "+r"(result_emul.op1_16), [op2_emul_16] "+r"(result_emul.op2_16), [op1_native_64] "+r"(result_native.op1_64), [op2_native_64] "+r"(result_native.op2_64), [op1_emul_64] "+r"(result_emul.op1_64), [op2_emul_64] "+r"(result_emul.op2_64), [flags_native_16] "=rm"(result_native.flags_16), [flags_native_64] "=rm"(result_native.flags_64), [flags_emul_16] "=rm"(result_emul.flags_16), [flags_emul_64] "=rm"(result_emul.flags_64)
                : [dbgport] "i"(DEBUG_PORTS.a), [dbgport_val] "i"(DEBUGPORT_EMUL_ONCE)
                : "rax");
            BARETEST_VERIFY(result_native == result_emul);
        }
    }
}
