#pragma once

#include "Vlayout.h"

class Vlayout__Syms : public VerilatedSyms
{
public:
    union {
        struct {
            Vlayout___024root TOP;
        } vlInst;
    };

    Vlayout__Syms(VerilatedContext* context, const char* name);
    ~Vlayout__Syms();
    const char* name() const { return vlInst.TOP.vlNamep; }
};
