# Configure the project's CMake build before using this helper.
DOCS_SOURCE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR ?= $(abspath $(DOCS_SOURCE_DIR)/../build)
DOCS_BUILD_DIR := $(BUILD_DIR)/docs

.PHONY: all pdf tex dox clean

all: pdf

pdf: tex
	cp "$(DOCS_BUILD_DIR)/latex/refman.pdf" "$(DOCS_BUILD_DIR)/vrtlmod.pdf"

tex: dox
	$(MAKE) -C "$(DOCS_BUILD_DIR)/latex"

dox:
	cmake --build "$(BUILD_DIR)" --target doc

clean:
	$(RM) -r "$(DOCS_BUILD_DIR)/html" "$(DOCS_BUILD_DIR)/latex" "$(DOCS_BUILD_DIR)/vrtlmod.pdf"
