####################################################################################################
# Copyright 2022 Chair of EDA, Technical University of Munich
#
# Licensed under the Apache License, Version 2.0 (the License);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
####################################################################################################

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
include(FetchContent)

####################################################################################################
# cv32e40p-RTL (ETH, openhardware group)
FetchContent_Declare(
    cv32e40p_rtl
    GIT_REPOSITORY https://github.com/openhwgroup/cv32e40p.git
    GIT_TAG        842bf0676f50589b84eea5fe2954eaf71b03a669
)

FetchContent_GetProperties(cv32e40p_rtl)
if(NOT cv32e40p_rtl_POPULATED)
    FetchContent_Populate(cv32e40p_rtl)
endif()
####################################################################################################
# fpnew-RTL (ETH, openhardware group)
FetchContent_Declare(
    fpnew_rtl
    GIT_REPOSITORY https://github.com/openhwgroup/cvfpu.git
    GIT_TAG        v0.6.1
)

FetchContent_GetProperties(fpnew_rtl)
if(NOT fpnew_rtl_POPULATED)
    FetchContent_Populate(fpnew_rtl)
endif()
####################################################################################################

set(RTL_DIR ${cv32e40p_rtl_SOURCE_DIR})
set(RTL_DIR ${RTL_DIR} PARENT_SCOPE)
set(TOP_NAME cv32e40p PARENT_SCOPE)
message("RTL_DIR: ${RTL_DIR}")

set(RTL_PKG_FILES
    ${fpnew_rtl_SOURCE_DIR}/src/fpnew_pkg.sv
    ${RTL_DIR}/rtl/include/cv32e40p_apu_core_pkg.sv
    ${RTL_DIR}/rtl/include/cv32e40p_fpu_pkg.sv
    ${RTL_DIR}/rtl/include/cv32e40p_pkg.sv
    #${RTL_DIR}/bhv/include/cv32e40p_tracer_pkg.sv
    ${fpnew_rtl_SOURCE_DIR}/src/common_cells/src/cf_math_pkg.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpu_div_sqrt_mvp/hdl/defs_div_sqrt_mvp.sv
)

set(RTL_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cv32e40p_top.sv
    ${RTL_DIR}/rtl/cv32e40p_alu.sv
    ${RTL_DIR}/rtl/cv32e40p_alu_div.sv
    ${RTL_DIR}/rtl/cv32e40p_ff_one.sv
    ${RTL_DIR}/rtl/cv32e40p_popcnt.sv
    ${RTL_DIR}/rtl/cv32e40p_compressed_decoder.sv
    ${RTL_DIR}/rtl/cv32e40p_controller.sv
    ${RTL_DIR}/rtl/cv32e40p_cs_registers.sv
    ${RTL_DIR}/rtl/cv32e40p_decoder.sv
    ${RTL_DIR}/rtl/cv32e40p_int_controller.sv
    ${RTL_DIR}/rtl/cv32e40p_ex_stage.sv
    ${RTL_DIR}/rtl/cv32e40p_hwloop_regs.sv
    ${RTL_DIR}/rtl/cv32e40p_id_stage.sv
    ${RTL_DIR}/rtl/cv32e40p_if_stage.sv
    ${RTL_DIR}/rtl/cv32e40p_load_store_unit.sv
    ${RTL_DIR}/rtl/cv32e40p_mult.sv
    ${RTL_DIR}/rtl/cv32e40p_prefetch_buffer.sv
    ${RTL_DIR}/rtl/cv32e40p_prefetch_controller.sv
    ${RTL_DIR}/rtl/cv32e40p_obi_interface.sv
    ${RTL_DIR}/rtl/cv32e40p_aligner.sv
    ${RTL_DIR}/rtl/cv32e40p_sleep_unit.sv
    ${RTL_DIR}/rtl/cv32e40p_core.sv
    ${RTL_DIR}/rtl/cv32e40p_apu_disp.sv
    ${RTL_DIR}/rtl/cv32e40p_fifo.sv

    ${RTL_DIR}/rtl/cv32e40p_register_file_ff.sv
)

set(RTL_FPU_SRC_FILES
    ${fpnew_rtl_SOURCE_DIR}/src/fpnew_top.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpnew_opgroup_block.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpnew_opgroup_fmt_slice.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpnew_opgroup_multifmt_slice.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpnew_fma_multi.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpnew_classifier.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpnew_cast_multi.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpnew_rounding.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpnew_noncomp.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpnew_divsqrt_multi.sv

    ${fpnew_rtl_SOURCE_DIR}/src/common_cells/src/lzc.sv
    ${fpnew_rtl_SOURCE_DIR}/src/common_cells/src/rr_arb_tree.sv

    ${fpnew_rtl_SOURCE_DIR}/src/fpu_div_sqrt_mvp/hdl/div_sqrt_top_mvp.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpu_div_sqrt_mvp/hdl/preprocess_mvp.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpu_div_sqrt_mvp/hdl/norm_div_sqrt_mvp.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpu_div_sqrt_mvp/hdl/nrbd_nrsc_mvp.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpu_div_sqrt_mvp/hdl/control_mvp.sv
    ${fpnew_rtl_SOURCE_DIR}/src/fpu_div_sqrt_mvp/hdl/iteration_div_sqrt_mvp.sv
)

set(RTL_UTIL_FILES
    ${RTL_DIR}/bhv/cv32e40p_wrapper.sv
    ${RTL_DIR}/bhv/cv32e40p_sim_clock_gate.sv
    ${RTL_DIR}/example_tb/core/cv32e40p_fp_wrapper.sv
)

set(RTL_SRCS
    ${RTL_PKG_FILES}
    ${RTL_SRC_FILES}
    ${RTL_FPU_SRC_FILES}
    ${RTL_UTIL_FILES}
    PARENT_SCOPE
)

set(RTL_INCLUDE_DIRS
    ${RTL_DIR}/rtl/include
    ${fpnew_rtl_SOURCE_DIR}/src/common_cells/include
    ${RTL_DIR}/bhv/
    PARENT_SCOPE
)

set(VERILATE_ARGS
    -sv
    -O3
    --unroll-count 256
    --pins-bv 2
    -Werror-PINMISSING
    -Wno-fatal
    -Wno-PINCONNECTEMPTY
    -Wno-ASSIGNDLY
    -Wno-DECLFILENAME
    -Wno-UNUSED
    -Wno-UNOPTFLAT
    -Wno-style
    -Wno-CASEINCOMPLETE
    -Wno-LITENDIAN
    -Wno-UNOPT
    -Wno-WIDTH
    -Wno-BLKANDNBLK
    PARENT_SCOPE
)
