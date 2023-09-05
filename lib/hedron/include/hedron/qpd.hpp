/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

namespace hedron
{
class qpd
{
public:
    enum
    {
        PRIO_SHIFT = 0,
        QUANTUM_SHIFT = 12,
    };

    enum priority : uint8_t
    {
        PRIO_IDLE = 0,
        PRIO_DEFAULT = PRIO_IDLE + 1,
        PRIO_INTERRUPT = PRIO_DEFAULT + 1,
        PRIO_TIMER = PRIO_INTERRUPT,
    };

    enum quantum
    {
        QUANTUM_DEFAULT = 10000,
        QUANTUM_INFINITE = ~0ull,
    };

    qpd() = default;
    static qpd default_qpd() { return qpd(QUANTUM_DEFAULT, PRIO_DEFAULT); }

    size_t value() const { return raw; }

    qpd(size_t quantum_, uint8_t prio_) { raw = (quantum_ << QUANTUM_SHIFT) | prio_; }

private:
    size_t raw {0};
};
} // namespace hedron
