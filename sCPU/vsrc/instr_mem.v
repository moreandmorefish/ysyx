module instr_mem(
    input  [31:0] pc,
    output [31:0] instr
);
    reg [31:0] mem [0:65535];
    
    assign instr = mem[pc >> 2];
    always @(*) begin
        $display("Fetch Instruction at PC=%h: %h", pc, instr);
    end

initial begin
    //initial $readmemh("instr_mem.txt", mem);
    //mem[0] = {12'b000000000001,5'b00001,3'b000,5'b00010,7'b0010011};
                     //      imm          rs1      func3   rd       opcode
    //mem[1] = {12'b000000000001,5'b00010,3'b001,5'b00011,7'b0010011};
    mem[0] = 32'h00100093; // addi x1, x0, 1
    mem[1] = 32'h00110113; // addi x2, x2, 1
    mem[2] = 32'h00210113; // addi x2, x2, 2
    mem[3] = 32'h00310113; // addi x2, x2, 3
    mem[4] = 32'h00410113; // addi x2, x2, 4
    mem[5] = 32'h00510113; // addi x2, x2, 5
    mem[6] = 32'h00610113; // addi x2, x2, 6
    mem[7] = 32'h00710113; // addi x2, x2, 7
    mem[8] = 32'h00810113; // addi x2, x2, 8
    mem[9] = 32'h00910113; // addi x2, x2, 9
    mem[10]= 32'h00a10113; // addi x2, x2, 10
    mem[11]= 32'h00100073; // ebreak
end

endmodule

