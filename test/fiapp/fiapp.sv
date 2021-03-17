module fiapp
	(
		input logic clk, reset,
		input logic a, enable,
		output logic o1, o2, o3
	);
	logic q1, q2, q3;
	logic[64:0] qext;
	logic[2:0] qmultidim[2];
	logic[2:0] qKLMmultidim[2][2];

	assign o1 = q1;
	assign o2 = q2;
	assign o3 = q3 & qext[64];
	
	always_ff @(posedge clk, posedge reset)
		if(reset) begin
			q1 <= 1'b0;
			q2 <= 1'b0;
			q3 <= 1'b0;
			qext <= 65'b0;
			qmultidim[0] <= 3'b0;
			qmultidim[1] <= 3'b0;
			qKLMmultidim/*[0]*/[0][0] <= 3'b0;
			qKLMmultidim/*[0]*/[0][1] <= 3'b0;
			qKLMmultidim/*[0]*/[1][0] <= 3'b0;
			qKLMmultidim/*[0]*/[1][1] <= 3'b0;
			//qKLMmultidim[1][0][0] <= 3'b0;
			//qKLMmultidim[1][0][1] <= 3'b0;
			//qKLMmultidim[1][1][0] <= 3'b0;
			//qKLMmultidim[1][1][1] <= 3'b0;
		end
		else begin
			if (enable) begin
				q1 <= a;
			end
			q2 <= q1;
			q3 <= !q1;
			qext <= qext + 1;
			qmultidim[1] <= qmultidim[0];
			qmultidim[0] <= qext[2:0];
			qKLMmultidim/*[0]*/[1][1] <= qmultidim[0];
    end
endmodule
