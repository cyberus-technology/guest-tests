/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include "virtual_msr.hpp"

virtual_msr::virtual_msr(uint32_t idx_, uint64_t init_val, const rd_func_t& rd_func_, const wr_func_t& wr_func_)
    : idx(idx_), rd_func(rd_func_), wr_func(wr_func_)
{
    value = init_val;
}
