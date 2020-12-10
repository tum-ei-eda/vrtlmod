# vrtlmod - Verilated RTL Injection modifier

## Brief
Verilated RTL modifier (vrtlmod) is a LLVM-based tool modifying <a href="https://www.veripool.org/wiki/verilator" title="Verilator homepage">Verilator</a> output for cycle accurate target injection. Additionally, Vrtlmod takes <a href="https://gitlab.lrz.de/ge29noy/regpicker" title="regpicker git">RegPicker</a>'s XML output as target source.

## Dependencies
Besides standard (gmake, cmake, gcc, ...)

1. Verilator  - min. v4.1 (see: https://www.veripool.org/wiki/verilator and install guide)
2. libxml2
3. llvm			  - min. v9.0.1

## Build
1. **Clone the repository:**
```
git clone git@gitlab.lrz.de:ge29noy/vrtlmod.git
```

2. **Change into directory:**
```
cd vrtlmod
```

### ... with Cmake
```
mkdir build && cd build
cmake .. -DLLVM_DIR=<path/to/llvm/install/dir>/lib/cmake/llvm -DVERILATOR_ROOT=<path/to/verilator>
make
```
or including doxygen docu run ```cmake -DBUILD_DOC=ON ..```

## Usage

1. **Required inputs:**

	- <a href="https://gitlab.lrz.de/ge29noy/regpicker" title="regpicker git">RegPicker</a>'s XML output: `<XML-file>`
	- <a href="https://www.veripool.org/wiki/verilator" title="Verilator homepage">Verilator</a> output: `<VRTL-Cpp-files>` `<VRTL-Hpp-files>`

3. **Execution:**

```
vrtlmod --out=<outputdir> --regxml=<XML-file> <VRTL-Cpp-files> -- clang++ -I<VRTL-Hpp-dir> -I$LLVM_DIR/lib/clang/9.0.1/include -I$VERILATOR_ROOT/include
```

The output can be found at `<outputdir>` in form of altered Cpp files (`<VRTL-Cpp-files>_vrtlmod.cpp`) and the built injection API inside `<outputdir>/vrtlmodapi` concluding:
- Target dictionary
- Injection classes
