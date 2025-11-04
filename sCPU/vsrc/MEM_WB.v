module MEM_WB (
    input clk,
    input reset,

    // ---- 来自 MEM 阶段的输入 ----
    input [31:0] mem_data_in,    // 从内存读取的数据
    input [31:0] alu_out_in,     // ALU结果
    input [4:0]  rd_in,          // 写回寄存器号
    input        reg_wr_in,      // 是否写回寄存器
    input        memtoreg_in,    // 写回数据选择信号
    input [31:0] pc_in,          // ✅ 新增：对应指令的 PC 值

    // ---- 输出到 WB 阶段 ----
    output reg [31:0] mem_data_out,
    output reg [31:0] alu_out_out,
    output reg [4:0]  rd_out,
    output reg        reg_wr_out,
    output reg        memtoreg_out,
    output reg [31:0] pc_out      // ✅ 新增：写回阶段的 PC 值
);

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            mem_data_out  <= 32'b0;
            alu_out_out   <= 32'b0;
            rd_out        <= 5'b0;
            reg_wr_out    <= 1'b0;
            memtoreg_out  <= 1'b0;
            pc_out        <= 32'b0;  // ✅ 初始化 PC
        end else begin
            mem_data_out  <= mem_data_in;
            alu_out_out   <= alu_out_in;
            rd_out        <= rd_in;
            reg_wr_out    <= reg_wr_in;
            memtoreg_out  <= memtoreg_in;
            pc_out        <= pc_in;  // ✅ 正常传递 PC
        end
    end

endmodule
