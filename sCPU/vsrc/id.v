module id(
    input [31:0] instr_in,                      //输入指令
    output reg [31:0] imm,                      //立即数
    output reg [3:0] ALUctr,                    //ALU控制信号，决定加减乘除操作
    output reg ALUBsrc,                         //ALU第二操作数来源选择信号 0寄存器 1立即数
    output reg RegWr,                           //寄存器写使能信号
    output reg [2:0] branch,                    //分支类型
    output reg [2:0] MemOp,                     //存储器操作类型
    output reg MemWr,                           //存储器写使能信号
    output reg MemtoReg,                        //写回寄存器数据来源选择信号
    output reg [4:0] rs1,                         //源寄存器1地址
    output reg [4:0] rs2,                         //源寄存器2地址
    output reg [4:0] rd                          //目的寄存器地址
);

    // 提取字段
    reg [6:0] opcode;
    reg [2:0] func3;
    reg [6:0] func7;
    reg [31:0] immI, immS, immB, immJ;

    always @(*) begin
        rs1 = instr_in[19:15];
        rs2 = instr_in[24:20];
        rd = instr_in[11:7];
        opcode = instr_in[6:0];
        func3  = instr_in[14:12];
        func7  = instr_in[31:25];
        immI   = {{20{instr_in[31]}}, instr_in[31:20]};
        immS   = {{20{instr_in[31]}}, instr_in[31:25],  instr_in[11:7]};
        immB   = {{20{instr_in[31]}}, instr_in[7],      instr_in[30:25],    instr_in[11:8],     1'b0};
        immJ   = {{12{instr_in[31]}}, instr_in[19:12],  instr_in[20],       instr_in[30:21],    1'b0};
    end

    //后面是具体的译码逻辑，比较繁杂，
    always @(*) begin
        case(opcode)
            7'b0010011: begin  // instr type I 
                case(func3)
                    3'b000: begin  // addi
                        imm     = immI;
                        ALUctr  = 4'b0000;
                        ALUBsrc = 1'b1;
                        RegWr   = 1'b1;
                        branch  = 3'b000;
                        MemOp   = 3'b000;
                        MemWr   = 1'b0;
                        MemtoReg = 1'b0;
                    end
                    3'b010: begin // slti : set less than
                        imm     = immI;
                        ALUctr  = 4'b0010;
                        ALUBsrc = 1'b1;
                        RegWr   = 1'b1;
                        branch  = 3'b000;
                        MemOp   = 3'b000;
                        MemWr   = 1'b0;
                        MemtoReg = 1'b0;
                    end
                    3'b011: begin // sltiu : set less than unsigned
                        imm     = immI;
                        ALUctr  = 4'b1010;
                        ALUBsrc = 1'b1;
                        RegWr   = 1'b1;
                        branch  = 3'b000;
                        MemOp   = 3'b000;
                        MemWr   = 1'b0;
                        MemtoReg = 1'b0;
                    end
                    3'b100: begin // xori
                        imm     = immI;
                        ALUctr  = 4'b0100;
                        ALUBsrc = 1'b1;
                        RegWr   = 1'b1;    
                        branch  = 3'b000;
                        MemOp   = 3'b000;
                        MemWr   = 1'b0;    
                        MemtoReg = 1'b0;              
                    end
                    3'b110: begin // ori
                        imm     = immI;
                        ALUctr  = 4'b0110;
                        ALUBsrc = 1'b1;
                        RegWr   = 1'b1;    
                        branch  = 3'b000;
                        MemOp   = 3'b000;
                        MemWr   = 1'b0;
                        MemtoReg = 1'b0;
                    end
                    3'b111: begin // andi
                        imm     = immI;
                        ALUctr  = 4'b0111;
                        ALUBsrc = 1'b1;
                        RegWr   = 1'b1;
                        branch  = 3'b000;
                        MemWr   = 1'b0;
                    end
                    3'b001: begin // slli : shfit left logically
                        imm     = immI;
                        ALUctr  = 4'b0001;
                        ALUBsrc = 1'b1;
                        RegWr   = 1'b1;
                        branch  = 3'b000;
                        MemWr   = 1'b0;
                    end
                    3'b101: begin
                        case(func7)
                            7'b0000000: begin // srli : shfit left logic
                                imm     = immI;
                                ALUctr  = 4'b0101;
                                ALUBsrc = 1'b1;
                                RegWr   = 1'b1;
                                branch  = 3'b000;
                                MemWr   = 1'b0;
                            end
                            7'b0100000: begin // srai : shift right arith
                                imm     = immI;
                                ALUctr  = 4'b1101;
                                ALUBsrc = 1'b1;
                                RegWr   = 1'b1;
                                branch  = 3'b000;
                                MemWr   = 1'b0;
                            end
                            default: begin
                                imm     = 32'd0;
                                ALUctr  = 4'b000;
                                ALUBsrc = 1'b1;
                                RegWr   = 1'b0;
                                branch  = 3'b000;
                                MemWr   = 1'b0;
                            end
                        endcase
                    end
                endcase
            end
            7'b0110011: begin // instr type R
                case(func3)
                    3'b000: begin
                        case(func7)
                            7'b0000000: begin // add
                                ALUctr  = 4'b0000;
                                ALUBsrc = 1'b0;
                                RegWr   = 1'b1;
                                branch  = 3'b000;
                                MemWr   = 1'b0;
                            end 
                            7'b0100000: begin // sub
                                ALUctr  = 4'b1000;
                                ALUBsrc = 1'b0;
                                RegWr   = 1'b1;
                                branch  = 3'b000;
                                MemWr   = 1'b0;
                            end
                            default: begin
                                ALUctr  = 4'b0000;
                                ALUBsrc = 1'b0;
                                RegWr   = 1'b0;  
                                branch  = 3'b000;
                                MemWr   = 1'b0;
                            end                                                                                
                        endcase
                    end
                    3'b001: begin // sll : shfit left logically
                        ALUctr  = 4'b0001;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b1;
                        branch  = 3'b000;
                        MemWr   = 1'b0;
                    end
                    3'b010: begin // slt : set less than
                        ALUctr  = 4'b0010;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b1;
                        branch  = 3'b000;
                        MemWr   = 1'b0;
                    end
                    3'b011: begin // sltu : set less than unsigned
                        ALUctr  = 4'b1010;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b1;   
                        branch  = 3'b000;   
                        MemWr   = 1'b0;                
                    end
                    3'b100: begin // xor
                        ALUctr  = 4'b0100;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b1;    
                        branch  = 3'b000;
                        MemWr   = 1'b0;
                    end
                    3'b101: begin
                        case(func7)
                            7'b0000000: begin  // srl : shfit right logically
                                ALUctr  = 4'b0101;
                                ALUBsrc = 1'b0;
                                RegWr   = 1'b1;
                                branch  = 3'b000;
                                MemWr   = 1'b0;
                            end
                            7'b0100000: begin // sra : shfit right arthimetically
                                ALUctr  = 4'b1101;
                                ALUBsrc = 1'b0;
                                RegWr   = 1'b1;
                                branch  = 3'b000;
                                MemWr   = 1'b0;
                            end
                            default: begin
                                ALUctr  = 4'b0000;
                                ALUBsrc = 1'b0;
                                RegWr   = 1'b0;
                                branch  = 3'b000;
                                MemWr   = 1'b0;
                                end                                
                        endcase
                    end
                    3'b110: begin // or
                        ALUctr  = 4'b0110;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b1;
                        branch  = 3'b000;
                        MemWr   = 1'b0;
                    end        
                    3'b111: begin // and
                        ALUctr  = 4'b0111;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b1;
                        branch  = 3'b000;
                        MemWr   = 1'b0;
                    end
                endcase
            end
            7'b1100011: begin // instr type B
                case(func3)
                    3'b000: begin // beq
                        imm     = immB;
                        ALUctr  = 4'b0010;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b0;
                        branch  = 3'b100;
                        MemWr   = 1'b0;
                    end
                    3'b001: begin  // bne
                        imm     = immB;
                        ALUctr  = 4'b0010;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b0;
                        branch  = 3'b101;
                        MemWr   = 1'b0;
                    end
                    3'b100: begin // blt
                        imm     = immB;
                        ALUctr  = 4'b0010;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b0;
                        branch  = 3'b110;
                        MemWr   = 1'b0;
                    end
                    3'b101: begin // bge
                        imm     = immB;
                        ALUctr  = 4'b0010;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b0;
                        branch  = 3'b111;
                        MemWr   = 1'b0;
                    end
                    3'b110: begin // bltu
                        imm     = immB;
                        ALUctr  = 4'b1010;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b0;
                        branch  = 3'b110;
                        MemWr   = 1'b0;
                    end
                    3'b111: begin // bgeu
                        imm     = immB;
                        ALUctr  = 4'b1010;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b0;
                        branch  = 3'b111;
                        MemWr   = 1'b0;
                    end      
                    default:begin
                        imm     = immB;
                        ALUctr  = 4'b1010;
                        ALUBsrc = 1'b0;
                        RegWr   = 1'b0;
                        branch  = 3'b111;
                        MemWr   = 1'b0; 
                    end                               
                endcase
            end
            7'b1101111: begin // jal
                imm     = immJ;
                ALUctr  = 4'b0000;
                ALUBsrc = 1'b1;
                RegWr   = 1'b1;
                branch  = 3'b001; 
                MemWr   = 1'b0;
            end
            7'b1100111: begin // jalr
                imm     = immI;
                ALUctr  = 4'b0000;
                ALUBsrc = 1'b1;
                RegWr   = 1'b1;
                branch  = 3'b010;
                MemWr   = 1'b0;
            end

            7'b0000011: begin
                case(func3)
                    3'b000: begin // lb
                        imm      = immI;
                        ALUctr   = 4'b0000;
                        ALUBsrc  = 1'b1;
                        RegWr    = 1'b1;
                        branch   = 3'b000;
                        MemtoReg = 1'b1;
                        MemOp    = 3'b000;
                        MemWr    = 1'b0;
                    end
                    3'b001: begin // lh
                        imm      = immI;
                        ALUctr   = 4'b0000;
                        ALUBsrc  = 1'b1;
                        RegWr    = 1'b1;
                        branch   = 3'b000;
                        MemtoReg = 1'b1;
                        MemOp    = 3'b001;
                        MemWr    = 1'b0;
                    end
                    3'b010: begin //lw
                        imm      = immI;
                        ALUctr   = 4'b0000;
                        ALUBsrc  = 1'b1;
                        RegWr    = 1'b1;
                        branch   = 3'b000;
                        MemtoReg = 1'b1;
                        MemOp    = 3'b010;
                        MemWr    = 1'b0;
                    end
                    3'b100: begin //lbu
                        imm      = immI;
                        ALUctr   = 4'b0000;
                        ALUBsrc  = 1'b1;
                        RegWr    = 1'b1;
                        branch   = 3'b000;
                        MemtoReg = 1'b1;
                        MemOp    = 3'b100;
                        MemWr    = 1'b0;
                    end
                    3'b101: begin //lhu
                        imm      = immI;
                        ALUctr   = 4'b0000;
                        ALUBsrc  = 1'b1;
                        RegWr    = 1'b1;
                        branch   = 3'b000;
                        MemtoReg = 1'b1;
                        MemOp    = 3'b101;
                        MemWr    = 1'b0;
                    end
                    default:begin
                        imm      = immI;
                        ALUctr   = 4'b0000;
                        ALUBsrc  = 1'b1;
                        RegWr    = 1'b1;
                        branch   = 3'b000;
                        MemtoReg = 1'b1;
                        MemOp    = 3'b101;
                        MemWr    = 1'b0;
                    end                       
                endcase
            end
             7'b0100011: begin
                case(func3)
                    3'b000: begin // sb
                        imm      = immS;
                        ALUctr   = 4'b0000;
                        ALUBsrc  = 1'b1;
                        RegWr    = 1'b0;
                        branch   = 3'b000;
                        MemOp    = 3'b000;
                        MemWr    = 1'b1;
                    end
                    3'b001: begin // sh
                        imm      = immS;
                        ALUctr   = 4'b0000;
                        ALUBsrc  = 1'b1;
                        RegWr    = 1'b0;
                        branch   = 3'b000;
                        MemOp    = 3'b001;
                        MemWr    = 1'b1;
                    end
                    3'b010: begin // sw
                        imm      = immS;
                        ALUctr   = 4'b0000;
                        ALUBsrc  = 1'b1;
                        RegWr    = 1'b0;
                        branch   = 3'b000;
                        MemOp    = 3'b010;
                        MemWr    = 1'b1;
                    end
                    default: begin
                        imm      = immS;
                        ALUctr   = 4'b0000;
                        ALUBsrc  = 1'b1;
                        RegWr    = 1'b0;
                        branch   = 3'b000;
                        MemOp    = 3'b010;
                        MemWr    = 1'b1;
                    end                        
                endcase
            end
            default:begin
                imm     = 32'b0;
                ALUctr  = 4'b0000;
                ALUBsrc = 1'b0;
                RegWr   = 0;
                branch  = 3'b000;
                MemWr   = 1'b0;
            end
        endcase
        // $display("instr:%b",instr);
    end

endmodule
