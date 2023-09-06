/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <segmentation.hpp>

#include <arch.hpp>
#include <math/math.hpp>

#include "decoder.hpp"

namespace decoder
{

x86::gdt_entry parse_gdt(decoder_vcpu& vcpu, uint16_t selector, bool is_64b_system = false);

} // namespace decoder
