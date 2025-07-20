// Copyright 2024 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51
//
// Authors:
// - TODO: Add authors

`include "common_cells/registers.svh"

module spi #(
    /// The OBI configuration for all ports.
    parameter obi_pkg::obi_cfg_t ObiCfg = obi_pkg::ObiDefaultConfig,
    /// OBI request type
    parameter type obi_req_t         = logic,
    /// OBI response type
    parameter type obi_rsp_t         = logic
) (
    /// Primary input clock
    input  logic         clk_i,
    /// Asynchronous active-low reset
    input  logic         rst_ni,

    /// Control interface from interconnect (request).
    input  obi_req_t     obi_req_i,
    /// Control interface back into interconnect (response).
    output obi_rsp_t     obi_rsp_o,

    /// SPI signals
    output logic         sclk_o,
    output logic         mosi_o,
    input  logic         miso_i,
    output logic         cs_n_o
);

  import spi_reg_pkg::*;
  localparam logic [7:0] CLK_DIV_MAX = 8'd5;
  //-----------------------------------------------------------------------------------------------
  // Instantiations
  //-----------------------------------------------------------------------------------------------

  // Internal Signals
  spi_reg2hw_t reg2hw; // Interface from Register to Internal SPI Logic(HW)
  spi_hw2reg_t hw2reg; // Interface from Internal SPI Logic(HW) to Register

  // Instantiate register file
    spi_reg_top #(
        .obi_req_t(obi_req_t),
        .obi_rsp_t(obi_rsp_t)
    ) i_reg_file (
        .clk_i,
        .rst_ni,
        .obi_req_i,
        .obi_rsp_o,
        .reg2hw(reg2hw),
        .hw2reg(hw2reg)
    );

    typedef enum logic [1:0] {
        IDLE,
        LOAD,
        SHIFT,
        DONE
    } spi_state_e;

    spi_state_e spi_state_d, spi_state_q;

    logic [7:0] tx_shift_reg, rx_shift_reg;
    logic [2:0] bit_cnt;
    logic [7:0] clk_div_count;
    logic       sclk_int;

    assign sclk_o = sclk_int;
    assign mosi_o = tx_shift_reg[7];
    assign cs_n_o = (spi_state_q == SHIFT) ? 1'b0 : 1'b1;

    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            spi_state_q = IDLE;
            tx_shift_reg  <= 0;
            rx_shift_reg  <= 0;
            bit_cnt       <= 0;
            clk_div_count <= 0;
            sclk_int      <= 0;
        end else begin
            spi_state_q = spi_state_d;
            case (spi_state_q) 
                LOAD: begin
                    rx_shift_reg <= 8'h0;
                    tx_shift_reg <= reg2hw.tx_data;
                    bit_cnt <= 3'h0;
                    sclk_int <= 0;
                    clk_div_count <= 0;
                end
                SHIFT: begin
                    clk_div_count <= clk_div_count +1;
                    if (clk_div_count == CLK_DIV_MAX) begin
                        clk_div_count <= 8'h0;
                        sclk_int <= ~sclk_int;
                        if(sclk_int == 1'b0) begin
                            rx_shift_reg <= {rx_shift_reg[6:0], miso_i};
                        end else begin
                            tx_shift_reg <= {tx_shift_reg[6:0], 1'b0};
                            bit_cnt <= bit_cnt + 1;
                        end
                    end
                end
                DONE: begin
                    hw2reg.rx_data <= rx_shift_reg;
                end
                default: ;
            endcase
        end
    end

    always_comb begin
        spi_state_d = spi_state_q;
        case (spi_state_q) 
            IDLE: begin
                if (reg2hw.control[0]) begin
                    spi_state_d = LOAD;
                end
            end
            LOAD: begin
                spi_state_d = SHIFT;
            end

            SHIFT: begin
                if((bit_cnt == 3'b111) && (clk_div_count == CLK_DIV_MAX)  && (sclk_int == 1'b1)) begin
                    spi_state_d = DONE;
                end
                // if(bit_cnt == 3'b111) begin
                //     spi_state_d = DONE;
                // end
            end

            DONE: begin
                if (!reg2hw.control[0]) begin
                    spi_state_d = IDLE;
                end
            end

            default: spi_state_d = IDLE;
        endcase
    end
endmodule