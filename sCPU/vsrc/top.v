module top(
    input clk,
    output [7:0] seg0,
    output [7:0] seg1,
    output [7:0] seg2,
    output [7:0] seg3,
    output [7:0] seg4,
    output [7:0] seg5,
    output [7:0] seg6,
    output [7:0] seg7
);

/* cpu */
reg [31:0] instr;
reg [4:0] rs1;
reg [4:0] rs2;
reg [4:0] rd;
wire [3:0] ALUctr;
wire ALUBsrc;
reg less;
reg zero;
wire of;
wire cf;
wire zf;
wire RegWr;
wire PCAsrc;
wire PCBsrc;
reg [2:0] MemOp;
reg MemtoReg;
reg [31:0] pc_next;
reg [2:0] branch;
reg [31:0] ALUout;
reg MemWr;
reg [31:0] data_out;
reg [6:0] opcode;
reg [31:0] op1;
reg [31:0] op2;
reg [31:0] out;
reg [31:0] imm;

assign rs1 = instr[19:15];
assign rs2 = instr[24:20];
assign rd = instr[11:7];

id my_id(
  .instr(instr),
  .imm(imm),
  .ALUctr(ALUctr),
  .ALUBsrc(ALUBsrc),
  .RegWr(RegWr),
  .branch(branch),
  .MemOp(MemOp),
  .MemtoReg(MemtoReg),
  .MemWr(MemWr)
);

Branch my_branch(
  .zf(zf),
  .less(less),
  .zero(zero),
  .branch(branch),
  .PCAsrc(PCAsrc),
  .PCBsrc(PCBsrc)
);

pc_reg my_pc(
  .clk(clk),
  .imm(imm),
  .op1(op1),
  .PCAsrc(PCAsrc),
  .PCBsrc(PCBsrc),
  .pc_next(pc_next)
);

instr_mem my_instrmem(
  .instr_addr(pc_next),
  .instr(instr)
);

always@(*)begin
  case(MemtoReg)
    1'b0: out = ALUout;
    1'b1: out = data_out;
  endcase
end

register my_reg(
  .Wrclk(clk),
  .RegWr(RegWr),
  .Ra(rs1),
  .Rb(rs2),
  .Rw(rd),
  .busA(op1),
  .busB(op2),
  .busW(out)
);

ALU my_alu(
  .A(op1),
  .rs2(op2),
  .imm(imm),
  .ALUctr(ALUctr),
  .ALUBsrc(ALUBsrc),
  .less(less),
  .zero(zero),
  .ALUout(ALUout),
  .of(of),
  .zf(zf),
  .cf(cf)
);

Ram my_ram(
  .Rdclk(clk),
  .Wrclk(clk),
  .Addr(ALUout),
  .MemOp(MemOp),
  .data_in(op2),
  .Wr_en(MemWr),
  .data_out(data_out)
);

seg my_seg(
    .out_data(data_out),
    .o_seg0(seg0),
    .o_seg1(seg1),
    .o_seg2(seg2),
    .o_seg3(seg3),
    .o_seg4(seg4),
    .o_seg5(seg5),
    .o_seg6(seg6),
    .o_seg7(seg7)
);
endmodule
