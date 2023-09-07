/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

/*
 * Helper for using VMX instructions.
 *
 * All VMX instructions follow the same conventions (Intel SDM, Vol. 3, 30.2) and set either CF or ZF in RFLAGS in case
 * of a failure. All the helper functions provided here use assertions to detect failures. Consequently, a returning
 * function call implies success. Both flags are checked separately to distinguish the different failures from each
 * other.
 */

#include <toyos/x86/x86defs.hpp>

#include <assert.h>

// declare local variables for failure check
#define CC_VARS bool fail_invalid, fail_valid;

// write error code bits from RFLAGS register to named output variables
#define CC_STORE    \
   "setb %[carry];" \
   "setz %[zero];"

// map local variables to named output variables
#define CC_PARAM [carry] "=q"(fail_invalid), [zero] "=q"(fail_valid)

// use assertions to check that no failure happened
#define CC_CHECK             \
   assert(not fail_invalid); \
   assert(not fail_valid);

// generate a vmx operation which has a parameter and no return value
// (all vmx instruction parameters are addresses)
#define VMX_OPERATION_PARAM(op)                       \
   inline void op(uintptr_t par)                      \
   {                                                  \
      CC_VARS                                         \
      asm volatile(#op " %[param];" CC_STORE:CC_PARAM \
                   : [param] "m"(par)                 \
                   : "cc", "memory");                 \
      CC_CHECK                                        \
   }

// generate a vmx operation which does not have a parameter and no return value
#define VMX_OPERATION(op)                    \
   inline void op()                          \
   {                                         \
      CC_VARS                                \
      asm volatile(#op ";" CC_STORE:CC_PARAM \
                   :                         \
                   : "cc", "memory");        \
      CC_CHECK                               \
   }

/**
 * \brief Put the processor in VMX operation.
 *
 * \param par address of vmxon region
 */
VMX_OPERATION_PARAM(vmxon);

/**
 * \brief Clear VMCS
 *
 * \param par VMCS address
 */
VMX_OPERATION_PARAM(vmclear);

/**
 * \brief Load pointer to VMCS
 *
 * \param par VMCS address
 */
VMX_OPERATION_PARAM(vmptrld);

/**
 * \brief Leave VMX operation
 */
VMX_OPERATION(vmxoff);

/**
 * \brief Launch the virtual machine managed by the current VMCS.
 */
VMX_OPERATION(vmlaunch);

/**
 * \brief Get current VMCS pointer.
 *
 * \return current VMCS pointer
 */
inline uintptr_t vmptrst()
{
   uint64_t res;
   CC_VARS
   asm volatile("vmptrst %[loc];" CC_STORE
                : CC_PARAM, [loc] "=m"(res)
                :
                : "cc");
   CC_CHECK
   return res;
}

/**
 * \brief Read VMCS field.
 *
 * \param enc field encoding
 *
 * \return field value
 */
inline uint64_t vmread(uint64_t enc)
{
   uint64_t res;
   CC_VARS
   asm volatile("vmread %[_enc], %[_res];" CC_STORE
                : CC_PARAM, [_res] "=rm"(res)
                : [_enc] "r"(enc)
                : "cc");
   CC_CHECK
   return res;
}

/**
 * \brief Write VMCS field.
 *
 * \param enc field encoding
 * \param value field value
 */
inline void vmwrite(uint64_t enc, uint64_t value)
{
   CC_VARS
   asm volatile("vmwrite %[_val], %[_enc];" CC_STORE:CC_PARAM
                : [_enc] "r"(enc), [_val] "rm"(value)
                : "cc", "memory");
   CC_CHECK
}
