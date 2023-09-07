/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "stddef.h"

namespace baretest
{
void success(const char* name);
void failure(const char* name);
void benchmark(const char* name, long int value, const char* unit);
void skip();
void hello(size_t test_count);
void goodbye();
} // namespace baretest
