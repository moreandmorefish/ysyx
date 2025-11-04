module top(
    input clk,
    input reset,
    input handshake,
    output [7:0] seg0,
    output [7:0] seg1,
    output [7:0] seg2,
    output [7:0] seg3,
    output [7:0] seg4,
    output [7:0] seg5,
    output [7:0] seg6,
    output [7:0] seg7
);

    // ---------------------------
    // IF 阶段：取指 + PC 更新
    // ---------------------------
    wire [31:0] pc_current, next_pc;
    wire [31:0] instr;
    wire halt = (instr == 32'h00100073); // ebreak 指令作为 halt 信号
    //wire clk_cpu = clk & ~halt;
    wire clk_cpu = handshake;
    wire jump_en;
    // PC寄存器
    PC my_pc(
        .clk(clk_cpu),
        .halt(halt),
        .reset(reset),
        .next_pc(next_pc),
        .pc(pc_current),
        .jump_en(jump_en)
    );

    // 指令存储器（组合读）
    instr_mem my_instr_mem(
        .pc(pc_current),
        .instr(instr)
    );

    // IF/ID流水寄存器
    wire [31:0] id_pc_in, id_instr_in;
    IF_ID my_if_id(
        .clk(clk_cpu),
        .reset(reset),
        .pc_in(pc_current),
        .instr_in(instr),
        .pc_out(id_pc_in),
        .instr_out(id_instr_in)
    );


    // ---------------------------
    // ID 阶段：译码 + 读寄存器
    // ---------------------------
    wire [31:0] imm;
    wire [3:0]  ALUctr;
    wire        ALUBsrc;
    wire        RegWr_ID;
    wire [2:0]  branch_ID;
    wire [2:0]  MemOp_ID;
    wire        MemWr_ID;
    wire        MemtoReg_ID;
    wire [4:0]  rs1_ID, rs2_ID, rd_ID;

    // 控制译码
    id my_decoder(
        .instr_in(id_instr_in),
        .imm(imm),
        .ALUctr(ALUctr),
        .ALUBsrc(ALUBsrc),
        .RegWr(RegWr_ID),
        .branch(branch_ID),
        .MemOp(MemOp_ID),
        .MemWr(MemWr_ID),
        .MemtoReg(MemtoReg_ID),
        .rs1(rs1_ID),
        .rs2(rs2_ID),
        .rd(rd_ID)
    );

    // 寄存器堆（组合读，同拍写）
    wire [31:0] busA_ID, busB_ID;
    wire [31:0] busW_WB;
    wire [4:0]  rd_WB;
    wire        RegWr_WB;
    RegisterFile my_regfile(
        .clk(clk_cpu),
        .RegWr(RegWr_WB),
        .Rw(rd_WB),
        .busW(busW_WB),
        .Ra(rs1_ID),
        .Rb(rs2_ID),
        .busA(busA_ID),
        .busB(busB_ID),
        .pc_WB(wb_pc)
    );

    // ID/EX 流水寄存器
    wire [31:0] ex_imm, ex_pc, busB_out, busA_out;
    wire [3:0]  ex_ALUctr;
    wire        ex_ALUBsrc, ex_RegWr, ex_MemWr, ex_MemtoReg;
    wire [2:0]  ex_branch, ex_MemOp;
    wire [4:0]  ex_rs1, ex_rs2, ex_rd;
    ID_EX my_id_ex(
        .clk(clk_cpu),
        .reset(reset),
        .imm_in(imm),
        .ALUctr_in(ALUctr),
        .ALUBsrc_in(ALUBsrc),
        .RegWr_in(RegWr_ID),
        .branch_in(branch_ID),
        .MemOp_in(MemOp_ID),
        .MemWr_in(MemWr_ID),
        .MemtoReg_in(MemtoReg_ID),
        .rs1_in(rs1_ID),
        .rs2_in(rs2_ID),
        .rd_in(rd_ID),
        .pc_out(id_pc_in),
        .imm(ex_imm),
        .busA_in(busA_ID),
        .busA_out(busA_out),
        .busB_in(busB_ID),
        .busB_out(busB_out),
        .ALUctr(ex_ALUctr),
        .ALUBsrc(ex_ALUBsrc),
        .RegWr(ex_RegWr),
        .branch(ex_branch),
        .MemOp(ex_MemOp),
        .MemWr(ex_MemWr),
        .MemtoReg(ex_MemtoReg),
        .rs1(ex_rs1),
        .rs2(ex_rs2),
        .rd(ex_rd),
        .pc_to_ex(ex_pc)
    );


    // ---------------------------
    // EX 阶段：ALU计算 + Branch
    // ---------------------------
    wire [31:0] alu_in2 = (ex_ALUBsrc) ? ex_imm : busB_out;  // ALU第二操作数选择
    wire [31:0] alu_out;
    wire less, zero, of, cf;

    ALU my_alu(
        .A(busA_out),
        .B(alu_in2),
        .ALUctr(ex_ALUctr),
        .less(less),
        .zero(zero),
        .of(of),
        .cf(cf),
        .ALUout(alu_out)
    );

    wire [31:0] branch_target;
    Branch my_branch(
        .A(busA_out),
        .B(busB_out),
        .branch(ex_branch),
        .zero(zero),
        .less(less),
        .pc_to_ex(ex_pc),
        .imm(ex_imm),
        .branch_target(branch_target),
        .jump_en(jump_en)
    );

    // 下一条PC选择
    assign next_pc = branch_target;  // 暂时直接使用（未加分支预测或flush逻辑）


    // EX/MEM 流水寄存器
    wire [31:0] mem_alu_out, mem_branch_target, mem_rs2_val;
    wire [2:0]  mem_MemOp, mem_branch;
    wire        mem_MemWr, mem_RegWr, mem_MemtoReg;
    wire [4:0]  mem_rd;
    wire [31:0] mem_pc, wb_pc;

    EX_MEM my_ex_mem(
        .clk(clk_cpu),
        .reset(reset),
        .alu_out_in(alu_out),
        .branch_target_in(branch_target),
        .rs2_val_in(busB_out),
        .mem_op_in(ex_MemOp),
        .mem_wr_in(ex_MemWr),
        .reg_wr_in(ex_RegWr),
        .memtoreg_in(ex_MemtoReg),
        .rd_in(ex_rd),
        .branch_in(ex_branch),
        .alu_out(mem_alu_out),
        .branch_target(mem_branch_target),
        .rs2_val(mem_rs2_val),
        .mem_op(mem_MemOp),
        .mem_wr(mem_MemWr),
        .reg_wr(mem_RegWr),
        .memtoreg(mem_MemtoReg),
        .rd(mem_rd),
        .branch(mem_branch),
        .pc_in(ex_pc),         // 传入当前指令的PC
        .pc_out(mem_pc)        // 输出到下一级
    );


    // ---------------------------
    // MEM 阶段：访存
    // ---------------------------
    wire [31:0] mem_data_out;

    Ram my_ram(
        .clk(clk_cpu),
        .Addr_byte(mem_alu_out),
        .MemOp(mem_MemOp),
        .data_in(mem_rs2_val),
        .Wr_en(mem_MemWr),
        .data_out(mem_data_out)
    );

    // MEM/WB 流水寄存器
    wire [31:0] wb_mem_data, wb_alu_out;
    wire        wb_RegWr, wb_MemtoReg;
    wire [31:0] wb_pc;

    MEM_WB my_mem_wb(
        .clk(clk_cpu),
        .reset(reset),
        .mem_data_in(mem_data_out),
        .alu_out_in(mem_alu_out),
        .reg_wr_in(mem_RegWr),
        .memtoreg_in(mem_MemtoReg),
        .rd_in(mem_rd),
        .mem_data_out(wb_mem_data),
        .alu_out_out(wb_alu_out),
        .reg_wr_out(wb_RegWr),
        .memtoreg_out(wb_MemtoReg),
        .rd_out(rd_WB),
        .pc_in(mem_pc),        // 传入当前指令的PC
        .pc_out(wb_pc)
    );


    // ---------------------------
    // WB 阶段：写回寄存器堆
    // ---------------------------
    assign busW_WB = (wb_MemtoReg) ? wb_mem_data : wb_alu_out;
    assign RegWr_WB = wb_RegWr;


seg my_seg(
    .out_data(busW_WB),
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
