/*
 * Copyright 2021 Chair of EDA, Technical University of Munich
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
