/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/x86/arch.hpp>

#include "cbl/math.hpp"
#include "decoder.hpp"
#include "../x86/segmentation.hpp"

namespace decoder
{

x86::gdt_entry parse_gdt(decoder_vcpu& vcpu, uint16_t selector, bool is_64b_system = false);

} // namespace decoder
