/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "x86defs.hpp"

class virtual_msr_access_result
{
    enum class msr_result
    {
        SUCCESS,
        EXCEPTION,
        UNHANDLED,
    };

    // the general ctor, to initialize all values
    explicit virtual_msr_access_result(msr_result res, uint64_t val, x86::exception exc, uint32_t err)
        : res_(res), val_(val), exc_(exc), err_(err)
    {}

    // ctor to only set the msr_result, especially when the msr access was unhandled
    explicit virtual_msr_access_result(msr_result res)
        : virtual_msr_access_result(res, 0, static_cast<x86::exception>(0), 0)
    {}

    // ctor to only set the return value, used when the msr access was successful
    explicit virtual_msr_access_result(uint64_t val)
        : virtual_msr_access_result(msr_result::SUCCESS, val, static_cast<x86::exception>(0), 0)
    {}

    // ctor to only set the exception and error, used then the msr access caused an exception
    explicit virtual_msr_access_result(x86::exception exc, uint32_t err)
        : virtual_msr_access_result(msr_result::EXCEPTION, 0, exc, err)
    {}

public:
    static virtual_msr_access_result access_succeeded_with(uint64_t val) { return virtual_msr_access_result(val); }

    static virtual_msr_access_result access_succeeded() { return virtual_msr_access_result(0); }

    static virtual_msr_access_result access_caused_exception(x86::exception exc, uint32_t err = 0)
    {
        return virtual_msr_access_result(exc, err);
    }

    static virtual_msr_access_result access_was_not_handled()
    {
        return virtual_msr_access_result(msr_result::UNHANDLED);
    }

    bool succeeded() const { return res_ == msr_result::SUCCESS; }
    bool caused_exception() const { return res_ == msr_result::EXCEPTION; }
    bool was_not_handled() const { return res_ == msr_result::UNHANDLED; }

    uint64_t value() const
    {
        assert(succeeded());
        return val_;
    }
    uint64_t value_or_zero() const
    {
        assert(not caused_exception());
        return val_;
    }

    x86::exception exc() const { return exc_; }
    uint32_t error() const { return err_; }

private:
    msr_result res_;
    uint64_t val_;
    x86::exception exc_;
    uint32_t err_;
};
