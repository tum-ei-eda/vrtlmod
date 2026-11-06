#pragma once

// The same deterministic trace is used by the C++ and SystemC harnesses. Each
// step observes the plain model and the rewritten model with injection disabled.
template <typename Step>
bool check_fiapp_equivalence(Step &&step)
{
    bool passed = true;
    unsigned event = 0;
    passed &= step(false, true, false, false, event++);
    // Fill even the longest shift paths before resetting again. The subsequent
    // changing pattern also exposes stale state and disabled writes.
    for (unsigned cycle = 0; cycle < 320; ++cycle)
    {
        const bool data = cycle < 280 || cycle % 7 < 3;
        const bool enable = cycle % 7 != 3;
        passed &= step(false, false, data, enable, event++);
        passed &= step(true, false, data, enable, event++);
    }
    for (unsigned cycle = 0; cycle < 160; ++cycle)
    {
        const bool reset = cycle % 47 == 0;
        const bool enable = cycle % 5 < 3;
        const bool data = (cycle % 23 < 17) != (cycle % 7 == 0);
        passed &= step(false, reset, data, enable, event++);
        passed &= step(true, reset, data, enable, event++);
        // Change inputs with the clock held high, then assert asynchronous reset
        // on selected cycles without introducing a clock edge.
        passed &= step(true, reset, !data, !enable, event++);
        if (cycle % 31 == 7)
            passed &= step(true, true, !data, !enable, event++);
    }
    return passed;
}
