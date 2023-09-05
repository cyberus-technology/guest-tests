/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <optional>
#include <string>

#include "console.hpp"
#include <mutex.hpp>

template <typename OUT_FN>
class buffered_console : public console
{
public:
    buffered_console(OUT_FN out_fn_) : out_fn(out_fn_) {}

    virtual void puts(const std::string& str) override { puts_default(str); }

    virtual void putc(char c) override
    {
        mutex::guard _(mtx);

        buffer += c;
        if (c == '\n') {
            out_fn(buffer);
            buffer.clear();
        }
    }

private:
    OUT_FN out_fn;
    std::string buffer;
    mutex mtx;
};
