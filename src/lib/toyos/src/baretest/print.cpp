// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>
#include <toyos/baretest/print.hpp>

void print(uint16_t v)
{
    printf("%#x", v);
}

void print(uint32_t v)
{
    printf("%#x", v);
}

void print(uint64_t v)
{
    printf("%#lx", v);
}

void print(bool v)
{
    printf("%s", v ? "true" : "false");
}
