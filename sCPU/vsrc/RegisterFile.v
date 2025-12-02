module RegisterFile(    
    input  wire        clk,         // 时钟信号
    input  wire        RegWr,       // 写使能
    input  wire [4:0]  Rw,          // 写地址
    input  wire [31:0] busW,        // 写数据
    output reg [31:0] data_out,       // 当前写回阶段对应指令的 PC 值（从顶层传入）
    input  wire [4:0]  Ra,          // 读地址 A
    input  wire [4:0]  Rb,          // 读地址 B
    input  wire [31:0] pc_WB,       // 当前写回阶段对应指令的 PC 值（从顶层传入）
    output wire [31:0] busA,        // 读数据 A
    output wire [31:0] busB,         // 读数据 B 
    output wire [31:0] a0        // 读数据 A0 (for test
);

    reg [31:0] regfile [0:31];

    // 监测输入信号：每个时钟周期打印 RegWr、Rw、busW、pc_WB 的值
    // always @(posedge clk) begin
    //     $display("t=%0t | Cycle=%0d | RegWr=%b | Rw=x%0d | busW=%h | pc_WB=%h",
    //              $realtime, cycle_count, RegWr, Rw, busW, pc_WB);
    // end

    // ---------------------------
    // 同步写：WB 阶段（在 posedge clk）
    // ---------------------------
    always @(posedge clk) begin

        if (RegWr && Rw != 5'd0) begin
            regfile[Rw] <= busW;
            data_out <= busW;
            $display("=== Write: %h to x%0d (PC=%h) ===", busW, Rw, pc_WB);
        end
        else begin
            data_out <= 32'b0;
        end
        //$display("寄存器状态：x0=0x%h, x1=0x%h, x2=0x%h, x3=0x%h, x4=0x%h",
        //     32'b0,                                  // x0恒为0
        //     regfile[1],                             // x1：直接读regfile[1]
        //     regfile[2],                             // x2：直接读regfile[2]
        //     regfile[3],                             // x3：直接读regfile[3]
        //     regfile[4]);                            // x4：直接读regfile[4]
        //$display(" Cycle=%0d", cycle_count);
    end

    //always @(*) begin
    //    if(Rw == 2)
    //        $display("Write: %h to x%0d (PC=%h)", busW, Rw, pc_WB);
    //end
    // ---------------------------
    // 组合读：ID 阶段
    // ---------------------------
    assign busA = (Ra == 5'd0) ? 32'b0 : regfile[Ra];
    assign busB = (Rb == 5'd0) ? 32'b0 : regfile[Rb];
    assign a0   = regfile[10];  // for test

endmodule