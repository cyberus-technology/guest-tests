/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <hedron/cap_sel.hpp>

#include <utility>

namespace hedron
{
namespace impl
{
/// The non-generic part of cap_range.
///
/// It is split into a non-generic class to avoid code-bloat.
struct cap_range_base
{
    static inline const hedron::cap_sel invalid_cap_sel {static_cast<hedron::cap_sel>(-1)};

    /// The start of the region that is managed by this range.
    hedron::cap_sel start_ {invalid_cap_sel};

    //// True, if this range need to be deallocated.
    bool valid_ {false};

    cap_range_base() = default;

    /// We cannot handle assignments, because we cannot free capabilities.
    cap_range_base& operator=(cap_range_base const& other) = delete;
    cap_range_base(cap_range_base const& other) = delete;

    cap_range_base& operator=(cap_range_base&& other)
    {
        start_ = other.start_;
        valid_ = other.valid_;

        other.valid_ = false;

        return *this;
    }

    cap_range_base(cap_range_base&& other) : start_ {other.start_}, valid_ {other.valid_} { other.valid_ = false; }

    explicit cap_range_base(hedron::cap_sel start) : start_ {start}, valid_ {true} {}
};

} // namespace impl

/// A RAII-wrapper around a capability selector range.
///
/// This wrapper works much like unique_ptr in that it can be moved, but not copied. When an object goes out of scope
/// the corresponding capability will be freed as well.
template <size_t SIZE, typename ALLOCATOR, typename REVOKE>
class generic_cap_range
    : private ALLOCATOR
    , private REVOKE
{
    static_assert(SIZE > 0);

    impl::cap_range_base base;

    struct invalid_tag
    {};
    generic_cap_range(invalid_tag) {}

public:
    /// Return an invalid (empty) capability range.
    static auto invalid() { return generic_cap_range(invalid_tag()); }

    /// Return a bare capability selector that has no ownership tracking.
    hedron::cap_sel get() const { return base.start_; }

    /// Return the number of capabilities allocated.
    static constexpr size_t size() { return SIZE; }

    generic_cap_range& operator=(generic_cap_range&& other)
    {
        base = std::move(other.base);

        return *this;
    }

    generic_cap_range& operator=(generic_cap_range const& other) = delete;
    generic_cap_range(generic_cap_range const& other) = delete;

    generic_cap_range() : base {this->allocate(SIZE)} {}
    generic_cap_range(generic_cap_range&& other) : base {std::move(other.base)} {}

    ~generic_cap_range()
    {
        if (base.valid_) {
            this->revoke(get(), SIZE);
            this->free(get(), SIZE);
        }
    }
};

} // namespace hedron
