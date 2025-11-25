module id(
    input  [31:0] instr_in,   // 输入指令
    output reg [31:0] imm,    // 立即数
    output reg [4:0]  rd,     // 写寄存器地址
    output reg [4:0]  rs1,    // 源寄存器1地址
    output reg [4:0]  rs2,    // 源寄存器2地址

    // 控制信号
    output reg        RegWr,  // 寄存器写使能
    output reg [3:0]  ALUctr, // ALU运算控制（仅用0000=ADD）
    output reg        ALUBsrc, // ALU B端口选择（0:rs2, 1:imm）
    output reg [2:0]  branch, // 分支控制（000:无分支, 010:JALR）
    output reg [2:0]  MemOp,  // 存储操作类型（010:LW, 011:LBU, 111:SW, 101:SB）
    output reg        MemWr,  // 存储写使能
    output reg        MemRd,  // 存储读使能
    output reg        MemtoReg, // 写回选择（0:ALU结果, 1:存储读出数据）
    output reg        illegal_instr // 非法指令标志
);

    // ----------------------------
    // RISC-V32 标准指令字段解析
    // ----------------------------
    wire [6:0] opcode = instr_in[6:0];  // 操作码（RISC-V标准）
    wire [2:0] func3  = instr_in[14:12];// 功能码3（细分加载/存储/分支）
    wire [6:0] func7  = instr_in[31:25];// 功能码7（R型指令细分）

    // ----------------------------
    // RISC-V32 标准立即数格式（符号扩展/补零）
    // ----------------------------
    wire [31:0] immI = {{20{instr_in[31]}}, instr_in[31:20]}; // I型（addi、jalr、lw、lbu）
    wire [31:0] immS = {{20{instr_in[31]}}, instr_in[31:25], instr_in[11:7]}; // S型（sw、sb）
    wire [31:0] immU = {instr_in[31:12], 12'b0}; // U型（lui）

    // ----------------------------
    // RISC-V32 标准译码逻辑（仅支持指定指令）
    // ----------------------------
    always @(*) begin
        // 默认值（避免latch，默认非法指令）
        imm            = 32'd0;
        rd             = 5'd0;
        rs1            = 5'd0;
        rs2            = 5'd0;
        RegWr          = 1'b0;
        ALUctr         = 4'b0000; // 所有指令仅需ADD运算
        ALUBsrc        = 1'b0;
        branch         = 3'b000;
        MemOp          = 3'b000;
        MemWr          = 1'b0;
        MemRd          = 1'b0;
        MemtoReg       = 1'b0;
        illegal_instr  = 1'b1; // 默认非法，匹配指令后置0

        case (opcode)
            //===========================================================
            // 1. LUI（Load Upper Immediate）- U型指令（RISC-V标准 opcode=0110111）
            //===========================================================
            7'b0110111: begin 
                imm            = immU;
                rd             = instr_in[11:7];
                RegWr          = 1'b1;    // 写回寄存器
                ALUBsrc        = 1'b1;    // ALU B端口选立即数（0+immU=immU）
                illegal_instr  = 1'b0;    // 合法指令
            end

            //===========================================================
            // 2. JALR（Jump and Link Register）- I型指令（RISC-V标准 opcode=0000111）
            //===========================================================
            7'b1100111: begin 
                imm            = immI;
                rd             = instr_in[11:7];
                rs1            = instr_in[19:15];
                RegWr          = 1'b1;    // 写回返回地址（PC+4）
                ALUBsrc        = 1'b1;    // ALU B端口选立即数（rs1+imm=目标地址）
                branch         = 3'b010;  // 标记JALR分支
                illegal_instr  = 1'b0;    // 合法指令
            end

            //===========================================================
            // 3. ADD（R型指令）- RISC-V标准 opcode=0110011，func7=0000000，func3=000
            //===========================================================
            7'b0110011: begin 
                if (func7 == 7'b0000000 && func3 == 3'b000) begin // 仅支持ADD（排除SUB）
                    rd             = instr_in[11:7];
                    rs1            = instr_in[19:15];
                    rs2            = instr_in[24:20];
                    RegWr          = 1'b1;    // 写回寄存器
                    ALUBsrc        = 1'b0;    // ALU B端口选rs2
                    illegal_instr  = 1'b0;    // 合法指令
                end
            end

            //===========================================================
            // 4. ADDI（I型指令）- RISC-V标准 opcode=0010011，func3=000
            //===========================================================
            7'b0010011: begin 
                if (func3 == 3'b000) begin // 仅支持ADDI
                    imm            = immI;
                    rd             = instr_in[11:7];
                    rs1            = instr_in[19:15];
                    ALUBsrc        = 1'b1;    // ALU B端口选立即数
                    RegWr          = 1'b1;    // 写回寄存器
                    illegal_instr  = 1'b0;    // 合法指令
                end
            end

            //===========================================================
            // 5. 加载指令（LW/LBU）- I型指令（RISC-V标准 opcode=0000011）
            //===========================================================
            7'b0000011: begin 
                imm            = immI;
                rd             = instr_in[11:7];
                rs1            = instr_in[19:15];
                RegWr          = 1'b1;    // 写回存储数据
                ALUBsrc        = 1'b1;    // ALU B端口选立即数（计算地址）
                MemRd          = 1'b1;    // 允许存储读
                MemtoReg       = 1'b1;    // 写回选择存储数据
                illegal_instr  = 1'b0;    // 合法指令

                case(func3)
                    3'b010: MemOp = 3'b010; // LW（字加载）
                    3'b100: MemOp = 3'b011; // LBU（无符号字节加载）
                    default: illegal_instr = 1'b1; // 非法func3
                endcase
            end

            //===========================================================
            // 6. 存储指令（SW/SB）- S型指令（RISC-V标准 opcode=0100011）
            //===========================================================
            7'b0100011: begin 
                imm            = immS;
                rs1            = instr_in[19:15];
                rs2            = instr_in[24:20];
                MemWr          = 1'b1;    // 允许存储写
                ALUBsrc        = 1'b1;    // ALU B端口选立即数（计算地址）
                RegWr          = 1'b0;    // 无寄存器写回
                illegal_instr  = 1'b0;    // 合法指令

                case(func3)
                    3'b010: MemOp = 3'b110; // SW（字存储）
                    3'b000: MemOp = 3'b100; // SB（字节存储）
                    default: illegal_instr = 1'b1; // 非法func3
                endcase
            end

            //===========================================================
            // 7. EBREAK（系统指令）- RISC-V标准 opcode=1110011，func3=000，imm12=000000000001
            //===========================================================
            7'b1110011: begin 
                if (func3 == 3'b000 && instr_in[31:20] == 12'b000000000001) begin
                    illegal_instr  = 1'b1; // 标记为非法（便于终止）
                end
            end
            default: begin
                illegal_instr  = 1'b0; // 非法指令
            end
        endcase
    end

endmodule

// module id(
//     input  [31:0] instr_in,   // 输入指令
//     output reg [31:0] imm,    // 立即数
//     output reg [4:0]  rd,     // 写寄存器地址
//     output reg [4:0]  rs1,    // 源寄存器1地址
//     output reg [4:0]  rs2,    // 源寄存器2地址
// 
//     // 控制信号
//     output reg        RegWr,  // 寄存器写使能
//     output reg [3:0]  ALUctr, // ALU运算控制
//     output reg        ALUBsrc, // ALU B端口选择（0:rs2, 1:imm）
//     output reg [2:0]  branch, // 分支控制（000:无分支, 001:JAL, 010:JALR, 011:AUIPC, 100:BEQ, 101:BNE, 110:BLT/BLTU, 111:BGE/BGEU）
//     output reg [2:0]  MemOp,  // 存储操作类型（见注释）
//     output reg        MemWr,  // 存储写使能
//     output reg        MemRd,  // 存储读使能
//     output reg        MemtoReg, // 写回选择（0:ALU结果, 1:存储读出数据）
//     output reg        illegal_instr // 非法指令标志（新增，便于调试）
// );
// 
//     // ----------------------------
//     // 指令字段解析（RISC-V规范）
//     // ----------------------------
//     wire [6:0] opcode = instr_in[6:0]; // 操作码（指令类型）
//     wire [2:0] func3  = instr_in[14:12]; // 功能码3（细分指令）
//     wire [6:0] func7  = instr_in[31:25]; // 功能码7（R-type指令细分）
// 
//     // ----------------------------
//     // 立即数格式（RISC-V规范，符号扩展/补零）
//     // ----------------------------
//     wire [31:0] immI = {{20{instr_in[31]}}, instr_in[31:20]}; // I-type：符号扩展20位
//     wire [31:0] immS = {{20{instr_in[31]}}, instr_in[31:25], instr_in[11:7]}; // S-type：符号扩展20位
//     wire [31:0] immB = {{20{instr_in[31]}}, instr_in[7], instr_in[30:25], instr_in[11:8], 1'b0}; // B-type：符号扩展+左移1位
//     wire [31:0] immU = {instr_in[31:12], 12'b0}; // U-type：低12位补零
//     wire [31:0] immJ = {{12{instr_in[31]}}, instr_in[19:12], instr_in[20], instr_in[30:21], 1'b0}; // J-type：符号扩展+左移1位
// 
//     // ----------------------------
//     // 编码定义注释（便于后续模块对接）
//     // ----------------------------
//     // ALUctr编码：
//     // 0000: ADD    1000: SUB
//     // 0001: SLL    1001: 保留
//     // 0010: SLT    1010: SLTU
//     // 0100: XOR    1100: 保留
//     // 0101: SRL    1101: SRA
//     // 0110: OR     1110: 保留
//     // 0111: AND    1111: 保留
//     // MemOp编码（与存储控制器对接，仅含0/1，无非法字符）：
//     // 000: LB      101: SB（修正为3'b101，原错误3'b105）
//     // 001: LH      110: SH
//     // 010: LW      111: SW
//     // 011: LBU
//     // 100: LHU
// 
//     // ----------------------------
//     // ID阶段组合逻辑译码
//     // ----------------------------
//     always @(*) begin
//         // 默认值（NOP：无操作，避免 latch）
//         imm            = 32'd0;
//         rd             = 5'd0;    // 默认写x0（无副作用）
//         rs1            = 5'd0;    // 默认读x0
//         rs2            = 5'd0;    // 默认读x0
//         RegWr          = 1'b0;    // 禁止寄存器写回
//         ALUctr         = 4'b0000; // 默认ADD
//         ALUBsrc        = 1'b0;    // 默认选择rs2作为ALU B端口
//         branch         = 3'b000;  // 默认无分支
//         MemOp          = 3'b000;  // 默认无存储操作
//         MemWr          = 1'b0;    // 禁止存储写
//         MemRd          = 1'b0;    // 禁止存储读
//         MemtoReg       = 1'b0;    // 默认写回ALU结果
//         illegal_instr  = 1'b0;    // 默认合法指令
// 
//         case (opcode)
//             //===========================================================
//             // U-type：LUI（Load Upper Immediate）→ rd = immU
//             //===========================================================
//             7'b0110111: begin 
//                 imm      = immU;
//                 rd       = instr_in[11:7];
//                 rs1      = 5'd0;    // 强制读x0（A端口=0）
//                 rs2      = 5'd0;    // 无用，显式置0
//                 RegWr    = 1'b1;    // 允许寄存器写回
//                 ALUBsrc  = 1'b1;    // 选择imm作为ALU B端口
//                 ALUctr   = 4'b0000; // ALU执行ADD（0+immU=immU）
//             end
// 
//             //===========================================================
//             // U-type：AUIPC（Add Upper Immediate to PC）→ rd = PC + immU
//             //===========================================================
//             7'b0010111: begin 
//                 imm      = immU;
//                 rd       = instr_in[11:7];
//                 rs1      = 5'd0;    // 无用，显式置0
//                 rs2      = 5'd0;    // 无用，显式置0
//                 RegWr    = 1'b1;    // 允许寄存器写回
//                 ALUBsrc  = 1'b1;    // 选择imm作为ALU B端口
//                 ALUctr   = 4'b0000; // ALU执行ADD（PC+immU）
//                 branch   = 3'b011;  // 标记使用PC作为ALU A端口
//             end
// 
//             //===========================================================
//             // J-type：JAL（Jump and Link）→ rd = PC+4，PC=PC+immJ
//             //===========================================================
//             7'b1101111: begin 
//                 imm      = immJ;
//                 rd       = instr_in[11:7];
//                 rs1      = 5'd0;    // 无用，显式置0
//                 rs2      = 5'd0;    // 无用，显式置0
//                 RegWr    = 1'b1;    // 允许寄存器写回（PC+4）
//                 branch   = 3'b001;  // 标记JAL分支
//                 ALUBsrc  = 1'bx;    // 无关项，显式标记
//                 ALUctr   = 4'bxxxx; // 无关项，显式标记
//             end
// 
//             //===========================================================
//             // I-type：JALR（Jump and Link Register）→ rd=PC+4，PC=(rs1+immI)&~1
//             //===========================================================
//             7'b1100111: begin 
//                 imm      = immI;
//                 rd       = instr_in[11:7];
//                 rs1      = instr_in[19:15];
//                 rs2      = 5'd0;    // 无用，显式置0
//                 RegWr    = 1'b1;    // 允许寄存器写回（PC+4）
//                 ALUBsrc  = 1'b1;    // 选择imm作为ALU B端口
//                 branch   = 3'b010;  // 标记JALR分支
//                 ALUctr   = 4'b0000; // ALU执行ADD（rs1+immI）
//             end
// 
//             //===========================================================
//             // B-type：分支指令（BEQ/BNE/BLT/BGE/BLTU/BGEU）
//             //===========================================================
//             7'b1100011: begin
//                 imm      = immB;
//                 rs1      = instr_in[19:15];
//                 rs2      = instr_in[24:20];
//                 ALUBsrc  = 1'b0;    // 选择rs2作为ALU B端口
//                 RegWr    = 1'b0;    // 禁止寄存器写回（关键修正）
//                 case (func3)
//                     3'b000: begin branch=3'b100; ALUctr=4'b1000; end // BEQ：ALU做SUB，结果为0则分支
//                     3'b001: begin branch=3'b101; ALUctr=4'b1000; end // BNE：ALU做SUB，结果非0则分支
//                     3'b100: begin branch=3'b110; ALUctr=4'b0010; end // BLT：ALU做SLT（有符号比较）
//                     3'b101: begin branch=3'b111; ALUctr=4'b0010; end // BGE：ALU做SLT（有符号比较）
//                     3'b110: begin branch=3'b110; ALUctr=4'b1010; end // BLTU：ALU做SLTU（无符号比较）
//                     3'b111: begin branch=3'b111; ALUctr=4'b1010; end // BGEU：ALU做SLTU（无符号比较）
//                     default: illegal_instr = 1'b1; // 非法func3，标记非法指令
//                 endcase
//             end
// 
//             //===========================================================
//             // I-type：Load指令（lb/lh/lw/lbu/lhu）
//             //===========================================================
//             7'b0000011: begin
//                 imm      = immI;
//                 rd       = instr_in[11:7];
//                 rs1      = instr_in[19:15];
//                 rs2      = 5'd0;    // 无用，显式置0
//                 RegWr    = 1'b1;    // 允许寄存器写回（存储读出数据）
//                 ALUBsrc  = 1'b1;    // 选择imm作为ALU B端口
//                 MemRd    = 1'b1;    // 允许存储读
//                 MemtoReg = 1'b1;    // 写回选择存储读出数据
//                 ALUctr   = 4'b0000; // ALU执行ADD（rs1+immI=地址）
//                 case(func3)
//                     3'b000: MemOp=3'b000; // LB：字节加载（有符号）
//                     3'b001: MemOp=3'b001; // LH：半字加载（有符号）
//                     3'b010: MemOp=3'b010; // LW：字加载
//                     3'b100: MemOp=3'b011; // LBU：字节加载（无符号）
//                     3'b101: MemOp=3'b100; // LHU：半字加载（无符号）
//                     default: illegal_instr = 1'b1; // 非法func3，标记非法指令
//                 endcase
//             end
// 
//             //===========================================================
//             // S-type：Store指令（sb/sh/sw）
//             //===========================================================
//             7'b0100011: begin
//                 imm      = immS;
//                 rs1      = instr_in[19:15];
//                 rs2      = instr_in[24:20];
//                 MemWr    = 1'b1;    // 允许存储写
//                 ALUBsrc  = 1'b1;    // 选择imm作为ALU B端口
//                 ALUctr   = 4'b0000; // ALU执行ADD（rs1+immS=地址）
//                 RegWr    = 1'b0;    // 禁止寄存器写回（关键修正）
//                 case(func3)
//                     3'b000: MemOp=3'b101; // SB：字节存储（修正：3'b105→3'b101，移除非法字符5）
//                     3'b001: MemOp=3'b110; // SH：半字存储
//                     3'b010: MemOp=3'b111; // SW：字存储
//                     default: illegal_instr = 1'b1; // 非法func3，标记非法指令
//                 endcase
//             end
// 
//             //===========================================================
//             // I-type：算术逻辑指令（ADDI/SLTI/SLTIU/XORI/ORI/ANDI/SLLI/SRLI/SRAI）
//             //===========================================================
//             7'b0010011: begin 
//                 imm      = immI;
//                 rd       = instr_in[11:7];
//                 rs1      = instr_in[19:15];
//                 rs2      = 5'd0;    // 无用，显式置0
//                 RegWr    = 1'b1;    // 允许寄存器写回
//                 ALUBsrc  = 1'b1;    // 选择imm作为ALU B端口
//                 case(func3)
//                     3'b000: ALUctr = 4'b0000; // ADDI：加法
//                     3'b010: ALUctr = 4'b0010; // SLTI：有符号小于则置1
//                     3'b011: ALUctr = 4'b1010; // SLTIU：无符号小于则置1
//                     3'b100: ALUctr = 4'b0100; // XORI：异或
//                     3'b110: ALUctr = 4'b0110; // ORI：或
//                     3'b111: ALUctr = 4'b0111; // ANDI：与
//                     3'b001: ALUctr = 4'b0001; // SLLI：逻辑左移
//                     3'b101: begin
//                         if(func7 == 7'b0000000)
//                             ALUctr = 4'b0101; // SRLI：逻辑右移（修正：4'b0105→4'b0105是笔误，实际为4'b0101，移除非法字符5）
//                         else if(func7 == 7'b0100000)
//                             ALUctr = 4'b1101; // SRAI：算术右移
//                         else
//                             illegal_instr = 1'b1; // 非法func7，标记非法指令
//                     end
//                     default: illegal_instr = 1'b1; // 非法func3，标记非法指令
//                 endcase
//             end
// 
//             //===========================================================
//             // R-type：算术逻辑指令（ADD/SUB/SLL/SLT/SLTU/XOR/SRL/SRA/OR/AND）
//             //===========================================================
//             7'b0110011: begin
//                 rd       = instr_in[11:7];
//                 rs1      = instr_in[19:15];
//                 rs2      = instr_in[24:20];
//                 RegWr    = 1'b1;    // 允许寄存器写回
//                 ALUBsrc  = 1'b0;    // 选择rs2作为ALU B端口
//                 case(func3)
//                     3'b000: ALUctr = (func7==7'b0100000) ? 4'b1000 : 4'b0000; // SUB（func7=1）/ADD（func7=0）
//                     3'b001: ALUctr = 4'b0001; // SLL：逻辑左移
//                     3'b010: ALUctr = 4'b0010; // SLT：有符号小于则置1
//                     3'b011: ALUctr = 4'b1010; // SLTU：无符号小于则置1
//                     3'b100: ALUctr = 4'b0100; // XOR：异或
//                     3'b101: ALUctr = (func7==7'b0100000) ? 4'b1101 : 4'b0101; // SRA（func7=1）/SRL（func7=0）（修正：4'b0105→4'b0101，移除非法字符5）
//                     3'b110: ALUctr = 4'b0110; // OR：或
//                     3'b111: ALUctr = 4'b0111; // AND：与
//                     default: illegal_instr = 1'b1; // 非法func3，标记非法指令
//                 endcase
//             end
// 
//             //===========================================================
//             // 非法指令（未定义的opcode）
//             //===========================================================
//             default: begin
//                 if(instr_in == 32'h00100073)  // ebreak 指令特殊处理
//                 begin        
//                     $display("Illegal Instruction Detected: instr_in=%h,prepare to exit", instr_in);
//                     illegal_instr = 1'b1; // 标记非法指令
//                 end // 所有输出保持默认值（写x0、无存储操作、无分支），避免副作用
//             end
//         endcase
//     end
// 
// endmodule
