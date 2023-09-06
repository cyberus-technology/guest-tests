/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include "toyos/console_serial.hpp"
#include <x86asm.hpp>

static console* active_console {nullptr};

void console_serial_base::putc(char c)
{
    unsigned retry_count {100000};
    while (not(inb(base + LSR) & THB_EMPTY)) {
        if (not --retry_count) {
            return;
        }
    }
    outb(base + THB, c);
}

console_serial_base::console_serial_base(uint16_t port, unsigned baud) : base(port)
{
    outb(base + IER, 0);                      // disable IRQs
    outb(base + LCR, inb(base + LCR) | DLAB); // enable latch access
    outb(base + DLL, (115200 / baud) & 0xFF); // set low divisor byte
    outb(base + DLH, (115200 / baud) >> 8);   // set hi  divisor byte
    outb(base + LCR, LCR_8BIT);               // disable latch access, enable 8n1
    outb(base + FCR, FCR_ENABLE | FCR_CLEAR); // clear and enable FIFOs
    outb(base + MCR, MCR_DTR | MCR_RTS);      // make RTS and DTR active

    // On some serial controllers this sequence can lead to a desynchronization
    // with the receiver, in which case the receiver goes into break state.
    // If we continue sending data immediately, the next couple frames can be
    // garbled. Add a small delay of around 22 bit times (twice the maximum
    // frame length), so that the receiver has time to go back to idle state and
    // we do not lose characters. This is ~190us @ 115200 baud.
    // FIXME: no udelay() available for guest tests.
    //    udelay(1000000 /* us */ * 22 / baud);
    // Just use a rough 2Ghz tick estimate meanwhile.
    uint64_t delay_ticks = 2000u * 1000000u * 22u / baud;
    for (uint64_t delay = 0; delay < delay_ticks; delay++) {
        cpu_pause();
    };
}

void console_serial::putchar(unsigned char c)
{
    if (active_console) {
        active_console->putc(c);
    }
}

void serial_init(uint16_t port_begin)
{
    static console_serial cons(port_begin, SERIAL_BAUD);
    active_console = &cons;
}
