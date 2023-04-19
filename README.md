# vRTLmod - Verilated RTL Injection modifier

## Brief
The verilated RTL modifier (vRTLmod) is an LLVM based open-source tool to enable fault injection in verilator RTL (vRTL) simulations.
Within an Clang-Frontend (LLVM) tool it automatically adds fault injection capability to <a href="https://www.veripool.org/wiki/verilator" title="Verilator homepage">Verilator</a> output. Additionally, `vRTLmod` can make use of its own XML output `*-vrtlmod.xml` as a whitelist filter argument to allow manual steering of wanted/unwanted injectable variables.

## Dependencies
Besides standard (gmake, cmake, gcc, ...)

1. Verilator  - tested with v4.204 (see: https://www.veripool.org/wiki/verilator and install guide)
2. LLVM - tested v13.0.1 built with `-D LLVM_ENABLE_PROJECTS="clang;clang-tools-extra"`
3. Boost filesystem
4. For Tests: Conan v<2.0, tested with 1.59.0 (`pip install --force-reinstall "conan==1.59.0"`)

## Build

1. **Required environment:**

```
	[BUILD]: export LLVM_DIR=<path/to/llvm/install/dir>/lib/cmake/llvm
	[BUILD]: export VERILATOR_ROOT=<path/to/verilator/build/or/install/directory>
```

2. **Or via CMake command line arguments:**

```
mkdir build && cd build
cmake -S . -B build -D LLVM_DIR=<path/to/llvm/install/dir> -D VERILATOR_ROOT=<path/to/verilator/build/or/install/directory> [-D BUILD_TESTING=Off]
cmake --build build
```

## Usage

1. **Required inputs:**
	- <a href="https://www.veripool.org/wiki/verilator" title="Verilator homepage">Verilator</a> output: `<VRTL-files>` `<VRTL-Hpp-files>`

2. **Execution:**

```
vrtlmod [--systemc] [--wl-regxml=<*-vrtlmod.xml>] --out=<outputdir> <VRTL-Cpp-files> -- clang++ -I<VRTL-Hpp-dir> -I$LLVM_DIR/lib/clang/.../include -I$VERILATOR_ROOT/include [-I<path/to/systemc/include>]
```

or use installed `vrtlmod-config.cmake` in CMake environment.

The output can be found at `<outputdir>` in form of altered Cpp files (`<vRTL-Cpp-files>_vrtlmod.cpp`) and the built injection API inside `<outputdir>/vrtlmodapi` concluding:
- Target dictionary
- Injection classes

## Examples

The `-D BUILD_TESTING=On` option in cmake enables a SystemC and C++ verilate->vrtlmod flow for the `test/fiapp/fiapp.sv` SystemVerilog example.
In addition, a SystemC (`test/fiapp/sc_fiapp_test.cpp`) and C++ (`test/fiapp/fiapp_test.cpp`) testbench showcases the usage of the generated fault injection API.

These tests are intended, both, as a form of unit-tests for `vrtlmod` as well as an example for its integration in other fault injection projects.
