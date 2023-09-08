/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <stdint.h>

enum
{
   PCI_PORT_REG = 0xcf8,
   PCI_PORT_RESET = 0xcf9,
   PCI_PORT_REG_END = 0xcfb,
   PCI_PORT_DATA = 0xcfc,
   PCI_PORT_DATA_END = 0xcff,

   PCI_NUM_PORTS = 8,
   PCI_NUM_BARS = 6,
   PCI_NUM_BARS_BRIDGE = 2,

   PCI_TYPE_GENERAL = 0,
   PCI_TYPE_PCI_BRIDGE = 1,

   PCI_CLASS_SIMPLE_COMM = 0x07,
   PCI_CLASS_SERIAL = 0x0C,

   PCI_SUBCLASS_SERIAL = 0x00,
   PCI_SUBCLASS_USB = 0x03,

   PCI_PROGIF_XHCI = 0x30
};

static constexpr const size_t MAX_BUSSES{ 256 };           ///< Maximum number of busses in a system.
static constexpr const size_t MAX_DEVICES_PER_BUS{ 32 };   ///< Maximum number of devices on a single bus.
static constexpr const size_t MAX_FUNCTIONS_PER_DEV{ 8 };  ///< Maximum number of functions on a single device.
