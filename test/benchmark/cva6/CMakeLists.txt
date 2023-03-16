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
# CVA6-RTL (ETH, openhardware group)
FetchContent_Declare(
    cva6_rtl
    GIT_REPOSITORY https://github.com/openhwgroup/cva6.git
    GIT_TAG        a7ad3ec195bdf2930504a70e11448b6bc36d731d
)

FetchContent_GetProperties(cva6_rtl)
if(NOT cva6_rtl_POPULATED)
    FetchContent_Populate(cva6_rtl)
endif()
set(RTL_DIR ${cva6_rtl_SOURCE_DIR})
set(RTL_DIR ${RTL_DIR} PARENT_SCOPE)
set(TOP_NAME cva6 PARENT_SCOPE)
message("RTL_DIR: ${RTL_DIR}")

set(RTL_PKG_FILES
    ${RTL_DIR}/include/riscv_pkg.sv
    ${RTL_DIR}/src/riscv-dbg/src/dm_pkg.sv
    ${RTL_DIR}/include/ariane_pkg.sv
    ${RTL_DIR}/include/std_cache_pkg.sv
    ${RTL_DIR}/include/wt_cache_pkg.sv
    ${RTL_DIR}/src/axi/src/axi_pkg.sv
    ${RTL_DIR}/src/register_interface/src/reg_intf.sv
    ${RTL_DIR}/src/register_interface/src/reg_intf_pkg.sv
    ${RTL_DIR}/include/axi_intf.sv
    ${RTL_DIR}/tb/ariane_soc_pkg.sv
    ${RTL_DIR}/include/ariane_axi_pkg.sv
    ${RTL_DIR}/src/fpu/src/fpnew_pkg.sv
    ${RTL_DIR}/src/fpu/src/fpu_div_sqrt_mvp/hdl/defs_div_sqrt_mvp.sv
)

set(RTL_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cva6_top.sv
    ${RTL_DIR}/src/serdiv.sv
    ${RTL_DIR}/src/ariane_regfile_ff.sv
    ${RTL_DIR}/src/amo_buffer.sv
    ${RTL_DIR}/src/ptw.sv
    ${RTL_DIR}/src/branch_unit.sv
    ${RTL_DIR}/src/mmu.sv
    ${RTL_DIR}/src/controller.sv
    ${RTL_DIR}/src/commit_stage.sv
    ${RTL_DIR}/src/issue_stage.sv
    ${RTL_DIR}/src/re_name.sv
    ${RTL_DIR}/src/csr_buffer.sv
    ${RTL_DIR}/src/tlb.sv
    ${RTL_DIR}/src/decoder.sv
    ${RTL_DIR}/src/scoreboard.sv
    ${RTL_DIR}/src/perf_counters.sv
    ${RTL_DIR}/src/store_unit.sv
    ${RTL_DIR}/src/ariane.sv
    ${RTL_DIR}/src/mult.sv
    ${RTL_DIR}/src/axi_adapter.sv
    ${RTL_DIR}/src/fpu_wrap.sv
    ${RTL_DIR}/src/load_store_unit.sv
    ${RTL_DIR}/src/alu.sv
    ${RTL_DIR}/src/multiplier.sv
    ${RTL_DIR}/src/store_buffer.sv
    ${RTL_DIR}/src/compressed_decoder.sv
    ${RTL_DIR}/src/axi_shim.sv
    ${RTL_DIR}/src/instr_realign.sv
    ${RTL_DIR}/src/ex_stage.sv
    ${RTL_DIR}/src/id_stage.sv
    ${RTL_DIR}/src/csr_regfile.sv
    ${RTL_DIR}/src/load_unit.sv
    ${RTL_DIR}/src/issue_read_operands.sv
    #${RTL_DIR}/src/fpu/src/fpnew_f2icast.sv
    ${RTL_DIR}/src/fpu/src/fpnew_fma.sv
    ${RTL_DIR}/src/fpu/src/fpnew_opgroup_fmt_slice.sv
    ${RTL_DIR}/src/fpu/src/fpnew_divsqrt_multi.sv
    #${RTL_DIR}/src/fpu/src/fpnew_f2fcast.sv
    ${RTL_DIR}/src/fpu/src/fpnew_fma_multi.sv
    #${RTL_DIR}/src/fpu/src/fpnew_pipe_out.sv
    ${RTL_DIR}/src/fpu/src/fpnew_opgroup_multifmt_slice.sv
    ${RTL_DIR}/src/fpu/src/fpnew_classifier.sv
    ${RTL_DIR}/src/fpu/src/fpnew_noncomp.sv
    ${RTL_DIR}/src/fpu/src/fpnew_cast_multi.sv
    #${RTL_DIR}/src/fpu/src/fpnew_pipe_inside_fma.sv
    ${RTL_DIR}/src/fpu/src/fpnew_opgroup_block.sv
    #${RTL_DIR}/src/fpu/src/fpnew_i2fcast.sv
    #${RTL_DIR}/src/fpu/src/fpnew_pipe_in.sv
    #${RTL_DIR}/src/fpu/src/fpnew_pipe_inside_cast.sv
    ${RTL_DIR}/src/fpu/src/fpnew_rounding.sv
    ${RTL_DIR}/src/fpu/src/fpnew_top.sv
    ${RTL_DIR}/src/fpu/src/fpu_div_sqrt_mvp/hdl/iteration_div_sqrt_mvp.sv
    ${RTL_DIR}/src/fpu/src/fpu_div_sqrt_mvp/hdl/nrbd_nrsc_mvp.sv
    ${RTL_DIR}/src/fpu/src/fpu_div_sqrt_mvp/hdl/div_sqrt_top_mvp.sv
    ${RTL_DIR}/src/fpu/src/fpu_div_sqrt_mvp/hdl/preprocess_mvp.sv
    ${RTL_DIR}/src/fpu/src/fpu_div_sqrt_mvp/hdl/control_mvp.sv
    ${RTL_DIR}/src/fpu/src/fpu_div_sqrt_mvp/hdl/norm_div_sqrt_mvp.sv
    ${RTL_DIR}/src/fpu/src/fpu_div_sqrt_mvp/hdl/div_sqrt_mvp_wrapper.sv
    ${RTL_DIR}/src/pmp/src/pmp.sv
    ${RTL_DIR}/src/pmp/src/pmp_entry.sv
    ${RTL_DIR}/src/frontend/frontend.sv
    ${RTL_DIR}/src/frontend/instr_scan.sv
    ${RTL_DIR}/src/frontend/instr_queue.sv
    ${RTL_DIR}/src/frontend/bht.sv
    ${RTL_DIR}/src/frontend/btb.sv
    ${RTL_DIR}/src/frontend/ras.sv
    ${RTL_DIR}/src/cache_subsystem/tag_cmp.sv
    ${RTL_DIR}/src/cache_subsystem/cache_ctrl.sv
    ${RTL_DIR}/src/cache_subsystem/amo_alu.sv
    ${RTL_DIR}/src/cache_subsystem/wt_axi_adapter.sv
    ${RTL_DIR}/src/cache_subsystem/std_nbdcache.sv
    ${RTL_DIR}/src/cache_subsystem/wt_dcache_ctrl.sv
    ${RTL_DIR}/src/cache_subsystem/miss_handler.sv
    ${RTL_DIR}/src/cache_subsystem/wt_cache_subsystem.sv
    ${RTL_DIR}/src/cache_subsystem/wt_dcache_missunit.sv
    ${RTL_DIR}/src/cache_subsystem/cva6_icache.sv
    ${RTL_DIR}/src/cache_subsystem/wt_dcache_wbuffer.sv
    ${RTL_DIR}/src/cache_subsystem/wt_l15_adapter.sv
    ${RTL_DIR}/src/cache_subsystem/wt_dcache_mem.sv
    ${RTL_DIR}/src/cache_subsystem/cva6_icache_axi_wrapper.sv
    ${RTL_DIR}/src/cache_subsystem/std_cache_subsystem.sv
    ${RTL_DIR}/src/cache_subsystem/wt_dcache.sv
    ${RTL_DIR}/src/fpga-support/rtl/SyncSpRamBeNx64.sv
    ${RTL_DIR}/src/common_cells/src/deprecated/generic_fifo.sv
    ${RTL_DIR}/src/common_cells/src/deprecated/pulp_sync.sv
    ${RTL_DIR}/src/common_cells/src/deprecated/find_first_one.sv
    ${RTL_DIR}/src/common_cells/src/rstgen_bypass.sv
    ${RTL_DIR}/src/common_cells/src/rstgen.sv
    ${RTL_DIR}/src/common_cells/src/stream_mux.sv
    ${RTL_DIR}/src/common_cells/src/stream_demux.sv
    ${RTL_DIR}/src/common_cells/src/unread.sv
    ${RTL_DIR}/src/common_cells/src/sync.sv
    ${RTL_DIR}/src/common_cells/src/cdc_2phase.sv
    ${RTL_DIR}/src/common_cells/src/spill_register.sv
    ${RTL_DIR}/src/common_cells/src/sync_wedge.sv
    ${RTL_DIR}/src/common_cells/src/edge_detect.sv
    ${RTL_DIR}/src/common_cells/src/stream_arbiter.sv
    ${RTL_DIR}/src/common_cells/src/stream_arbiter_flushable.sv
    ${RTL_DIR}/src/common_cells/src/deprecated/fifo_v1.sv
    ${RTL_DIR}/src/common_cells/src/deprecated/fifo_v2.sv
    ${RTL_DIR}/src/common_cells/src/fifo_v3.sv
    ${RTL_DIR}/src/common_cells/src/lzc.sv
    ${RTL_DIR}/src/common_cells/src/popcount.sv
    ${RTL_DIR}/src/common_cells/src/rr_arb_tree.sv
    ${RTL_DIR}/src/common_cells/src/deprecated/rrarbiter.sv
    ${RTL_DIR}/src/common_cells/src/stream_delay.sv
    ${RTL_DIR}/src/common_cells/src/lfsr_8bit.sv
    ${RTL_DIR}/src/common_cells/src/lfsr_16bit.sv
    ${RTL_DIR}/src/common_cells/src/counter.sv
    ${RTL_DIR}/src/common_cells/src/shift_reg.sv
    ${RTL_DIR}/src/tech_cells_generic/src/cluster_clock_gating.sv
    ${RTL_DIR}/src/tech_cells_generic/src/pulp_clock_gating.sv
    ${RTL_DIR}/src/tech_cells_generic/src/pulp_clock_mux2.sv
    ${RTL_DIR}/src/common_cells/src/exp_backoff.sv
    ${RTL_DIR}/src/axi_riscv_atomics/src/axi_riscv_atomics.sv
    ${RTL_DIR}/src/axi_riscv_atomics/src/axi_riscv_amos.sv
    ${RTL_DIR}/src/axi_riscv_atomics/src/axi_riscv_amos_alu.sv
    ${RTL_DIR}/src/axi_riscv_atomics/src/axi_riscv_lrsc.sv
    ${RTL_DIR}/src/axi_riscv_atomics/src/axi_res_tbl.sv
)

set(RTL_UTIL_FILES
    ${RTL_DIR}/src/util/sram.sv
)

set(RTL_SRCS ${RTL_PKG_FILES} ${RTL_SRC_FILES} ${RTL_UTIL_FILES}
    PARENT_SCOPE
)

set(RTL_INCLUDE_DIRS
    ${RTL_DIR}/src/axi_node ${RTL_DIR}/include ${RTL_DIR}/src/common_cells/include/
    PARENT_SCOPE
)

set(VERILATE_ARGS
    -sv
    +define+WT_DCACHE
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

####################################################################################################
# AXI-TLM lib (Xilinx open source)
FetchContent_Declare(
    axitlm
    GIT_REPOSITORY https://github.com/Xilinx/libsystemctlm-soc.git
    GIT_TAG        f655e5e
    PATCH_COMMAND git apply --ignore-space-change --ignore-whitespace ${CMAKE_CURRENT_SOURCE_DIR}/../patches/axitlm-addrwidth.patch
)

FetchContent_GetProperties(axitlm)
if(NOT axitlm_POPULATED)
    FetchContent_Populate(axitlm)
    SET(AXITLM_SRC ${axitlm_SOURCE_DIR})
endif()

add_library(axitlm OBJECT
    ${AXITLM_SRC}/tests/test-modules/memory.cc
    ${AXITLM_SRC}/trace/trace.cc
)

target_include_directories(axitlm PUBLIC
    ${AXITLM_SRC}/
    ${AXITLM_SRC}/tests/
    ${AXITLM_SRC}/tests/test-modules/
)

target_link_libraries(axitlm PUBLIC
    ${SystemC_LIBRARIES}
)

set(TGT_SPECIFIC_LIBS axitlm PARENT_SCOPE)
