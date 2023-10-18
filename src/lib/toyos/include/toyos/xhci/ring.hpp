/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "trb.hpp"

#include "atomic"
#include "cstring"
#include "functional"

#include "toyos/util/cast_helpers.hpp"

#include "atomic"
#include "cstring"
#include "functional"

/**
 * A class representing the ring buffers employed
 * by the hardware.
 *
 * The ring buffer items have a "cycle" bit, which identifies work items
 * that can be consumed. A consumer will fetch items as long as the cycle
 * bit in the entry matches the current cycle state of the ring.
 *
 * When the cycle bit does not match, this location is considered the current
 * enqueue pointer of the producer.
 *
 * The ring cycle state wraps when a wrap-around occurs (in an event ring)
 * or a Link TRB with the "toggle cycle" flag is encountered (in a transfer ring).
 *
 * The Link TRB should be considered as a standalone entry that does not
 * describe a transfer, but signals the next entry to be accessed. Therefore,
 * the Link TRB needs to have a cycle bit that matches the ring state,
 * so it is dequeued by the consumer.
 *
 * Theoretically, Link TRBs can point to a different ring segment to form
 * a non-contiguous ring. This implementation does NOT support that, Link TRBs
 * always point to the beginning of the ring.
 *
 * Note that these buffers are designed to be used uni-directional, i.e.,
 * the system software is either the producer or the consumer, never both.
 * This is due to the automatic cycle bit toggling upon enqueue/dequeue operations.
 *
 * Transfer rings are used as a producer by the software, and as consumer by the controller.
 *
 * Event rings are the opposite, the hardware enqueues events, the software consumes them.
 *
 * The intended use for HAS_LINK is to distinguish between the two:
 *  - Event rings do not have a Link TRB, but instead wrap automatically.
 *  - Transfer rings are dynamically sized, therefore a Link TRB is needed.
 *
 * \tparam SIZE     The number of entries in this buffer.
 * \tparam HAS_LINK Whether or not the ring is formed using a Link TRB.
 */
template<size_t SIZE, bool HAS_LINK = true>
class trb_ring
{
 public:
    explicit trb_ring(std::array<trb, SIZE>& buffer, uintptr_t dma_addr = 0)
        : ring_(buffer), dma_addr_(dma_addr ?: ptr_to_num(buffer.data()))
    {
        initialize();
    }

    /// Initialize the ring (can also be called after a xHC reset)
    void initialize()
    {
        memset(ring_.data(), 0, SIZE * sizeof(trb));

        enqueue_ptr = ptr_to_num(ring_.data());
        dequeue_ptr = enqueue_ptr.load();

        // Prepare Link TRB
        if (HAS_LINK) {
            auto& link = ring_.back();
            link.toggle(true);
            link.type(trb_link::TYPE);
            link.buffer = dma_addr_;
        }

        cycle = true;
    }

    /// \return True, if the queue is full, False otherwise.
    bool full() const
    {
        return check_enqueue_increment(enqueue_ptr) == dequeue_ptr;
    }

    /// \return True, if the queue is empty, False otherwise.
    bool empty() const
    {
        return enqueue_ptr == dequeue_ptr;
    }

    /// \return The index of a given pointer to a TRB (linear address)
    size_t index(uintptr_t ptr) const
    {
        return (ptr - ptr_to_num(ring_.data())) / sizeof(trb);
    }

    /**
     * Enqueue a TRB.
     *
     * This function returns a reference to the currently free
     * entry, and advances the enqueue pointer.
     *
     * It automatically follows Link TRBs and sets the appropriate cycle bit in them.
     *
     * The TRB will be in an inactive state so the appropriate values can be set,
     * before committing them to the queue using their commit() method. This effectively
     * toggles the cycle bit, marking them as ready to be dequeued.
     *
     * \return A reference to the TRB entry to be filled.
     */
    trb& enqueue()
    {
        PANIC_ON(full(), "Trying to enqueue in full queue!");
        bool cycle_old = cycle;
        auto enqueue_ptr_old = increment(std::ref(enqueue_ptr), cycle);
        auto& ret = ring_[index(enqueue_ptr_old)];
        ret.cycle(not cycle);
        if (cycle_old != cycle and HAS_LINK) {
            auto& link = ring_.back();
            link.commit();
        }
        return ret;
    }

    /**
     * Dequeue a TRB.
     *
     * This function returns a reference to the TRB that is currently
     * in line for processing.
     *
     * It also updates the dequeue pointer and manages the cycle bit
     * accordingly.
     *
     * \return A reference to the TRB entry to process.
     */
    trb& dequeue()
    {
        PANIC_ON(empty(), "Trying to dequeue from empty queue!");
        auto dequeue_ptr_old = increment(std::ref(dequeue_ptr), cycle);
        return ring_[index(dequeue_ptr_old)];
    }

    /**
     * Update the enqueue pointer.
     *
     * This function is used by a consumer to update the
     * current state of the enqueue pointer.
     *
     * This is done by advancing the enqueue pointer until
     * a cycle bit transition is encountered.
     *
     * It leaves this ring's cycle state untouched.
     */
    void update_enqueue_ptr()
    {
        bool cycle_ = cycle;
        while (ring_[index(enqueue_ptr)].cycle() == cycle_) {
            increment(std::ref(enqueue_ptr), cycle_);
        }
    }

    /// Get the current dequeue pointer.
    uintptr_t get_dequeue_ptr() const
    {
        return dequeue_ptr;
    }

    /**
     * Update the dequeue pointer.
     *
     * This function is used by a produces to update
     * the current dequeue pointer of the consumer (i.e., the xHC).
     *
     * This is done by taking the TRB address reported by the transfer event
     * and translating it into a linear address. Afterwards, the pointer is
     * incremented, because it should point to the next item to be processed
     * by the consumer.
     */
    void update_dequeue_ptr(uintptr_t new_ptr_dma)
    {
        dequeue_ptr = ptr_to_num(ring_.data()) + (new_ptr_dma - dma_addr_);
        bool cycle_ = cycle;
        increment(std::ref(dequeue_ptr), cycle_);
    }

    /// Convenience function to dump the current state.
    void dump() const
    {
        info("---- {s} RING DUMP ---- ({}), cycle {}", not HAS_LINK ? "EVENT" : "XFER ", ring_.data(), cycle);

        for (const auto& trb : ring_) {
            auto ptr = ptr_to_num(&trb);
            info("[{02}] {#02x} BUF {#x} CYC {} {s} {s}{s}", index(ptr), trb.type(), trb.buffer, trb.cycle(), trb.type() == trb_link::TYPE ? "LINK" : "", ptr == enqueue_ptr ? " <-- ENQ" : "", ptr == dequeue_ptr ? " <-- DEQ" : "");
        }

        info("---------------------------");
    }

 private:
    // Pretent incrementing the enqueue pointer without
    // touching the cycle, while following Link TRBs.
    uintptr_t check_enqueue_increment(uintptr_t ptr) const
    {
        const auto& cur = ring_[index(ptr)];
        if (cur.type() == trb_link::TYPE) {
            return ptr_to_num(ring_.data());
        }

        auto ret = ptr + sizeof(trb);
        if (index(ret) == SIZE) {
            ret = ptr_to_num(ring_.data());
        }
        return ret;
    }

    // Increment the referenced pointer, while adjusting
    // the referenced cycle bit accordingly.
    template<typename T>
    uintptr_t increment(std::reference_wrapper<T> ptr_, bool& cycle_) const
    {
        auto& ptr = ptr_.get();

        // If there is a link TRB, this wraps first
        const auto& cur = ring_[index(ptr)];
        if (cur.type() == trb_link::TYPE) {
            if (cur.toggle()) {
                cycle_ = not cycle_;
            }
            ptr = ptr_to_num(ring_.data());
        }

        uintptr_t old = ptr;
        ptr += sizeof(trb);

        // Event rings just wrap and toggle automatically.
        if (index(ptr) == SIZE) {
            ptr = ptr_to_num(ring_.data());
            cycle_ = not cycle_;
            return old;
        }

        return old;
    }

    std::array<trb, SIZE>& ring_;
    uintptr_t dma_addr_{ 0 };

    std::atomic<uintptr_t> enqueue_ptr{ 0 }, dequeue_ptr{ 0 };
    bool cycle{ true };
};

struct __PACKED__ event_ring_segment_table_entry
{
    uint64_t base_address;
    uint32_t size;
    uint32_t reservedz;

    uint8_t padding[64 - 16];
};
