/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <functional>
#include <memory>

/// Interface for a slave vCPU that is operated by an in-guest VMM.
///
/// Slave vCPUs can run instead of the "normal" vCPUs of the passthrough guest. Their execution is under full control of
/// the passthrough guest, i.e. they are not scheduled concurrently with normal vCPUs.
class slave_vcpu_interface
{
public:
    /// A non-owning pointer to the slave vCPU.
    using weak_ptr = std::weak_ptr<slave_vcpu_interface>;

    /// A function type to register vCPUs with.
    ///
    /// Whenever slave vCPUs are created via the VMCALL interface they are registered to the containing VM using this
    /// interface.
    using register_fn = std::function<void(unsigned pcpu, const weak_ptr& slave_vcpu)>;

    /// Force a slave vCPU to stop executing and return to the VMM as soon as possible.
    ///
    /// This functionality is necessary to deliver interrupts.
    virtual void recall() = 0;

    virtual ~slave_vcpu_interface() {}
};
