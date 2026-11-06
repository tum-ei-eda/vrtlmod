#pragma once

#include "verilated.h"

class Vhelperdest__Syms;

// Keep real Verilator storage and helper signatures, but spell out the calls so
// optimizer changes cannot remove the operand shapes this regression exercises.
class Vhelperdest___024root
{
  public:
    // Preserve generated width comments adjacent to the storage type.
    // clang-format off
    VlWide<3>/*95:0*/ clean_dst{};
    VlWide<3>/*95:0*/ cond_dst{};
    VlWide<3>/*95:0*/ bit_dst{};
    VlWide<3>/*95:0*/ wide_dst{};
    VlUnpacked<VlWide<3>/*95:0*/, 2> one{};
    VlUnpacked<VlUnpacked<VlWide<3>/*95:0*/, 2>, 2> two{};
#ifdef HELPER_THREE_UNPACKED
    VlUnpacked<VlUnpacked<VlUnpacked<VlWide<3>/*95:0*/, 2>, 2>, 2> three{};
#endif
    VlWide<3>/*95:0*/ src0{};
    VlWide<3>/*95:0*/ src1{};
    IData/*31:0*/ index = 1;
    CData/*0:0*/ bit_source = 1;
    // clang-format on
    VlUnpacked<VlUnpacked<VlUnpacked<VlWide<3>, 2>, 2>, 2> __Vtemp{};
    Vhelperdest__Syms *const vlSymsp;

    explicit Vhelperdest___024root(Vhelperdest__Syms *symsp);
};

class Vhelperdest
{
  public:
    Vhelperdest___024root *rootp;
    explicit Vhelperdest(const char *name);
    ~Vhelperdest();
    void eval();

  private:
    Vhelperdest__Syms *__Vsymsp;
};
