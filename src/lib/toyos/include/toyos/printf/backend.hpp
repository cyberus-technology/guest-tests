/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

using printf_backend_fn = void (*)(unsigned char);

#ifdef PRINTF_BACKENDS_DISABLED

static inline void add_printf_backend(printf_backend_fn)
{}
static inline void remove_printf_backend(printf_backend_fn)
{}
static inline void remove_all_printf_backends()
{}

#else

/// Set the backend that is used to print a character to the screen.
///
/// Normal printing functions, such as printf, will call this function for every character they need to print. This
/// function does not do anything in a hosted environment as all output goes via the normal libc.
void add_printf_backend(printf_backend_fn backend);
void remove_printf_backend(printf_backend_fn backend);
void remove_all_printf_backends();

#endif
