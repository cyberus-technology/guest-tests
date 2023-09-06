/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include <debugport_interface.h>
#include <x86asm.hpp>

#include <cbl/baretest/baretest.hpp>
#include <cbl/statistics.hpp>

#ifdef RELEASE
TEST_CASE(debugport_should_never_exist_in_release_build)
{
    BARETEST_VERIFY(debugport_present() == false);
}
#else
TEST_CASE(debugport_should_exist_in_supernova_run_on_default_builds)
{
    BARETEST_ASSERT(debugport_present());
}
#endif

BARETEST_RUN;
