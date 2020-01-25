################################################################################
### @file Makefile
### @brief GNU makefile for vrtlmod tool
### @details For development and debug purposes only! Use Cmake for more stable build
### @date Created on Mon Jan 15 11:22:11 2020
### @author Johannes Geier (johannes.geier@tum.de)
################################################################################


BUILD_DIR=build
EXE=vrtlmod

# include LLVM
LLVMINSTALLED?=0
ifeq ($(LLVMINSTALLED),0)
	LLVMTOOLDIR?=$(LLVM_DIR)/..
	LLVMVERSION?=9.0.0
	LLVMPATH=$(LLVMTOOLDIR)/$(LLVMVERSION)
	LLVMPATH_BIN=$(LLVMPATH)/bin
	LLVMPATH_LIB=$(LLVMPATH)/lib
	LLVMPATH_INCLUDE=$(LLVMPATH)/include
	CLANG_HEADERS=$(realpath $(LLVMPATH_LIB)/clang/$(LLVMVERSION)/include)
else
	LLVMVERSION?=$(shell llvm-config --version)
	LLVMPATH_BIN=$(shell llvm-config --bindir)
	LLVMPATH_LIB=$(shell llvm-config --libdir)
	LLVMPATH_INCLUDE=$(shell llvm-config --includedir)
	CLANG_HEADERS=$(realpath $(LLVMPATH_LIB)/clang/$(LLVMVERSION)/include)
endif


CLANG_LIBS := \
	-lclangFrontend \
	-lclangCodeGen \
	-lclangTooling \
	-lclangARCMigrate \
	-lclangAST \
	-lclangASTMatchers \
	-lclangAnalysis \
	-lclangBasic \
	-lclangCrossTU \
	-lclangDependencyScanning \
	-lclangDirectoryWatcher \
	-lclangDriver \
	-lclangDynamicASTMatchers \
	-lclangEdit \
	-lclangFormat \
	-lclangFrontendTool \
	-lclangHandleCXX \
	-lclangHandleLLVM \
	-lclangIndex \
	-lclangLex \
	-lclangParse \
	-lclangRewrite \
	-lclangRewriteFrontend \
	-lclangSema \
	-lclangSerialization \
	-lclangStaticAnalyzerCheckers \
	-lclangStaticAnalyzerCore \
	-lclangStaticAnalyzerFrontend \
	-lclangToolingASTDiff \
	-lclangToolingCore \
	-lclangToolingInclusions \
	-lclangToolingRefactoring \
	-lclangToolingSyntax

# Set Debug Variables
DEBUG?=1
NATIVE?=0
ifeq ($(DEBUG),0)
	DBGPARAM=
	ifeq ($(NATIVE),0)
		OPTLEVEL?=-O3
	else
		OPTLEVEL?=-O3 -mtune=native -march=native 
	endif
else
	DBGPARAM=-g
	OPTLEVEL?=-O0
endif


# Flags
FORCED_CFLAGS= $(DBGPARAM) $(OPTLEVEL)
LLVMFLAGS=$($(LLVMPATH_BIN)/llvm-config --cxxflags)

_LLVM_LIBS=$(shell $(LLVMPATH_BIN)/llvm-config --libnames)
LLVM_LIBS=$(_LLVM_LIBS:%=$(LLVMPATH_LIB)/%)

LD=c++
CXX=g++

# specify source files and their object files
TARGETFILES= $(wildcard src/*.cpp) $(wildcard src/*/*.cpp) $(wildcard src/*/*/*.cpp) $(wildcard src/*/*/*/*.cpp)
OBJFILES=$(TARGETFILES:src/%.cpp=$(BUILD_DIR)/%.o)
OBJFILES+=$(BUILD_DIR)/$(EXE).o

all: $(EXE)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	
$(BUILD_DIR)/%.o: src/%.cpp 
	@mkdir -p $(dir $@)
	$(CXX) $(DBGPARAM) $(OPTLEVEL) `$(LLVMPATH_BIN)/llvm-config --cxxflags` -DCLANG_HEADERS=$(CLANG_HEADERS) -DCLANG_FALLBACK_PATH=$(realpath $(LLVMPATH_LIB))/ -DCLANG_HEADER_REALTIVE_PATH=clang/$(LLVMVERSION)/include -c -Wall -fPIC -Iinclude -I/usr/include/libxml2 -MMD -I$(../include) -I$(LLVMPATH_INCLUDE) -DDEBUG=$(DEBUG) $(FORCED_CFLAGS) -o $@ $<

$(BUILD_DIR)/$(EXE).o: $(EXE).cpp
	$(CXX) $(DBGPARAM) $(OPTLEVEL) `$(LLVMPATH_BIN)/llvm-config --cxxflags` -DCLANG_HEADERS=$(CLANG_HEADERS) -DCLANG_FALLBACK_PATH=$(realpath $(LLVMPATH_LIB))/ -DCLANG_HEADER_REALTIVE_PATH=clang/$(LLVMVERSION)/include -c -Wall -fPIC -Iinclude -I/usr/include/libxml2 -MMD -I$(../include) -I$(LLVMPATH_INCLUDE) -DDEBUG=$(DEBUG) $(FORCED_CFLAGS) -o $@ $< 

-include $(BUILD_DIR)/*.d
-include $(BUILD_DIR)/*/*.d
-include $(BUILD_DIR)/*/*/*.d
-include $(BUILD_DIR)/*/*/*/*.d
-include $(BUILD_DIR)/*/*/*/*/*.d
-include $(BUILD_DIR)/*/*/*/*/*/*.d

$(EXE): $(OBJFILES)
	$(LD) $(DBGPARAM) $(OPTLEVEL) -DCLANG_HEADERS=$(CLANG_HEADERS) -DDEBUG=$(DEBUG) $(FORCED_CFLAGS) \
		$(OBJFILES) -o $(EXE) -Iinclude `xml2-config --cflags` `$(LLVMPATH_BIN)/llvm-config --cxxflags --ldflags` $(LLVM_LIBS) $(CLANG_LIBS) `xml2-config --libs`

docu:
	$(MAKE) -C doc

clean:
	$(MAKE) -C doc -f Makefile clean
	rm -R -f $(BUILD_DIR)
	rm -f $(EXE)
	
cleanall: clean
	
#$(BUILD_DIR)/$(EXE).o
