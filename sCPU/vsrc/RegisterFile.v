module RegisterFile(    
    input  wire        clk,         // 时钟信号
    input  wire        RegWr,       // 写使能
    input  wire [4:0]  Rw,          // 写地址
    input  wire [31:0] busW,        // 写数据
    output reg [31:0] data_out,       // 当前写回阶段对应指令的 PC 值（从顶层传入）
    input  wire [4:0]  Ra,          // 读地址 A
    input  wire [4:0]  Rb,          // 读地址 B
    input  wire [31:0] pc_WB_in,       // 当前写回阶段对应指令的 PC 值（从顶层传入）
    output reg [31:0] pc_WB_out,
    input instr_valid_in,
    output reg instr_flag,
    output wire [31:0] busA,        // 读数据 A
    output wire [31:0] busB,         // 读数据 B 
    output wire [31:0] a0        // 读数据 A0 (for test
);

    reg [31:0] regfile [0:31];
    // ---------------------------
    // 同步写：WB 阶段（在 posedge clk）
    // ---------------------------
    always @(posedge clk) begin

        if (RegWr && Rw != 5'd0) begin
            regfile[Rw] <= busW;
        end
        data_out <= busW;
        pc_WB_out <= pc_WB_in;
        instr_flag <= instr_valid_in;
        //$display("=== Write: %h to x%0d (PC=%h) ===", busW, Rw, pc_WB_in);
    end

    // ---------------------------
    // 组合读：ID 阶段
    // ---------------------------
    assign busA = (Ra == 5'd0) ? 32'b0 : regfile[Ra];
    assign busB = (Rb == 5'd0) ? 32'b0 : regfile[Rb];
    assign a0   = regfile[10];  // for test

endmodule