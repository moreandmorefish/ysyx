module ID_EX(
    input clk, reset,
    input [31:0] imm_in,
    input [3:0]  ALUctr_in,
    input        ALUBsrc_in,
    input        RegWr_in,
    input [2:0]  branch_in,
    input [2:0]  MemOp_in,
    input        MemWr_in,
    input        MemtoReg_in,
    input [4:0]  rs1_in, rs2_in, rd_in,

    input [31:0] pc_out,
    output reg [31:0] pc_to_ex,

    output reg [31:0] imm,
    output reg [3:0]  ALUctr,
    output reg        ALUBsrc,
    output reg        RegWr,
    output reg [2:0]  branch,
    output reg [2:0]  MemOp,
    output reg        MemWr,
    output reg        MemtoReg,
    output reg [4:0]  rs1, rs2, rd
);
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            imm <= 0; ALUctr <= 0; ALUBsrc <= 0; RegWr <= 0;
            branch <= 0; MemOp <= 0; MemWr <= 0; MemtoReg <= 0;
            rs1 <= 0; rs2 <= 0; rd <= 0;pc_to_ex <= 0;
        end else begin
            imm <= imm_in; ALUctr <= ALUctr_in; ALUBsrc <= ALUBsrc_in;
            RegWr <= RegWr_in; branch <= branch_in;
            MemOp <= MemOp_in; MemWr <= MemWr_in; MemtoReg <= MemtoReg_in;
            rs1 <= rs1_in; rs2 <= rs2_in; rd <= rd_in;pc_to_ex <= pc_out;
        end
    end
endmodule
