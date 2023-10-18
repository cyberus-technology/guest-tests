/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "virtual_msr_access_result.hpp"

#include <functional>

#include <assert.h>
#include <stdint.h>

/// A container for a model specific register (MSR).
///
/// The MSR stores its contents and provides read and write access.
/// Hence, it behaves as a shadow by default.
/// This behaviour can be changed by passing custom read and write handlers.
class virtual_msr
{
    using res_t = virtual_msr_access_result;

 public:
    // These functions are called upon MSR access with a reference to the
    // MSR contents stored in a virtual_msr.
    // The msr_index and shadow are passed along so that multiple MSRs may share a handler.
    using wr_func_t = std::function<res_t(uint32_t msr_index, uint64_t new_value, uint64_t& shadow)>;
    using rd_func_t = std::function<res_t(uint32_t msr_index, uint64_t& shadow)>;

    /**
     * \brief Create a new MSR.
     *
     * \param idx MSR index
     * \param init_val initial contents
     * \param rd_func function invoked when reading
     * \param wr_func function invoked when writing
     */
    virtual_msr(uint32_t idx, uint64_t init_val = 0, const rd_func_t& rd_func = shadow_read, const wr_func_t& wr_func = shadow_write);

    res_t read()
    {
        return rd_func(idx, value);
    }
    res_t write(uint64_t val)
    {
        return wr_func(idx, val, value);
    }

    uint32_t index() const
    {
        return idx;
    }

    /**
     * \brief Write a new value directly to the MSR.
     *
     * \param MSR index
     * \param new_val new contents
     * \param shadow shadow
     *
     * \return success - can not fail
     */
    static res_t shadow_write(uint32_t, uint64_t new_val, uint64_t& shadow)
    {
        shadow = new_val;
        return res_t::access_succeeded();
    };

    /**
     * \brief Return the shadowed MSR value.
     *
     * \param MSR index
     * \param shadow shadow
     *
     * \return success with the current value - can not fail
     */
    static res_t shadow_read(uint32_t, const uint64_t& shadow)
    {
        return res_t::access_succeeded_with(shadow);
    };

 private:
    /// MSR index.
    uint32_t idx;

    /// Read callback.
    rd_func_t rd_func;

    /// Write callback.
    wr_func_t wr_func;

    /// Current value.
    uint64_t value;
};
