/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <hedron/cap_sel.hpp>
#include <hedron/event_selectors.hpp>

#include <cstdint>

static constexpr uint64_t OWN_PD_CAP_OFFSET {0};
static constexpr uint64_t ROOTTASK_SERVICE_PT_CAP_OFFSET {1};

static constexpr hedron::cap_sel PD_CAP {hedron::NUMBER_OF_EVENTS + OWN_PD_CAP_OFFSET};

static constexpr hedron::cap_sel SERVICE_CAP {hedron::NUMBER_OF_EVENTS + ROOTTASK_SERVICE_PT_CAP_OFFSET};
