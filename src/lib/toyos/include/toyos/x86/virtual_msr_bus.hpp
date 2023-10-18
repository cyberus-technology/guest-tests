/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "virtual_msr.hpp"
#include "virtual_msr_access_result.hpp"

#include <map>

/// A simple bus abstraction that can handle multiple MSRs.
class virtual_msr_bus
{
 public:
    /**
     * \brief Add an MSR to the bus.
     *
     * The operation is not carried out if an MSR with the given index
     * is already registered.
     *
     * \param msr A copy of the MSR is added to the bus.
     *
     * \return Whether the MSR has been added.
     */
    bool add(const virtual_msr& msr);

    /**
     * \brief Read the value of the MSR with the given index.
     *
     * \param idx MSR index
     *
     * \return Access result. Might be unhandled if no MSR for the given index
     *         has been registered.
     */
    virtual_msr_access_result read(uint32_t idx);

    /**
     * \brief Write to the MSR with the given index.
     *
     * \param idx MSR index
     * \param val MSR value
     *
     * \return Access result. Might be unhandled if no MSR for the given index
     *         has been registered.
     */
    virtual_msr_access_result write(uint32_t idx, uint64_t val);

 private:
    std::map<uint32_t, virtual_msr> msrs;
};
