module discovery (
    input logic clk, reset,
    input logic [7:0] data,
    output logic [7:0] q
);
    always_ff @(posedge clk)
        if (reset) q <= 8'h00;
        else q <= data ^ 8'ha5;
endmodule
