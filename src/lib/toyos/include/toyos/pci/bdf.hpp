/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "toyos/pci/pci.hpp"

#include "toyos/util/math.hpp"
#include <toyos/x86/arch.hpp>

#include <cstdint>

namespace pci
{

    /// BDF (bus, device, function) descriptor.
    struct bdf_t
    {
     private:
        enum
        {
            BUS_WIDTH = 8,
            BUS_SHIFT = 8,
            DEVICE_WIDTH = 5,
            DEVICE_SHIFT = 3,
            FUNCTION_WIDTH = 3,
            FUNCTION_SHIFT = 0,

            MMCFG_OFFSET = PAGE_BITS,
        };

     public:
        uint8_t bus;       ///< The bus component.
        uint8_t device;    ///< The device component.
        uint8_t function;  ///< The function component.

        /// Constructs an instance for given individual components.
        bdf_t(uint8_t bus_, uint8_t device_, uint8_t function_)
            : bus(bus_), device(device_), function(function_) {}

        /// Constructs an instance for a given RID.
        explicit bdf_t(uint16_t rid)
            : bus(rid >> BUS_SHIFT), device((rid >> DEVICE_SHIFT) & math::mask(DEVICE_WIDTH)), function((rid >> FUNCTION_SHIFT) & math::mask(FUNCTION_WIDTH))
        {}

        /// Constructs an instance from an offset into the MMCONFIG space.
        explicit bdf_t(uintptr_t offset)
            : bus((offset >> (MMCFG_OFFSET + BUS_SHIFT)) & math::mask(BUS_WIDTH)), device((offset >> (MMCFG_OFFSET + DEVICE_SHIFT)) & math::mask(DEVICE_WIDTH)), function((offset >> (MMCFG_OFFSET + FUNCTION_SHIFT)) & math::mask(FUNCTION_WIDTH))
        {}

        /// Returns a requestor ID (16-bit value encoding the BDF).
        uint16_t rid() const
        {
            return uint16_t(bus) << BUS_SHIFT | ((device & math::mask(DEVICE_WIDTH)) << DEVICE_SHIFT)
                   | ((function & math::mask(FUNCTION_WIDTH)) << FUNCTION_SHIFT);
        }

        /// Compares this BDF to o by comparing bus, device, and function, individually.
        bool operator==(const bdf_t& o) const
        {
            return o.bus == bus and o.device == device and o.function == function;
        }

        /// Compares this BDF to o, negating the equals operator.
        bool operator!=(const bdf_t& o) const
        {
            return not operator==(o);
        }

        /// Computes the next BDF by incrementing the function and handling overflows.
        bdf_t operator++()
        {
            function = (function + 1) % MAX_FUNCTIONS_PER_DEV;
            device = (device + (function == 0 ? 1 : 0)) % MAX_DEVICES_PER_BUS;
            bus += device == 0 and function == 0 ? 1 : 0;
            return *this;
        }
    };

}  // namespace pci
