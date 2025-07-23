module spi_slave (
    input  logic clk_i,       // simulation time clock
    input  logic rst_ni,

    input  logic sclk_i,      // SPI clock from master
    input  logic cs_n_i,      // active-low chip select
    input  logic mosi_i,      // data from master
    output logic miso_o       // data to master
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

  logic cs_n_i_d;  // delayed version for edge detection

    // Load memory from HEX file
  initial begin
    $readmemh("memory.hex", mem);
  end

  // ------------------------------
  // CS edge detection (falling)
  // ------------------------------
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      cs_n_i_d <= 1'b1;
    else
      cs_n_i_d <= cs_n_i;
  end

  wire cs_falling_edge  = (cs_n_i_d == 1'b1)  && (cs_n_i == 1'b0);
  wire cs_rising_edge   = (cs_n_i_d == 1'b0)  && (cs_n_i == 1'b1);
  // ------------------------------
  // Load and store values at CS falling and rising edge
  // ------------------------------
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      shift_reg_out <= 8'hFA;
      shift_reg_in <= 8'h00;
      mem[addr] <= mem[addr];
    end else if (cs_falling_edge) begin
      // Update with new value each time CS goes low
      // (Optional: cycle through dummy values)
      // shift_reg_out <= shift_reg_out + 8'h1;  // e.g. A5, A6, A7, ...
      miso_o        <= shift_reg_out[7];
      shift_reg_out <= mem[addr*8 += 8];
      shift_reg_in <= 8'h00;
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
      // $display("@%t | [SPI SLAVE] Received byte: 0x%02h", $time, {shift_reg_in[6:0], mosi_i});
    end

  end

endmodule
