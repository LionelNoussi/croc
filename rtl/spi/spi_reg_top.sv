// Copyright 2024 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

`include "common_cells/registers.svh"

module spi_reg_top import spi_reg_pkg::*; #(
    parameter obi_pkg::obi_cfg_t ObiCfg = obi_pkg::ObiDefaultConfig,
    parameter type obi_req_t = logic,
    parameter type obi_rsp_t = logic
) (
    input  logic clk_i,
    input  logic rst_ni,

    input  obi_req_t  obi_req_i,
    output obi_rsp_t obi_rsp_o,

    output spi_reg2hw_t reg2hw,
    input  spi_hw2reg_t hw2reg
);

  // OBI preparation signals
  logic valid_d, valid_q;
  logic we_d, we_q;
  logic req_d, req_q;
  logic [AddressWidth-1:0] write_addr;
  logic [AddressWidth-1:0] read_addr_d, read_addr_q;
  logic [ObiCfg.IdWidth-1:0] id_d, id_q;
  logic obi_err;
  logic w_err_d, w_err_q;
  logic [ObiCfg.DataWidth-1:0] obi_rdata, obi_wdata;
  logic obi_read_request, obi_write_request;

  // OBI response assignment
  always_comb begin
    obi_rsp_o              = '0;
    obi_rsp_o.r.rdata      = obi_rdata;
    obi_rsp_o.r.rid        = id_q;
    obi_rsp_o.r.err        = obi_err;
    obi_rsp_o.gnt          = obi_req_i.req;
    obi_rsp_o.rvalid       = valid_q;
  end

  assign obi_wdata         = obi_req_i.a.wdata;
  assign obi_read_request  = req_q & ~we_q;
  assign obi_write_request = obi_req_i.req & obi_req_i.a.we;
  assign id_d              = obi_req_i.a.aid;
  assign valid_d           = obi_req_i.req;
  assign write_addr        = obi_req_i.a.addr[AddressWidth-1:2];
  assign read_addr_d       = obi_req_i.a.addr[AddressWidth-1:2];
  assign we_d              = obi_req_i.a.we;
  assign req_d             = obi_req_i.req;

  `FF(id_q, id_d, '0, clk_i, rst_ni)
  `FF(valid_q, valid_d, '0, clk_i, rst_ni)
  `FF(read_addr_q, read_addr_d, '0, clk_i, rst_ni)
  `FF(req_q, req_d, '0, clk_i, rst_ni)
  `FF(we_q, we_d, '0, clk_i, rst_ni)
  `FF(w_err_q, w_err_d, '0, clk_i, rst_ni)

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // Registers
  ////////////////////////////////////////////////////////////////////////////////////////////////

  typedef struct packed {
      logic [7:0] control;
      logic [7:0] txrx_buffer [0:31];   
      logic [15:0] address;
      logic [7:0] length;
      logic [7:0] status;
  } spi_reg_fields_t;

  spi_reg_fields_t reg_d, reg_q;
  `FF(reg_q, reg_d, '0, clk_i, rst_ni)

  spi_reg_fields_t new_reg;

   ////////////////////////////////////////////////////////////////////////////////////////////////
  // COMB LOGIC
  ////////////////////////////////////////////////////////////////////////////////////////////////

  logic [ObiCfg.DataWidth-1:0] bit_mask;
  for (genvar i = 0; unsigned'(i) < ObiCfg.DataWidth/8; ++i ) begin : gen_write_mask
    assign bit_mask[8*i +: 8] = {8{obi_req_i.a.be[i]}};
  end

  always_comb begin
    // Defaults
    obi_rdata = '0;
    obi_err   = w_err_q;
    w_err_d   = 1'b0;
    new_reg   = reg_q;

    // Assign outputs to logic
    reg2hw.control = reg_q.control;
    reg2hw.txrx_buffer = reg_q.txrx_buffer;
    reg2hw.address = reg_q.address;
    reg2hw.length  = reg_q.length;

    // Update from logic
    new_reg.status = hw2reg.status;

    reg_d = new_reg;

    // WRITE
    if (obi_write_request) begin
      case ({write_addr, 2'b00})
        SPI_CONTROL_OFFSET: begin
          reg_d.control = (~bit_mask & new_reg.control) | (bit_mask & obi_wdata[7:0]);
        end
        // Write to TXRX buffer entries 0x008–0x027
        default: begin
          if (({write_addr, 2'b00} >= 12'h008) && ({write_addr, 2'b00} <= 12'h027)) begin
            int idx = {write_addr, 2'b00} - 12'h008;
            reg_d.txrx_buffer[idx] = obi_wdata[7:0];
          end else if ({write_addr, 2'b00} == 12'h028) begin
            reg_d.address[7:0] = obi_wdata[7:0];
          end else if ({write_addr, 2'b00} == 12'h029) begin
            reg_d.address[15:8] = obi_wdata[7:0];
          end else if ({write_addr, 2'b00} == 12'h02A) begin
            reg_d.length = obi_wdata[7:0];
          end else begin
            w_err_d = 1'b1;
          end
        end
      endcase
    end

    // READ
    if (obi_read_request) begin
      case ({read_addr_q, 2'b00})
        SPI_CONTROL_OFFSET: obi_rdata = {{24{1'b0}}, reg_q.control};
        SPI_STATUS_OFFSET:  obi_rdata = {{24{1'b0}}, reg_q.status};
        default: begin
          if (({read_addr_q, 2'b00} >= 12'h008) && ({read_addr_q, 2'b00} <= 12'h027)) begin
            int idx = {read_addr_q, 2'b00} - 12'h008;
            obi_rdata = {{24{1'b0}}, reg_q.txrx_buffer[idx]};
          end else if ({read_addr_q, 2'b00} == 12'h028) begin
            obi_rdata = {{24{1'b0}}, reg_q.address[7:0]};
          end else if ({read_addr_q, 2'b00} == 12'h029) begin
            obi_rdata = {{24{1'b0}}, reg_q.address[15:8]};
          end else if ({read_addr_q, 2'b00} == 12'h02A) begin
            obi_rdata = {{24{1'b0}}, reg_q.length};
          end else begin
            obi_rdata = 32'hDEAD_BEEF;
            obi_err   = 1'b1;
          end
        end
      endcase
    end
  end
endmodule