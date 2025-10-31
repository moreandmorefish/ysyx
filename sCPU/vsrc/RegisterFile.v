module RegisterFile(    
    input  wire        clk,         // 时钟信号
    input  wire        RegWr,       // 写使能
    input  wire [4:0]  Rw,          // 写地址
    input  wire [31:0] busW,        // 写数据
    input  wire [4:0]  Ra,          // 读地址 A
    input  wire [4:0]  Rb,          // 读地址 B
    output wire [31:0] busA,        // 读数据 A
    output wire [31:0] busB         // 读数据 B
);
    reg [31:0] regfile [0:31];

    // 同步写：WB 阶段
    always @(posedge clk) begin
        if (RegWr && Rw != 5'd0) begin
            regfile[Rw] <= busW;
            $display("Write %h to x%0d", busW, Rw);
        end
    end

    // 组合读：ID 阶段
    assign busA = (Ra == 5'd0) ? 32'b0 : regfile[Ra];
    assign busB = (Rb == 5'd0) ? 32'b0 : regfile[Rb];

endmodule
