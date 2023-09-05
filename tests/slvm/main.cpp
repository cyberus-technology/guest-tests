/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

// This test tests functional aspects of SLVM. Proper resource reclamation is tested in the slvm-resources test.

#include <cstdint>

#include <baretest/baretest.hpp>
#include <slvm/api.h>
#include <slvm_vmcall_backend.hpp>

TEST_CASE(slvm_is_available)
{
    BARETEST_ASSERT(slvm_vmcall_is_present());
}

TEST_CASE(create_works)
{
    slvm_vm_id_t vm_id;

    vm_id = slvm_vmcall_create_vm();

    BARETEST_ASSERT(vm_id >= 0);
    BARETEST_ASSERT(slvm_vmcall_destroy_vm(vm_id) == SLVMAPI_VMCALL_SUCCESS);
}

TEST_CASE(double_create_returns_different_id)
{
    slvm_vm_id_t vm_id_1;
    slvm_vm_id_t vm_id_2;

    vm_id_1 = slvm_vmcall_create_vm();
    vm_id_2 = slvm_vmcall_create_vm();

    BARETEST_ASSERT(vm_id_1 >= 0);
    BARETEST_ASSERT(vm_id_2 >= 0);

    BARETEST_ASSERT(vm_id_1 != vm_id_2);

    BARETEST_ASSERT(slvm_vmcall_destroy_vm(vm_id_1) == SLVMAPI_VMCALL_SUCCESS);
    BARETEST_ASSERT(slvm_vmcall_destroy_vm(vm_id_2) == SLVMAPI_VMCALL_SUCCESS);
}

TEST_CASE(destroy_fails_on_nonexistent_vm)
{
    const slvm_vm_id_t arbitrary_vm_id {123};

    BARETEST_ASSERT(slvm_vmcall_destroy_vm(arbitrary_vm_id) == SLVMAPI_VMCALL_ENOENT);
}

TEST_CASE(query_features_returns_correct_feature_map)
{
    constexpr uint64_t feature_level {0};
    auto res {slvm_vmcall_query_features(feature_level)};

    BARETEST_ASSERT(res.status == SLVMAPI_VMCALL_SUCCESS);

    BARETEST_ASSERT(res.feature_map == slvm_vmcall_backend::supported_features[feature_level]);

    // Check that we do not have a second feature level yet. As soon as we have
    // a second feature level this test should be updated.
    BARETEST_ASSERT((res.feature_map & SVP_FEATURE_LVL0_NEXT_LVL_AVAIL) == 0);
}

BARETEST_RUN;
