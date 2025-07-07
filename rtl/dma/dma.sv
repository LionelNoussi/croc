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
  output  mgr_obi_req_t         dma_to_periph_port_req_o,
  input   mgr_obi_rsp_t         dma_to_periph_port_rsp_i,

  // // Master port to xbar
  output  mgr_obi_req_t         dma_to_xbar_port_req_o,
  input   mgr_obi_rsp_t         dma_to_xbar_port_rsp_i,

  // Slave port to enable setup by CPU/DEBUG
  input  sbr_obi_req_t          xbar_to_dma_port_req_i,
  output sbr_obi_rsp_t          xbar_to_dma_port_rsp_o
);

// DMA STATE REGISTERS AND SIGNALS
// -------------------------------

// Address Map
localparam logic [7:0] REG_SRC_ADDR       = 8'h00;
localparam logic [7:0] REG_DST_ADDR       = 8'h04;
localparam logic [7:0] REG_CONTROL        = 8'h08;
localparam logic [7:0] REG_CONDITION      = 8'h0C;
localparam logic [7:0] REG_INTERRUPT      = 8'h10;
localparam logic [7:0] REG_STATUS         = 8'h14;

// Registers to buffer setup requests
logic req_d, req_q;
logic we_d, we_q;
logic [ObiCfg.AddrWidth-1:0] addr_d, addr_q;
logic [ObiCfg.DataWidth-1:0] wdata_d, wdata_q;
logic [ObiCfg.IdWidth-1:0] id_d, id_q; // Id of the request, must be same for the response

// Signals used to create the response
logic [ObiCfg.DataWidth-1:0] rsp_data; // Data field of the obi response
logic rsp_err; // Error field of the obi response

// DMA-setup state-registers
logic[31:0] src_addr_d, src_addr_q;
logic[31:0] dst_addr_d, dst_addr_q;
logic[7:0]  offset_d, offset_q; // for source address
logic[10:0] repeat_d, repeat_q;
logic       byte_mode_d, byte_mode_q;
logic       activate_signal_d, activate_signal_q;
logic       interrupt_signal_d, interrupt_signal_q;

logic[7:0]  condition_offset_d, condition_offset_q;
logic[7:0]  condition_mask_d, condition_mask_q;
logic       condition_negate_d, condition_negate_q;
logic       condition_valid_d, condition_valid_q;

// RECEIVER STATE MACHINE REGISTERS AND SIGNALS
// --------------------------------------------

typedef enum logic [2:0] {
  REICEIVER_IDLE,
  REICEVER_WAIT_FOR_FIFO_SPACE,
  RECEIVER_WAIT_FOR_COND_GNT,
  RECEIVER_WAIT_FOR_COND_RVALID,
  RECEIVER_WAIT_FOR_GNT,
  REICEVER_WAIT_FOR_RVALID
} receiver_states_t;

receiver_states_t RCV_STARTING_STATE;
logic [31:0] RCV_STARTING_ADDR;
assign RCV_STARTING_STATE = condition_valid_q ? RECEIVER_WAIT_FOR_COND_GNT : RECEIVER_WAIT_FOR_GNT;
assign RCV_STARTING_ADDR = condition_valid_q ? (src_addr_q + condition_offset_q) : (src_addr_q + offset_q);

receiver_states_t receiver_state_d, receiver_state_q;
logic[10:0]       receiver_counter_d, receiver_counter_q;
logic[31:0]       read_addr_d, read_addr_q;
logic             receiver_req_d, receiver_req_q;

// ASSIGN DMA TO PERIPHERAL REQUEST SIGNAL
assign dma_to_periph_port_req_o.a.addr = read_addr_q;
assign dma_to_periph_port_req_o.a.we = 1'b0;
assign dma_to_periph_port_req_o.req = receiver_req_q;
assign dma_to_periph_port_req_o.a.wdata = '0;
assign dma_to_periph_port_req_o.a.be = 4'b1111;

// FIFO BUFFER
// ------------

// FIFO write side (inputs)
logic        fifo_wr_en_d,    fifo_wr_en_q;
logic [31:0] fifo_wr_data_d,  fifo_wr_data_q;

// FIFO read side (inputs)
logic        fifo_rd_en_d,    fifo_rd_en_q;

// FIFO outputs
logic        fifo_full;
logic        fifo_almost_full;
logic        fifo_empty;
logic        fifo_almost_empty;
logic [31:0] fifo_rd_data;

dma_fifo_buffer #(
  .DEPTH(4)
) i_dma_fifo_buffer (
  .clk_i          (clk_i),
  .rst_ni         (rst_ni),

  .wr_en_i        (fifo_wr_en_q),
  .wr_data_i      (fifo_wr_data_q),
  .full_o         (fifo_full),
  .almost_full_o  (fifo_almost_full),

  .rd_en_i        (fifo_rd_en_q),
  .rd_data_o      (fifo_rd_data),
  .empty_o        (fifo_empty),
  .almost_empty_o (fifo_almost_empty)
);

logic             inputs_ready;
assign            inputs_ready = !fifo_empty;

// SENDER STATE MACHINE REGISTERS AND SIGNALS
// ------------------------------------------

typedef enum logic [1:0] {
  SENDER_IDLE,
  SENDER_WAIT_FOR_NEW_INPUTS,
  SENDER_WAIT_FOR_GNT
} sender_states_t;

sender_states_t   sender_state_d, sender_state_q;
logic[10:0]       sender_counter_d, sender_counter_q;
logic             sender_req_d, sender_req_q;
logic[3:0]        sender_be_d, sender_be_q;
logic[31:0]       send_addr_d, send_addr_q;
logic[31:0]       sender_wdata_d, sender_wdata_q;

logic             dma_is_active;
assign            dma_is_active = (sender_state_q != SENDER_IDLE);

// ASSIGN DMA TO XBAR REQUEST SIGNAL
assign dma_to_xbar_port_req_o.a.addr = send_addr_q;
assign dma_to_xbar_port_req_o.a.we = 1'b1;
assign dma_to_xbar_port_req_o.req = sender_req_q;
assign dma_to_xbar_port_req_o.a.be = sender_be_q;
assign dma_to_xbar_port_req_o.a.wdata = sender_wdata_q;


// #############################################################################################################

// ------- PROCESS INCOMING REQUEST TO SETUP THE DMA OR READ STATUS REGISTERS -------------

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

// DMA SETUP LOGIC: Process Incoming Data from Slave Port
// ------------------------------------------------------
// Update internal state and set response signals
logic[7:0] reg_addr;
always_comb begin
  src_addr_d = src_addr_q;
  dst_addr_d = dst_addr_q;
  offset_d = offset_q;
  repeat_d = repeat_q;
  byte_mode_d = byte_mode_q;
  activate_signal_d = '0;
  interrupt_signal_d = '0;

  condition_offset_d = condition_offset_q;
  condition_mask_d = condition_mask_q;
  condition_negate_d = condition_negate_q;
  condition_valid_d = condition_valid_q;

  rsp_data = '0;
  rsp_err = '0;

  reg_addr = addr_q[9:2] << 2;  // Keep word alignment, supports 256B total space

  // If there is a request
  if (req_q) begin

    // Process write requests
    if (we_q) begin
      if (dma_is_active && reg_addr != REG_INTERRUPT) begin
        rsp_err = 1'b1; // Block writes while DMA is active, unless it's an interrupt
      end else begin
        case (reg_addr)
          REG_SRC_ADDR:     src_addr_d = wdata_q;
          REG_DST_ADDR:     dst_addr_d = wdata_q;
          REG_CONTROL: begin
            offset_d            = wdata_q[31:24];
            repeat_d            = wdata_q[23:13];
            byte_mode_d         = wdata_q[1];
            activate_signal_d   = wdata_q[0];
          end
          REG_CONDITION: begin
            condition_offset_d  = wdata_q[31:24];
            condition_mask_d    = wdata_q[23:16];
            condition_negate_d  = wdata_q[1];
            condition_valid_d   = wdata_q[0];
          end
          REG_INTERRUPT: interrupt_signal_d = 1'b1;
          default: rsp_err = 1'b1;
        endcase
      end

    // Process read requests
    end else begin
      case (reg_addr)
        REG_SRC_ADDR: rsp_data = src_addr_q;                                                    // Used to debug for now
        REG_DST_ADDR: rsp_data = dst_addr_q;                                                    // Used to debug for now
        REG_CONTROL: rsp_data = {offset_q, repeat_q, 11'b0, byte_mode_q, dma_is_active};  // Used to debug for now
        REG_CONDITION: rsp_data = {condition_offset_q, condition_mask_q, 14'b0, condition_negate_q, condition_valid_q};
        REG_STATUS: rsp_data = {31'b0, dma_is_active};   // All status registers
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
    byte_mode_q  <= '0;
    activate_signal_q   <= '0;
    interrupt_signal_q  <= '0;

    condition_offset_q  <= '0;
    condition_mask_q    <= '0;
    condition_negate_q  <= '0;
    condition_valid_q   <= '0;
  end else begin
    src_addr_q          <= src_addr_d;
    dst_addr_q          <= dst_addr_d;
    offset_q            <= offset_d;
    repeat_q            <= repeat_d;
    byte_mode_q         <= byte_mode_d;
    activate_signal_q   <= activate_signal_d;
    interrupt_signal_q  <= interrupt_signal_d;

    condition_offset_q  <= condition_offset_d;
    condition_mask_q    <= condition_mask_d;
    condition_negate_q  <= condition_negate_d;
    condition_valid_q   <= condition_valid_d;
  end
end

// -------- RECEIVER STATE MACHINE -------------

logic[7:0] condition_satisfied;
always_comb begin: RECEIVER_STATE_MACHINE
  condition_satisfied = '0;

  receiver_state_d = receiver_state_q;
  receiver_counter_d = receiver_counter_q;
  read_addr_d = read_addr_q;
  receiver_req_d = '0;
  fifo_wr_en_d = '0;
  fifo_wr_data_d = fifo_wr_data_q;

  case (receiver_state_q)
    REICEIVER_IDLE: begin
      if (activate_signal_q) begin
        receiver_state_d = RCV_STARTING_STATE;
        read_addr_d = RCV_STARTING_ADDR;
        receiver_req_d = 1'b1;
        receiver_counter_d = 1'b1;
      end
    end

    REICEVER_WAIT_FOR_FIFO_SPACE: begin
      if (interrupt_signal_q) begin
        receiver_state_d = REICEIVER_IDLE;
      end else begin
        if (!fifo_full) begin
          receiver_state_d = RCV_STARTING_STATE;
          read_addr_d = RCV_STARTING_ADDR;
          receiver_req_d = 1'b1;
        end
      end
    end

    RECEIVER_WAIT_FOR_COND_GNT: begin
      if (interrupt_signal_q) begin
        receiver_state_d = REICEIVER_IDLE;
      end else begin
        if (dma_to_periph_port_rsp_i.gnt) begin
          // TODO do I already need to check for rvalid here?
          receiver_state_d = RECEIVER_WAIT_FOR_COND_RVALID;
        end else begin // request hasn't been granted yet, request again
          receiver_req_d = 1'b1;
        end
      end
    end

    RECEIVER_WAIT_FOR_COND_RVALID: begin
      if (interrupt_signal_q) begin
        receiver_state_d = REICEIVER_IDLE;
      end else begin
        if (dma_to_periph_port_rsp_i.rvalid && !dma_to_periph_port_rsp_i.r.err) begin
          condition_satisfied = dma_to_periph_port_rsp_i.r.rdata >> (8 * read_addr_q[1:0]) & 8'hFF;

          condition_satisfied = condition_satisfied & condition_mask_q;
          if (condition_negate_q) begin
            condition_satisfied = !condition_satisfied;
          end

          if (condition_satisfied) begin
            receiver_state_d = RECEIVER_WAIT_FOR_GNT;
            read_addr_d = src_addr_q + offset_q;
            receiver_req_d = 1'b1;
          end else begin
            receiver_state_d = RECEIVER_WAIT_FOR_COND_GNT;
            read_addr_d = src_addr_q + condition_offset_q;
            receiver_req_d = 1'b1;
          end
        end
      end
    end

    RECEIVER_WAIT_FOR_GNT: begin
      if (interrupt_signal_q) begin
        receiver_state_d = REICEIVER_IDLE;
      end else begin
        if (dma_to_periph_port_rsp_i.gnt) begin
          // TODO do I already need to check for rvalid here,
          // or is it impossible to have the return data in the same
          // cycle as a request is made and granted?
          receiver_state_d = REICEVER_WAIT_FOR_RVALID;
        end else begin // request hasn't been granted yet, request again
          receiver_req_d = 1'b1;
        end
      end
    end

    REICEVER_WAIT_FOR_RVALID: begin
      if (interrupt_signal_q) begin
        receiver_state_d = REICEIVER_IDLE;
      end else begin
        if (dma_to_periph_port_rsp_i.rvalid && !dma_to_periph_port_rsp_i.r.err) begin
          fifo_wr_en_d = 1'b1;
          fifo_wr_data_d = dma_to_periph_port_rsp_i.r.rdata;

          receiver_counter_d = receiver_counter_q + 1;

          if (receiver_counter_q == repeat_q) begin
            receiver_state_d = REICEIVER_IDLE;
          end else if (!fifo_almost_full) begin

            receiver_state_d = RCV_STARTING_STATE;
            read_addr_d = RCV_STARTING_ADDR;
            receiver_req_d = 1'b1;

          end else begin
            receiver_state_d = REICEVER_WAIT_FOR_FIFO_SPACE;
          end
        end
      end
    end

    default: receiver_state_d = REICEIVER_IDLE;
  endcase
end

always_ff @(posedge (clk_i) or negedge (rst_ni)) begin
  if (!rst_ni) begin
    receiver_state_q <= REICEIVER_IDLE;
    receiver_counter_q <= '0;
    read_addr_q <= '0;
    receiver_req_q <= '0;
    
    fifo_wr_en_q <= '0;
    fifo_wr_data_q <= '0;
  end else begin
    receiver_counter_q <= receiver_counter_d;
    receiver_state_q <= receiver_state_d;
    read_addr_q <= read_addr_d;
    receiver_req_q <= receiver_req_d;

    fifo_wr_en_q <= fifo_wr_en_d;
    fifo_wr_data_q <= fifo_wr_data_d;
  end
end

// -------- SENDER STATE MACHINE -----------

always_comb begin: SENDER_STATE_MACHINE
  sender_counter_d = sender_counter_q;
  send_addr_d = send_addr_q;
  sender_state_d = sender_state_q;
  sender_req_d = '0;
  sender_be_d = sender_be_q;
  sender_wdata_d = sender_wdata_q;
  fifo_rd_en_d = '0;

  // dma_to_xbar_port_req_o.a.wdata = 32'hADAC_ABAA;

  case (sender_state_q)

    SENDER_IDLE: begin
      if (activate_signal_q) begin
        // TODO add asserts here that check the validity of all the fields, before activating the dma
        sender_counter_d = 11'd1;
        sender_state_d = SENDER_WAIT_FOR_NEW_INPUTS;
        send_addr_d = dst_addr_q;
      end
    end

    SENDER_WAIT_FOR_NEW_INPUTS: begin
      if (interrupt_signal_q) begin
        sender_state_d = SENDER_IDLE;
      end else if (inputs_ready) begin

        // Read the inputs from the fifo buffer and set correct byte enable
        if (byte_mode_q) begin
          sender_wdata_d = {fifo_rd_data[7:0], fifo_rd_data[7:0], fifo_rd_data[7:0], fifo_rd_data[7:0]};
          sender_be_d = 4'b0001 << (send_addr_q % 4);
        end else begin
          sender_wdata_d = fifo_rd_data;
          sender_be_d = 4'b1111;
        end
        
        fifo_rd_en_d = 1'b1;  // Tell the fifo that we read the inputs.
        sender_req_d = 1'b1;  // Send a write request
        sender_state_d = SENDER_WAIT_FOR_GNT;  // Switch to state which waits until the write request is granted

      end else begin
        sender_state_d = SENDER_WAIT_FOR_NEW_INPUTS;
      end
    end

    SENDER_WAIT_FOR_GNT: begin
      if (interrupt_signal_q) begin
        sender_state_d = SENDER_IDLE;
      end else begin

        // Write request has been granted.
        // Update counters and setup next phase
        if (dma_to_xbar_port_rsp_i.gnt) begin

          // Increase counter to check if finished
          sender_counter_d = sender_counter_q + 1;

          // Increase address and set proper byte_enable
          if (byte_mode_q) begin
            send_addr_d = send_addr_q + 1;
            sender_be_d = 4'b0001 << ((send_addr_q + 1) % 4);
          end else begin // if not in byte mode, then we are in word-mode
            send_addr_d = send_addr_q + 4;
            sender_be_d = 4'b1111;
          end

          // Check if finished or if new inputs are ready
          if (sender_counter_q == repeat_q) begin
            sender_state_d = SENDER_IDLE;
          end else if (inputs_ready) begin

            // Read the inputs from the fifo buffer and set correct byte enable
            if (byte_mode_q) begin
              sender_wdata_d = {fifo_rd_data[7:0], fifo_rd_data[7:0], fifo_rd_data[7:0], fifo_rd_data[7:0]};
            end else begin
              sender_wdata_d = fifo_rd_data;
            end

            fifo_rd_en_d = 1'b1;  // Tell the fifo that we read the inputs.
            sender_req_d = 1'b1;  // Send an obi request.
            sender_state_d = SENDER_WAIT_FOR_GNT;
          end else begin
            sender_req_d = 1'b0;
            sender_state_d = SENDER_WAIT_FOR_NEW_INPUTS;
          end
        
        // Else still waiting for grant, so keep requesting
        // with same data
        end else begin
          sender_req_d = 1'b1;
        end
      end
    end

    default: begin
      sender_state_d = SENDER_IDLE;
    end
  endcase
end

always_ff @(posedge (clk_i) or negedge (rst_ni)) begin

  if (!rst_ni) begin
    sender_counter_q <= '0;
    sender_state_q <= SENDER_IDLE;
    sender_req_q <= '0;
    send_addr_q <= '0;
    sender_be_q <= '0;
    sender_wdata_q <= '0;
    fifo_rd_en_q <= '0;
  end else begin
    sender_counter_q <= sender_counter_d;
    sender_state_q <= sender_state_d;
    sender_req_q <= sender_req_d;
    sender_be_q <= sender_be_d;
    send_addr_q <= send_addr_d;
    sender_wdata_q <= sender_wdata_d;
    fifo_rd_en_q <= fifo_rd_en_d;
  end

end



endmodule