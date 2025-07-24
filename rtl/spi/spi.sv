// Copyright 2024 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51
//
// Authors:


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
//   localparam logic [4:0] CLK_DIV_MAX = 8'd5;
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
        .hw2reg(hw2reg),
        .rx_read_pulse(rx_pulse),
        .tx_write_pulse(tx_pulse)
    );

    typedef enum logic [2:0] {
        IDLE,
        LOAD,
        SHIFT,
        DONE
    } spi_state_e;

    logic spi_fifo_write;
    assign spi_fifo_write = (spi_state_q == DONE) && (rw_type_q == 2'b01) && (byte_type == BYTE_DATA);
    logic rx_pulse;
    logic obi_read_enable;
    // assign obi_read_enable = (obi_req_i.addr == SPI_RXBUFFER_OFFSET) && obi_req_i.req && !obi_req_i.we;

    logic [7:0] fifo_status;
    spi_fifo_buffer #(
        .DEPTH(16),
        .DATA_WIDTH(8)
    ) i_spi_rx_fifo (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .spi_write_i(spi_fifo_write),      // from FSM
        .spi_data_i(rx_shift_reg_q),  // from FSM
        .obi_read_en(rx_pulse),
        .rd_data_o(hw2reg.rx_data),
        .full_o(fifo_status[7]),
        .almost_full_o(fifo_status[6]),
        .empty_o(fifo_status[5]),
        .almost_empty_o(fifo_status[4])
    );


    logic tx_pulse;
    logic spi_tx_fifo_rd_en_q, spi_tx_fifo_rd_en_d;
    logic [7:0] tx_fifo_data;


    spi_fifo_buffer #(
        .DEPTH(16),
        .DATA_WIDTH(8)
    ) i_spi_tx_fifo (
        .clk_i(clk_i),
        .rst_ni(rst_ni),
        .spi_write_i(tx_pulse),        // core writes to tx_data register
        .spi_data_i(reg2hw.tx_data),     // core register value
        .obi_read_en(spi_tx_fifo_rd_en_q),    // FSM reads when sending
        .rd_data_o(tx_fifo_data),       // drive tx_shift_reg from here
        .full_o(fifo_status[3]),
        .almost_full_o(fifo_status[2]),
        .empty_o(fifo_status[1]),
        .almost_empty_o(fifo_status[0])
    );



    spi_state_e spi_state_d, spi_state_q;

    logic [7:0] rx_shift_reg_q, rx_shift_reg_d, tx_shift_reg_q, tx_shift_reg_d;
    logic [2:0] bit_cnt_q, bit_cnt_d;
    logic [7:0] clk_div_count_q, clk_div_count_d;
    logic       sclk_int_q, sclk_int_d;
    logic [5:0] byte_cnt_q, byte_cnt_d;
    logic [7:0] cmd_q, cmd_d;
    logic [7:0] length_d, length_q;
    logic [2:0] rw_type_d, rw_type_q;
    logic [4:0] clk_scale_q, clk_scale_d;
    logic [7:0] status_q, status_d;



    typedef enum logic [2:0] {
        BYTE_CMD, BYTE_LENGTH, BYTE_ADDR1, BYTE_ADDR0, BYTE_DATA
        } byte_type_e;
    byte_type_e byte_type;


    assign sclk_o = sclk_int_q;
    assign mosi_o = tx_shift_reg_q[7];
    assign cs_n_o = (spi_state_q == SHIFT) ? 1'b0 : 1'b1;
    
    assign hw2reg.status = status_q;
    assign hw2reg.fifo_status = fifo_status;

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
        spi_tx_fifo_rd_en_q <= 0;
        clk_scale_q     <= 1;
        status_q        <= 0;
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
        spi_tx_fifo_rd_en_q <= spi_tx_fifo_rd_en_d;
        clk_scale_q     <= clk_scale_d;
        status_q        <= status_d;
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
        spi_tx_fifo_rd_en_d = 0;
        clk_scale_d     = clk_scale_q;
        status_d        = status_q;
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
                clk_scale_d = reg2hw.control[7:3];
                length_d = reg2hw.length;
                rw_type_d = reg2hw.control[2:1];
                spi_tx_fifo_rd_en_d = (byte_type == BYTE_DATA) && (rw_type_q == 2'b10);
            end

            SHIFT: begin

                if((bit_cnt_q == 3'b111) && (clk_div_count_q +1  == clk_scale_q) && (sclk_int_q == 1'b1)) begin
                    spi_state_d = DONE;
                end

                clk_div_count_d = clk_div_count_q +1;
                if (clk_div_count_d == clk_scale_q) begin
                    clk_div_count_d = 8'h0;
                    sclk_int_d = ~sclk_int_q; //eventuell jetzt vertauscht
                    if(sclk_int_d == 1'b1) begin
                        rx_shift_reg_d = {rx_shift_reg_q[6:0], miso_i}; // does this still conflicts with non shift laoding
                    end else begin
                        tx_shift_reg_d = {tx_shift_reg_q[6:0], 1'b0}; //FIX
                        bit_cnt_d  = bit_cnt_q + 1;
                    end
                end

                // write logic to set a flag and write to a buffer when a transaction is done and its a read

            end

            DONE: begin
                if (byte_cnt_q >= length_q +3) begin
                    spi_state_d = IDLE;
                end else begin
                    spi_state_d = LOAD;
                end

                sclk_int_d = 0;
                if(byte_cnt_q >= 4) begin
                    status_d = byte_cnt_q - 3; // with this we can only send 32 bits, do we need to extend this?
                end
                if (byte_cnt_q >= length_q + 3) begin
                    byte_cnt_d = 0;
                end else begin
                    byte_cnt_d = byte_cnt_q+1;
                end
            end

            default: spi_state_d = IDLE;
        endcase

        // determines which data has to be sent during the shift
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
                        tx_shift_reg_d = tx_fifo_data;  // pull from fifo     // set fifo enable bit
                    end
                end
            endcase
        end
    end
endmodule