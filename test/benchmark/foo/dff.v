module dff
  (
    input logic clk_i, rst_ni,
    input logic d_i,
    output logic q_o
  );
  //signal declarations
  logic r_q;

  // continious body
  assign q_o = r_q;

  //sequential body
  always_ff @(posedge clk_i, negedge rst_ni)
    if(!rst_ni)
      r_q <= 1'b0;
    else
      r_q <= d_i;
endmodule
