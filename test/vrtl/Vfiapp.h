// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Primary design header
//
// This header should be included by all source files instantiating the design.
// The class here is then constructed to instantiate the design.
// See the Verilator manual for examples.

#ifndef _Vfiapp_H_
#define _Vfiapp_H_

#include "verilated.h"

class Vfiapp__Syms;

//----------

VL_MODULE(Vfiapp) {
  public:
    
    // PORTS
    // The application code writes and reads these signals to
    // propagate new values into/out from the Verilated model.
    VL_IN8(clk,0,0);
    VL_IN8(reset,0,0);
    VL_IN8(a,0,0);
    VL_IN8(enable,0,0);
    VL_OUT8(o1,0,0);
    VL_OUT8(o2,0,0);
    VL_OUT8(o3,0,0);
    
    // LOCAL SIGNALS
    // Internals; generally not touched by application code
    CData/*0:0*/ fiapp__DOT__q1;
    CData/*0:0*/ fiapp__DOT__q2;
    CData/*0:0*/ fiapp__DOT__q3;
    
    // LOCAL VARIABLES
    // Internals; generally not touched by application code
    CData/*0:0*/ __Vclklast__TOP__clk;
    CData/*0:0*/ __Vclklast__TOP__reset;
    
    // INTERNAL VARIABLES
    // Internals; generally not touched by application code
    Vfiapp__Syms* __VlSymsp;  // Symbol table
    
    // PARAMETERS
    // Parameters marked /*verilator public*/ for use by application code
    
    // CONSTRUCTORS
  private:
    VL_UNCOPYABLE(Vfiapp);  ///< Copying not allowed
  public:
    /// Construct the model; called by application code
    /// The special name  may be used to make a wrapper with a
    /// single model invisible with respect to DPI scope names.
    Vfiapp(const char* name = "TOP");
    /// Destroy the model; called (often implicitly) by application code
    ~Vfiapp();
    
    // API METHODS
    /// Evaluate the model.  Application must call when inputs change.
    void eval();
    /// Simulation complete, run final blocks.  Application must call on completion.
    void final();
    
    // INTERNAL METHODS
  private:
    static void _eval_initial_loop(Vfiapp__Syms* __restrict vlSymsp);
  public:
    void __Vconfigure(Vfiapp__Syms* symsp, bool first);
  private:
    static QData _change_request(Vfiapp__Syms* __restrict vlSymsp);
    void _ctor_var_reset() VL_ATTR_COLD;
  public:
    static void _eval(Vfiapp__Syms* __restrict vlSymsp);
  private:
#ifdef VL_DEBUG
    void _eval_debug_assertions();
#endif // VL_DEBUG
  public:
    static void _eval_initial(Vfiapp__Syms* __restrict vlSymsp) VL_ATTR_COLD;
    static void _eval_settle(Vfiapp__Syms* __restrict vlSymsp) VL_ATTR_COLD;
    static void _sequent__TOP__1(Vfiapp__Syms* __restrict vlSymsp);
    static void _settle__TOP__2(Vfiapp__Syms* __restrict vlSymsp) VL_ATTR_COLD;
} VL_ATTR_ALIGNED(128);

#endif // guard
