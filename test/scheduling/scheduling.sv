module scheduling (
    input logic clk, clk_b, reset, data, data_b, enable,
    output logic blocking_state, clocked_state, independent_state, combinational
);
    // A blocking assignment that generates another clock forces active-region
    // scheduling on Verilator 5; the dependent register uses the NBA region.
    always_ff @(posedge clk or posedge reset)
        if (reset) blocking_state = 1'b0;
        else if (enable) blocking_state = data;

    always_ff @(posedge blocking_state or posedge reset)
        if (reset) clocked_state <= 1'b0;
        else clocked_state <= data;

    always_ff @(posedge clk_b or posedge reset)
        if (reset) independent_state <= 1'b0;
        else independent_state <= data_b;

    // Keep this independent of state to exercise input-combinational scheduling.
    // Generated code can merge combinational logic downstream of state into clocked
    // regions; vRTLmod historically treats those merged assignments as targets.
    always_comb combinational = !data;
endmodule
