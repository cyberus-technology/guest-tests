/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

/**
 * This file defines everything needed for emulation of simple 2op instructions:
 *  - Instruction list
 *  - Translation from udis -> instruction
 *  - Assembly jumptable
 *  - extern declarations
 *  - switch statement to select jumptable entry
 *  - emulator functions
 *  - emulator case statements to select emulator function
 */

#ifndef EMULATOR_MACROS_DEFINED
#define EMULATOR_MACROS_DEFINED

#define INSN_LOCKABLE_1OP(mnemonic, suffix, op_type, op_reg)                                                           \
    .global lock_##mnemonic##_##op_type##, ##mnemonic##_##op_type##;                                                   \
    lock_##mnemonic##_##op_type## : lock;                                                                              \
    ##mnemonic##_##op_type## : mnemonic##suffix op_reg;                                                                \
    ret;

#define INSN_LOCKABLE_2OP(mnemonic, suffix, src_type, src_reg, dst_type, dst_reg)                                      \
    .global lock_##mnemonic##_##src_type##_##dst_type##, ##mnemonic##_##src_type##_##dst_type##;                       \
    lock_##mnemonic##_##src_type##_##dst_type## : lock;                                                                \
    ##mnemonic##_##src_type##_##dst_type##                                                                             \
        : mnemonic##suffix src_reg                                                                                     \
        , dst_reg;                                                                                                     \
    ret;

#define INSN_1OP(mnemonic, suffix, op_type, op_reg)                                                                    \
    .global##mnemonic##_##op_type##;                                                                                   \
    ##mnemonic##_##op_type## : mnemonic##suffix op_reg;                                                                \
    ret;

#define INSN_2OP(mnemonic, src_type, src_reg, dst_type, dst_reg)                                                       \
    .global mnemonic##_##src_type##_##dst_type##;                                                                      \
    ##mnemonic##_##src_type##_##dst_type##                                                                             \
        : mnemonic src_reg                                                                                             \
        , dst_reg;                                                                                                     \
    ret;

// Only allow flags that are relevant to conditional jumps: CF, SF, ZF, PF, OF
#define JCC_MASK 0x8C5

#define INSN_JCC(insn, mnemonic)                                                                                       \
    .global mnemonic##_helper;                                                                                         \
    ##mnemonic##_helper                                                                                                \
        : mov $1                                                                                                       \
        , % rax;                                                                                                       \
    and $JCC_MASK, % rdi;                                                                                              \
    push % rdi;                                                                                                        \
    popf;                                                                                                              \
    mnemonic 1f;                                                                                                       \
    xor % rax, % rax;                                                                                                  \
    1 : ret;

#define INSN_LOCKABLE_SET_1OP(insn, mnemonic)                                                                          \
    INSN_LOCKABLE_1OP(mnemonic, b, r8, % bl)                                                                           \
    INSN_LOCKABLE_1OP(mnemonic, w, r16, % bx)                                                                          \
    INSN_LOCKABLE_1OP(mnemonic, l, r32, % ebx)                                                                         \
    INSN_LOCKABLE_1OP(mnemonic, q, r64, % rbx)                                                                         \
    INSN_LOCKABLE_1OP(mnemonic, b, m8, (% rbx))                                                                        \
    INSN_LOCKABLE_1OP(mnemonic, w, m16, (% rbx))                                                                       \
    INSN_LOCKABLE_1OP(mnemonic, l, m32, (% rbx))                                                                       \
    INSN_LOCKABLE_1OP(mnemonic, q, m64, (% rbx))

#define INSN_LOCKABLE_SET_2OP(insn, mnemonic)                                                                          \
    INSN_LOCKABLE_2OP(mnemonic, , r8, % cl, r8, % bl)                                                                  \
    INSN_LOCKABLE_2OP(mnemonic, , r16, % cx, r16, % bx)                                                                \
    INSN_LOCKABLE_2OP(mnemonic, , r32, % ecx, r32, % ebx)                                                              \
    INSN_LOCKABLE_2OP(mnemonic, , r64, % rcx, r64, % rbx)                                                              \
    INSN_LOCKABLE_2OP(mnemonic, , r8, % cl, m8, (% rbx))                                                               \
    INSN_LOCKABLE_2OP(mnemonic, , r16, % cx, m16, (% rbx))                                                             \
    INSN_LOCKABLE_2OP(mnemonic, , r32, % ecx, m32, (% rbx))                                                            \
    INSN_LOCKABLE_2OP(mnemonic, , r64, % rcx, m64, (% rbx))                                                            \
    INSN_LOCKABLE_2OP(mnemonic, , m8, (% rcx), r8, % bl)                                                               \
    INSN_LOCKABLE_2OP(mnemonic, , m16, (% rcx), r16, % bx)                                                             \
    INSN_LOCKABLE_2OP(mnemonic, , m32, (% rcx), r32, % ebx)                                                            \
    INSN_LOCKABLE_2OP(mnemonic, , m64, (% rcx), r64, % rbx)

#define INSN_SHIFTS_2OP(insn, mnemonic)                                                                                \
    INSN_LOCKABLE_2OP(mnemonic, , r8, % cl, r8, % bl)                                                                  \
    INSN_LOCKABLE_2OP(mnemonic, , r8, % cl, r16, % bx)                                                                 \
    INSN_LOCKABLE_2OP(mnemonic, , r8, % cl, r32, % ebx)                                                                \
    INSN_LOCKABLE_2OP(mnemonic, , r8, % cl, r64, % rbx)                                                                \
    INSN_LOCKABLE_2OP(mnemonic, b, r8, % cl, m8, (% rbx))                                                              \
    INSN_LOCKABLE_2OP(mnemonic, w, r8, % cl, m16, (% rbx))                                                             \
    INSN_LOCKABLE_2OP(mnemonic, l, r8, % cl, m32, (% rbx))                                                             \
    INSN_LOCKABLE_2OP(mnemonic, q, r8, % cl, m64, (% rbx))

#define EXTERNS_SET_1OP(insn, mnemonic)                                                                                \
    EXTERN_C func_t lock_##mnemonic##_r8 asm("lock_" #mnemonic "_r8"), mnemonic##_r8 asm(#mnemonic "_r8");             \
    EXTERN_C func_t lock_##mnemonic##_m8 asm("lock_" #mnemonic "_m8"), mnemonic##_m8 asm(#mnemonic "_m8");             \
    EXTERN_C func_t lock_##mnemonic##_r16 asm("lock_" #mnemonic "_r16"), mnemonic##_r16 asm(#mnemonic "_r16");         \
    EXTERN_C func_t lock_##mnemonic##_m16 asm("lock_" #mnemonic "_m16"), mnemonic##_m16 asm(#mnemonic "_m16");         \
    EXTERN_C func_t lock_##mnemonic##_r32 asm("lock_" #mnemonic "_r32"), mnemonic##_r32 asm(#mnemonic "_r32");         \
    EXTERN_C func_t lock_##mnemonic##_m32 asm("lock_" #mnemonic "_m32"), mnemonic##_m32 asm(#mnemonic "_m32");         \
    EXTERN_C func_t lock_##mnemonic##_r64 asm("lock_" #mnemonic "_r64"), mnemonic##_r64 asm(#mnemonic "_r64");         \
    EXTERN_C func_t lock_##mnemonic##_m64 asm("lock_" #mnemonic "_m64"), mnemonic##_m64 asm(#mnemonic "_m64");

#define EXTERNS_SET_2OP(insn, mnemonic)                                                                                \
    EXTERN_C func_t lock_##mnemonic##_r8_r8 asm("lock_" #mnemonic "_r8_r8"), mnemonic##_r8_r8 asm(#mnemonic "_r8_r8"); \
    EXTERN_C func_t lock_##mnemonic##_r8_m8 asm("lock_" #mnemonic "_r8_m8"), mnemonic##_r8_m8 asm(#mnemonic "_r8_m8"); \
    EXTERN_C func_t lock_##mnemonic##_m8_r8 asm("lock_" #mnemonic "_m8_r8"), mnemonic##_m8_r8 asm(#mnemonic "_m8_r8"); \
    EXTERN_C func_t lock_##mnemonic##_r16_r16 asm("lock_" #mnemonic "_r16_r16"),                                       \
        mnemonic##_r16_r16 asm(#mnemonic "_r16_r16");                                                                  \
    EXTERN_C func_t lock_##mnemonic##_r16_m16 asm("lock_" #mnemonic "_r16_m16"),                                       \
        mnemonic##_r16_m16 asm(#mnemonic "_r16_m16");                                                                  \
    EXTERN_C func_t lock_##mnemonic##_m16_r16 asm("lock_" #mnemonic "_m16_r16"),                                       \
        mnemonic##_m16_r16 asm(#mnemonic "_m16_r16");                                                                  \
    EXTERN_C func_t lock_##mnemonic##_r32_r32 asm("lock_" #mnemonic "_r32_r32"),                                       \
        mnemonic##_r32_r32 asm(#mnemonic "_r32_r32");                                                                  \
    EXTERN_C func_t lock_##mnemonic##_r32_m32 asm("lock_" #mnemonic "_r32_m32"),                                       \
        mnemonic##_r32_m32 asm(#mnemonic "_r32_m32");                                                                  \
    EXTERN_C func_t lock_##mnemonic##_m32_r32 asm("lock_" #mnemonic "_m32_r32"),                                       \
        mnemonic##_m32_r32 asm(#mnemonic "_m32_r32");                                                                  \
    EXTERN_C func_t lock_##mnemonic##_r64_r64 asm("lock_" #mnemonic "_r64_r64"),                                       \
        mnemonic##_r64_r64 asm(#mnemonic "_r64_r64");                                                                  \
    EXTERN_C func_t lock_##mnemonic##_r64_m64 asm("lock_" #mnemonic "_r64_m64"),                                       \
        mnemonic##_r64_m64 asm(#mnemonic "_r64_m64");                                                                  \
    EXTERN_C func_t lock_##mnemonic##_m64_r64 asm("lock_" #mnemonic "_m64_r64"),                                       \
        mnemonic##_m64_r64 asm(#mnemonic "_m64_r64");

#define EXTERNS_SET_SHIFTS_2OP(insn, mnemonic)                                                                         \
    EXTERN_C func_t lock_##mnemonic##_r8_r8 asm("lock_" #mnemonic "_r8_r8"), mnemonic##_r8_r8 asm(#mnemonic "_r8_r8"); \
    EXTERN_C func_t lock_##mnemonic##_r8_r16 asm("lock_" #mnemonic "_r8_r16"),                                         \
        mnemonic##_r8_r16 asm(#mnemonic "_r8_r16");                                                                    \
    EXTERN_C func_t lock_##mnemonic##_r8_r32 asm("lock_" #mnemonic "_r8_r32"),                                         \
        mnemonic##_r8_r32 asm(#mnemonic "_r8_r32");                                                                    \
    EXTERN_C func_t lock_##mnemonic##_r8_r64 asm("lock_" #mnemonic "_r8_r64"),                                         \
        mnemonic##_r8_r64 asm(#mnemonic "_r8_r64");                                                                    \
    EXTERN_C func_t lock_##mnemonic##_r8_m8 asm("lock_" #mnemonic "_r8_m8"), mnemonic##_r8_m8 asm(#mnemonic "_r8_m8"); \
    EXTERN_C func_t lock_##mnemonic##_r8_m16 asm("lock_" #mnemonic "_r8_m16"),                                         \
        mnemonic##_r8_m16 asm(#mnemonic "_r8_m16");                                                                    \
    EXTERN_C func_t lock_##mnemonic##_r8_m32 asm("lock_" #mnemonic "_r8_m32"),                                         \
        mnemonic##_r8_m32 asm(#mnemonic "_r8_m32");                                                                    \
    EXTERN_C func_t lock_##mnemonic##_r8_m64 asm("lock_" #mnemonic "_r8_m64"),                                         \
        mnemonic##_r8_m64 asm(#mnemonic "_r8_m64");

#define EXTERNS_JCC(insn, mnemonic) EXTERN_C bool mnemonic##_helper(uint64_t guest_flags) asm(#mnemonic "_helper");

#define ASM_BLOCK_SWITCH_1OP(mnemonic)                                                                                 \
    if (op_type == optype::MEM) {                                                                                      \
        switch (width) {                                                                                               \
        case 8: return lock ? &lock_##mnemonic##_m8 : &mnemonic##_m8;                                                  \
        case 16: return lock ? &lock_##mnemonic##_m16 : &mnemonic##_m16;                                               \
        case 32: return lock ? &lock_##mnemonic##_m32 : &mnemonic##_m32;                                               \
        case 64: return lock ? &lock_##mnemonic##_m64 : &mnemonic##_m64;                                               \
        default: PANIC("Unsupported instruction width!");                                                              \
        };                                                                                                             \
    } else {                                                                                                           \
        switch (width) {                                                                                               \
        case 8: return lock ? &lock_##mnemonic##_r8 : &mnemonic##_r8;                                                  \
        case 16: return lock ? &lock_##mnemonic##_r16 : &mnemonic##_r16;                                               \
        case 32: return lock ? &lock_##mnemonic##_r32 : &mnemonic##_r32;                                               \
        case 64: return lock ? &lock_##mnemonic##_r64 : &mnemonic##_r64;                                               \
        default: PANIC("Unsupported instruction width!");                                                              \
        };                                                                                                             \
    }

#define ASM_BLOCK_SWITCH_2OP(mnemonic)                                                                                 \
    if (src_type == optype::MEM) {                                                                                     \
        switch (width) {                                                                                               \
        case 8: return lock ? &lock_##mnemonic##_m8_r8 : &mnemonic##_m8_r8;                                            \
        case 16: return lock ? &lock_##mnemonic##_m16_r16 : &mnemonic##_m16_r16;                                       \
        case 32: return lock ? &lock_##mnemonic##_m32_r32 : &mnemonic##_m32_r32;                                       \
        case 64: return lock ? &lock_##mnemonic##_m64_r64 : &mnemonic##_m64_r64;                                       \
        default: PANIC("Unsupported instruction width!");                                                              \
        }                                                                                                              \
    } else {                                                                                                           \
        if (dst_type == optype::MEM) {                                                                                 \
            switch (width) {                                                                                           \
            case 8: return lock ? &lock_##mnemonic##_r8_m8 : &mnemonic##_r8_m8;                                        \
            case 16: return lock ? &lock_##mnemonic##_r16_m16 : &mnemonic##_r16_m16;                                   \
            case 32: return lock ? &lock_##mnemonic##_r32_m32 : &mnemonic##_r32_m32;                                   \
            case 64: return lock ? &lock_##mnemonic##_r64_m64 : &mnemonic##_r64_m64;                                   \
            default: PANIC("Unsupported instruction width!");                                                          \
            }                                                                                                          \
        } else {                                                                                                       \
            switch (width) {                                                                                           \
            case 8: return lock ? &lock_##mnemonic##_r8_r8 : &mnemonic##_r8_r8;                                        \
            case 16: return lock ? &lock_##mnemonic##_r16_r16 : &mnemonic##_r16_r16;                                   \
            case 32: return lock ? &lock_##mnemonic##_r32_r32 : &mnemonic##_r32_r32;                                   \
            case 64: return lock ? &lock_##mnemonic##_r64_r64 : &mnemonic##_r64_r64;                                   \
            default: PANIC("Unsupported instruction width!");                                                          \
            }                                                                                                          \
        }                                                                                                              \
    }

#define ASM_BLOCK_SWITCH_SHIFTS_2OP(mnemonic)                                                                          \
    PANIC_UNLESS(src_type == optype::REG, "Invalid source operand type");                                              \
    if (dst_type == optype::MEM) {                                                                                     \
        switch (width) {                                                                                               \
        case 8: return lock ? &lock_##mnemonic##_r8_m8 : &mnemonic##_r8_m8;                                            \
        case 16: return lock ? &lock_##mnemonic##_r8_m16 : &mnemonic##_r8_m16;                                         \
        case 32: return lock ? &lock_##mnemonic##_r8_m32 : &mnemonic##_r8_m32;                                         \
        case 64: return lock ? &lock_##mnemonic##_r8_m64 : &mnemonic##_r8_m64;                                         \
        default: PANIC("Unsupported instruction width!");                                                              \
        }                                                                                                              \
    } else {                                                                                                           \
        switch (width) {                                                                                               \
        case 8: return lock ? &lock_##mnemonic##_r8_r8 : &mnemonic##_r8_r8;                                            \
        case 16: return lock ? &lock_##mnemonic##_r8_r16 : &mnemonic##_r8_r16;                                         \
        case 32: return lock ? &lock_##mnemonic##_r8_r32 : &mnemonic##_r8_r32;                                         \
        case 64: return lock ? &lock_##mnemonic##_r8_r64 : &mnemonic##_r8_r64;                                         \
        default: PANIC("Unsupported instruction width!");                                                              \
        }                                                                                                              \
    }

#define EMULATE_SIMPLE_0OP(insn, mnemonic)                                                                             \
    template <typename DECODER>                                                                                        \
    static bool emulate_##mnemonic(emulator_vcpu&, DECODER&)                                                           \
    {                                                                                                                  \
        asm volatile(#mnemonic);                                                                                       \
        return true;                                                                                                   \
    }

#define EMULATE_SIMPLE_1OP(insn, mnemonic)                                                                             \
    template <typename DECODER>                                                                                        \
    static bool emulate_##mnemonic(emulator_vcpu& vcpu, DECODER& decoder_)                                             \
    {                                                                                                                  \
        auto op = decoder_.get_operand(0);                                                                             \
        using optype = common_operand::type;                                                                           \
        auto determine_asm_block = [](optype op_type, size_t width, bool lock) -> func_t* {                            \
            ASM_BLOCK_SWITCH_1OP(mnemonic)                                                                             \
        };                                                                                                             \
        auto fn = determine_asm_block(op->get_type(), op->get_width() * 8, decoder_.has_lock_prefix());                \
        return helper_1op(vcpu, decoder_, fn, op);                                                                     \
    }

#define EMULATE_SIMPLE_2OP_INTERNAL(insn, mnemonic, switch_macro)                                                      \
    template <typename DECODER>                                                                                        \
    static bool emulate_##mnemonic(emulator_vcpu& vcpu, DECODER& decoder_)                                             \
    {                                                                                                                  \
        auto src = decoder_.get_operand(1);                                                                            \
        auto dst = decoder_.get_operand(0);                                                                            \
        using optype = common_operand::type;                                                                           \
        auto determine_asm_block = [](optype src_type, optype dst_type, size_t width, bool lock) -> func_t* {          \
            switch_macro(mnemonic)                                                                                     \
        };                                                                                                             \
        auto fn =                                                                                                      \
            determine_asm_block(src->get_type(), dst->get_type(), dst->get_width() * 8, decoder_.has_lock_prefix());   \
        return helper_2op(vcpu, decoder_, fn, src, dst, has_read_only_dst(#insn));                                     \
    }

#define EMULATE_SIMPLE_2OP(insn, mnemonic) EMULATE_SIMPLE_2OP_INTERNAL(insn, mnemonic, ASM_BLOCK_SWITCH_2OP)
#define EMULATE_SIMPLE_2OP_SHIFTS(insn, mnemonic)                                                                      \
    EMULATE_SIMPLE_2OP_INTERNAL(insn, mnemonic, ASM_BLOCK_SWITCH_SHIFTS_2OP)

#define EMULATE_JCC(insn, mnemonic)                                                                                    \
    template <typename DECODER>                                                                                        \
    static bool emulate_##mnemonic(emulator_vcpu& vcpu, DECODER& decoder_)                                             \
    {                                                                                                                  \
        uint64_t guest_flags = vcpu.read(gpr::FLAGS);                                                                  \
        if (mnemonic##_helper(guest_flags)) {                                                                          \
            return emulate_jmp(vcpu, decoder_, true);                                                                  \
        }                                                                                                              \
        return true;                                                                                                   \
    }

#define FUNCTION_SELECTION(insn, mnemonic)                                                                             \
case instruction::insn:                                                                                                \
    return emulate_##mnemonic(vcpu_, decoder_) ? status_t::OK : status_t::FAULT;

#define INSN_TRANSLATION(insn, mnemonic)                                                                               \
case UD_I##mnemonic:                                                                                                   \
    return instruction::insn;

#define INSN_LIST(insn, mnemonic) insn,

#define FULLY_IMPLEMENTED_LIST(insn, mnemonic) case instruction::insn:

#define CALL_MACRO_DEAD_SIMPLE(macro) macro(NOP, nop) macro(PAUSE, pause)

#define CALL_MACRO_LOCKABLE_1OP(macro) macro(INC, inc) macro(DEC, dec) macro(NEG, neg) macro(NOT, not )

#define CALL_MACRO_LOCKABLE_2OP(macro)                                                                                 \
    macro(ADD, add) macro(OR, or) macro(AND, and) macro(SUB, sub) macro(XOR, xor) macro(XCHG, xchg) macro(SBB, sbb)    \
        macro(ADC, adc) macro(CMP, cmp) macro(TEST, test)

#define CALL_MACRO_SHIFTS_2OP(macro) macro(SHL, shl) macro(SHR, shr) macro(SAR, sar)

#define CALL_MACRO_JCC(macro)                                                                                          \
    macro(JA, ja) macro(JAE, jae) macro(JB, jb) macro(JBE, jbe) macro(JG, jg) macro(JGE, jge) macro(JL, jl)            \
        macro(JLE, jle) macro(JNO, jno) macro(JNP, jnp) macro(JNS, jns) macro(JNZ, jnz) macro(JO, jo) macro(JP, jp)    \
            macro(JS, js) macro(JZ, jz)

#endif // EMULATOR_MACROS_DEFINED

#ifdef GENERATE_JUMPTABLE
CALL_MACRO_LOCKABLE_1OP(INSN_LOCKABLE_SET_1OP)
CALL_MACRO_LOCKABLE_2OP(INSN_LOCKABLE_SET_2OP)
CALL_MACRO_SHIFTS_2OP(INSN_SHIFTS_2OP)
CALL_MACRO_JCC(INSN_JCC)
#endif

#ifdef GENERATE_EXTERNS
CALL_MACRO_LOCKABLE_1OP(EXTERNS_SET_1OP)
CALL_MACRO_LOCKABLE_2OP(EXTERNS_SET_2OP)
CALL_MACRO_SHIFTS_2OP(EXTERNS_SET_SHIFTS_2OP)
CALL_MACRO_JCC(EXTERNS_JCC)
#endif

#ifdef GENERATE_EMULATOR_FUNCTIONS
CALL_MACRO_DEAD_SIMPLE(EMULATE_SIMPLE_0OP)
CALL_MACRO_LOCKABLE_1OP(EMULATE_SIMPLE_1OP)
CALL_MACRO_LOCKABLE_2OP(EMULATE_SIMPLE_2OP)
CALL_MACRO_SHIFTS_2OP(EMULATE_SIMPLE_2OP_SHIFTS)
CALL_MACRO_JCC(EMULATE_JCC)
#endif

#ifdef GENERATE_FUNCTION_SELECTION
CALL_MACRO_DEAD_SIMPLE(FUNCTION_SELECTION)
CALL_MACRO_LOCKABLE_1OP(FUNCTION_SELECTION)
CALL_MACRO_LOCKABLE_2OP(FUNCTION_SELECTION)
CALL_MACRO_SHIFTS_2OP(FUNCTION_SELECTION)
CALL_MACRO_JCC(FUNCTION_SELECTION)
#endif

#ifdef GENERATE_INSN_IMPLEMENTED_LIST
CALL_MACRO_DEAD_SIMPLE(FULLY_IMPLEMENTED_LIST)
CALL_MACRO_LOCKABLE_1OP(FULLY_IMPLEMENTED_LIST)
CALL_MACRO_LOCKABLE_2OP(FULLY_IMPLEMENTED_LIST)
CALL_MACRO_SHIFTS_2OP(FULLY_IMPLEMENTED_LIST)
CALL_MACRO_JCC(FULLY_IMPLEMENTED_LIST)
#endif
