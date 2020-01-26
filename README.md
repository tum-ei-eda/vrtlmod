# vrtlmod - Verilated RTL Injection modifier

## Brief
Verilated RTL modifier (vrtlmod) is a LLVM-based tool modifying <a href="https://www.veripool.org/wiki/verilator" title="Verilator homepage">Verilator</a> output for cycle accurate target injection. Additionally, Vrtlmod takes <a href="https://gitlab.lrz.de/ge29noy/regpicker" title="regpicker git">RegPicker</a>'s XML output as target source.

## Dependencies
Besides standard (gmake, cmake, gcc, ...)

1. Verilator      - min. v4.023 (see: https://www.veripool.org/wiki/verilator and install guide)
2. libxml2
3. llvm			  - min. v9.0.0

## Build
1. **Clone the repository:**
```
git clone git@gitlab.lrz.de:ge29noy/vrtlmod.git
```

2. **Change into directory:**
```
cd vrtlmod
```

3. **Prepare environment:**
```
export VERILATOR_ROOT=<path-to-verilator-installation>
export LLVM_DIR=<path-to-llvm-installation>
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LLVM_DIR/lib
```

### ... with generic (Gnu) make
For executable:
```
make
```
For test on a simple VRTL model (shell output should list all "passed"):
```
make test
```
For documentation (doxygen):
```
make docu
```

### ... with Cmake and TUMEDA_devenv
1. Clone repository into <a href="https://gitlab.lrz.de/de-tum-ei-eda-esl/TUMEDA_devenv" title="TUMEDA development environment">TUMEDA_devenv</a>/src
2. Prepare environment according to TUMEDA_devenv.
3. Run for executable only:
```
cd <>/TUMEDA_devenv
mkdir build && cd build
cmake ..
make
```
or including doxygen docu run ```cmake -DBUILD_DOC=ON ..``` 

## Usage
1. **Prepare environment:**
```
export LLVM_DIR=<path-to-llvm-installation>
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LLVM_DIR/lib
export VERILATOR_ROOT=<path-to-verilator-installation>
```

2. **Required inputs:**

	- <a href="https://gitlab.lrz.de/ge29noy/regpicker" title="regpicker git">RegPicker</a>'s XML output: `<XML-file>`
	- <a href="https://www.veripool.org/wiki/verilator" title="Verilator homepage">Verilator</a> output: `<VRTL-Cpp-files>` `<VRTL-Hpp-files>`
	
3. **Execution:**

```
vrtlmod --out=<outputdir> --regxml=<XML-file> <VRTL-Cpp-files> -- clang++ -std=c++0x -I<VRTL-Hpp-files> -I$LLVM_DIR/lib/clang/9.0.0/include -I$VERILATOR_ROOT/include
```	

The output can be found at `<outputdir>` in form of altered Cpp files (`<VRTL-Cpp-files>_vrtlmod.cpp`) and the built injection API inside `<outputdir>/InjAPI` concluding:
- Target dictionary
- Injection methods 
