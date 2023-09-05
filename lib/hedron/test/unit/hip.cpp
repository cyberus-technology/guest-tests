/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <algorithm>
#include <gtest/gtest.h>
#include <hedron/hip.hpp>
#include <list>
#include <math.hpp>
#include <new>

static hedron::hip::data& create_hip_data(void* backing, size_t backing_size, size_t cpus, size_t mems)
{
    size_t hip_size {sizeof(hedron::hip::data) + cpus * sizeof(hedron::hip::cpu_descriptor)
                     + mems * sizeof(hedron::hip::mem_descriptor)};

    ASSERT(backing_size >= hip_size, "Need larger backing store to create HIP");

    auto raw_data {new (backing) hedron::hip::data {}};

    raw_data->cpu_.size_ = sizeof(hedron::hip::cpu_descriptor);
    raw_data->mem_.size_ = sizeof(hedron::hip::mem_descriptor);
    raw_data->cpu_.offset_ = sizeof(hedron::hip::data);
    raw_data->mem_.offset_ = sizeof(hedron::hip::data) + cpus * raw_data->cpu_.size_;

    raw_data->length_ = sizeof(hedron::hip::data) + cpus * raw_data->cpu_.size_ + mems * raw_data->mem_.size_;

    return *raw_data;
}

struct testing_hip_data
{
    alignas(hedron::hip::data) char hip_backing_[PAGE_SIZE];
    hedron::hip::data& hip_data_;

    testing_hip_data(size_t cpu_descriptors, size_t mem_descriptors)
        : hip_data_ {create_hip_data(hip_backing_, sizeof(hip_backing_), cpu_descriptors, mem_descriptors)}
    {}
};

class testing_hip
    : private testing_hip_data
    , public hedron::hip
{
private:
    uint16_t valid_checksum() const
    {
        hip_data_.checksum_ = 0;
        return math::checksum<uint16_t>(&hip_, hip_.length_);
    }

public:
    testing_hip(size_t cpu_descriptors = 0, size_t mem_descriptors = 0)
        : testing_hip_data {cpu_descriptors, mem_descriptors}, hip(hip_data_)
    {}

    void invalidate_signature() { hip_data_.signature_ = SIGNATURE_VALID + 1; }

    void invalidate_checksum()
    {
        uint16_t checksum = valid_checksum();
        hip_data_.checksum_ = checksum + 1;
    }

    void make_valid()
    {
        hip_data_.signature_ = SIGNATURE_VALID;
        hip_data_.checksum_ = valid_checksum();
    }

    void set_version(uint32_t ver) { hip_data_.api_version_ = ver; }

    span<mem_descriptor> mem_descriptors_mutable() const
    {
        return {const_cast<mem_descriptor*>(first_mem()), num_mem_descriptors()};
    }
};

TEST(hip, hip_with_valid_signature_and_invalid_checksum_is_invalid)
{
    testing_hip hip;
    hip.invalidate_checksum();
    ASSERT_FALSE(hip.valid());
}

TEST(hip, hip_with_invalid_signature_is_invalid)
{
    testing_hip hip;
    hip.invalidate_signature();
    ASSERT_FALSE(hip.valid());
}

TEST(hip, hip_with_valid_signature_and_valid_checksum_is_valid)
{
    testing_hip hip;
    hip.make_valid();
    ASSERT_TRUE(hip.valid());
}

TEST(hip, hip_reports_correct_amount_of_cpus)
{
    size_t num = 3;
    testing_hip hip(num);
    ASSERT_EQ(num, hip.num_cpu_descriptors());
}

TEST(hip, hip_descriptors_can_be_used_with_modern_for_loops)
{
    // XXX: This test case is a big fail!

    constexpr size_t descriptor_count = 3;
    size_t num = descriptor_count;

    testing_hip hip(num, num);
    for ([[maybe_unused]] const auto& e : hip.mem_descriptors()) {
        num--;
    }

    for ([[maybe_unused]] const auto& e : hip.cpu_descriptors()) {
        num++;
    }

    ASSERT_EQ(descriptor_count, num);
}

TEST(hip, memory_region_merging_should_work_as_expected)
{
    using mem_descriptor = hedron::hip::mem_descriptor;
    using mem_type = mem_descriptor::mem_type;
    std::list<mem_descriptor> descs {
        {0x0, 0x4000, mem_type::AVAILABLE, 0},
        {0x5000, 0x3000, mem_type::AVAILABLE, 0},
        {0x2000, 0x5000, mem_type::RESERVED, 0},
    };

    testing_hip hip(0, 3);
    for (auto& entry : hip.mem_descriptors_mutable()) {
        entry = descs.back();
        descs.pop_back();
    }

    auto merged = hip.merged_mem_descriptors();

    // We use RESERVED as default type for the list, i.e. all regions that are not
    // occupied by an interval that we've inserted will be using this type. This is
    // why the final list contains the entire rest of the address space at the end.
    std::list<mem_descriptor> expect {
        {0x0, 0x2000, mem_type::AVAILABLE, 0},
        {0x2000, 0x5000, mem_type::RESERVED, 0},
        {0x7000, 0x1000, mem_type::AVAILABLE, 0},
        {0x8000, 0xffffffffffff7fff, mem_type::RESERVED, 0},
    };
    for (const auto& elem : merged) {
        auto pair {merged.find(elem.first)};
        auto expect_val {expect.front()};

        ASSERT_EQ(pair->first.a, expect_val.address);
        ASSERT_EQ(pair->first.b, expect_val.address + expect_val.size);
        ASSERT_EQ(pair->second, expect_val.type);
        expect.pop_front();
    }
}

TEST(hip, version_number_detects_mismatch_correctly)
{
    testing_hip hip;

    // Exact match is fine
    hip.set_version(hedron::hip::API_VERSION);
    ASSERT_TRUE(hip.version_ok());

    // Newer minor version is fine
    hip.set_version(hedron::hip::API_VERSION + 1);
    ASSERT_TRUE(hip.version_ok());

    // Older minor version is bad
    hip.set_version(hedron::hip::API_VERSION - 1);
    ASSERT_FALSE(hip.version_ok());

    // Different major version is bad
    hip.set_version(hedron::hip::API_VERSION + 1000);
    ASSERT_FALSE(hip.version_ok());

    hip.set_version(hedron::hip::API_VERSION - 1000);
    ASSERT_FALSE(hip.version_ok());

    hip.set_version(hedron::hip::API_VERSION + 1001);
    ASSERT_FALSE(hip.version_ok());
}
