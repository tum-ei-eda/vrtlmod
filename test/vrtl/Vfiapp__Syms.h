// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table internal header
//
// Internal details; most calling programs do not need this header,
// unless using verilator public meta comments.

#ifndef _Vfiapp__Syms_H_
#define _Vfiapp__Syms_H_

#include "verilated.h"

// INCLUDE MODULE CLASSES
#include "Vfiapp.h"

// SYMS CLASS
class Vfiapp__Syms : public VerilatedSyms {
  public:
    
    // LOCAL STATE
    const char* __Vm_namep;
    bool __Vm_didInit;
    
    // SUBCELL STATE
    Vfiapp*                        TOPp;
    
    // CREATORS
    Vfiapp__Syms(Vfiapp* topp, const char* namep);
    ~Vfiapp__Syms() {}
    
    // METHODS
    inline const char* name() { return __Vm_namep; }
    
} VL_ATTR_ALIGNED(64);

#endif  // guard
