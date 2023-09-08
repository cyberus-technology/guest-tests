/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"

static_assert(STACK_SIZE_PHYS >= PAGE_SIZE, "invalid stack configuration");
static_assert(STACK_SIZE_PHYS < STACK_SIZE_VIRT, "invalid stack configuration");

static constexpr uint16_t SERIAL_PORT_DEFAULT{ 0x3f8 };
static constexpr uint16_t SERIAL_IRQ_DEFAULT{ 0x4 };
static constexpr unsigned SERIAL_BAUD{ 115200 };
static constexpr unsigned SERIAL_PORT_RANGE{ 8 };

// The maximum supported cpus.
static constexpr uint64_t MAX_CPUS{ 128 };
