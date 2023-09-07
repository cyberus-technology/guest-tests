/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/printf/backend.hpp>
#include <toyos/xhci/console.hpp>

static console* active_console;

void xhci_console_base::puts(const std::string& str)
{
   dbc_dev_.write_line(str);
}

void xhci_console_base::putc(char c)
{
   dbc_dev_.write_byte(c);
   if(is_line_ending(c)) {
      dbc_dev_.flush();
   }
}

void xhci_console_base::putchar(unsigned char c)
{
   if(active_console) {
      active_console->putc(c);
   }
}

void xhci_console_init(xhci_console_base& cons)
{
   active_console = &cons;
   add_printf_backend(xhci_console_base::putchar);
}
