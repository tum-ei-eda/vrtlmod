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
# ri5cy-RTL (ETH, openhardware group)
FetchContent_Declare(
    cv32e40s_rtl
    GIT_REPOSITORY https://github.com/openhwgroup/cv32e40s.git
    GIT_TAG        b8d1efba11d878caa0e78021b37f67d960d82af8
    PATCH_COMMAND git apply --ignore-space-change --ignore-whitespace ${CMAKE_CURRENT_SOURCE_DIR}/rvfi-remove.patch
)

FetchContent_GetProperties(cv32e40s_rtl)
if(NOT cv32e40s_rtl_POPULATED)
    FetchContent_Populate(cv32e40s_rtl)
endif()
set(RTL_DIR ${cv32e40s_rtl_SOURCE_DIR})
set(RTL_DIR ${RTL_DIR} PARENT_SCOPE)
set(TOP_NAME cv32e40s PARENT_SCOPE)
message("RTL_DIR: ${RTL_DIR}")

set(RTL_PKG_FILES
    ${RTL_DIR}/rtl/include/cv32e40s_pkg.sv
)

set(RTL_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cv32e40s_top.sv
    ${RTL_DIR}/rtl/cv32e40s_alert.sv                ${RTL_DIR}/rtl/cv32e40s_controller_bypass.sv   ${RTL_DIR}/rtl/cv32e40s_decoder.sv      ${RTL_DIR}/rtl/cv32e40s_if_stage.sv             ${RTL_DIR}/rtl/cv32e40s_mpu.sv        ${RTL_DIR}/rtl/cv32e40s_prefetcher.sv             ${RTL_DIR}/rtl/cv32e40s_wb_stage.sv
    ${RTL_DIR}/rtl/cv32e40s_alignment_buffer.sv     ${RTL_DIR}/rtl/cv32e40s_controller_fsm.sv      ${RTL_DIR}/rtl/cv32e40s_div.sv          ${RTL_DIR}/rtl/cv32e40s_instr_obi_interface.sv  ${RTL_DIR}/rtl/cv32e40s_mult.sv       ${RTL_DIR}/rtl/cv32e40s_prefetch_unit.sv          ${RTL_DIR}/rtl/cv32e40s_write_buffer.sv
    ${RTL_DIR}/rtl/cv32e40s_alu_b_cpop.sv           ${RTL_DIR}/rtl/cv32e40s_controller.sv          ${RTL_DIR}/rtl/cv32e40s_dummy_instr.sv  ${RTL_DIR}/rtl/cv32e40s_int_controller.sv       ${RTL_DIR}/rtl/cv32e40s_pc_check.sv   ${RTL_DIR}/rtl/cv32e40s_register_file_ecc.sv      ${RTL_DIR}/rtl/if_c_obi.sv
    ${RTL_DIR}/rtl/cv32e40s_alu.sv                  ${RTL_DIR}/rtl/cv32e40s_core.sv                ${RTL_DIR}/rtl/cv32e40s_ex_stage.sv     ${RTL_DIR}/rtl/cv32e40s_lfsr.sv                 ${RTL_DIR}/rtl/cv32e40s_pc_target.sv  ${RTL_DIR}/rtl/cv32e40s_register_file.sv          ${RTL_DIR}/rtl/if_xif.sv
    ${RTL_DIR}/rtl/cv32e40s_b_decoder.sv            ${RTL_DIR}/rtl/cv32e40s_cs_registers.sv        ${RTL_DIR}/rtl/cv32e40s_ff_one.sv       ${RTL_DIR}/rtl/cv32e40s_load_store_unit.sv      ${RTL_DIR}/rtl/cv32e40s_pma.sv        ${RTL_DIR}/rtl/cv32e40s_register_file_wrapper.sv
    ${RTL_DIR}/rtl/cv32e40s_clic_int_controller.sv  ${RTL_DIR}/rtl/cv32e40s_csr.sv                 ${RTL_DIR}/rtl/cv32e40s_i_decoder.sv    ${RTL_DIR}/rtl/cv32e40s_lsu_response_filter.sv  ${RTL_DIR}/rtl/cv32e40s_pmp.sv        ${RTL_DIR}/rtl/cv32e40s_sequencer.sv
    ${RTL_DIR}/rtl/cv32e40s_compressed_decoder.sv   ${RTL_DIR}/rtl/cv32e40s_data_obi_interface.sv  ${RTL_DIR}/rtl/cv32e40s_id_stage.sv     ${RTL_DIR}/rtl/cv32e40s_m_decoder.sv            ${RTL_DIR}/rtl/cv32e40s_popcnt.sv     ${RTL_DIR}/rtl/cv32e40s_sleep_unit.sv

    ${RTL_DIR}/bhv/cv32e40s_sim_sffr.sv
    ${RTL_DIR}/bhv/cv32e40s_sim_sffs.sv

)

set(RTL_UTIL_FILES
    ${RTL_DIR}/bhv/cv32e40s_wrapper.sv
    ${RTL_DIR}/bhv/cv32e40s_sim_clock_gate.sv
)

set(RTL_SRCS
    ${RTL_PKG_FILES}
    ${RTL_SRC_FILES}
    #${RTL_FPU_SRC_FILES}
    ${RTL_UTIL_FILES}
    PARENT_SCOPE
)

set(RTL_INCLUDE_DIRS
    ${RTL_DIR}/rtl/include
    ${RTL_DIR}/bhv
    ${RTL_DIR}/bhv/include
    PARENT_SCOPE
)

set(VERILATE_ARGS
    +define+COREV_ASSERT_OFF
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
