/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <hedron/crd.hpp>
#include <hedron/syscalls.hpp>

namespace hedron
{

class typed_item
{
public:
    enum delegate_item_definitions : uint64_t
    {
        DELEGATE_ITEM = 1ul << 0,
        MAP_NOT_HOST = 1ul << 8,
        MAP_DMA = 1ul << 9,
        MAP_GUEST = 1ul << 10,
        HYPERVISOR = 1ul << 11,
        HOTSPOT_SHIFT = 12,
    };

    static typed_item delegate_item(const crd& item, bool host, bool guest, bool dma, bool hypervisor)
    {
        return {item, (item.base() << HOTSPOT_SHIFT) | MAP_NOT_HOST * (not host) | MAP_DMA * dma | MAP_GUEST * guest
                          | HYPERVISOR * hypervisor | DELEGATE_ITEM};
    }

    static typed_item delegate_item(const crd& item, space spaces, bool hypervisor)
    {
        bool host {!!(spaces & hedron::space::HOST)};
        bool guest {!!(spaces & hedron::space::GUEST)};
        bool dma {!!(spaces & hedron::space::DMA)};
        return delegate_item(item, host, guest, dma, hypervisor);
    }

    static typed_item translate_item(const crd& item) { return {item, 0ul}; }

    uint64_t get_value() const { return item.value(); }
    uint64_t get_flags() const { return flags; }
    const crd& get_crd() const { return item; }

private:
    typed_item(const crd& item_, uint64_t flags_) : item(item_), flags(flags_) {}

    crd item;
    uint64_t flags;
};

} // namespace hedron
