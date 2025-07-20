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
      shift_reg_out <= 8'hA5;
    end else if (cs_falling_edge) begin
      // Update with new value each time CS goes low
      // (Optional: cycle through dummy values)
      shift_reg_out <= shift_reg_out + 8'h1;  // e.g. A5, A6, A7, ...
    end
  end

  // ------------------------------
  // Sample incoming bits (rising edge of SCLK)
  // ------------------------------
  always_ff @(posedge sclk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      shift_reg_in <= 8'h00;
      bit_cnt      <= 3'd0;
    end else if (!cs_n_i) begin
      shift_reg_in <= {shift_reg_in[6:0], mosi_i};
      bit_cnt      <= bit_cnt + 1;
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
      $display("@%t | [SPI SLAVE] Received byte: 0x%02h", $time, {shift_reg_in[6:0], mosi_i});
    end
  end

endmodule



// module spi_slave (
//     input  logic clk_i,       // simulation time clock
//     input  logic rst_ni,

//     input  logic sclk_i,      // SPI clock from master
//     input  logic cs_n_i,      // active-low chip select
//     input  logic mosi_i,      // data from master
//     output logic miso_o       // data to master
// );

//   logic [7:0] shift_reg_in;
//   logic [7:0] shift_reg_out;
//   logic [2:0] bit_cnt;
//   logic       cs_n_i_d;
//   logic [7:0] shift_reg_out_next;
//   logic       miso_bit;

//   // CS falling edge detection
//   always_ff @(posedge clk_i or negedge rst_ni) begin
//     if (!rst_ni)
//       cs_n_i_d <= 1'b1;
//     else
//       cs_n_i_d <= cs_n_i;
//   end

//   wire cs_falling_edge = (cs_n_i_d == 1'b1) && (cs_n_i == 1'b0);

//   // Load initial dummy value at CS falling edge
//   always_ff @(posedge clk_i or negedge rst_ni) begin
//     if (!rst_ni) begin
//       shift_reg_out <= 8'hA5;
//     end else if (cs_falling_edge) begin
//       shift_reg_out <= shift_reg_out + 8'h1;  // optional: cycle through dummy values
//     end
//   end

//   // Sample incoming bits on rising edge of SCLK
//   always_ff @(posedge sclk_i or negedge rst_ni) begin
//     if (!rst_ni) begin
//       shift_reg_in <= 8'h00;
//       bit_cnt      <= 3'd0;
//     end else if (!cs_n_i) begin
//       shift_reg_in <= {shift_reg_in[6:0], mosi_i};
//       bit_cnt      <= bit_cnt + 1;
//     end
//   end

//   // Set miso_o from current MSB; shift register in next cycle
//   always_ff @(negedge sclk_i or negedge rst_ni) begin
//     if (!rst_ni) begin
//       miso_bit      <= 1'b0;
//       shift_reg_out <= 8'hA5;
//     end else if (!cs_n_i) begin
//       miso_bit      <= shift_reg_out[7];                         // set output bit
//       shift_reg_out <= {shift_reg_out[6:0], 1'b0};               // then shift
//     end else begin
//       miso_bit <= 1'b0;
//     end
//   end

//   assign miso_o = (!cs_n_i) ? miso_bit : 1'b0;

//   // Optional debug
//   always_ff @(posedge sclk_i) begin
//     if (!cs_n_i && bit_cnt == 3'd7) begin
//       $display("@%t | [SPI SLAVE] Received byte: 0x%02h", $time, {shift_reg_in[6:0], mosi_i});
//     end
//   end

// endmodule