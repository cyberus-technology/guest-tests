// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <toyos/x86/arch.hpp>

namespace cbl
{

    template<typename T>
    class mmio_reg
    {
     private:
        volatile T* const ptr_;

     public:
        explicit mmio_reg(uintptr_t p)
            : ptr_(reinterpret_cast<T*>(p)) {}
        explicit mmio_reg(phy_addr_t p)
            : ptr_(reinterpret_cast<T*>(uintptr_t(p))) {}
        T read() const
        {
            return *ptr_;
        }
        void write(T val)
        {
            *ptr_ = val;
        }

        mmio_reg<T>& operator=(T val)
        {
            write(val);
            return *this;
        }

        mmio_reg<T>& operator=(const mmio_reg<T>&) = delete;
        mmio_reg<T>& operator=(mmio_reg<T>&&) = delete;
    };

}  // namespace cbl
