/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <assert.h>

#include <algorithm>
#include <array>
#include <string>

#include <compiler.hpp>
#include "cbl/math.hpp"

static constexpr uint8_t USB_STRING_MAX_LENGTH {62};
static constexpr uint8_t USB_STRING_TYPE {3};

struct __PACKED__ usb_string_t
{
    void set(const std::u16string& str)
    {
        // Note: Lengths are multiplied by two because of UTF-16
        const auto len = str.length() * 2;
        assert(len <= USB_STRING_MAX_LENGTH);

        std::copy(str.begin(), str.end(), data.begin());
        type = USB_STRING_TYPE;
        length = len + 2;
    }

    uint8_t length;
    uint8_t type;

    std::array<uint16_t, USB_STRING_MAX_LENGTH / 2> data;
};
