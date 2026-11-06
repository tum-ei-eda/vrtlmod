#include "Vlayout__Syms.h"

#include <new>

Vlayout___024root::Vlayout___024root(Vlayout__Syms* symbols, const char* name)
    : vlSymsp(symbols), vlNamep(name)
{
    state = 0;
    nested = 0;
    __VprimaryPorts.flag = 0;
    for (unsigned word = 0; word < 3; ++word)
        __VprimaryPorts.port[word] = 0;
}

Vlayout__Syms::Vlayout__Syms(VerilatedContext* context, const char* name)
    : VerilatedSyms(context)
{
    new (&vlInst.TOP) Vlayout___024root(this, name);
}

Vlayout__Syms::~Vlayout__Syms()
{
    vlInst.TOP.~Vlayout___024root();
}

Vlayout::Vlayout(const char* name)
{
    auto* symbols = new Vlayout__Syms(Verilated::threadContextp(), name);
    rootp = &symbols->vlInst.TOP;
}

Vlayout::~Vlayout()
{
    delete rootp->vlSymsp;
}

// Recognized sequential naming on both Verilator 4 and 5. The accesses exercise
// pointer/reference roots and a symbol-table path through the named wrappers.
void Vlayout___nba_sequent__TOP__0__sequent__(Vlayout___024root* vlSelf)
{
    auto& vlSelfRef = *vlSelf;
    vlSelf->state = 5;
    vlSelfRef.nested = 0;
    ::VlWide<3> zero{};
    VL_ASSIGN_W(65, vlSelfRef.__VprimaryPorts.port, zero);
    vlSelf->vlSymsp->vlInst.TOP.__VprimaryPorts.flag = 0;
}

void Vlayout::eval()
{
    Vlayout___nba_sequent__TOP__0__sequent__(rootp);
}
