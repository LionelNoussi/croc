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

    typedef enum logic [2:0] {
        IDLE,
        LOAD,
        SHIFT,
        DONE
    } spi_state_e;



    spi_state_e spi_state_d, spi_state_q;

    logic [7:0] rx_shift_reg_q, rx_shift_reg_d, tx_shift_reg_q, tx_shift_reg_d;
    logic [2:0] bit_cnt_q, bit_cnt_d;
    logic [7:0] clk_div_count_q, clk_div_count_d;
    logic       sclk_int_q, sclk_int_d;
    logic [5:0] byte_cnt_q, byte_cnt_d;
    logic [7:0] cmd_q, cmd_d;
    logic [7:0] length_d, length_q;
    logic [2:0] rw_type_d, rw_type_q;

    // wire [1:0] rw_type = reg2hw.control[2:1];


    typedef enum logic [2:0] {
        BYTE_CMD, BYTE_LENGTH, BYTE_ADDR1, BYTE_ADDR0, BYTE_DATA
        } byte_type_e;
    byte_type_e byte_type;


    assign sclk_o = sclk_int_q;
    assign mosi_o = tx_shift_reg_q[7];
    assign cs_n_o = (spi_state_q == SHIFT) ? 1'b0 : 1'b1;

    always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
        spi_state_q     <= IDLE;
        bit_cnt_q       <= 0;
        clk_div_count_q <= 0;
        sclk_int_q      <= 0;
        byte_cnt_q      <= 0;
        cmd_q           <= 0;
        length_q        <= 0;
        rx_shift_reg_q  <= 0;
        tx_shift_reg_q  <= 0;
        rw_type_q       <= 0;
    end else begin
        spi_state_q     <= spi_state_d;
        bit_cnt_q       <= bit_cnt_d;
        clk_div_count_q <= clk_div_count_d;
        sclk_int_q      <= sclk_int_d;
        byte_cnt_q      <= byte_cnt_d;
        cmd_q           <= cmd_d;
        length_q        <= length_d;
        rx_shift_reg_q  <= rx_shift_reg_d;
        tx_shift_reg_q  <= tx_shift_reg_d;
        rw_type_q       <= rw_type_d;
    end
    end

    always_comb begin
        case (byte_cnt_q)
            6'd0: byte_type = BYTE_CMD;
            6'd1: byte_type = BYTE_LENGTH;
            6'd2: byte_type = BYTE_ADDR1;
            6'd3: byte_type = BYTE_ADDR0;
            default: byte_type = BYTE_DATA;
        endcase
    end

    always_comb begin
        spi_state_d     = spi_state_q;
        bit_cnt_d       = bit_cnt_q;
        clk_div_count_d = clk_div_count_q;
        sclk_int_d      = sclk_int_q;
        byte_cnt_d      = byte_cnt_q;
        cmd_d           = cmd_q;
        length_d        = length_q;
        rx_shift_reg_d  = rx_shift_reg_q;
        tx_shift_reg_d  = tx_shift_reg_q;
        rw_type_d       = rw_type_q;
        hw2reg.status[0] = 0; //is this correct? can i double assign here and in shift
        case (spi_state_q) 
            IDLE: begin
                if (reg2hw.control[0]) begin
                    spi_state_d = LOAD;
                end
            end
            LOAD: begin
                spi_state_d = SHIFT;
                rx_shift_reg_d = 0;
                bit_cnt_d = 0;
                sclk_int_d = 0;
                clk_div_count_d = 0;
                cmd_d = reg2hw.control;
                length_d = reg2hw.length;
                rw_type_d = reg2hw.control[2:1];
            end

            SHIFT: begin

                if((bit_cnt_q == 3'b111) && (clk_div_count_q +1  == CLK_DIV_MAX) && (sclk_int_q == 1'b1)) begin
                    spi_state_d = DONE;
                end

                clk_div_count_d = clk_div_count_q +1;
                if (clk_div_count_d == CLK_DIV_MAX) begin
                    clk_div_count_d = 8'h0;
                    sclk_int_d = ~sclk_int_q; //eventuell jetzt vertauscht
                    if(sclk_int_d == 1'b1) begin
                        rx_shift_reg_d = {rx_shift_reg_q[6:0], miso_i}; // does this still conflicts with non shift laoding
                    end else begin
                        tx_shift_reg_d = {tx_shift_reg_q[6:0], 1'b0}; //FIX
                        bit_cnt_d  = bit_cnt_q + 1;
                    end
                end

            end

            DONE: begin
                if (byte_cnt_q >= length_q +4) begin
                    spi_state_d = IDLE;
                end else begin
                    spi_state_d = LOAD;
                end

                if(rw_type_q == 2'b10) begin
                    hw2reg.rx_data[8*(byte_cnt_q - 4) +: 8] = rx_shift_reg_q; // how do i assign this properly, i dont understand anymore what this does
                end
                sclk_int_d = 0;
                
                if (byte_cnt_q >= length_q + 4) begin
                    hw2reg.status[0] = 1;
                    byte_cnt_d = 0;
                end else begin
                    byte_cnt_d = byte_cnt_q+1;
                end
            end

            default: spi_state_d = IDLE;
        endcase

        // determines which data has to be sent during the shift, possible problem with the shift state
        if(spi_state_q != SHIFT) begin
            case (byte_type)
                BYTE_CMD: begin 
                    tx_shift_reg_d = reg2hw.control;
                end
                BYTE_LENGTH: begin
                    tx_shift_reg_d = reg2hw.length;
                end
                BYTE_ADDR0: begin
                    tx_shift_reg_d = reg2hw.address_low;
                end
                BYTE_ADDR1: begin
                    tx_shift_reg_d = reg2hw.address_high;
                end
                BYTE_DATA: begin
                    if(rw_type_q == 2'b10) begin //write
                        tx_shift_reg_d = 8'h3d;
                    end
                end
            endcase
        end
    end
endmodule