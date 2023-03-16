/*
 * Copyright 2021 Chair of EDA, Technical University of Munich
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 ////////////////////////////////////////////////////////////////////////////////////////////////////
 /// @file cv32e40s_top.sv
 /// @date 2021-10-26
 /// @brief cv32e40s top-level module with traced signals
 /// for more convenient verilator interface
 ////////////////////////////////////////////////////////////////////////////////////////////////////

module cv32e40s
#(
  parameter INSTR_RDATA_WIDTH = 32,
  parameter ADDR_WIDTH = 32,
  //parameter BOOT_ADDR = 'h80,
  parameter PULP_XPULP = 0,
  parameter PULP_CLUSTER = 0,
  parameter PULP_ZFINX = 0,
  parameter NUM_MHPMCOUNTERS = 1,
  parameter DM_HALTADDRESS = 32'h1A110800
 )
(
// Clock and Reset
    input logic           clk_i,
    input logic           rst_ni,

// Instruction Memory Interface
    output logic        instr_req_o, /*Request ready, must stay high until instr_gnt_i is high for one cycle */
    output logic        instr_reqpar_o,
    input  logic        instr_gnt_i, /*The other side accepted the request. instr_addr_o may change in the next cycle */
    input  logic        instr_gntpar_i,
    input  logic        instr_rvalid_i, /*instr_rdata_is holds valid data when instr_rvalid_i is high. This signal will be high for exactly one cycle per request. */
    input  logic        instr_rvalidpar_i,
    output logic [31:0] instr_addr_o, /*Address */
    output logic [11:0] instr_achk_o,
    output logic [1:0]  instr_memtype_o,
    output logic [2:0]  instr_prot_o,
    output logic        instr_dbg_o,
    input  logic [31:0] instr_rdata_i, /*Data read from memory */
    input  logic [4:0]  instr_rchk_i,
    input  logic        instr_err_i,

// Data Memory Interface
    output logic        data_req_o, /*Request ready, must stay high until data_gnt_i is high for one cycle */
    output logic        data_reqpar_o,
    input  logic        data_gnt_i, /*The other side accepted the request. data_addr_o may change in the next cycle */
    input  logic        data_gntpar_i,
    input  logic        data_rvalid_i, /*data_rdata_i holds valid data when data_rvalid_i is high. This signal will be high for exactly one cycle per request. */
    input  logic        data_rvalidpar_i,
    output logic        data_we_o, /*Write Enable, high for writes, low for reads. Sent together with data_req_o */
    output logic [3:0]  data_be_o,  /*Byte Enable. Is set for the bytes to write/read, sent together with data_req_o */
    output logic [31:0] data_addr_o, /*Address */
    output logic [11:0] data_achk_o,
    output logic [1:0]  data_memtype_o,
    output logic [2:0]  data_prot_o,
    output logic        data_dbg_o,
    output logic [31:0] data_wdata_o,  /*Data to be written to memory, sent together with data_req_o */
    input  logic [31:0] data_rdata_i,  /*Data read from memory */
    input  logic [4:0]  data_rchk_i,
    input  logic        data_err_i
 );

    logic        alert_minor;
    logic        alert_major;

    logic core_sleep_o;

    logic        fencei_flush_ack;
    logic        fencei_flush_req;

    logic        [63:0] mcycle;

 // instantiate the core
    cv32e40s_wrapper #(
//        .PULP_XPULP (PULP_XPULP),
//        .PULP_CLUSTER (PULP_CLUSTER),
//        .FPU (FPU),
//        .PULP_ZFINX (PULP_ZFINX),
//        .NUM_MHPMCOUNTERS(NUM_MHPMCOUNTERS)
    ) wrapper_i (
        .clk_i (clk_i),
        .rst_ni (rst_ni),

        //.pulp_clock_en_i (1'b1),
        .scan_cg_en_i (1'b0),

        .boot_addr_i (32'h80),
        .mtvec_addr_i (32'h0),
        .dm_halt_addr_i (DM_HALTADDRESS),
        .mhartid_i (32'h0),
        .mimpid_patch_i (4'b0),
        .dm_exception_addr_i (32'h0),

        .instr_addr_o (instr_addr_o),
        .instr_achk_o (instr_achk_o),
        .instr_memtype_o (instr_memtype_o),
        .instr_prot_o (instr_prot_o),
        .instr_dbg_o(instr_dbg_o),
        .instr_req_o (instr_req_o),
        .instr_reqpar_o (instr_reqpar_o),
        .instr_rdata_i (instr_rdata_i),
        .instr_gnt_i (instr_gnt_i),
        .instr_gntpar_i (instr_gntpar_i),
        .instr_rvalid_i (instr_rvalid_i),
        .instr_rvalidpar_i (instr_rvalidpar_i),
        .instr_rchk_i (instr_rchk_i),
        .instr_err_i (instr_err_i),

        .data_req_o (data_req_o),
        .data_reqpar_o (data_reqpar_o),
        .data_gnt_i (data_gnt_i),
        .data_gntpar_i (data_gntpar_i),
        .data_rvalid_i (data_rvalid_i),
        .data_rvalidpar_i (data_rvalidpar_i),
        .data_we_o (data_we_o),
        .data_be_o (data_be_o),
        .data_addr_o (data_addr_o),
        .data_achk_o (data_achk_o),
        .data_memtype_o (data_memtype_o),
        .data_prot_o (data_prot_o),
        .data_dbg_o (data_dbg_o),
        .data_wdata_o (data_wdata_o),
        .data_rdata_i (data_rdata_i),
        .data_rchk_i (data_rchk_i),
        .data_err_i (data_err_i),

        .mcycle_o (mcycle),

        .irq_i (32'h0),

        .clic_irq_i(),
        .clic_irq_id_i(),
        .clic_irq_level_i(),
        .clic_irq_priv_i(),
        .clic_irq_shv_i(),

        .fencei_flush_req_o(fencei_flush_req),
        .fencei_flush_ack_i(1'b1),

        .alert_minor_o(alert_minor),
        .alert_major_o(alert_major),

        .debug_req_i (1'b0),
        .debug_havereset_o (),
        .debug_running_o (),
        .debug_halted_o (),

        .fetch_enable_i (1'b1),
        .core_sleep_o (core_sleep_o)
    );

endmodule
