/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <hedron/cap_sel.hpp>

#include "generic_cap_range.hpp"

namespace hedron
{

// A RAII-wrapper around protection domains.
template <typename CREATE, typename CAPABILITY>
class generic_pd : private CREATE
{
    static_assert(CAPABILITY::size() == 1, "Cannot create a PD from a capability range");

    /// The capability selector for the protection domain.
    CAPABILITY pd_sel;

public:
    /// Return the capability selector for the protection domain.
    hedron::cap_sel get() const { return pd_sel.get(); }

    /// Create a new protection domain.
    generic_pd() { this->create_pd(pd_sel.get()); }
};

} // namespace hedron
