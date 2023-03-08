/*
 * Copyright 2021 Chair of EDA, Technical University of Munich
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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
/// @file cva6.sv
/// @date 2020-09-29
/// @brief Ariane/CVA6 top-level module with resolved AXI-signal components
/// for more convenient verilator interface
////////////////////////////////////////////////////////////////////////////////////////////////////

module cva6 import ariane_pkg::*; #(
	parameter ariane_pkg::ariane_cfg_t ArianeCfg     = ariane_pkg::ArianeDefaultConfig
) (
	input  logic                clk_i,
	input  logic                rst_ni,
// Core ID, Cluster ID and boot address are considered more or less static
	// input  logic [63:0]         boot_addr_i,  // reset boot address
	// input  logic [63:0]         hart_id_i,    // hart id in a multicore environment (reflected in a CSR)

// Interrupt inputs
	// input  logic [1:0]     irq_i,        // level sensitive IR lines, mip & sip (async)
	// input logic                 ipi_i,        // inter-processor interrupts (async)
// Timer facilities
	// input logic                 time_irq_i,   // timer interrupt in (async)
	// input logic                 debug_req_i,  // debug request (async)

// memory side, AXI Master
// output ariane_axi::req_t             axi_req_o,
// input  ariane_axi::resp_t            axi_resp_i
/* Compatibility with xiltlm dummies (AXI3, AXI4 autoconnectors) */
	output ariane_axi::id_t wid,

// Request/Response structs resolve
// typedef struct packed {
// aw_chan_t aw;
	output ariane_axi::id_t     awid,
	output ariane_axi::addr_t   awaddr,
	output axi_pkg::len_t       awlen,
	output axi_pkg::size_t      awsize,
	output axi_pkg::burst_t     awburst,
	output logic                awlock,
	output axi_pkg::cache_t     awcache,
	output axi_pkg::prot_t      awprot,
	output axi_pkg::qos_t       awqos,
	output axi_pkg::region_t    awregion,
// output axi_pkg::atop_t    awatop,
	output ariane_axi::user_t   awuser,
	output logic                awvalid,
// w_chan_t  w,
	output ariane_axi::data_t   wdata,
	output ariane_axi::strb_t   wstrb,
	output logic                wlast,
	output ariane_axi::user_t   wuser,
	output logic                wvalid,
	output logic                bready,
// ar_chan_t ar,
	output ariane_axi::id_t     arid,
	output ariane_axi::addr_t   araddr,
	output axi_pkg::len_t       arlen,
	output axi_pkg::size_t      arsize,
	output axi_pkg::burst_t     arburst,
	output logic                arlock,
	output axi_pkg::cache_t     arcache,
	output axi_pkg::prot_t      arprot,
	output axi_pkg::qos_t       arqos,
	output axi_pkg::region_t    arregion,
	output ariane_axi::user_t   aruser,
	output logic                arvalid,
	output logic                rready,
//} req_t,

// typedef struct packed {
	input logic                 awready,
	input logic                 arready,
	input logic                 wready,
	input logic                 bvalid,
// b_chan_t  b,
	input ariane_axi::id_t      bid,
	input axi_pkg::resp_t       bresp,
	input ariane_axi::user_t    buser,
	input logic	rvalid,
// r_chan_t  r,
	input ariane_axi::id_t      rid,
	input ariane_axi::data_t    rdata,
	input axi_pkg::resp_t       rresp,
	input logic                 rlast,
	input ariane_axi::user_t    ruser
//} resp_t;

);

	ariane_axi::req_t    axi_ariane_req;
	ariane_axi::resp_t   axi_ariane_resp;

	axi_pkg::atop_t      awatop;

// Request/Response structs resolve
// typedef struct packed {
// aw_chan_t aw;
	assign wid        = axi_ariane_req.aw.id;
	assign awaddr     = axi_ariane_req.aw.addr;
	assign awlen      = axi_ariane_req.aw.len;
	assign awsize     = axi_ariane_req.aw.size;
	assign awburst    = axi_ariane_req.aw.burst;
	assign awlock     = axi_ariane_req.aw.lock;
	assign awcache    = axi_ariane_req.aw.cache;
	assign awprot     = axi_ariane_req.aw.prot;
	assign awqos      = axi_ariane_req.aw.qos;
	assign awregion   = axi_ariane_req.aw.region;
	assign awatop     = axi_ariane_req.aw.atop;
	assign awuser     = axi_ariane_req.aw.user;
	assign awvalid    = axi_ariane_req.aw_valid;
// w_chan_t  w;
	assign wdata      = axi_ariane_req.w.data;
	assign wstrb      = axi_ariane_req.w.strb;
	assign wlast      = axi_ariane_req.w.last;
	assign wuser      = axi_ariane_req.w.user;
	assign wvalid     = axi_ariane_req.w_valid;
	assign bready     = axi_ariane_req.b_ready;
// ar_chan_t ar;
	assign arid       = axi_ariane_req.ar.id;
	assign araddr     = axi_ariane_req.ar.addr;
	assign arlen      = axi_ariane_req.ar.len;
	assign arsize     = axi_ariane_req.ar.size;
	assign arburst    = axi_ariane_req.ar.burst;
	assign arlock     = axi_ariane_req.ar.lock;
	assign arcache    = axi_ariane_req.ar.cache;
	assign arprot     = axi_ariane_req.ar.prot;
	assign arqos      = axi_ariane_req.ar.qos;
	assign arregion   = axi_ariane_req.ar.region;
	assign aruser     = axi_ariane_req.ar.user;
	assign arvalid    = axi_ariane_req.ar_valid;
	assign rready     = axi_ariane_req.r_ready;
//} req_t;

// typedef struct packed {
	assign axi_ariane_resp.aw_ready   = awready;
	assign axi_ariane_resp.ar_ready   = arready;
	assign axi_ariane_resp.w_ready    = wready;
	assign axi_ariane_resp.b_valid    = bvalid;
// b_chan_t  b;
	assign axi_ariane_resp.b.id       = bid;
	assign axi_ariane_resp.b.resp     = bresp;
	assign axi_ariane_resp.b.user     = buser;
	assign axi_ariane_resp.r_valid    = rvalid;
// r_chan_t  r;
	assign axi_ariane_resp.r.id       = rid;
	assign axi_ariane_resp.r.data     = rdata;
	assign axi_ariane_resp.r.resp     = rresp;
	assign axi_ariane_resp.r.last     = rlast;
	assign axi_ariane_resp.r.user     = ruser;
//} resp_t;


	ariane #(
		.ArianeCfg  ( ariane_soc::ArianeSocCfg )
	) i_ariane (
		.clk_i                ( clk_i ),
		.rst_ni               ( rst_ni ),
		.boot_addr_i          ( 32'h80000000 ), // start fetching from RAM
		.hart_id_i            ( 32'h0 ),
		.irq_i                (  ),
		.ipi_i                (  ),
		.time_irq_i           (  ),
		.debug_req_i          (  ),
		.axi_req_o            ( axi_ariane_req ),
		.axi_resp_i           ( axi_ariane_resp )
	);
endmodule // cva6
