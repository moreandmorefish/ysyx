module Branch(
    input [31:0] A,          // 来自寄存器堆的源操作数 A
    input [31:0] B,          // 来自寄存器堆的源操作数 B
    input [2:0]  branch,     // 分支控制信号
    input        zero,        // 来自 ALU 的 zero 标志
    input        less,        // 来自 ALU 的 less 标志（有符号比较）
    input [31:0] pc_to_ex,    // 当前指令对应的 PC 值
    input [31:0] imm,         // 分支偏移立即数
    output reg jump_en,         // 是否进行跳转
    output reg [31:0] branch_target   // 跳转目标地址
);

    always @(*) begin
    // 默认：不跳
    jump_en = 0;
    branch_target = pc_to_ex + 4;

    case(branch)

        3'b100: begin  // BEQ
            if (zero) begin
                jump_en = 1;
                branch_target = pc_to_ex + imm;
            end
        end

        3'b101: begin  // BNE
            if (!zero) begin
                jump_en = 1;
                branch_target = pc_to_ex + imm;
            end
        end

        3'b110: begin  // BLT
            if (less) begin
                jump_en = 1;
                branch_target = pc_to_ex + imm;
            end
        end

        3'b111: begin  // BGE
            if (!less) begin
                jump_en = 1;
                branch_target = pc_to_ex + imm;
            end
        end

        3'b001: begin  // JAL
            jump_en = 1;
            branch_target = pc_to_ex + imm;
        end

        3'b010: begin  // JALR
            jump_en = 1;
            branch_target = (A + imm) & 32'hFFFFFFFE;
        end

        default: begin
            jump_en = 0;
            branch_target = pc_to_ex + 4;
        end
    endcase
end

endmodule