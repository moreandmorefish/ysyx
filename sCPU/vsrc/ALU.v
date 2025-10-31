module ALU(
  input [31:0] A,  // rs1
  //B在EX阶段选择是rs2还是imm，由ALUBsrc决定，在top.v中已经处理好
  input [31:0] B,  // rs2 or imm based on ALUBsrc
  input [3:0] ALUctr,
  output reg less,  // cmp 1: A < B, 0: A >= B
  output reg zero,  // equal 1: A == B, 0: A != B
  output reg of,    // 溢出标志（有符号）
  output reg cf,    // 进位/借位标志（无符号）
  output reg [31:0] ALUout
);
    always @(*) begin
        // 初始化所有输出，避免生成latch
        ALUout = 32'b0;
        less = 1'b0;
        zero = 1'b0;
        of = 1'b0;
        cf = 1'b0;

        case(ALUctr)
            // 4'b0000: begin // ADD（加法）
            //     {cf, ALUout} = A + B;
            //     of = (A[31] == B[31]) && (ALUout[31] != A[31]);
            // end
            4'b0000: begin // ADD（加法）
                {cf, ALUout} = {1'b0, A} + {1'b0, B};  // 显式扩展为33位
                of = (A[31] == B[31]) && (ALUout[31] != A[31]); // 有符号溢出判断
            end
            // 4'b1000: begin // SUB（减法）
            //     {cf, ALUout} = A + (~B) + 1;
            //     of = (A[31] != B[31]) && (ALUout[31] != A[31]);
            // end
            4'b1000: begin // SUB（减法）
                // 显式扩展为33位，避免位宽警告
                {cf, ALUout} = {1'b0, A} + (~{1'b0, B}) + 1'b1;
                of = (A[31] != B[31]) && (ALUout[31] != A[31]); // 有符号溢出判断
            end
            4'b0001: ALUout = A << B[4:0];                // SLL（逻辑左移）

            4'b0101: ALUout = A >> B[4:0];                // SRL（逻辑右移）

            4'b1101: begin // SRA（算术右移，显式符号扩展）
                ALUout = (A >> B[4:0]) | ({32{A[31]}} << (32 - B[4:0]));
            end

            4'b0100: ALUout = A ^ B;                      // XOR（按位异或）

            4'b0110: ALUout = A | B;                      // OR（按位或）

            4'b0111: ALUout = A & B;                      // AND（按位与）

            4'b0010: begin // SLT（有符号比较）
                ALUout = {31'b0, ($signed(A) < $signed(B))}; // 扩展为32位
                less = ALUout[0]; // 关联less信号
            end

            4'b1010: begin // SLTU（无符号比较）
                ALUout = {31'b0, (A < B)}; // 扩展为32位
                less = ALUout[0]; // 关联less信号
            end

            default: ALUout = 32'b0;                      // 无效控制信号
        endcase

        // 统一判断结果是否为0（覆盖所有操作）
        zero = (ALUout == 0);
    end
endmodule
