/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <arch.h>
#include <musl_app_config.hpp>

#include <cstdint>

namespace slvm_pd
{

// The multiboot module index of the slvm_pd binary.
static constexpr uint64_t SLVM_APP_MODULE_INDEX {0x2};

// Following constants define the address space layout of the slvm-pd's HPT.
// Care must be taken that no overlapping regions exist.

static constexpr uintptr_t STARTUP_UTCB_ADDRESS {musl::STACK_ADDRESS + musl::STACK_SIZE};

static constexpr uintptr_t SIP_MAPPING_ADDRESS {STARTUP_UTCB_ADDRESS + PAGE_SIZE};
static constexpr uintptr_t SIP_MAX_SIZE {6 * PAGE_SIZE};

static constexpr uintptr_t SLVM_VMEM_POOL_START {SIP_MAPPING_ADDRESS + SIP_MAX_SIZE};
static constexpr uintptr_t SLVM_VMEM_POOL_SIZE {0x1000000};

static constexpr uintptr_t SLVM_WRITE_PAGE_START {SLVM_VMEM_POOL_START + SLVM_VMEM_POOL_SIZE};
static constexpr uintptr_t SLVM_WRITE_PAGE_SIZE {PAGE_SIZE};

// The slvm VMs guest memory is mapped into the host address space with
// following offset. Only guest memory should be mapped behind that address.
static constexpr uint64_t GUEST_MEMORY_START {0x10000000};
static_assert(SLVM_WRITE_PAGE_START + SLVM_WRITE_PAGE_SIZE <= GUEST_MEMORY_START);

/**
 * SLVM specific vm exit reasons.
 *
 * To avoid intersection between actual hardware VM EXIT reasons and slvm specific
 * We start with a value above 16 bits (0x10000) the hardware would not support.
 * The exit reason field in the VMCS is actually 32 bits wide
 */
enum slvm_exit_reason : uint32_t
{
    SLVM_EXIT_REASON_BASE = 0x10000,

    // Custom exits related to GVT optimizations
    SLVM_EXIT_GVT_BASE = SLVM_EXIT_REASON_BASE + 0,
    SLVM_EXIT_GVT_MMIO_WRITE = SLVM_EXIT_GVT_BASE + 0,
    SLVM_EXIT_GVT_PAGE_TRACK = SLVM_EXIT_GVT_BASE + 1,
};

/**
 * SLVM specific errors
 *
 * Errors are passed as exit reasons from slvm-pd to Supernova
 * Because of this we have to ensure that no intersection between actual
 * VM EXIT reasons, SLVM specific exit reasons and error reasons occur.
 * Because of this error reasons are negative.
 */
enum slvm_error_reason : int32_t
{
    SLVM_EXIT_EXCEPTION_BASE = -0x10000,
    SLVM_EXIT_EXCEPTION = SLVM_EXIT_EXCEPTION_BASE - 0,
};

} // namespace slvm_pd
