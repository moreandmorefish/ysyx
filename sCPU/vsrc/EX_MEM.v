module EX_MEM (
    input clk,
    input reset,

    // ---- 来自 EX 阶段的输入 ----
    input [31:0] alu_out_in,        // ALU 计算结果
    input [31:0] rs2_val_in,        // store 写入数据
    input [2:0]  mem_op_in,         // 内存操作类型
    input        mem_wr_in,         // 内存写使能
    input        mem_rd_in,         // 内存读使能
    input        reg_wr_in,         // 寄存器写使能
    input        memtoreg_in,       // 写回选择
    input [4:0]  rd_in,             // 目标寄存器号
    input [2:0]  branch_in,         // 分支控制信号（用于 JAL/JALR 在 WB 写回 PC+4）
    input [31:0] pc_in,             // 该指令的 PC

    // ---- 输出到 MEM 阶段 ----
    output reg [31:0] alu_out,
    output reg [31:0] rs2_val,
    output reg [2:0]  mem_op,
    output reg        mem_wr,
    output reg        mem_rd,
    output reg        reg_wr,
    output reg        memtoreg,
    output reg [4:0]  rd,
    output reg [2:0]  branch,
    output reg [31:0] pc_out,
    input [31:0] a0,
    input em_illegal_instr_in,
    input instr_valid_in,
    output reg instr_valid
);
    import "DPI-C" function void cpu_halt(input code);
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            alu_out   <= 32'b0;
            rs2_val   <= 32'b0;
            mem_op    <= 3'b0;
            mem_wr    <= 1'b0;
            mem_rd    <= 1'b0;
            reg_wr    <= 1'b0;
            memtoreg  <= 1'b0;
            rd        <= 5'b0;
            branch    <= 3'b0;
            pc_out    <= 32'b0;
            instr_valid <= 1'b0;
        end
        else if(em_illegal_instr_in) begin
            alu_out   <= 32'b0;
            rs2_val   <= 32'b0;
            mem_op    <= 3'b0;
            mem_wr    <= 1'b0;
            mem_rd    <= 1'b0;
            reg_wr    <= 1'b0;
            memtoreg  <= 1'b0;
            rd        <= 5'b0;
            branch    <= 3'b0;
            pc_out    <= 32'b0;
            instr_valid <= 1'b0;
            $display("CPU Halted due to Illegal Instruction at EX Stage. a0=%h  pc=%h", a0, pc_in);
            if(alu_out_in != 32'b0) cpu_halt(1);
            else cpu_halt(0);
        end else begin
            if (branch_in == 3'b010 || branch_in == 3'b001) begin
                alu_out <= pc_in + 32'd4; 
            end else begin
                alu_out <= alu_out_in;
            end
            rs2_val   <= rs2_val_in;
            mem_op    <= mem_op_in;
            mem_wr    <= mem_wr_in;
            mem_rd    <= mem_rd_in;
            reg_wr    <= reg_wr_in;
            memtoreg  <= memtoreg_in;
            rd        <= rd_in;
            branch    <= branch_in;
            pc_out    <= pc_in;
            instr_valid <= instr_valid_in;
        end
    end

endmodule


