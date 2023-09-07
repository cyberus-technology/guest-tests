/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/x86/arch.hpp>
#include <toyos/util/cast_helpers.hpp>
#include <toyos/util/interval.hpp>
#include <toyos/util/device_driver_adapter.hpp>
#include "testhelper/hpet.hpp"
#include "memory/splitting_buddy.hpp"
#include "x86/x86asm.hpp"

/**
 * Baremetal platform device driver adapter.
 *
 * This adapter receives a previously acquired
 * 1:1 mapped contiguous DMA region to initialize the pool with.
 *
 * It then hands out page-aligned buffers.
 *
 */
class baremetal_device_driver_adapter final : public device_driver_adapter
{
public:
    /**
     * Construct an adapter.
     *
     * \param dma_region An interval describing a 1:1 mapped DMA-able region.
     */
    explicit baremetal_device_driver_adapter(const cbl::interval& dma_region) : dma_region_(dma_region)
    {
        dma_pool_.free(dma_region);

        tsc_ticks_per_micro_second_ = tsc_ticks_per_us_using_hpet();
    }

    virtual ~baremetal_device_driver_adapter() {}

    /**
     * Allocate a DMA buffer.
     *
     * Note that currently this hands out 1:1 mappings.
     * If the pool does not have enough space left, this
     * function PANICs.
     *
     * \param count Number of contiguous pages to be contained in the buffer.
     * \return The DMA buffer descriptor for the desired size.
     */
    virtual dma_buffer_t allocate_pages(size_t count) override
    {
        auto ival {dma_pool_.alloc(count * PAGE_SIZE)};
        PANIC_UNLESS(ival, "Could not allocate DMA buffer of size {#x} bytes", count * PAGE_SIZE);
        return {ival->a, num_to_ptr<void>(ival->a), count};
    }

    /// Free the previously allocated buffer.
    virtual void free_buffer(const dma_buffer_t& buffer) override
    {
        dma_pool_.free(cbl::interval::from_size(uintptr_t(buffer.lin_addr), buffer.pages * PAGE_SIZE));
    }

    class baremetal_mutex final : public mutex_interface
    {
    public:
        baremetal_mutex() : mutex_interface() {}
        virtual void acquire() override
        { /*TODO*/
        }
        virtual void release() override
        { /*TODO*/
        }
    };

    /**
     * Allocate a new mutex object.
     *
     * This places a mutex implementation in the vector,
     * which automatically constructs a new mutex object
     * and hands out a reference to it.
     *
     * \return A reference to a mutex object.
     */
    virtual std::unique_ptr<mutex_interface> get_mutex() override { return std::make_unique<baremetal_mutex>(); }

protected:
    /// Baremetal delay function.
    virtual void udelay(std::chrono::microseconds duration) override
    {
        assert(tsc_ticks_per_micro_second_ != 0);

        auto target {rdtsc() + tsc_ticks_per_micro_second_ * duration.count()};
        while (rdtsc() < target) {
            cpu_pause();
        }
    }

private:
    uint64_t tsc_ticks_per_us_using_hpet()
    {
        hpet* hpet_ {hpet::get()};

        auto micros {std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::seconds(1))};
        auto target {hpet_->main_counter() + hpet_->microseconds_to_ticks(micros.count())};
        uint64_t tsc {rdtsc()};
        while (hpet_->main_counter() < target) {
            cpu_pause();
        }

        return (rdtsc() - tsc) / micros.count();
    }

    cbl::interval dma_region_;
    splitting_buddy dma_pool_ {32};
    uint64_t tsc_ticks_per_micro_second_ {0};
};
