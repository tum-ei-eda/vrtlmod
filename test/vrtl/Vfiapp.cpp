// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vfiapp.h for the primary calling header

#include "Vfiapp.h"
#include "Vfiapp__Syms.h"


//--------------------
// STATIC VARIABLES


//--------------------

VL_CTOR_IMP(Vfiapp) {
    Vfiapp__Syms* __restrict vlSymsp = __VlSymsp = new Vfiapp__Syms(this, name());
    Vfiapp* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Reset internal values
    
    // Reset structure values
    _ctor_var_reset();
}

void Vfiapp::__Vconfigure(Vfiapp__Syms* vlSymsp, bool first) {
    if (0 && first) {}  // Prevent unused
    this->__VlSymsp = vlSymsp;
}

Vfiapp::~Vfiapp() {
    delete __VlSymsp; __VlSymsp=NULL;
}

//--------------------


void Vfiapp::eval() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Vfiapp::eval\n"); );
    Vfiapp__Syms* __restrict vlSymsp = this->__VlSymsp;  // Setup global symbol table
    Vfiapp* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
#ifdef VL_DEBUG
    // Debug assertions
    _eval_debug_assertions();
#endif  // VL_DEBUG
    // Initialize
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) _eval_initial_loop(vlSymsp);
    // Evaluate till stable
    int __VclockLoop = 0;
    QData __Vchange = 1;
    do {
        VL_DEBUG_IF(VL_DBG_MSGF("+ Clock loop\n"););
        _eval(vlSymsp);
        if (VL_UNLIKELY(++__VclockLoop > 100)) {
            // About to fail, so enable debug to see what's not settling.
            // Note you must run make with OPT=-DVL_DEBUG for debug prints.
            int __Vsaved_debug = Verilated::debug();
            Verilated::debug(1);
            __Vchange = _change_request(vlSymsp);
            Verilated::debug(__Vsaved_debug);
            VL_FATAL_MT("/home/joh/Repos/.Re/regpicker/examples/FIapp_rtl/fiapp.sv", 7, "",
                "Verilated model didn't converge\n"
                "- See DIDNOTCONVERGE in the Verilator manual");
        } else {
            __Vchange = _change_request(vlSymsp);
        }
    } while (VL_UNLIKELY(__Vchange));
}

void Vfiapp::_eval_initial_loop(Vfiapp__Syms* __restrict vlSymsp) {
    vlSymsp->__Vm_didInit = true;
    _eval_initial(vlSymsp);
    // Evaluate till stable
    int __VclockLoop = 0;
    QData __Vchange = 1;
    do {
        _eval_settle(vlSymsp);
        _eval(vlSymsp);
        if (VL_UNLIKELY(++__VclockLoop > 100)) {
            // About to fail, so enable debug to see what's not settling.
            // Note you must run make with OPT=-DVL_DEBUG for debug prints.
            int __Vsaved_debug = Verilated::debug();
            Verilated::debug(1);
            __Vchange = _change_request(vlSymsp);
            Verilated::debug(__Vsaved_debug);
            VL_FATAL_MT("/home/joh/Repos/.Re/regpicker/examples/FIapp_rtl/fiapp.sv", 7, "",
                "Verilated model didn't DC converge\n"
                "- See DIDNOTCONVERGE in the Verilator manual");
        } else {
            __Vchange = _change_request(vlSymsp);
        }
    } while (VL_UNLIKELY(__Vchange));
}

//--------------------
// Internal Methods

VL_INLINE_OPT void Vfiapp::_sequent__TOP__1(Vfiapp__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vfiapp::_sequent__TOP__1\n"); );
    Vfiapp* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    if (vlTOPp->reset) {
        vlTOPp->fiapp__DOT__q2 = 0U;
        vlTOPp->fiapp__DOT__q3 = 0U;
    } else {
        vlTOPp->fiapp__DOT__q2 = vlTOPp->fiapp__DOT__q1;
        vlTOPp->fiapp__DOT__q3 = (1U & (~ (IData)(vlTOPp->fiapp__DOT__q1)));
    }
    vlTOPp->o2 = vlTOPp->fiapp__DOT__q2;
    vlTOPp->o3 = vlTOPp->fiapp__DOT__q3;
    if (vlTOPp->reset) {
        vlTOPp->fiapp__DOT__q1 = 0U;
    } else {
        if (vlTOPp->enable) {
            vlTOPp->fiapp__DOT__q1 = vlTOPp->a;
        }
    }
    vlTOPp->o1 = vlTOPp->fiapp__DOT__q1;
}

void Vfiapp::_settle__TOP__2(Vfiapp__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vfiapp::_settle__TOP__2\n"); );
    Vfiapp* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->o2 = vlTOPp->fiapp__DOT__q2;
    vlTOPp->o3 = vlTOPp->fiapp__DOT__q3;
    vlTOPp->o1 = vlTOPp->fiapp__DOT__q1;
}

void Vfiapp::_eval(Vfiapp__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vfiapp::_eval\n"); );
    Vfiapp* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    if ((((IData)(vlTOPp->clk) & (~ (IData)(vlTOPp->__Vclklast__TOP__clk))) 
         | ((IData)(vlTOPp->reset) & (~ (IData)(vlTOPp->__Vclklast__TOP__reset))))) {
        vlTOPp->_sequent__TOP__1(vlSymsp);
    }
    // Final
    vlTOPp->__Vclklast__TOP__clk = vlTOPp->clk;
    vlTOPp->__Vclklast__TOP__reset = vlTOPp->reset;
}

void Vfiapp::_eval_initial(Vfiapp__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vfiapp::_eval_initial\n"); );
    Vfiapp* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->__Vclklast__TOP__clk = vlTOPp->clk;
    vlTOPp->__Vclklast__TOP__reset = vlTOPp->reset;
}

void Vfiapp::final() {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vfiapp::final\n"); );
    // Variables
    Vfiapp__Syms* __restrict vlSymsp = this->__VlSymsp;
    Vfiapp* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
}

void Vfiapp::_eval_settle(Vfiapp__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vfiapp::_eval_settle\n"); );
    Vfiapp* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->_settle__TOP__2(vlSymsp);
}

VL_INLINE_OPT QData Vfiapp::_change_request(Vfiapp__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vfiapp::_change_request\n"); );
    Vfiapp* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    // Change detection
    QData __req = false;  // Logically a bool
    return __req;
}

#ifdef VL_DEBUG
void Vfiapp::_eval_debug_assertions() {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vfiapp::_eval_debug_assertions\n"); );
    // Body
    if (VL_UNLIKELY((clk & 0xfeU))) {
        Verilated::overWidthError("clk");}
    if (VL_UNLIKELY((reset & 0xfeU))) {
        Verilated::overWidthError("reset");}
    if (VL_UNLIKELY((a & 0xfeU))) {
        Verilated::overWidthError("a");}
    if (VL_UNLIKELY((enable & 0xfeU))) {
        Verilated::overWidthError("enable");}
}
#endif // VL_DEBUG

void Vfiapp::_ctor_var_reset() {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vfiapp::_ctor_var_reset\n"); );
    // Body
    clk = VL_RAND_RESET_I(1);
    reset = VL_RAND_RESET_I(1);
    a = VL_RAND_RESET_I(1);
    enable = VL_RAND_RESET_I(1);
    o1 = VL_RAND_RESET_I(1);
    o2 = VL_RAND_RESET_I(1);
    o3 = VL_RAND_RESET_I(1);
    fiapp__DOT__q1 = VL_RAND_RESET_I(1);
    fiapp__DOT__q2 = VL_RAND_RESET_I(1);
    fiapp__DOT__q3 = VL_RAND_RESET_I(1);
}
