// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

namespace cbl
{

    class buffer_view
    {
     private:
        void* buf_;
        size_t sz_;

     public:
        buffer_view(void* buf, size_t sz)
            : buf_(buf), sz_(sz) {}

        template<typename T>
        T read(off_t offset = 0) const
        {
            ASSERT(sizeof(T) + offset <= sz_, "Read from buffer {} with incorrect size. Expected: {#x} Got: {#x}", buf_, sizeof(T), sz_);
            return *reinterpret_cast<T*>(reinterpret_cast<char*>(buf_) + offset);
        }

        template<typename T>
        void write(T val, off_t offset = 0)
        {
            ASSERT(sizeof(T) + offset <= sz_, "Write to buffer {} with incorrect size. Expected: {#x} Got: {#x}", buf_, sizeof(T), sz_);
            *reinterpret_cast<T*>(reinterpret_cast<char*>(buf_) + offset) = val;
        }

        size_t bytes() const
        {
            return sz_;
        }
    };

}  // namespace cbl
