module foo (
    input logic clk_i, rst_ni, a_i,
    output logic o0_o, o1_o);

    logic q0, q1;

    assign o0_o = q0;
    assign o1_o = q1;

    dff d0(
      .clk_i (clk_i),
      .rst_ni (rst_ni),
      .d_i (a_i),
      .q_o (q0)
    );

    dff d1(
      .clk_i (clk_i),
      .rst_ni (rst_ni),
      .d_i (!q0),
      .q_o (q1)
    );
endmodule
