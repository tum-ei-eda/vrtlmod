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
/// @file cv32e40p_top.sv
/// @date 2021-10-26
/// @brief cv32e40p top-level module with traced signals
/// for more convenient verilator interface
////////////////////////////////////////////////////////////////////////////////////////////////////

module cv32e40p
#(
  parameter INSTR_RDATA_WIDTH = 32,
  parameter ADDR_WIDTH = 32,
  //parameter BOOT_ADDR = 'h80,
  parameter PULP_XPULP = 0,
  parameter PULP_CLUSTER = 0,
  parameter FPU = 0,
  parameter PULP_ZFINX = 0,
  parameter NUM_MHPMCOUNTERS = 1,
  parameter DM_HALTADDRESS = 32'h1A110800
 )
(
// Clock and Reset
    input logic           clk_i,
    input logic           rst_ni,

// Instruction Memory Interface
    output logic instr_req_o,  /*Request ready, must stay high until instr_gnt_i is high for one cycle */
    input logic instr_gnt_i,  /*The other side accepted the request. instr_addr_o may change in the next cycle */
    input logic instr_rvalid_i, /*instr_rdata_is holds valid data when instr_rvalid_i is high. This signal will be high for exactly one cycle per request. */
    output logic [ADDR_WIDTH-1:0] instr_addr_o,  /*Address */
    input logic [INSTR_RDATA_WIDTH-1:0] instr_rdata_i,  /*Data read from memory */

// Data Memory Interface
    output logic data_req_o,     /*Request ready, must stay high until data_gnt_i is high for one cycle */
    input logic data_gnt_i,    /*The other side accepted the request. data_addr_o may change in the next cycle */
    input logic data_rvalid_i,  /*data_rdata_i holds valid data when data_rvalid_i is high. This signal will be high for exactly one cycle per request. */
    output logic [ADDR_WIDTH-1:0] data_addr_o,    /*Address */
    output logic data_we_o,    /*Write Enable, high for writes, low for reads. Sent together with data_req_o */
    output logic [3:0] data_be_o,    /*Byte Enable. Is set for the bytes to write/read, sent together with data_req_o */
    input logic [31:0] data_rdata_i,  /*Data read from memory */
    output logic [31:0] data_wdata_o   /*Data to be written to memory, sent together with data_req_o */

// Interrupt inputs
    //input logic irq_software_i,
    //input logic irq_timer_i,
    //input logic irq_external_i,
    //input logic [15:0] irq_fast_i,

// CPU Control Signals
    //input logic [ADDR_WIDTH-1:0] boot_addr_i,
    //input logic [ADDR_WIDTH-1:0] hart_id_i,
    //input logic fetch_enable_i,
    //input logic scan_cg_en_i,
 );

    import cv32e40p_apu_core_pkg::*;

    logic irq_ack_o;
    logic [4:0] irq_id_o;


    logic debug_req_i;
    assign debug_req_i = 1'b0;

    logic core_sleep_o;

// APU Core to FP Wrapper
    logic apu_req;
    logic [APU_NARGS_CPU-1:0][31:0] apu_operands;
    logic [APU_WOP_CPU-1:0] apu_op;
    logic [APU_NDSFLAGS_CPU-1:0] apu_flags;


// APU FP Wrapper to Core
    logic apu_gnt;
    logic apu_rvalid;
    logic [31:0] apu_rdata;
    logic [APU_NUSFLAGS_CPU-1:0] apu_rflags;

 // instantiate the core
    cv32e40p_wrapper #(
        .PULP_XPULP (PULP_XPULP),
        .PULP_CLUSTER (PULP_CLUSTER),
        .FPU (FPU),
        .PULP_ZFINX (PULP_ZFINX),
        .NUM_MHPMCOUNTERS(NUM_MHPMCOUNTERS)
    ) wrapper_i (
        .clk_i (clk_i),
        .rst_ni (rst_ni),

        .pulp_clock_en_i (1'b1),
        .scan_cg_en_i (1'b0),

        .boot_addr_i (32'h80),
        .mtvec_addr_i (32'h0),
        .dm_halt_addr_i (DM_HALTADDRESS),
        .hart_id_i (32'h0),
        .dm_exception_addr_i (32'h0),

        .instr_addr_o (instr_addr_o),
        .instr_req_o (instr_req_o),
        .instr_rdata_i (instr_rdata_i),
        .instr_gnt_i (instr_gnt_i),
        .instr_rvalid_i (instr_rvalid_i),

        .data_addr_o (data_addr_o),
        .data_wdata_o (data_wdata_o),
        .data_we_o (data_we_o),
        .data_req_o (data_req_o),
        .data_be_o (data_be_o),
        .data_rdata_i (data_rdata_i),
        .data_gnt_i (data_gnt_i),
        .data_rvalid_i (data_rvalid_i),

        .apu_req_o (apu_req),
        .apu_gnt_i (apu_gnt),
        .apu_operands_o(apu_operands),
        .apu_op_o (apu_op),
        .apu_flags_o (apu_flags),
        .apu_rvalid_i (apu_rvalid),
        .apu_result_i (apu_rdata),
        .apu_flags_i (apu_rflags),

        .irq_i (32'h0),
        .irq_ack_o(irq_ack_o),
        .irq_id_o (irq_id_o),

        .debug_req_i (1'b0),
        .debug_havereset_o (),
        .debug_running_o (),
        .debug_halted_o (),

        .fetch_enable_i (1'b1),
        .core_sleep_o (core_sleep_o)
    );
    generate
        if (FPU) begin
            cv32e40p_fp_wrapper fp_wrapper_i (
                .clk_i         (clk_i),
                .rst_ni        (rst_ni),
                .apu_req_i     (apu_req),
                .apu_gnt_o     (apu_gnt),
                .apu_operands_i(apu_operands),
                .apu_op_i      (apu_op),
                .apu_flags_i   (apu_flags),
                .apu_rvalid_o  (apu_rvalid),
                .apu_rdata_o   (apu_rdata),
                .apu_rflags_o  (apu_rflags)
            );
        end else begin
            assign apu_gnt      = '0;
            assign apu_operands = '0;
            assign apu_op       = '0;
            assign apu_flags    = '0;
            assign apu_rvalid   = '0;
            assign apu_rdata    = '0;
            assign apu_rflags   = '0;
        end
    endgenerate
endmodule // cv32e40p
