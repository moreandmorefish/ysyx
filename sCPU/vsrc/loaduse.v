module loaduse (
    input [4:0] id_rs1,    // 当前 ID 阶段源寄存器1
    input [4:0] id_rs2,    // 当前 ID 阶段源寄存器2
    
    // 来自 EX 阶段 (上一条指令)
    input [4:0] id_ex_rd_no,     
    input       id_ex_read_en, // id_ex_MemRd (是否是Load)
    
    // 来自 MEM 阶段 (上上一条指令) -- 针对同步RAM必须加这一级检测
    input [4:0] ex_mem_rd,
    input       ex_mem_read_en, // ex_mem_MemRd (是否是Load)

    output reg  stall
);
    always @(*) begin
        stall = 1'b0;

        // 情况1：上一条指令是 Load，且目标是我的源 (Stall T3)
        if (id_ex_read_en && (id_ex_rd_no != 0) && (id_ex_rd_no == id_rs1 || id_ex_rd_no == id_rs2)) begin
            stall = 1'b1;
        end
        // 情况2：上上一条指令是 Load，且目标是我的源 (Stall T4)
        // 注意：因为是同步RAM，MEM阶段数据还没出来，也得等
        else if (ex_mem_read_en && (ex_mem_rd != 0) && (ex_mem_rd == id_rs1 || ex_mem_rd == id_rs2)) begin
            stall = 1'b1;
        end
    end
endmodule