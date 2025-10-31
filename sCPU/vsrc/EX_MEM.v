module EX_MEM (
    input clk, reset,

    // ---- 来自 EX 阶段的输入 ----
    input [31:0] alu_out_in,        // ALU计算结果
    input [31:0] branch_target_in,  // 跳转目标地址
    input [31:0] rs2_val_in,        // store写入数据
    input [2:0]  mem_op_in,         // 内存操作类型
    input        mem_wr_in,         // 内存写使能
    input        reg_wr_in,         // 寄存器写使能
    input        memtoreg_in,       // 写回选择
    input [4:0]  rd_in,             // 目标寄存器号
    input [2:0]  branch_in,         // 分支控制信号

    // ---- 输出到 MEM 阶段 ----
    output reg [31:0] alu_out,      
    output reg [31:0] branch_target,
    output reg [31:0] rs2_val,
    output reg [2:0]  mem_op,
    output reg        mem_wr,
    output reg        reg_wr,
    output reg        memtoreg,
    output reg [4:0]  rd,
    output reg [2:0]  branch
);

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            alu_out        <= 32'b0;
            branch_target  <= 32'b0;
            rs2_val        <= 32'b0;
            mem_op         <= 3'b0;
            mem_wr         <= 1'b0;
            reg_wr         <= 1'b0;
            memtoreg       <= 1'b0;
            rd             <= 5'b0;
            branch         <= 3'b0;
        end else begin
            alu_out        <= alu_out_in;
            branch_target  <= branch_target_in;
            rs2_val        <= rs2_val_in;
            mem_op         <= mem_op_in;
            mem_wr         <= mem_wr_in;
            reg_wr         <= reg_wr_in;
            memtoreg       <= memtoreg_in;
            rd             <= rd_in;
            branch         <= branch_in;
        end
    end
endmodule
