// Copyright 2023 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// gives us the `FF(...) macro making it easy to have properly defined flip-flops
`include "common_cells/registers.svh"

// simple ROM
module user_rom #(
  /// The OBI configuration for all ports.
  parameter obi_pkg::obi_cfg_t           ObiCfg      = obi_pkg::ObiDefaultConfig,
  /// The request struct.
  parameter type                         obi_req_t   = logic,
  /// The response struct.
  parameter type                         obi_rsp_t   = logic
) (
  /// Clock
  input  logic clk_i,
  /// Active-low reset
  input  logic rst_ni,

  /// OBI request interface
  input  obi_req_t obi_req_i,
  /// OBI response interface
  output obi_rsp_t obi_rsp_o
);

  // Define some registers to hold the requests fields
  logic req_d, req_q; // Request valid
  logic we_d, we_q; // Write enable
  logic [ObiCfg.AddrWidth-1:0] addr_d, addr_q; // Internal address of the word to read
  logic [ObiCfg.IdWidth-1:0] id_d, id_q; // Id of the request, must be same for the response

  // Signals used to create the response
  logic [ObiCfg.DataWidth-1:0] rsp_data; // Data field of the obi response
  logic rsp_err; // Error field of the obi response

  // Wire the registers holding the request
  assign req_d = obi_req_i.req;
  assign id_d = obi_req_i.a.aid;
  assign we_d = obi_req_i.a.we;
  assign addr_d = obi_req_i.a.addr;
  always_ff @(posedge (clk_i) or negedge (rst_ni)) begin
    if (!rst_ni) begin
      req_q <= '0;
      id_q <= '0;
      we_q <= '0;
      addr_q <= '0;
    end else begin
      req_q <= req_d;
      id_q <= id_d;
      we_q <= we_d;
      addr_q <= addr_d;
    end
  end

  logic [31:0] rom_words[0:7];

  initial begin
    // Pack bytes into 32-bit words, LSB = lowest address byte
    rom_words[0] = { "L", "N", "&", "L" };    // bytes 3..0
    rom_words[1] = { "K", "'", "s", " " };    // bytes 7..4
    rom_words[2] = { "A", "S", "I", "C" };    // bytes 11..8
    rom_words[3] = 32'h00000000;
    rom_words[4] = 32'h00000000;
    rom_words[5] = 32'h00000000;
    rom_words[6] = 32'h00000000;
    rom_words[7] = 32'h00000000;
  end

  logic [2:0] word_addr;  // 3 bits to address 8 words

  always_comb begin
    rsp_data = '0;
    rsp_err  = 1'b0;

    word_addr = addr_q[4:2];  // assuming addr_q is byte address, aligned to 4 bytes

    if (req_q) begin
      if (!we_q) begin
        rsp_data = {
          rom_words[word_addr][7:0],      // lowest byte
          rom_words[word_addr][15:8],
          rom_words[word_addr][23:16],
          rom_words[word_addr][31:24]     // highest byte
        };
      end else begin
        rsp_err = 1'b1;
      end
    end
  end

  // Wire the response
  // A channel
  assign obi_rsp_o.gnt = obi_req_i.req;
  // R channel:
  assign obi_rsp_o.rvalid = req_q;
  assign obi_rsp_o.r.rdata = rsp_data;
  assign obi_rsp_o.r.rid = id_q;
  assign obi_rsp_o.r.err = rsp_err;
  assign obi_rsp_o.r.r_optional = '0;

endmodule