// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"

static_assert(STACK_SIZE_PHYS >= PAGE_SIZE, "invalid stack configuration");
static_assert(STACK_SIZE_PHYS < STACK_SIZE_VIRT, "invalid stack configuration");

static constexpr uint16_t SERIAL_PORT_DEFAULT{ 0x3f8 };
static constexpr uint16_t SERIAL_IRQ_DEFAULT{ 0x4 };
static constexpr unsigned SERIAL_BAUD{ 115200 };
