module MEM_WB (
    input clk,
    input reset,

    // ---- 来自 MEM 阶段的输入 ----
    input [31:0] mw_mem_data_in,       // 从内存读取的数据
    input [31:0] mw_alu_out_in,     // ALU结果
    input [4:0]  mw_rd_in,          // 写回寄存器号
    input        mw_reg_wr_in,      // 是否写回寄存器
    input        mw_memtoreg_in,    // 写回数据选择信号
    input [31:0] mw_pc_in,          // ✅ 新增：对应指令的 PC 值

    // ---- 输出到 WB 阶段 ----
    output reg [31:0] mw_mem_data_out,
    output reg [31:0] mw_alu_out_out,
    output reg [4:0]  mw_rd_out,
    output reg        mw_reg_wr_out,
    output reg        mw_memtoreg_out,
    output reg [31:0] mw_pc_out,      // ✅ 新增：写回阶段的 PC 值
    input [2:0] mw_branch,
    output reg [2:0] mw_branch_out
);

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            mw_mem_data_out  <= 32'b0;
            mw_alu_out_out   <= 32'b0;
            mw_rd_out        <= 5'b0;
            mw_reg_wr_out    <= 1'b0;
            mw_memtoreg_out  <= 1'b0;
            mw_pc_out        <= 32'b0;  // ✅ 初始化 PC
            mw_branch_out    <= 3'b0;
        end else begin
            mw_mem_data_out  <= mw_mem_data_in;
            mw_alu_out_out   <= mw_alu_out_in;
            mw_rd_out        <= mw_rd_in;
            mw_reg_wr_out    <= mw_reg_wr_in;
            mw_memtoreg_out  <= mw_memtoreg_in;
            mw_pc_out        <= mw_pc_in;  // ✅ 正常传递 PC
            mw_branch_out    <= mw_branch;
        end
    end
    //always @(posedge clk) begin
    //    $display("MEM_WB Output:");
    //    $display("PC: %h, Reg Write: %b, MemToReg: %b", mw_pc_out, mw_reg_wr_out, mw_memtoreg_out);
    //    if (mw_memtoreg_out) 
    //        $display("Data from Memory: %h, Write to Register: %d", mw_mem_data_out, mw_rd_out);
    //    else
    //        $display("ALU Result: %h, Write to Register: %d", mw_alu_out_out, mw_rd_out);
    //end

endmodule
