// module instr_mem (
//     input  [31:0] instr_addr,
//     output reg [31:0] instr
//     // input  clk   
//     // input  rst
//     // input  en,
// );
//     reg [31:0] instr_memory [0:131071];
// 
//     initial begin
//         //$readmemh("instr_mem.txt", mem);
//         instr_memory[0] = {12'b000000000000,   5'b00000,   3'b000,     5'b00101,   7'b0010011};
//         //addi             imm              rs1         funct3      rd           opcode 
//         instr_memory[1] = {12'b000000000001,   5'b00101,   3'b000,     5'b00101,   7'b0010011};
//     end
// 
//     always @(*) begin
//         instr = instr_memory[instr_addr>>2];
//     end
// 
// //    always @(posedge clk) begin
// //        if(en) begin
// //            instr = instr_memory[instr_addr>>2];
// //        end
// //    end
// endmodule

module instr_mem(
    input [31:0] instr_addr,
    output reg [31:0] instr
);
    reg [31:0] instr_memory [65535:0];

    always@(*)begin
        instr = instr_memory[instr_addr>>2];  // pc + 4 -> instr_addr + 1
    end

initial begin
    instr_memory[0] = {12'b000000000001,5'b00001,3'b000,5'b00010,7'b0010011};
                     //      imm          rs1      func3   rd       opcode
    instr_memory[1] = {12'b000000000001,5'b00010,3'b001,5'b00011,7'b0010011};
end

endmodule

