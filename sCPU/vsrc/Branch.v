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
        branch_target = pc_to_ex + imm;  // 默认目标地址为PC + imm
        jump_en = 0;  // 默认不跳转
        case(branch)
            3'b100: begin  // BEQ: Branch if Equal
                if (zero)  // 如果A和B相等
                    branch_target = pc_to_ex + imm;
                else
                    branch_target = pc_to_ex + 4;  // 不跳转，继续执行
            end

            3'b101: begin  // BNE: Branch if Not Equal
                if (~zero)  // 如果A和B不等
                    branch_target = pc_to_ex + imm;
                else
                    branch_target = pc_to_ex + 4;  // 不跳转，继续执行
            end

            3'b110: begin  // BLT: Branch if Less Than (有符号比较)
                if (less)  // 如果A小于B
                    branch_target = pc_to_ex + imm;
                else
                    branch_target = pc_to_ex + 4;  // 不跳转，继续执行
            end

            3'b111: begin  // BGE: Branch if Greater or Equal (有符号比较)
                if (~less)  // 如果A大于或等于B
                    branch_target = pc_to_ex + imm;
                else
                    branch_target = pc_to_ex + 4;  // 不跳转，继续执行
            end

            3'b001: begin  // JAL (无条件跳转)
                branch_target = pc_to_ex + imm;
            end

            3'b010: begin  // JALR (无条件跳转)
                branch_target = A + imm;  // 计算跳转地址
            end

            default: begin
                branch_target = pc_to_ex + 4;  // 默认跳转到下一条指令
            end
        endcase
    jump_en = (branch_target != (pc_to_ex + 4));  // 如果目标地址不是下一条指令，则跳转
    end

endmodule
