module ID_EX(
    input clk, reset, flush,
    input [31:0] imm_in,
    input [31:0] busA_in,  // rs1 数据（来自RegisterFile.busA）
    input [31:0] busB_in,  // rs2 数据（来自RegisterFile.busB）
    input [3:0]  ALUctr_in,
    input        ALUBsrc_in,
    input        RegWr_in,
    input [2:0]  branch_in,
    input [2:0]  MemOp_in,
    input        MemWr_in,
    input        MemRd_in,
    input        MemtoReg_in,
    input [4:0]  rs1_in, rs2_in, rd_in,
    input [31:0] pc_in,  // 输入：IF_ID的PC
    output reg [31:0] pc_to_ex,  // 输出：EX阶段的PC
    output reg [31:0] imm,
    output reg [31:0] busA_out,  // 输出：rs1数据到EX阶段
    output reg [31:0] busB_out,  // 输出：rs2数据到EX阶段
    output reg [3:0]  ALUctr,
    output reg        ALUBsrc,
    output reg        RegWr,
    output reg [2:0]  branch,
    output reg [2:0]  MemOp,
    output reg        MemWr,
    output reg        MemRd,
    output reg        MemtoReg,
    output reg [4:0]  rs1, rs2, rd,
    input ie_legal_instr_in,
    output reg ie_illegal_instr,  // 输出：非法指令标志
    input [31:0] ie_a0_in,
    output reg [31:0] ie_a0
);

always @(posedge clk or posedge reset) begin
    if (reset) begin
        // 复位时清零所有信号
        imm <= 32'b0;
        busA_out <= 32'b0;
        busB_out <= 32'b0;
        ALUctr <= 4'b0;
        ALUBsrc <= 1'b0;
        RegWr <= 1'b0;
        branch <= 3'b0;
        MemOp <= 3'b0;
        MemWr <= 1'b0;
        MemRd <= 1'b0;
        MemtoReg <= 1'b0;
        rs1 <= 5'b0;
        rs2 <= 5'b0;
        rd <= 5'b0;
        pc_to_ex <= 32'b0;
        ie_illegal_instr <= 1'b0;
        ie_a0 <= 32'b0;
    end else if (flush) begin
        // 冲刷流水线：清零所有控制信号和数据信号（避免气泡）
        imm <= 32'b0;
        busA_out <= 32'b0;
        busB_out <= 32'b0;
        ALUctr <= 4'b0;
        ALUBsrc <= 1'b0;
        RegWr <= 1'b0;
        branch <= 3'b0;
        MemOp <= 3'b0;
        MemWr <= 1'b0;
        MemRd <= 1'b0;
        MemtoReg <= 1'b0;
        rs1 <= 5'b0;
        rs2 <= 5'b0;
        rd <= 5'b0;
        pc_to_ex <= 32'b0;
        ie_illegal_instr <= 1'b0;
        ie_a0 <= 32'b0;
    end else begin
        // 正常传递：输入→输出
        imm <= imm_in;
        busA_out <= busA_in;  // rs1数据传递
        busB_out <= busB_in;  // rs2数据传递
        ALUctr <= ALUctr_in;
        ALUBsrc <= ALUBsrc_in;
        RegWr <= RegWr_in;
        branch <= branch_in;
        MemOp <= MemOp_in;
        MemWr <= MemWr_in;
        MemRd <= MemRd_in;
        MemtoReg <= MemtoReg_in;
        rs1 <= rs1_in;
        rs2 <= rs2_in;
        rd <= rd_in;
        pc_to_ex <= pc_in;
        ie_illegal_instr <= ie_legal_instr_in;
        ie_a0 <= ie_a0_in;
    end
end

endmodule