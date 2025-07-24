module spi_slave (
    input  logic clk_i,       // simulation time clock
    input  logic rst_ni,

    input  logic sclk_i,      // SPI clock from master
    input  logic cs_n_i,      // active-low chip select
    input  logic mosi_i,      // data from master
    output logic miso_o       // data to master
);

  logic [7:0] shift_reg_in;
  logic [7:0] shift_reg_out;
  logic [2:0] bit_cnt;
  logic [7:0] dummy_value;
  logic [7:0] dummy_next;

  assign dummy_next = dummy_value + 1;

  logic cs_n_i_d;  // delayed version for edge detection

  // ------------------------------
  // CS edge detection (falling)
  // ------------------------------
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      cs_n_i_d <= 1'b1;
    else
      cs_n_i_d <= cs_n_i;
  end

  wire cs_falling_edge = (cs_n_i_d == 1'b1) && (cs_n_i == 1'b0);

  // ------------------------------
  // Load dummy value at CS falling edge
  // ------------------------------
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      shift_reg_out <= 8'hF5;
      dummy_value    <= 8'hF5;
    end else if (cs_falling_edge) begin
      // Update with new value each time CS goes low
      // (Optional: cycle through dummy values)
      // shift_reg_out <= shift_reg_out + 8'h1;  // e.g. A5, A6, A7, ...
      miso_o        <= dummy_value[7];
      shift_reg_out <= {dummy_value[6:0], 1'b0};
    end
  end

  // ------------------------------
  // Sample incoming bits (rising edge of SCLK)
  // ------------------------------
  always_ff @(posedge sclk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      shift_reg_in <= 8'h00;
      bit_cnt      <= 3'd0;
      miso_o <= 1'b0;
    end else if (!cs_n_i) begin

      // bit_cnt      <= bit_cnt + 1;


      if (bit_cnt == 3'd8) begin
          bit_cnt         <= 3'd0;
          // shift_reg_out   <= {(dummy_value + 1) [7:1], 1'b0};
          shift_reg_in    <= {7'b0000000, mosi_i};
      end else begin
          bit_cnt <= bit_cnt + 1;
          shift_reg_in <= {shift_reg_in[6:0], mosi_i};

      end
    end
  end

  // ------------------------------
  // Shift outgoing bits (falling edge of SCLK)
  // ------------------------------
  always_ff @(negedge sclk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      miso_o <= 1'b0;
    end else if (!cs_n_i) begin
      miso_o        <= shift_reg_out[7];
      shift_reg_out <= {shift_reg_out[6:0], 1'b0};
    end else begin
      miso_o <= 1'b0;
    end
  end

  // Print debug when byte is received
  always_ff @(posedge sclk_i) begin
    if (!cs_n_i && bit_cnt == 3'd7) begin
      // $display("@%t | [SPI SLAVE] Received byte: 0x%02h", $time, {shift_reg_in[6:0], mosi_i});
    end
  end

endmodule