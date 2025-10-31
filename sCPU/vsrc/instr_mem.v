module instr_mem(
    input  [31:0] pc,
    output [31:0] instr
);
    reg [31:0] mem [0:65535];
    
    assign instr = mem[pc >> 2];

initial begin
    //initial $readmemh("instr_mem.txt", mem);
    //mem[0] = {12'b000000000001,5'b00001,3'b000,5'b00010,7'b0010011};
                     //      imm          rs1      func3   rd       opcode
    //mem[1] = {12'b000000000001,5'b00010,3'b001,5'b00011,7'b0010011};
    mem[0] = 32'h00100093; // addi x1, x0, 1
    mem[1] = 32'h00000113; // addi x2, x0, 0
    mem[2] = 32'h00A00193; // addi x3, x0, 10
    mem[3] = 32'h00110133; // add  x2, x2, x1
    mem[4] = 32'h00108093; // addi x1, x1, 1
    mem[5] = 32'hFE3186E3; // blt  x1, x3, loop
    mem[6] = 32'h00000013; // nop
end

endmodule

