/*
  SPDX-License-Identifier: MIT

  Copyright (c) 2021-2023 Cyberus Technology GmbH

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#pragma once

#ifdef __cplusplus
#    define SLVM_INLINE inline
#else
#    define SLVM_INLINE static inline
#endif

/* This file contains the definitions to interact with supernova's virtualization VMCALL interface. It does not include
 * any dependencies on supernova internal libraries or other headers (except stdint.h and stddef.h). The intention is to
 * ship this as part of other software that needs to be aware of this interface.
 *
 * This interface has to be C-compatible, because it is used from the Linux kernel.
 */

/*
 * CPUID Interface
 *
 * When the VMCALL interface is active, SuperNOVA announces the hypervisor feature bit in CPUID. To identify, whether we
 * are actually running on SuperNOVA, the guest can check the hypervisor vendor leaf.
 */

/** CPUID leaf to check for vendor information */
#define SLVMAPI_CPUID_VENDOR_LEAF 0x40000000

/** The vendor string presented in EBX/ECX/EDX.
 *
 * The low byte of EBX contains the first byte of the string, the highest
 * byte of EDX contains the last. */
#define SLVMAPI_CPUID_VENDOR_STRING "SuperNOVA   "

/**
 * A bitmap of features exposed by SuperNOVA.
 *
 * The corresponding feature bits are SLVMAPI_CPUID_FEATURE_*.
 */
#define SLVMAPI_CPUID_FEATURE_LEAF 0x40000001

/** Set if SuperNOVA exposes virtualization features to this guest. */
#define SLVMAPI_CPUID_FEATURE_VIRT 0x00000001

/** SLVM Specific VM Exit reasons */
#define SLVMAPI_SLVM_SPECIFIC_EXIT_BASE 0x10000

/** The base of SLVM specific GVT accesses */
#define SLVMAPI_EXIT_GVT_BASE (SLVMAPI_SLVM_SPECIFIC_EXIT_BASE + 0)

/**
 * The run_vcpu call returned information about a MMIO BAR write that needs to
 * be forwarded to the mediator.
 */
#define SLVMAPI_EXIT_GVT_MMIO_WRITE (SLVMAPI_EXIT_GVT_BASE + 0)

/**
 * The run_vcpu call returned information about a page track that needs to be
 * forwarded to the mediator.
 */
#define SLVMAPI_EXIT_GVT_PAGE_TRACK (SLVMAPI_EXIT_GVT_BASE + 1)

/*
 * Common VMCALL types and constants. These are typically returned in EAX after VMCALLs.
 */

typedef int32_t slvm_status_t;
typedef int16_t slvm_vm_id_t;

/** The VMCALL was successful. */
#define SLVMAPI_VMCALL_SUCCESS 0

/** The VMCALL tried to create a resource that is already there. */
#define SLVMAPI_VMCALL_EEXIST -1

/** The VMCALL referred to a resource that does not exist. */
#define SLVMAPI_VMCALL_ENOENT -2

/** The VMCALL is not supported. */
#define SLVMAPI_VMCALL_ENOTSUP -3

/** The VMCALL referred to physical memory that does not exist or is not mappable. */
#define SLVMAPI_VMCALL_EFAULT -4

/** There were not enough resources to complete the VMCALL. */
#define SLVMAPI_VMCALL_ENOMEM -5

/** The VMCALL is not implemented at the moment.*/
#define SLVMAPI_VMCALL_ENOTIMP -6

/** The VM the VMCALL is related to is crashed */
#define SLVMAPI_VMCALL_ECRASHED -7

/** The VMCALL failed because the VMM is wrongly configured. */
#define SLVMAPI_VMCALL_EWRONGCONFIG -8

/** Inline assembly constraints for the different VMCALL parameters. Other registers are preserved. */
#define SLVMAPI_VMCALL_FUN "a"
#define SLVMAPI_VMCALL_PARAM1 "D"
#define SLVMAPI_VMCALL_PARAM2 "S"
#define SLVMAPI_VMCALL_PARAM3 "d"
#define SLVMAPI_VMCALL_PARAM4 "c"
#define SLVMAPI_VMCALL_PARAM5 "r"

struct slvm_vmcall_result
{
    slvm_status_t status;
    uint64_t result1;
    uint64_t result2;
    uint64_t result3;
};

SLVM_INLINE struct slvm_vmcall_result slvm_vmcall(uint32_t fun, uint64_t param1, uint64_t param2, uint64_t param3,
                                                  uint64_t param4, uint64_t param5_)
{
    struct slvm_vmcall_result result;

    register uint64_t param5 asm("r8") = param5_;

    asm volatile("vmcall"
                 : "=" SLVMAPI_VMCALL_FUN(result.status), "=" SLVMAPI_VMCALL_PARAM1(result.result1),
                   "=" SLVMAPI_VMCALL_PARAM2(result.result2), "=" SLVMAPI_VMCALL_PARAM3(result.result3),
                   "+" SLVMAPI_VMCALL_PARAM4(param4), "+" SLVMAPI_VMCALL_PARAM5(param5)
                 : SLVMAPI_VMCALL_FUN(fun), SLVMAPI_VMCALL_PARAM1(param1), SLVMAPI_VMCALL_PARAM2(param2),
                   SLVMAPI_VMCALL_PARAM3(param3)
                 : "memory");

    return result;
}

/*
 * Virtualization VMCALL interface (available if SLVMAPI_CPUID_FEATURE_VIRT is set)
 */

/** The version of the API as it is described in this header. */
#define SLVMAPI_VMCALL_VERSION 3

/** Start of the VMCALL identifiers for the virtualization protocol. */
#define SLVMAPI_VMCALL_VIRT_BASE 0x00010000
#define SLVMAPI_VMCALL_VIRT_LAST 0x0001FFFF

#define SLVMAPI_VMCALL_VIRT_CHECK_PRESENCE (SLVMAPI_VMCALL_VIRT_BASE)

#define SLVMAPI_IS_PRESENT_MAGIC 0x83652345U

/**
 * Check presence of SuperNOVA Linux Virtualization Module API.
 *
 * \return a boolean indicating presence.
 */
SLVM_INLINE int slvm_vmcall_is_present(void)
{
    return slvm_vmcall(SLVMAPI_VMCALL_VIRT_CHECK_PRESENCE, 0, 0, 0, 0, 0).status
           == (slvm_status_t) SLVMAPI_IS_PRESENT_MAGIC;
}

#define SLVMAPI_VMCALL_VIRT_CREATE_VM (SLVMAPI_VMCALL_VIRT_BASE + 1)

/**
 * Create a virtual machine.
 *
 * The returned VM ID needs to be specified for all other VMCALLs that operate on VMs.
 *
 * Callable from Ring0 only.
 *
 * \return A positive VM ID or a negative error value (one of SLVMAPI_VMCALL_*).
 */
SLVM_INLINE slvm_vm_id_t slvm_vmcall_create_vm(void)
{
    return slvm_vmcall(SLVMAPI_VMCALL_VIRT_CREATE_VM, 0, 0, 0, 0, 0).status;
}

#define SLVMAPI_VMCALL_VIRT_MAP_MEMORY (SLVMAPI_VMCALL_VIRT_BASE + 3)

/**
 * \see slvm_vmcall_map_memory
 */
#define SLVMAPI_PROT_READ (1U << 0)
#define SLVMAPI_PROT_WRITE (1U << 1)
#define SLVMAPI_PROT_EXEC (1U << 2)

/**
 * Map memory into the virtual machine's extended page table.
 *
 * Callable from Ring0 only.
 *
 * \param vm_id   The unique identifier for the VM.
 * \param src_gfn The frame number in the Control VM to map from.
 * \param dst_gfn The frame number in the guest to map to.
 * \param pages   The number of consecutive frames to map.
 * \param prot    A combination of SLVMAPI_VMCALL_PROT_* via bitwise OR.
 *                Read rights will potentially be implied by other rights.
 *
 * \return SLVMAPI_VMCALL_SUCCESS on success, otherwise one of SLVMAPI_VMCALL_*
 */
SLVM_INLINE slvm_status_t slvm_vmcall_map_memory(slvm_vm_id_t vm_id, uint64_t src_gfn, uint64_t dst_gfn, uint32_t pages,
                                                 uint32_t prot)
{
    return slvm_vmcall(SLVMAPI_VMCALL_VIRT_MAP_MEMORY, vm_id, src_gfn, dst_gfn, ((uint64_t) pages) << 32 | prot, 0)
        .status;
}

#define SLVMAPI_VMCALL_VIRT_UNMAP_MEMORY (SLVMAPI_VMCALL_VIRT_BASE + 4)

/**
 * Unmap memory from the virtual machine's extended page table.
 *
 * Callable from Ring0 only.
 *
 * \param vm_id   The unique identifier for the VM.
 * \param dst_gfn The frame number in the guest to unmap at.
 * \param pages   The number of consecutive frames to unmap.
 *
 * \return SLVMAPI_VMCALL_SUCCESS on success, otherwise one of SLVMAPI_VMCALL_*
 */
SLVM_INLINE slvm_status_t slvm_vmcall_unmap_memory(slvm_vm_id_t vm_id, uint64_t dst_gfn, uint32_t pages)
{
    return slvm_vmcall(SLVMAPI_VMCALL_VIRT_UNMAP_MEMORY, vm_id, dst_gfn, pages, 0, 0).status;
}

#define SLVMAPI_VMCALL_VIRT_DESTROY_VM (SLVMAPI_VMCALL_VIRT_BASE + 5)

/**
 * Destroy a virtual machine.
 *
 * Instruct SuperNOVA to destroy a VM associated with the current CR3 value.

 * \param vm_id The unique identifier for the VM.
 *
 * Callable from Ring0 only.
 *
 * \return SLVMAPI_VMCALL_SUCCESS on success, otherwise one of SLVMAPI_VMCALL_*
 */
SLVM_INLINE slvm_status_t slvm_vmcall_destroy_vm(slvm_vm_id_t vm_id)
{
    return slvm_vmcall(SLVMAPI_VMCALL_VIRT_DESTROY_VM, vm_id, 0, 0, 0, 0).status;
}

/*
 * The VMCALL with the number SLVMAPI_VMCALL_VIRT_BASE + 6 was formerly used
 * to register a ahci port region for VirtualBox AHCI controller performance
 * optimizations, which are removed now.
 * the call can be reused now.
 */

#define SLVMAPI_VMCALL_VIRT_REGISTER_GVT_MMIO_BAR0 (SLVMAPI_VMCALL_VIRT_BASE + 7)

/**
 * Register the GVT MMIO BAR0 in SuperNOVA that is used to emulate certain
 * accesses to the MMIO BAR0 to avoid going up to the userspace VMM.
 * The BAR0 information is only deleted when destructing the VM. Registering
 * the BAR turns on the MMIO BAR write and page track write optimization.
 *
 * \param vm_id The unique identifier for the VM.
 * \param gvt_bar0_gpa The guest physical address of the beginning of the GVT MMIO BAR0.
 * \param gvt_bar0_size The size of the GVT MMIO BAR0.
 * \return SLVMAPI_VMCALL_SUCCESS on success, otherwise one of SLVMAPI_VMCALL_*
 */
SLVM_INLINE slvm_status_t slvm_vmcall_register_gvt_mmio_bar0(slvm_vm_id_t vm_id, uint64_t gvt_bar0_gpa,
                                                             uint64_t gvt_bar0_size)
{
    return slvm_vmcall(SLVMAPI_VMCALL_VIRT_REGISTER_GVT_MMIO_BAR0, vm_id, gvt_bar0_gpa, gvt_bar0_size, 0, 0).status;
}

#define SLVMAPI_VMCALL_QUERY_FEATURES (SLVMAPI_VMCALL_VIRT_BASE + 8)

struct slvm_query_features_result
{
    slvm_status_t status;
    uint64_t feature_map;
};

enum sn_feature_flags_level0
{
    /** SuperNOVA supports version 3 of the core API. This is obsolete. */
    SVP_FEATURE_LVL0_CORE_API_V3 = 1ull << 0,
    /** SuperNOVA supports decoding and emulating GVT MMIO write accesses and page track writes if possible. */
    SVP_FEATURE_LVL0_EMULATE_GVT = 1ull << 1,
    /** SuperNOVA supports the posted MMIO interface. */
    SVP_FEATURE_LVL0_MMIO_POSTING_IF = 1ull << 3,
    /** SuperNOVA supports the SLVM_VMCALL_RUN_VCPU_V4 call. */
    SVP_FEATURE_LVL0_CORE_API_V4 = 1ull << 4,

    /** Indicates that the next level of features is available. */
    SVP_FEATURE_LVL0_NEXT_LVL_AVAIL = 1ull << 63,
};

/**
 * Query the supported features of the SuperNOVA vmcall backend.
 *
 * The user is supposed to start querying feature_level = 0. If
 * SVP_FEATURE_LVL0_NEXT_LVL_AVAIL is set, the user can increment the
 * feature_level by one and query the next group of features.
 *
 * \param feature_level The level of features to retrieve.
 * \return A feature result struct. If the status member is
 *         SLVMAPI_VMCALL_SUCCESS, the feature_map of the structure contains a
 *         valid feature map. If the requested feature level is not available
 *         the status contains SLVMAPI_VMCALL_ENOENT.
 */
SLVM_INLINE struct slvm_query_features_result slvm_vmcall_query_features(uint64_t feature_level)
{
    struct slvm_query_features_result result_query_features;
    struct slvm_vmcall_result result_vmcall;

    result_vmcall = slvm_vmcall(SLVMAPI_VMCALL_QUERY_FEATURES, feature_level, 0, 0, 0, 0);

    result_query_features.status = result_vmcall.status;
    result_query_features.feature_map = result_vmcall.result1;

    return result_query_features;
}

#define SLVMAPI_VMCALL_REGISTER_WRITE_PAGE (SLVMAPI_VMCALL_VIRT_BASE + 9)

/*
 * Register a write page where the shared queue is initialized that can be
 * used for posted MMIO writes.
 *
 * What does it do?
 *
 * If the posted write mechanism is successfully initialized and MMIO regions
 * are registered, EPT violations due to writes to those regions are not passed
 * up to the userspace VMM but are decoded by SVP and the write access
 * information is enqueued in the shared queue. The mechanism reduces
 * the number of VM exits that must be passed up to the userspace VMM.
 *
 * How to use?
 *
 * The user is only able to register a single write page. The page cannot be
 * changed afterwards and is only cleared on VM destruction. After the call has
 * succeeded it is guaranteed that SVP has initialized the shared queue and it
 * can be used by the userspace VMM.
 * Registering a write page is required to use the posted MMIO write feature.
 * The user is allowed to register MMIO regions before registering a write
 * page, but no posted MMIO writes will be enqueued until a write page is
 * registered. Once SVP decodes and enqueues write accesses, the userspace VMM
 * must ensure those posted writes are handled in the enqueued order.
 *
 * For the design of the posted MMIO writes see
 * supernova-core/doc/slvm-write-posting.md.
 *
 * The shared queue design and memory layout can be found under
 * standalone/lib/cbl/include/cbl/lock_free_queue_def.h.
 *
 * The example C++ implementation of the shared queue can be found under
 * standalone/lib/cbl/include/cbl/lock_free_queue.hpp.
 *
 * \param write_page_gfn
 *        Guest physical frame used for the shared queue between slvm-pd and
 *        the userspace VMM. After the call succeeded, the queue meta data is
 *        initialized and the userspace VMM can start dequeuing.
 * \param max_timeout_ms
 *        After the given amount of time in ms, if no other vmexit happened
 *        in between, slvm-pd will issue a timeout exit. This prevents that
 *        due to the posted writes the userspace VMM is not able to execute
 *        for a long time.
 * \param max_outstanding_requests
 *        Defines the maximum number of writes that will be enqueued in the
 *        shared queue by slvm-pd.
 * \param buffer_layout_version
 *        The version of the shared queue structure to prevent invalid
 *        queue versions between userspace VMM and slvm-pd.
 */
SLVM_INLINE slvm_status_t slvm_vmcall_register_write_page(slvm_vm_id_t vm_id, uint64_t write_page_gfn,
                                                          uint64_t max_timeout_ms, uint64_t max_outstanding_requests,
                                                          uint64_t buffer_layout_version)
{
    return slvm_vmcall(SLVMAPI_VMCALL_REGISTER_WRITE_PAGE, vm_id, write_page_gfn, max_timeout_ms,
                       max_outstanding_requests, buffer_layout_version)
        .status;
}

#define SLVMAPI_VMCALL_REGISTER_MMIO_REGION (SLVMAPI_VMCALL_VIRT_BASE + 10)

/*
 * Register some MMIO area to participate in the posted write optimization.
 * Writes to this area will be queued by SVP and must be handled by the
 * userspace VMM by draining posted write buffer.
 *
 * This call fails unless a write page is registered.
 *
 * \param mmio_gpa The guest physical address of the start of the MMIO
 *                 region to register. Must be at least 4K aligned.
 * \param mmio_size The size of the MMIO region to register. Must be a
 *                  power of two.
 */
SLVM_INLINE slvm_status_t slvm_vmcall_register_mmio_region(slvm_vm_id_t vm_id, uint64_t mmio_gpa, uint64_t mmio_size)
{
    return slvm_vmcall(SLVMAPI_VMCALL_REGISTER_MMIO_REGION, vm_id, mmio_gpa, mmio_size, 0, 0).status;
}

#define SLVMAPI_VMCALL_UNREGISTER_MMIO_REGION (SLVMAPI_VMCALL_VIRT_BASE + 11)

/*
 * Unregister some MMIO area from the posted write optimization. After
 * unregistering vmexits due to accesses to those regions are passed up to the
 * userspace VMM as usual. The unregister call requires to use the exact same
 * region used for registration, otherwise the call will fail.
 *
 * This call fails unless a write page is registered.
 *
 * \param mmio_gpa The guest physical address of the start of the MMIO
 *                 region to unregister. Must be at least 4K aligned.
 * \param mmio_size The size of the MMIO region to unregister. Must be a
 *                  power of two.
 */
SLVM_INLINE slvm_status_t slvm_vmcall_unregister_mmio_region(slvm_vm_id_t vm_id, uint64_t mmio_gpa, uint64_t mmio_size)
{
    return slvm_vmcall(SLVMAPI_VMCALL_UNREGISTER_MMIO_REGION, vm_id, mmio_gpa, mmio_size, 0, 0).status;
}

#define SLVMAPI_VMCALL_VIRT_RUN_VCPU_V4 (SLVMAPI_VMCALL_VIRT_BASE + 12)

/**
 * The result of a slvm_vmcall_run_vcpu_v4 call.
 *
 * In case the GVT performance optimizations are disabled, only the status member
 * is populated.
 *
 * In case the GVT performance optimizations are enabled and the hypervisor
 * layer emulated a page track or MMIO BAR write access the
 * slvm_run_vcpu_result contains additional information. Currently, the MMIO
 * BAR access tracking and page tracking in the hypervisor layer are always
 * disabled.
 *
 * In case of a page track event following values are used with the described
 * meaning:
 * * status: is equal to SLVMAPI_VMCALL_GVT_PAGE_TRACK
 * * gpa: contains the guest physical address of the tracked access
 * * data: contains the value to be written by the write access
 * * size: length of the access in bytes
 *
 * In case of a MMIO BAR write event following values are used with the
 * described meaning:
 * * status: is equal to SLVMAPI_VMCALL_GVT_MMIO_WRITE
 * * gpa: contains the guest physical address of the MMIO write
 * * data: contains the value to be written by the write access
 * * size: length of the access in bytes
 *
 * The access information is used by the slvm kmod to forward to slvmgt.
 */
struct slvm_run_vcpu_result
{
    slvm_status_t status;
    uint64_t gpa;
    uint64_t data;
    uint64_t size;
};

/**
 * Start guest execution with the given vCPU state.
 *
 * Callable from Ring0 only.
 *
 * \param vm_id             The unique identifier for the VM.
 * \param state_gfn         The guest frame number of a UTCB data structure.
 * \param xsave_area_gfn    The guest frame number of a XSTATE page.
 * \param migrated          Whether the vCPU has been migrated between physical CPUs.
 *
 * \return a structure containing positive exit reason on success, a negative
 *         error value on failure (SLVMAPI_VMCALL_*).
 *         \See slvm_run_vcpu_result documentation for additional information.
 */
SLVM_INLINE struct slvm_run_vcpu_result slvm_vmcall_run_vcpu_v4(slvm_vm_id_t vm_id, uint64_t state_gfn,
                                                                uint64_t xsave_area_gfn, uint64_t migrated)
{
    struct slvm_run_vcpu_result result_run_vcpu;
    struct slvm_vmcall_result result_vmcall;
    result_vmcall = slvm_vmcall(SLVMAPI_VMCALL_VIRT_RUN_VCPU_V4, vm_id, state_gfn, xsave_area_gfn, migrated, 0);

    result_run_vcpu.status = result_vmcall.status;
    result_run_vcpu.gpa = result_vmcall.result1;
    result_run_vcpu.data = result_vmcall.result2;
    result_run_vcpu.size = result_vmcall.result3;

    return result_run_vcpu;
}

/** Start of the VMCALL identifiers for the log query protocol. */
#define SLVMAPI_VMCALL_LOG_BASE 0x00020000
#define SLVMAPI_VMCALL_LOG_LAST 0x0002FFFF

#define SLVMAPI_VMCALL_LOG_CHECK_PRESENCE (SLVMAPI_VMCALL_LOG_BASE)

#define SLVMAPI_VMCALL_LOG_IS_PRESENT_MAGIC 0x3257b333U

/**
 * Check presence of SuperNOVA Linux Virtualization Module Logging API.
 *
 * \return a boolean indicating presence.
 */
SLVM_INLINE int slvm_vmcall_log_is_present(void)
{
    return slvm_vmcall(SLVMAPI_VMCALL_LOG_CHECK_PRESENCE, 0, 0, 0, 0, 0).status
           == (slvm_status_t) SLVMAPI_VMCALL_LOG_IS_PRESENT_MAGIC;
}

#define SLVMAPI_VMCALL_LOG_READ_VMM (SLVMAPI_VMCALL_LOG_BASE + 1)

/**
 * Request VMM log.
 *
 * Callable from Ring0 only.
 *
 * \param dst_addr Guest physical address of a buffer to copy the log to.
 * \param dst_len  Size of the buffer.
 *
 * \return Number of characters copied to the buffer or a negative error value (one of SLVMAPI_VMCALL_*).
 *
 * Concurrent invocations are serialized.
 */
SLVM_INLINE int32_t slvm_vmcall_log_read_vmm(uint64_t dst, int64_t dst_len)
{
    return slvm_vmcall(SLVMAPI_VMCALL_LOG_READ_VMM, dst, dst_len, 0, 0, 0).status;
}
