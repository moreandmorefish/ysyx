module seg(
  input [31:0] out_data,  // 输入的32位计算结果（用于确定数值，实际亮灯仅与位数相关）
  output [7:0] o_seg0,    // 个位：最低1位亮起（bit0亮）
  output [7:0] o_seg1,    // 十位：最低2位亮起（bit0~1亮）
  output [7:0] o_seg2,    // 百位：最低3位亮起（bit0~2亮）
  output [7:0] o_seg3,    // 千位：最低4位亮起（bit0~3亮）
  output [7:0] o_seg4,    // 万位：最低5位亮起（bit0~4亮）
  output [7:0] o_seg5,    // 十万位：最低6位亮起（bit0~5亮）
  output [7:0] o_seg6,    // 百万位：最低7位亮起（bit0~6亮）
  output [7:0] o_seg7     // 千万位：最低8位亮起（bit0~7亮）
);

// --------------------------
// 1. 定义各数位的亮灯模式（共阴极：1=亮，0=灭）
// bit0为最低位，bit7为最高位
// --------------------------
localparam [7:0] PATTERN [7:0] = {
  8'b00000001,  // 个位（bit0亮）→ 1位
  8'b00000011,  // 十位（bit0~1亮）→ 2位
  8'b00000111,  // 百位（bit0~2亮）→ 3位
  8'b00001111,  // 千位（bit0~3亮）→ 4位
  8'b00011111,  // 万位（bit0~4亮）→ 5位
  8'b00111111,  // 十万位（bit0~5亮）→ 6位
  8'b01111111,  // 百万位（bit0~6亮）→ 7位
  8'b11111111   // 千万位（bit0~7亮）→ 8位
};

// --------------------------
// 2. 拆分out_data获取各数位数值（仅用于判断该位是否需要显示）
//    若该位数值为0且是高位，可选择不亮（此处默认全亮，按位数亮灯）
// --------------------------
reg [3:0] digit [7:0];  // 存储各数位数值（0~9）
reg [31:0] temp;
reg [31:0] mod_res;

always @(*) begin
  temp = out_data;
  
  // 拆分各数位数值（用于后续可选的"零消隐"，此处仅保留逻辑）
  mod_res = temp % 10;
  digit[0] = mod_res[3:0];  // 个位数值
  temp = temp / 10;
  
  mod_res = temp % 10;
  digit[1] = mod_res[3:0];  // 十位数值
  temp = temp / 10;
  
  mod_res = temp % 10;
  digit[2] = mod_res[3:0];  // 百位数值
  temp = temp / 10;
  
  mod_res = temp % 10;
  digit[3] = mod_res[3:0];  // 千位数值
  temp = temp / 10;
  
  mod_res = temp % 10;
  digit[4] = mod_res[3:0];  // 万位数值
  temp = temp / 10;
  
  mod_res = temp % 10;
  digit[5] = mod_res[3:0];  // 十万位数值
  temp = temp / 10;
  
  mod_res = temp % 10;
  digit[6] = mod_res[3:0];  // 百万位数值
  temp = temp / 10;
  
  mod_res = temp % 10;
  digit[7] = mod_res[3:0];  // 千万位数值
end

// --------------------------
// 3. 输出亮灯模式（按位数亮灯，与具体数值无关）
//    若需"零消隐"（高位为0时不亮），可在此处添加条件判断
// --------------------------
assign o_seg0 = PATTERN[0];  // 个位：1位亮
assign o_seg1 = PATTERN[1];  // 十位：2位亮
assign o_seg2 = PATTERN[2];  // 百位：3位亮
assign o_seg3 = PATTERN[3];  // 千位：4位亮
assign o_seg4 = PATTERN[4];  // 万位：5位亮
assign o_seg5 = PATTERN[5];  // 十万位：6位亮
assign o_seg6 = PATTERN[6];  // 百万位：7位亮
assign o_seg7 = PATTERN[7];  // 千万位：8位亮

endmodule