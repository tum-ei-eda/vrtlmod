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

module dff
  (
    input logic clk, reset,
    input logic d,
    output logic q
  );
  //signal declarations
  logic r_q;

  // continious body
  assign q = r_q;

  //sequential body
  always_ff @(posedge clk, posedge reset)
    if(reset)
      r_q <= 1'b0;
    else
      r_q <= d;
endmodule


module fiapp
  (
    input logic clk, reset,
    input logic a, enable,
    output logic o1, o2, o3,
    output logic[64:0] o4,
    output logic[63:0] state_signature
  );
  logic q1, q2, q3;
  logic[64:0] reg_3_1_VlWide_q; // 1 overhang bits in 3 word-wide 32-bit VlWide basetype
  logic[95:0] reg_3_0_VlWide_q; // 0 overhang bits in 3 word-wide 32-bit VlWide basetype
  logic[2:0] reg_VlUnpacked_CData_q[2];
  logic[2:0] reg_VlUnpacked_VlUnpacked_CData_q[2][2];
  logic[64:0] reg_VlUnpacked_VlWide_q[2];
  logic[64:0] reg_VlUnpacked_VlUnpacked_VlWide_q[2][2];
  logic[7:0] reg_8_q;
  logic[8:0] reg_9_q;
  logic[15:0] reg_16_q;
  logic[16:0] reg_17_q;
  logic[31:0] reg_32_q;
  logic[32:0] reg_33_q;
  logic[63:0] reg_64_q;
  logic[2:0] reg_asym_CData_q[2][3];
  logic[64:0] reg_asym_VlWide_q[2][3];
  wire[5:0] asym_sink;
  for (genvar row = 0; row < 2; row++) begin
    for (genvar column = 0; column < 3; column++) begin
      assign asym_sink[row * 3 + column] = reg_asym_CData_q[row][column][2]
                                           ^ reg_asym_VlWide_q[row][column][64];
    end
  end

  dff d0(
    .clk (clk),
    .reset (reset),
    .d (q1),
    .q (q2)
  );

  dff d1(
    .clk (clk),
    .reset (reset),
    .d (!q1),
    .q (q3)
  );

  assign o1 = q1;
  assign o2 = q2;
  assign o3 = q3 & reg_3_1_VlWide_q[64] & reg_3_0_VlWide_q[95] & reg_VlUnpacked_CData_q[1][2] & reg_VlUnpacked_VlWide_q[1][64] & reg_VlUnpacked_VlUnpacked_CData_q[1][1][2] & reg_VlUnpacked_VlUnpacked_VlWide_q[1][1][64]
              & reg_8_q[7] & reg_9_q[8] & reg_16_q[15] & reg_17_q[16]
              & reg_32_q[31] & reg_33_q[32] & reg_64_q[63] & (&asym_sink);
  assign o4 = reg_3_1_VlWide_q;

  // Observe every state shape in the plain/re-written comparison. The original
  // o3 AND chain alone can hide changes whenever any of its inputs stays zero.
  function automatic logic[63:0] mix_state(input logic[63:0] prior, input logic[95:0] value);
    return {prior[56:0], prior[63:57]} ^ value[63:0] ^ {value[95:64], 32'b0};
  endfunction
  always_comb begin
    state_signature = mix_state(64'h6a09e667f3bcc909, {q1, q2, q3});
    state_signature = mix_state(state_signature, reg_3_1_VlWide_q);
    state_signature = mix_state(state_signature, reg_3_0_VlWide_q);
    state_signature = mix_state(state_signature, reg_8_q);
    state_signature = mix_state(state_signature, reg_9_q);
    state_signature = mix_state(state_signature, reg_16_q);
    state_signature = mix_state(state_signature, reg_17_q);
    state_signature = mix_state(state_signature, reg_32_q);
    state_signature = mix_state(state_signature, reg_33_q);
    state_signature = mix_state(state_signature, reg_64_q);
    for (int row = 0; row < 2; row++) begin
      state_signature = mix_state(state_signature, reg_VlUnpacked_CData_q[row]);
      state_signature = mix_state(state_signature, reg_VlUnpacked_VlWide_q[row]);
      for (int column = 0; column < 2; column++) begin
        state_signature = mix_state(state_signature, reg_VlUnpacked_VlUnpacked_CData_q[row][column]);
        state_signature = mix_state(state_signature, reg_VlUnpacked_VlUnpacked_VlWide_q[row][column]);
      end
      for (int column = 0; column < 3; column++) begin
        state_signature = mix_state(state_signature, reg_asym_CData_q[row][column]);
        state_signature = mix_state(state_signature, reg_asym_VlWide_q[row][column]);
      end
    end
  end

  always_ff @(posedge clk, posedge reset)
    if(reset) begin
      q1 <= 1'b0;
      reg_3_1_VlWide_q <= 65'b0;
      reg_3_0_VlWide_q <= 95'b0;
      reg_VlUnpacked_CData_q[0] <= 3'b0;
      reg_VlUnpacked_CData_q[1] <= 3'b0;
      reg_VlUnpacked_VlWide_q/*[64:0]*/[0] <= 65'b0;
      reg_VlUnpacked_VlWide_q/*[64:0]*/[1] <= 65'b0;
      reg_VlUnpacked_VlUnpacked_CData_q/*[0]*/[0][0] <= 3'b0;
      reg_VlUnpacked_VlUnpacked_CData_q/*[0]*/[0][1] <= 3'b0;
      reg_VlUnpacked_VlUnpacked_CData_q/*[0]*/[1][0] <= 3'b0;
      reg_VlUnpacked_VlUnpacked_CData_q/*[0]*/[1][1] <= 3'b0;
      reg_VlUnpacked_VlUnpacked_VlWide_q/*[64:0]*/[0][0] <= 65'b0;
      reg_VlUnpacked_VlUnpacked_VlWide_q/*[64:0]*/[0][1] <= 65'b0;
      reg_VlUnpacked_VlUnpacked_VlWide_q/*[64:0]*/[1][0] <= 65'b0;
      reg_VlUnpacked_VlUnpacked_VlWide_q/*[64:0]*/[1][1] <= 65'b0;
      reg_8_q <= '0;
      reg_9_q <= '0;
      reg_16_q <= '0;
      reg_17_q <= '0;
      reg_32_q <= '0;
      reg_33_q <= '0;
      reg_64_q <= '0;
      for (int row = 0; row < 2; row++) begin
        for (int column = 0; column < 3; column++) begin
          reg_asym_CData_q[row][column] <= '0;
          reg_asym_VlWide_q[row][column] <= '0;
        end
      end
    end
    else begin
      if (enable) begin
        q1 <= a;
      end

      // nonsense linear shift to test dominant injection points for multidimensional packed
      reg_3_1_VlWide_q <= {reg_3_1_VlWide_q[63:0], a};

      // nonsense linear shift to test dominant injection points for multidimensional packed
      reg_3_0_VlWide_q <= {reg_3_0_VlWide_q[94:0], a};

      // nonsense linear shift to test dominant injection points for multidimensional unpacked
      reg_VlUnpacked_CData_q[1] <= {reg_VlUnpacked_CData_q[1][1:0], reg_VlUnpacked_CData_q[0][2]};
      reg_VlUnpacked_CData_q[0] <= {reg_VlUnpacked_CData_q[0][1:0], q1};

      // nonsense linear shift to test dominant injection points for multidimensional unpacked
      reg_VlUnpacked_VlWide_q/*[64:0]*/[1] <= {reg_VlUnpacked_VlWide_q[1][63:0], reg_VlUnpacked_VlWide_q[0][64]};
      reg_VlUnpacked_VlWide_q/*[64:0]*/[0] <= {reg_VlUnpacked_VlWide_q[0][63:0], q1};

      // nonsense linear shift to test dominant injection points for multidimensional unpacked
      reg_VlUnpacked_VlUnpacked_CData_q/*[0]*/[1][1] <= {reg_VlUnpacked_VlUnpacked_CData_q[1][1][1:0], reg_VlUnpacked_VlUnpacked_CData_q[1][0][2]};
      reg_VlUnpacked_VlUnpacked_CData_q/*[0]*/[1][0] <= {reg_VlUnpacked_VlUnpacked_CData_q[1][0][1:0], reg_VlUnpacked_VlUnpacked_CData_q[0][1][2]};
      reg_VlUnpacked_VlUnpacked_CData_q/*[0]*/[0][1] <= {reg_VlUnpacked_VlUnpacked_CData_q[0][1][1:0], reg_VlUnpacked_VlUnpacked_CData_q[0][0][2]};
      reg_VlUnpacked_VlUnpacked_CData_q/*[0]*/[0][0] <= {reg_VlUnpacked_VlUnpacked_CData_q[0][0][1:0], q1};

      // nonsense linear shift to test dominant injection points for multidimensional unpacked
      reg_VlUnpacked_VlUnpacked_VlWide_q/*[64:0]*/[1][1] <= {reg_VlUnpacked_VlUnpacked_VlWide_q[1][1][63:0], reg_VlUnpacked_VlUnpacked_VlWide_q[1][0][64]};
      reg_VlUnpacked_VlUnpacked_VlWide_q/*[64:0]*/[1][0] <= {reg_VlUnpacked_VlUnpacked_VlWide_q[1][0][63:0], reg_VlUnpacked_VlUnpacked_VlWide_q[0][1][64]};
      reg_VlUnpacked_VlUnpacked_VlWide_q/*[64:0]*/[0][1] <= {reg_VlUnpacked_VlUnpacked_VlWide_q[0][1][63:0], reg_VlUnpacked_VlUnpacked_VlWide_q[0][0][64]};
      reg_VlUnpacked_VlUnpacked_VlWide_q/*[64:0]*/[0][0] <= {reg_VlUnpacked_VlUnpacked_VlWide_q[0][0][63:0], q1};

      // nonsense to test array subscripts with non-constant (literals) subscript operations
      reg_VlUnpacked_VlUnpacked_VlWide_q/*[64:0]*/[reg_VlUnpacked_CData_q[1][2]][reg_VlUnpacked_CData_q[0][2]] <= q1;

      reg_8_q <= {reg_8_q[6:0], a};
      reg_9_q <= {reg_9_q[7:0], a};
      reg_16_q <= {reg_16_q[14:0], a};
      reg_17_q <= {reg_17_q[15:0], a};
      reg_32_q <= {reg_32_q[30:0], a};
      reg_33_q <= {reg_33_q[31:0], a};
      reg_64_q <= {reg_64_q[62:0], a};
      for (int row = 0; row < 2; row++) begin
        for (int column = 0; column < 3; column++) begin
          reg_asym_CData_q[row][column] <= {reg_asym_CData_q[row][column][1:0], q1 ^ (row == 1) ^ (column == 1)};
          reg_asym_VlWide_q[row][column] <= {reg_asym_VlWide_q[row][column][63:0], q1 ^ (row == 1) ^ (column == 1)};
        end
      end
    end
endmodule
