/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

static constexpr size_t BINARY_MULTIPLY{ 1024 };

constexpr uint64_t operator"" _KiB(unsigned long long int value)
{
   return value * BINARY_MULTIPLY;
}

constexpr uint64_t operator"" _MiB(unsigned long long int value)
{
   return value * BINARY_MULTIPLY * 1_KiB;
}

constexpr uint64_t operator"" _GiB(unsigned long long int value)
{
   return value * BINARY_MULTIPLY * 1_MiB;
}

constexpr uint64_t operator"" _TiB(unsigned long long int value)
{
   return value * BINARY_MULTIPLY * 1_GiB;
}

constexpr uint64_t operator"" _PiB(unsigned long long int value)
{
   return value * BINARY_MULTIPLY * 1_TiB;
}

constexpr uint64_t operator"" _EiB(unsigned long long int value)
{
   return value * BINARY_MULTIPLY * 1_PiB;
}
