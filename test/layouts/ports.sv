module port_types(
    input logic clk,
    input logic r,
    input logic [31:0] e,
    output logic o,
    output logic [31:0] t,
    output logic [64:0] wide
);
    // Keep real clocked storage even when SystemC outputs can be written
    // directly by older Verilator optimizers.
    logic state_bit /*verilator public_flat*/;
    logic [31:0] state_word /*verilator public_flat*/;
    logic [64:0] state_wide /*verilator public_flat*/;

    assign o = state_bit;
    assign t = state_word;
    assign wide = state_wide;

    always_ff @(posedge clk) begin
        state_bit <= r;
        state_word <= e;
        state_wide <= {r, e, e};
    end
endmodule
