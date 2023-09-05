/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "slvm_config.hpp"
#include "slvm_gvt_bar_info.hpp"

#include <arch.hpp>
#include <array_lock_free_reservable.hpp>
#include <cbl/interval.hpp>
#include <cbl/lock_free_queue.hpp>
#include <config.hpp>
#include <hedron/cap_sel.hpp>
#include <hedron/utcb.hpp>
#include <slvm_posted_writes.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <type_traits>

static_assert(not cbl::interval::from_size(slvm_pd::SIP_MAPPING_ADDRESS, slvm_pd::SIP_MAX_SIZE)
                      .intersects(cbl::interval::from_size(slvm_pd::SLVM_VMEM_POOL_START,
                                                           slvm_pd::SLVM_VMEM_POOL_SIZE)),
              "sip mapping address is in range of UTCBs");

using shared_page = std::array<uint8_t, PAGE_SIZE>;

using posted_write_queue = cbl::lock_free_queue_producer<posted_writes_descriptor, POSTED_WRITES_QUEUE_SIZE>;

static_assert(sizeof(posted_write_queue::queue_storage) <= PAGE_SIZE);

/**
 * The slvm info page.
 *
 * It contains any required information to run a slave VM in a separate PD.
 * It is shared alongside the slvm-pd app and the SuperNOVA VMM via shared memory.
 * For a general overview about the design please see
 * https://gitlab.vpn.cyberus-technology.de/groups/supernova-core/-/wikis/SVP-WS/MUSL-SLVM-Design
 */
struct alignas(PAGE_SIZE) slvm_info_page
{
    static constexpr uint64_t INVALID_STATE_PAGE_INDEX {~0ull};
    static constexpr uint64_t INVALID_MMIO_REGION_INDEX {~0ull};

    /**
     * Information about a GVT page track or MMIO write event.
     */
    struct gvt_access_info
    {
        uint64_t gpa;
        uint64_t value;
        uint64_t size;
    };

    /**
     * Contains all the results of a run() call on a slvm vcpu.
     *
     *  modifiable by SLVM-PD
     */
    struct vcpu_run_result
    {
        vcpu_run_result() = default;

        explicit vcpu_run_result(int64_t last_exit_) : last_exit(last_exit_) {}

        /**
         *  The VMEXIT reason for the last caused VMEXIT.
         */
        int32_t last_exit {0};

        /**
         * If slvm-pd emulated a page track or MMIO write access the gvt_access
         * member is populated with the required values to pass up to slvm.
         *
         * The values are populated if last_exit is one of:
         * VMX_PT_GVT_PAGE_WRITE
         * VMX_PT_GVT_MMIO_WRITE
         */
        std::optional<gvt_access_info> gvt_access;
    };

    /**
     * The vcpu information structure.
     * It contains all information to manage the
     * slave vms virtual cpu.
     */
    struct vcpu_info
    {
        /**
         * The vcpu kernel object capability.
         * The capability selector is required to handle interrupts/pokes.
         *
         * modifiable by: SLVM-PD
         */
        hedron::cap_sel vcpu_obj_sel;

        /**
         * A portal that can be used to transfer control
         * to a SLVM vCPU
         *
         * modifiable by SLVM-PD
         */
        hedron::cap_sel run_portal;

        /**
         * Result values of a run() vcpu call.
         *
         * modifiable by SLVM-PD
         */
        vcpu_run_result run_result;

        /**
         * The shared vcpu state page indices.
         * The state is shared between the userland VMM and the slave vm.
         * The vcpu_state and vcpu_xstate addresses address has to be
         * accessible from the slave PD.
         *
         * modifiable by VMM / SLVM-PD
         */
        uint64_t state_index {INVALID_STATE_PAGE_INDEX};

        /**
         * Indicated whether the vcpu is migrated between physical cpus
         * If true the vTLB have to be flushed for the slave vm.
         *
         * modifiable by VMM
         */
        bool migrated {false};

        /**
         * Indicates whether the vcpu is active and should be used
         * for guest code execution.
         *
         * modifiable by: VMM
         */
        bool enabled {false};
    };

    /**
     * Indicator whether the slvm slave VM's protection domain is initialized and ready to execute
     * guest code on its vcpus.
     *
     * modifiable by SLVM-PD
     */
    volatile bool slvm_pd_initialized {false};

    /**
     * The vcpu state page memory region.
     * The state pages are mapped consecutively at a fixed address in the slave vm PD.
     * The parameters determines the beginning of both the state and the xstate region.
     *
     * modifiable by SLVM-PD
     */
    hedron::utcb* state_pages_ptr;
    shared_page* xstate_pages_ptr;

    /**
     * The range of capabilities that are free to use for the slave PD.
     *
     * modifiable by VMM
     */
    cbl::interval cap_range;

    // ########################################################################
    // # Device specific data                                                 #
    // ########################################################################

    /**
     * Info about the GVT MMIO BAR0. If set, slvm-pd emulates BAR and page
     * track writes.
     */
    mmio_bar_info gvt_mmio_bar0_info;

    /**
     * Indicates whether the mmio access emulation in supernova is active.
     *
     * modifiable by VMM
     */
    std::atomic<bool> gvt_mmio_access_emulation_active {false};

    /**
     * Indicates whether the page track access emulation in supernova is
     * active.
     *
     * modifiable by VMM
     */
    std::atomic<bool> gvt_page_track_access_emulation_active {false};

    /**
     * Information about the posted MMIO optimization.
     */
    struct posted_mmio_info_
    {
        /**
         * Indicates if a write page for the MMIO posted write optimization has
         * been delegated to the slvm-pd.
         *
         * modifiable by VMM
         */
        std::atomic<bool> write_page_delegated {false};

        /**
         * Timeout after which we should return to the userspace VMM if no
         * other vmexit occurred in between.
         *
         * modifiable by VMM
         */
        std::atomic<uint64_t> max_timeout_ms {0};

        /**
         * Contains MMIO regions for which the write posting optimization should be done.
         *
         * modifiable by VMM
         */
        array_lock_free_with_remove<uint64_t, 64, INVALID_MMIO_REGION_INDEX> mmio_regions;

        void reset()
        {
            write_page_delegated = false;
            max_timeout_ms = 0;
            mmio_regions.clear();
        }
    };

    posted_mmio_info_ posted_mmio_info;

    /**
     * TSC frequency in kHz.
     * modifiable by VMM
     */
    uint64_t tsc_frequency_khz {0};

    // ########################################################################
    // # Per vCPU information                                                 #
    // ########################################################################

    /**
     * The vcpu info structure.
     *
     * It contains any runtime information required to execute guest code.
     * Information that is required by either the VMM to manage the execution is provided
     * by this structure too.
     *
     * modifiable by VMM and SLVM-PD
     */
    std::array<vcpu_info, MAX_CPUS> vcpu_information;
};

/**
 * The VMM has to map the memory for the slvm info page into the SLVM-PD
 * protection domain.
 * Because of this we have to ensure that the mapping size is modified
 * accordingly if the information stored in these pages grows.
 */
static_assert(sizeof(slvm_info_page) <= slvm_pd::SIP_MAX_SIZE,
              "The slvm_info_page grew too large. If this is intentional, please adapt SIP_MAX_SIZE.");

/**
 * As the Sip is mapped in both VMM and SLVM-PD we can not share pointers between VMM and SLVM-PD.
 * Especially virtual functions can not be called from the other side.
 */
static_assert(std::is_trivially_copyable<slvm_info_page>::value, "The slvm_info_page has to be trivially copyable");
