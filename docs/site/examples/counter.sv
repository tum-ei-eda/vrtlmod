// SPDX-License-Identifier: Apache-2.0
module counter (
    input  logic       clk,
    input  logic       reset,
    input  logic       enable,
    output logic [7:0] count
);
    always_ff @(posedge clk) begin
        if (reset)
            count <= 8'd0;
        else if (enable)
            count <= count + 8'd1;
    end
endmodule
