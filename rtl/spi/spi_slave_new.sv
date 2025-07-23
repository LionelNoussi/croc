module spi_slave (
    input  logic clk_i,
    input  logic rst_ni,

    input  logic sclk_i,
    input  logic cs_n_i,
    input  logic mosi_i,
    output logic miso_o
);

  typedef enum logic [1:0] {
    RX_CMD, RX_ADDR1, RX_ADDR0, RX_DATA
  } state_t;

  state_t state_q, state_d;

  logic [7:0] shift_reg_in;
  logic [7:0] shift_reg_out;
  logic [2:0] bit_cnt;
  logic [15:0] addr;
  logic [7:0] cmd;
  logic [15:0] current_addr;
  logic [7:0] mem [0:65535];  // 64 KB

  logic cs_n_i_d;
  wire cs_falling_edge = (cs_n_i_d == 1'b1) && (cs_n_i == 1'b0);

  // Load memory from HEX file
  initial begin
    $readmemh("memory.hex", mem);
  end

  // CS edge detection
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      cs_n_i_d <= 1'b1;
    else
      cs_n_i_d <= cs_n_i;
  end

  // Incoming shift reg
  always_ff @(posedge sclk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      shift_reg_in <= 8'h00;
      bit_cnt      <= 3'd0;
    end else if (!cs_n_i) begin
      shift_reg_in <= {shift_reg_in[6:0], mosi_i};
      bit_cnt      <= bit_cnt + 1;
    end
  end

  // Outgoing shift reg
  always_ff @(negedge sclk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      shift_reg_out <= 8'hFA;
      miso_o        <= 1'b0;
    end else if (!cs_n_i) begin
      miso_o        <= shift_reg_out[7];
      shift_reg_out <= {shift_reg_out[6:0], 1'b0};
    end else begin
      miso_o <= 1'b0;
    end
  end

  // State machine (protocol level)
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      state_q      <= RX_CMD;
      cmd          <= 8'h00;
      addr         <= 16'h0000;
      current_addr <= 16'h0000;
    end else if (cs_falling_edge) begin
      state_q <= RX_CMD;
    end else if (!cs_n_i && bit_cnt == 3'd7 && sclk_i == 1'b1) begin
      case (state_q)
        RX_CMD: begin
          cmd <= {shift_reg_in[6:0], mosi_i};
          state_q <= RX_ADDR1;
        end
        RX_ADDR1: begin
          addr[15:8] <= {shift_reg_in[6:0], mosi_i};
          state_q <= RX_ADDR0;
        end
        RX_ADDR0: begin
          addr[7:0] <= {shift_reg_in[6:0], mosi_i};
          current_addr <= {addr[15:8], {shift_reg_in[6:0], mosi_i}};
          state_q <= RX_DATA;
        end
        RX_DATA: begin
          if (cmd[2:1] == 2'b10) begin  // Write
            mem[current_addr] <= {shift_reg_in[6:0], mosi_i};
          end
          current_addr <= current_addr + 1;
        end
      endcase
    end
  end

  // Prepare shift_reg_out for next byte (if read)
  always_ff @(posedge sclk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      shift_reg_out <= 8'hFA;
    end else if (!cs_n_i && bit_cnt == 3'd0 && state_q == RX_DATA && cmd[2:1] == 2'b01) begin
      shift_reg_out <= mem[current_addr];
    end
  end

  // Debug
  always_ff @(posedge sclk_i) begin
    if (!cs_n_i && bit_cnt == 3'd7)
      $display("@%t [SPI SLAVE] RX Byte = 0x%02h", $time, {shift_reg_in[6:0], mosi_i});
  end

endmodule
