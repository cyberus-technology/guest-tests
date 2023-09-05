/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <hedron/sm.hpp>
#include <hedron/utcb.hpp>

#include "slave_vcpu_interface.hpp"

/// This interface will some day hold the entire functionality
/// that is required to interface the VCPU with Hedron.
/// Right now, it only contains functions that have been
/// re-worked already.
/// The interface will grow as we clean up more interfaces.
class vcpu_hedron_interface
{
public:
    virtual ~vcpu_hedron_interface() {}

    /// Add a slave vCPU companion to this vCPU.
    ///
    /// vcpu_hedron_interface will retain a weak reference to this slave vCPU. This is needed to force execution from
    /// slave vCPUs into the VMM when an interrupt needs to be injected.
    virtual void add_slave_vcpu(const slave_vcpu_interface::weak_ptr& slave_vcpu) = 0;

    /// Start the vcpu. This should live in the hypervisor interface once available.
    virtual void start() = 0;

    virtual hedron::utcb& get_state() const = 0;

    /// Can be used to force a VM exit or wakeup a CPU from HLT.
    virtual void recall() = 0;

    /// Used by the recall handler to confirm the recall request.
    virtual void ack_recall() = 0;

    /// First function to be called on every vm exit.
    virtual void portal_prologue() = 0;
    /// Last function to be called on every vm exit.
    virtual void portal_epilogue() = 0;

    /// Defines which part of the architectural state should be transferred to the vcpu.
    virtual void set_mtd_out(hedron::mtd_bits::field) = 0;

    /// Checks whether the message transfer descriptor contains a certain field.
    virtual bool mtd_out_contains(hedron::mtd_bits::field) = 0;
};
