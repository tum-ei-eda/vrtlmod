#pragma once

#include "verilated.h"

class Vlayout__Syms;

// Deliberately has no VerilatedModule base, like recent generated roots.
class Vlayout___024root
{
public:
    union {
        struct {
            VL_OUTW(port,64,0,3);
            VL_OUT8(flag,0,0);
        } __VprimaryPorts;
    };
    struct {
        ::IData/*31:0*/ state;
        // }; a comment must not close this anonymous block
        struct {
            ::CData/*0:0*/ nested;
        };
        /* } }; another misleading block boundary */
        char __Vchar[('}' == '}') ? 1 : 2];
        char __Vquoted[sizeof("}; quoted \" {")];
    };
    Vlayout__Syms* vlSymsp;
    const char* vlNamep;

    Vlayout___024root(Vlayout__Syms* symbols, const char* name);
};

class Vlayout
{
public:
    Vlayout___024root* rootp;
    explicit Vlayout(const char* name = "TOP");
    ~Vlayout();
    void eval();
};
