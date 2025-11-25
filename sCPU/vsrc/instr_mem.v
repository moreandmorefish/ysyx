import "DPI-C" function int pmem_read(input int addr);// 
module instr_mem(
    input         clk,
    input  [31:0] pc,
    input flush,
    output reg [31:0] pc_addr,
    output reg [31:0] instr
);
    always @(posedge clk) begin
        if(flush)begin
            instr <= 32'h00000013;
            pc_addr <=pc_addr;
        end else begin
            instr <= pmem_read(pc);   // 同步读
            pc_addr <= pc;
        end
    end
endmodule


// module instr_mem(
//     input         clk,
//     input  [31:0] pc,
//     input flush,
//     output reg [31:0] pc_addr,
//     output reg [31:0] instr
// );
//     reg [31:0] memory [0:255]; // 1KB 指令内存（PC=0~1023 → 索引0~255，步长4）
// 
//     initial begin
//         // 初始化：所有位置填NOP（addi x0, x0, 0，RISC-V标准合法指令）
//         integer i;
//         for(i = 0; i < 256; i = i + 1) begin
//             memory[i] = 32'h00000013; // NOP：二进制00000000000000000000000000010011
//         end
// 
//         // -------------------------- RISC-V标准测试指令序列 --------------------------
//         // 核心逻辑：PC初始=0 → 初始化x1=0x30 → JALR跳转至PC=0x40 → 执行特征指令验证
//         // 每条有效指令间插8个NOP（索引间隔9，避免流水冲突）
// 
//         // 索引0（PC=0x0）：addi x1, x0, 0x30 → 初始化x1=0x30（RISC-V标准机器码）
//         // 编码说明：opcode=0010011（ADDI），rs1=x0(00000)，rd=x1(00001)，imm=0x30(0000000000110000)
//         memory[0]  = 32'h03000093; // 二进制：00000000001100000000000010010011
// 
//         // 索引9（PC=0x24）：jalr x2, x1, 0x10 → 跳转目标PC=x1+0x10=0x30+0x10=0x40（索引10）
//         // 编码说明：opcode=0000111（JALR标准），rs1=x1(00001)，rd=x2(00010)，imm=0x10(000000010000)
//         memory[9]  = 32'h01008107; // 二进制：00000001000000001000000100000111（关键修正！）
// 
//         // 索引10（PC=0x40）：addi t0, x0, 0x1234 → 特征指令（验证跳转成功）
//         // 编码说明：opcode=0010011（ADDI），rs1=x0(00000)，rd=t0(x5,00101)，imm=0x234(0000001000110100)
//         memory[10] = 32'h23400293; // 二进制：00000010001101000000001010010011
// 
//         // 索引19（PC=0x78）：addi t1, x0, 0x8 → 二次验证指令
//         // 编码说明：opcode=0010011（ADDI），rs1=x0(00000)，rd=t1(x6,00110)，imm=0x8(0000000000001000)
//         memory[19] = 32'h00800313; // 二进制：00000000000010000000001100010011
// 
//         // 索引28（PC=0xB0）：ebreak → 终止指令（RISC-V标准编码）
//         // 编码说明：opcode=1110011（system），func3=000，imm12=000000000001
//         memory[28] = 32'h00100073; // 二进制：00000000000100000000000001110011
//         // ---------------------------------------------------------------------------
//     end
// 
//     // 同步读：PC右移2位（除以4）得到内存索引，超出范围输出NOP
//     always @(posedge clk) begin
//         if(flush)begin
//             instr <=32'h00000013;
//         end
//         else if((pc >> 2) < 256) begin
//             instr <= memory[pc >> 2];
//             pc_addr <= pc;
//         end else begin
//             instr <= 32'h00000013; // 超出范围输出NOP
//         end
//     end
// endmodule