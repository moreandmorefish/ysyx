module seg(
  input  [31:0] out_data,  // 输入数据
  output [7:0] o_seg0,
  output [7:0] o_seg1,
  output [7:0] o_seg2,
  output [7:0] o_seg3,
  output [7:0] o_seg4,
  output [7:0] o_seg5,
  output [7:0] o_seg6,
  output [7:0] o_seg7
);

// 拆分 out_data
    reg [3:0] digit [7:0];
    integer i;
    reg [31:0] temp;
    reg [31:0] mod_res;

    always @(*) begin
        temp = out_data;
        for (i = 0; i < 8; i = i + 1) begin
            mod_res   = temp % 10;         // 计算余数
            digit[i]  = mod_res[3:0];      // ✅ 只取低 4 位
            temp      = temp / 10;
        end
    end

// 数码管段码函数（共阳极，低电平点亮）
function [7:0] seg_pattern;
  input [3:0] val;
  case (val)
    4'd0: seg_pattern = 8'b11000000;
    4'd1: seg_pattern = 8'b11111001;
    4'd2: seg_pattern = 8'b10100100;
    4'd3: seg_pattern = 8'b10110000;
    4'd4: seg_pattern = 8'b10011001;
    4'd5: seg_pattern = 8'b10010010;
    4'd6: seg_pattern = 8'b10000010;
    4'd7: seg_pattern = 8'b11111000;
    4'd8: seg_pattern = 8'b10000000;
    4'd9: seg_pattern = 8'b10010000;
    default: seg_pattern = 8'b11111111; // 全灭
  endcase
endfunction

// 输出到各数码管
assign o_seg0 = seg_pattern(digit[0]);
assign o_seg1 = seg_pattern(digit[1]);
assign o_seg2 = seg_pattern(digit[2]);
assign o_seg3 = seg_pattern(digit[3]);
assign o_seg4 = seg_pattern(digit[4]);
assign o_seg5 = seg_pattern(digit[5]);
assign o_seg6 = seg_pattern(digit[6]);
assign o_seg7 = seg_pattern(digit[7]);

endmodule
