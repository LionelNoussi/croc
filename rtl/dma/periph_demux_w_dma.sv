module periph_demux_w_dma #(
  /// The OBI configuration for all ports.
  parameter obi_pkg::obi_cfg_t ObiCfg      = obi_pkg::ObiDefaultConfig,
  /// The request struct for all ports.
  parameter type               obi_req_t   = logic,
  /// The response struct for all ports.
  parameter type               obi_rsp_t   = logic,
  /// The number of subordinate ports.
  parameter int unsigned       NumSbrPorts = 32'd0,
  /// The maximum number of outstanding transactions.
  parameter int unsigned       NumMaxTrans = 32'd0,
  /// The type of the port select signal.
  parameter type               select_t    = logic [cf_math_pkg::idx_width(NumSbrPorts)-1:0]
) (
  input  logic                       clk_i,
  input  logic                       rst_ni,

  input  select_t                    xbar_select_i,
  input  obi_req_t                   xbar_port_req_i,
  output obi_rsp_t                   xbar_port_rsp_o,

  input  logic                       dma_active_i,
  input  select_t                    dma_select_i,
  input  obi_req_t                   dma_port_req_i,
  output obi_rsp_t                   dma_port_rsp_o,

  output obi_req_t [NumSbrPorts-1:0] sbr_port_req_o,
  input  obi_rsp_t [NumSbrPorts-1:0] sbr_port_rsp_i
);

  /*

  I think this module should work likt this:

  1. Implements an extra demux for the dma
  2. Checks conflicts between dma and xbar select index, first come first serve
  3. If there is a conflict (both access at the same time), it stalls the dma requests
  */

  if (ObiCfg.Integrity) begin : gen_integrity_err
    $fatal(1, "unimplemented");
  end

  // stall requests to ensure in-order behavior. This only stalls requests to same ports.
  // If multiple requests are made to different ports, they are ignored until the first
  // one finishes.
  localparam int unsigned CounterWidth = cf_math_pkg::idx_width(NumMaxTrans);

  // XBAR SIGNALS
  logic xbar_cnt_up, xbar_cnt_down, xbar_overflow;
  logic [CounterWidth-1:0] xbar_in_flight;
  logic xbar_port_gnt;
  logic xbar_port_rready;

  select_t xbar_select_d, xbar_select_q;

  // DMA SIGNALS
  logic dma_cnt_up, dma_cnt_down, dma_overflow;
  logic [CounterWidth-1:0] dma_in_flight;
  logic dma_port_gnt;
  logic dma_port_rready;

  select_t dma_select_d, dma_select_q;

  // PROCESS REQUESTS AND CHECK FOR CONFLICTS
  always_comb begin : proc_req
    for (int i = 0; i < NumSbrPorts; i++) begin
      sbr_port_req_o[i].req = 1'b0;
      sbr_port_req_o[i].a   = '0;
    end

    xbar_select_d = xbar_select_q;
    xbar_cnt_up = 1'b0;
    xbar_port_gnt = 1'b0;

    dma_select_d = dma_select_q;
    dma_cnt_up = 1'b0;
    dma_port_gnt = 1'b0;

    // DMA MAKES NEW REQUEST
    // Check for overflow (i.e. too many requests to same port)
    if (!dma_overflow && dma_active_i) begin
      // Cannot make a request to a port, to which the xbar currently has access to,
      // or is trying to gain access right now
      if ((dma_select_i != xbar_select_i) && (dma_select_i != xbar_select_q)) begin
        // Let the dma make a new request, if:
        //    1. The request is to the same port as a previous in-flight request
        //    2. There are no in-flight requests yet
        //    3. There are in-flight requests, but it is being granted right now
        if (dma_select_i == dma_select_q || dma_in_flight == '0 || (dma_in_flight == 1 && dma_cnt_down)) begin
          sbr_port_req_o[dma_select_i].req      = dma_port_req_i.req;
          sbr_port_req_o[dma_select_i].a        = dma_port_req_i.a;
          dma_port_gnt                          = sbr_port_rsp_i[dma_select_i].gnt;
        end
      end
    end

    // XBAR MAKES NEW REQUESTS
    // Check for oveflow (i.e. too many requests to same port)
    if (!xbar_overflow) begin
      // The select signal cannot be to a port, to which the dma currently has access.
      // However, if both the cpu and dma want to access a port at the same time, the cpu wins
      if (!dma_active_i || xbar_select_i != dma_select_q) begin
        // Let the xbar make a new request, if:
        //    1. The request is to the same port as a previous in-flight request
        //    2. There are no in-flight requests yet
        //    3. There are in-flight requests, but it is being granted right now
        if (xbar_select_i == xbar_select_q || xbar_in_flight == '0 || (xbar_in_flight == 1 && xbar_cnt_down)) begin
          sbr_port_req_o[xbar_select_i].req     = xbar_port_req_i.req;
          sbr_port_req_o[xbar_select_i].a       = xbar_port_req_i.a;
          xbar_port_gnt                         = sbr_port_rsp_i[xbar_select_i].gnt;
        end
      end
    end

    // GRANT XBAR REQUESTS
    // granting requests works by setting select_d/q, so that in the next clk cycle,
    // the correct response gets read back. Furthermore, we count up to track the amount of
    // pending requests.
    if (sbr_port_req_o[xbar_select_i].req && sbr_port_rsp_i[xbar_select_i].gnt) begin
      if (!dma_active_i || xbar_select_i != dma_select_q) begin
        xbar_select_d = xbar_select_i;
        xbar_cnt_up = 1'b1;
      end
    end

    // GRANT DMA REQUESTS
    if (sbr_port_req_o[dma_select_i].req && sbr_port_rsp_i[dma_select_i].gnt) begin
      if (dma_active_i && (xbar_select_i != dma_select_i) && (xbar_select_q != dma_select_i)) begin
        dma_select_d = dma_select_i;
        dma_cnt_up = 1'b1;
      end
    end
  end

  // ASSIGN XBAR RESPONSE
  assign xbar_port_rsp_o.gnt    = xbar_port_gnt;
  assign xbar_port_rsp_o.r      = sbr_port_rsp_i[xbar_select_q].r;
  assign xbar_port_rsp_o.rvalid = sbr_port_rsp_i[xbar_select_q].rvalid;

  if (ObiCfg.UseRReady) begin : gen_rready
    assign xbar_port_rready = xbar_port_req_i.rready;
    for (genvar i = 0; i < NumSbrPorts; i++) begin : gen_rready
      assign sbr_port_req_o[i].rready = xbar_port_req_i.rready;
    end
  end else begin : gen_no_rready
    assign xbar_port_rready = 1'b1;
  end

  assign xbar_cnt_down = sbr_port_rsp_i[xbar_select_q].rvalid && xbar_port_rready;

  // ASSIGN DMA RESPONSE
  assign dma_port_rsp_o.gnt    = dma_port_gnt;
  assign dma_port_rsp_o.r      = sbr_port_rsp_i[dma_select_q].r;
  assign dma_port_rsp_o.rvalid = sbr_port_rsp_i[dma_select_q].rvalid;
  assign dma_port_rready = 1'b1;

  assign dma_cnt_down = sbr_port_rsp_i[dma_select_q].rvalid && dma_port_rready;

  // Counters for both xbar and dma
  delta_counter #(
    .WIDTH           ( CounterWidth ),
    .STICKY_overflow ( 1'b0         )
  ) i_xbar_counter (
    .clk_i,
    .rst_ni,

    .clear_i   ( 1'b0                           ),
    .en_i      ( xbar_cnt_up ^ xbar_cnt_down              ),
    .load_i    ( 1'b0                           ),
    .down_i    ( xbar_cnt_down                       ),
    .delta_i   ( {{CounterWidth-1{1'b0}}, 1'b1} ),
    .d_i       ( '0                             ),
    .q_o       ( xbar_in_flight                      ),
    .xbar_overflow_o( xbar_overflow                       )
  );

  delta_counter #(
    .WIDTH           ( CounterWidth ),
    .STICKY_overflow ( 1'b0         )
  ) i_dma_counter (
    .clk_i,
    .rst_ni,

    .clear_i   ( 1'b0                           ),
    .en_i      ( dma_cnt_up ^ dma_cnt_down              ),
    .load_i    ( 1'b0                           ),
    .down_i    ( dma_cnt_down                       ),
    .delta_i   ( {{CounterWidth-1{1'b0}}, 1'b1} ),
    .d_i       ( '0                             ),
    .q_o       ( dma_in_flight                      ),
    .xbar_overflow_o( dma_overflow                       )
  );

  always_ff @(posedge clk_i or negedge rst_ni) begin : proc_select
    if(!rst_ni) begin
      xbar_select_q <= '0;
      dma_select_q <= '0;
    end else begin
      xbar_select_q <= xbar_select_d;
      dma_select_q <= dma_select_d;
    end
  end

endmodule