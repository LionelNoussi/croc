module dma #(
  /// The OBI configuration for all ports.
  parameter obi_pkg::obi_cfg_t           ObiCfg      = obi_pkg::ObiDefaultConfig,
  /// The subordinate request struct.
  parameter type                         sbr_obi_req_t   = logic,
  /// The subordinate response struct.
  parameter type                         sbr_obi_rsp_t   = logic,
  /// The manager request struct.
  parameter type                         mgr_obi_req_t   = logic,
  /// The manager response struct.
  parameter type                         mgr_obi_rsp_t   = logic
) (
  input  logic                       clk_i,
  input  logic                       rst_ni,

  // // Master port to periph_demux
  // output  mgr_obi_req_t         dma_to_periph_port_req_i,
  // input   mgr_obi_rsp_t         dma_to_periph_port_rsp_o,

  // // Master port to xbar
  // output  mgr_obi_req_t         dma_to_xbar_port_req_i,
  // input   mgr_obi_rsp_t         dma_to_xbar_port_rsp_o,

  // Slave port to enable setup by CPU/DEBUG
  input  sbr_obi_req_t          xbar_to_dma_port_req_i,
  output sbr_obi_rsp_t          xbar_to_dma_port_rsp_o
  // output  logic             dma_active_o,
);

// Address Map
localparam logic [7:0] REG_SRC_ADDR       = 8'h00;
localparam logic [7:0] REG_DST_ADDR       = 8'h04;
localparam logic [7:0] REG_CONTROL        = 8'h08;
localparam logic [7:0] REG_CONDITION      = 8'h0C;
localparam logic [7:0] REG_INTERRUPT      = 8'h10;
localparam logic [7:0] REG_STATUS         = 8'h14;

// Registers to buffer requests
logic req_d, req_q;
logic we_d, we_q;
logic [ObiCfg.AddrWidth-1:0] addr_d, addr_q;
logic [ObiCfg.DataWidth-1:0] wdata_d, wdata_q;
logic [ObiCfg.IdWidth-1:0] id_d, id_q; // Id of the request, must be same for the response

// Signals used to create the response
logic [ObiCfg.DataWidth-1:0] rsp_data; // Data field of the obi response
logic rsp_err; // Error field of the obi response

// DMA-setup state
logic[31:0] src_addr_d, src_addr_q;
logic[31:0] dst_addr_d, dst_addr_q;
logic[7:0]  offset_d, offset_q;
logic[10:0] repeat_d, repeat_q;
logic       byte_word_select_d, byte_word_select_q;
logic       dma_active_d, dma_active_q;

logic[7:0]  condition_offset_d, condition_offset_q;
logic[7:0]  condition_mask_d, condition_mask_q;
logic       condition_negate_d, condition_negate_q;
logic       condition_valid_d, condition_valid_q;

// Internal counters
logic [15:0] repeat_counter_d, repeat_counter_q;

// Wire the request into the internal buffers
assign req_d    = xbar_to_dma_port_req_i.req;
assign id_d     = xbar_to_dma_port_req_i.a.aid;
assign we_d     = xbar_to_dma_port_req_i.a.we;
assign addr_d   = xbar_to_dma_port_req_i.a.addr;
assign wdata_d  = xbar_to_dma_port_req_i.a.wdata;
always_ff @(posedge (clk_i) or negedge (rst_ni)) begin
  if (!rst_ni) begin
    req_q <= '0;
    id_q <= '0;
    we_q <= '0;
    addr_q <= '0;
    wdata_q <= '0;
  end else begin
    req_q <= req_d;
    id_q <= id_d;
    we_q <= we_d;
    addr_q <= addr_d;
    wdata_q <= wdata_d;
  end
end

// ASSIGN SLAVE RESPONSE SIGNAL
// Always grant immediately, and throw errors if a bad request happened instead
assign xbar_to_dma_port_rsp_o.gnt = xbar_to_dma_port_req_i.req;
assign xbar_to_dma_port_rsp_o.rvalid = req_q;
assign xbar_to_dma_port_rsp_o.r.rdata = rsp_data; // 32'hffffffff;
assign xbar_to_dma_port_rsp_o.r.err = rsp_err;
assign xbar_to_dma_port_rsp_o.r.rid = id_q;
assign xbar_to_dma_port_rsp_o.r.r_optional = '0;

// DMA SETUP LOGIC
// ---------------
// Process Incoming Data from Slave Port
// Update internal state and set response signals
logic[7:0] reg_addr;
always_comb begin
  src_addr_d = src_addr_q;
  dst_addr_d = dst_addr_q;
  offset_d = offset_q;
  repeat_d = repeat_q;
  byte_word_select_d = byte_word_select_q;
  dma_active_d = dma_active_q;

  condition_offset_d = condition_offset_q;
  condition_mask_d = condition_mask_q;
  condition_negate_d = condition_negate_q;
  condition_valid_d = condition_valid_q;

  rsp_data = 32'd8;
  rsp_err = '0;

  reg_addr = addr_q[9:2] << 2;  // Keep word alignment, supports 256B total space

  // If there is a request
  if (req_q) begin

    // Process write requests
    if (we_q) begin
      if (dma_active_q && reg_addr != REG_INTERRUPT) begin
        rsp_err = 1'b1; // Block writes while DMA is active, unless it's an interrupt
      end else begin
        case (reg_addr)
          REG_SRC_ADDR:     src_addr_d = wdata_q;
          REG_DST_ADDR:     dst_addr_d = wdata_q;
          REG_CONTROL: begin
            offset_d           = wdata_q[31:24];
            repeat_d           = wdata_q[23:13];
            byte_word_select_d = wdata_q[1];
            dma_active_d       = wdata_q[0];
          end
          REG_CONDITION: begin
            condition_offset_d = wdata_q[31:24];
            condition_mask_d = wdata_q[23:16];
            condition_negate_d = wdata_q[1];
            condition_valid_d = wdata_q[0];
          end
          REG_INTERRUPT: dma_active_d = 1'b0;
          default: rsp_err = 1'b1;
        endcase
      end

    // Process read requests
    end else begin
      case (reg_addr)
        REG_SRC_ADDR: rsp_data = src_addr_q;                                                    // Used to debug for now
        REG_DST_ADDR: rsp_data = dst_addr_q;                                                    // Used to debug for now
        REG_CONTROL: rsp_data = {offset_q, repeat_q, 11'b0, byte_word_select_q, dma_active_q};  // Used to debug for now
        REG_CONDITION: rsp_data = {condition_offset_q, condition_mask_q, 14'b0, condition_negate_q, condition_valid_q};
        REG_STATUS: rsp_data = {31'b0, dma_active_q};   // All status registers
        default: rsp_err = 1'b1;
      endcase
    end
  end
end

// Internal-State Registers
always_ff @(posedge (clk_i) or negedge (rst_ni)) begin
  if (!rst_ni) begin
    src_addr_q          <= '0;
    dst_addr_q          <= '0;
    offset_q            <= '0;
    repeat_q            <= '0;
    byte_word_select_q  <= '0;
    dma_active_q        <= '0;

    condition_offset_q  <= '0;
    condition_mask_q    <= '0;
    condition_negate_q  <= '0;
    condition_valid_q   <= '0;
  end else begin
    src_addr_q          <= src_addr_d;
    dst_addr_q          <= dst_addr_d;
    offset_q            <= offset_d;
    repeat_q            <= repeat_d;
    byte_word_select_q  <= byte_word_select_d;
    dma_active_q        <= dma_active_d;

    condition_offset_q  <= condition_offset_d;
    condition_mask_q    <= condition_mask_d;
    condition_negate_q  <= condition_negate_d;
    condition_valid_q   <= condition_valid_d;
  end
end


// always_comb begin
//   if (dma_active_q) begin
    
//   end
// end

// assign dma_active_o = dma_active_q;

endmodule