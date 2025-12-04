module hazard(
    input [4:0] srcA_no,
    input [31:0] srcA_data,
    input [4:0] srcB_no,
    input [31:0] srcB_data,
    input [4:0] ex_mem_in_rd,
    input [31:0] ex_mem_in_data,
    input [4:0] ram_in_rd,
    input [31:0] ram_in_data,
    input [4:0] mem_wb_in_rd,
    input [31:0] mem_wb_in_data,
    input [4:0] wb_in_rd,
    input [31:0] wb_in_data,
    output reg [31:0] hazard_out_A,
    output reg [31:0] hazard_out_B
);
    // ========================================================
    // 通道 A 的独立前递逻辑
    // ========================================================
    always @(*) begin
        // 默认值：无冲突，使用寄存器堆读出的原值
        hazard_out_A = srcA_data;

        // 优先级 1: EX_MEM (最新鲜的数据，距离 EX 最近)
        if (srcA_no == ex_mem_in_rd && ex_mem_in_rd != 5'b0) begin
            hazard_out_A = ex_mem_in_data;
        end
        // 优先级 2: RAM (次新)
        else if (srcA_no == ram_in_rd && ram_in_rd != 5'b0) begin
            hazard_out_A = ram_in_data;
        end
        // 优先级 3: MEM_WB
        else if (srcA_no == mem_wb_in_rd && mem_wb_in_rd != 5'b0) begin
            hazard_out_A = mem_wb_in_data;
        end
        // 优先级 4: WB (最旧，但比 ID 阶段新)
        else if (srcA_no == wb_in_rd && wb_in_rd != 5'b0) begin
            hazard_out_A = wb_in_data;
        end
    end

    // ========================================================
    // 通道 B 的独立前递逻辑 (完全复制一份，针对 srcB)
    // ========================================================
    always @(*) begin
        // 默认值
        hazard_out_B = srcB_data;

        // 优先级 1: EX_MEM
        if (srcB_no == ex_mem_in_rd && ex_mem_in_rd != 5'b0) begin
            hazard_out_B = ex_mem_in_data;
        end
        // 优先级 2: RAM
        else if (srcB_no == ram_in_rd && ram_in_rd != 5'b0) begin
            hazard_out_B = ram_in_data;
        end
        // 优先级 3: MEM_WB
        else if (srcB_no == mem_wb_in_rd && mem_wb_in_rd != 5'b0) begin
            hazard_out_B = mem_wb_in_data;
        end
        // 优先级 4: WB
        else if (srcB_no == wb_in_rd && wb_in_rd != 5'b0) begin
            hazard_out_B = wb_in_data;
        end
    end
endmodule
