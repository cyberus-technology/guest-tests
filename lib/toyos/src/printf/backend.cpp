/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/printf/backend.hpp>
#include <toyos/printf/xprintf.h>

#include <array>

// The library could be used in context where dynamic memory allocation
// is unsupported, e.g. in early boot stages.
// Hence, the number of possible backends is fixed.
static constexpr size_t MAX_BACKENDS {3};

static std::array<printf_backend_fn, MAX_BACKENDS> backends;

void print_to_all_backends(unsigned char c)
{
    for (const auto& print_fn : backends) {
        if (print_fn != nullptr) {
            print_fn(c);
        }
    }
}

void add_printf_backend(printf_backend_fn backend)
{
    auto free_slot {std::find(backends.begin(), backends.end(), nullptr)};
    if (free_slot == backends.end()) {
        // no more space - alert on already registered backends
        puts("maximum number of printf backends already registered");
        __builtin_trap();
    }
    *free_slot = backend;

    // backends are expected to be seldom added/removed, so re-registering
    // on every call ain't harmful
    xdev_out(print_to_all_backends);
}

void remove_printf_backend(printf_backend_fn backend)
{
    auto existing = std::find(backends.begin(), backends.end(), backend);
    if (existing != backends.end()) {
        *existing = nullptr;
    }
}

void remove_all_printf_backends()
{
    backends.fill(nullptr);
}
