/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <arch.h>
#include <cbl/interval.hpp>
#include <math.hpp>
#include <trace.hpp>

#include <atomic>
#include <optional>

/**
 * This class represents an MMIO BAR region by its address and size.
 * The BAR information can be read and written atomically without locking.
 * The information is encoded into 8 bytes by leveraging the fact that BAR
 * addresses are naturally aligned. This class requires an alignment of at
 * least 4k. See following graphic for the encoding of the values:
 *
 * |  63 ... 12  |  11 ... 0  |
 * |--------------------------|
 * | BAR address | size order |
 */
class mmio_bar_info
{
public:
    static constexpr uint64_t INVALID_BAR_INFO {~0ul};

    mmio_bar_info() = default;

    // Given a PAGE aligned address and a power of two size, this functions
    // encodes both values into a single 64 bit value by placing the order of
    // the size in the lower free 12 bits of the address.
    static std::optional<uint64_t> encode(uint64_t address, uint64_t size)
    {
        if (not math::is_aligned(address, PAGE_BITS)) {
            warning("BAR address is not aligned to 4k: {#x}", address);
            return {};
        }

        if (not math::is_power_of_2(size)) {
            warning("BAR size is not a power of two: {#x}", size);
            return {};
        }

        return address | math::order_max(size);
    }

    // Decode a value (that was previously encoded with encode) into a
    // cbl::interval.
    static cbl::interval decode(uint64_t value)
    {
        return cbl::interval::from_order(value & ~math::mask(PAGE_BITS), value & math::mask(PAGE_BITS));
    }

    /**
     * Set the BAR to the given address and size. Note that address must be
     * a 4k page-aligned address and size must be a power of two.
     *
     * \param address 4k page aligned address of the beginning of the BAR
     * \param size Size of the BAR that must be a power of two
     */
    [[nodiscard]] bool set(uint64_t address, uint64_t size)
    {
        auto opt_val {encode(address, size)};

        if (not opt_val) {
            return false;
        }

        bar_info.store(*opt_val);

        return true;
    }

    [[nodiscard]] bool set(const cbl::interval& region) { return set(region.a, region.size()); }

    /**
     * Retrieve the address and the size of the BAR.
     *
     * \returns optional interval describing the mmio region
     */
    std::optional<cbl::interval> get() const
    {
        const auto val {bar_info.load()};

        if (val == INVALID_BAR_INFO) {
            return {};
        }

        return decode(val);
    }

    /**
     * Reset bar info to initial state.
     */
    void reset() { bar_info = INVALID_BAR_INFO; }

private:
    std::atomic<uint64_t> bar_info {INVALID_BAR_INFO};
};
