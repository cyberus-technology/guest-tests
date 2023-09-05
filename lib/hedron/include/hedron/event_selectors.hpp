/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

namespace hedron
{
/**
 * The event specific capability selectors defined by Hedron.
 */
enum event_selector
{
    DE = 0x0,
    DB,
    BP = 0x3,
    OF,
    BR,
    UD,
    NM,
    DF,
    TS = 0xa,
    NP,
    SS,
    GP,
    PF,
    MF = 0x10,
    AC,
    MC,
    XM,
    STARTUP = 0x1e,
    RECALL,
    NUMBER_OF_EVENTS,
};

} // namespace hedron
