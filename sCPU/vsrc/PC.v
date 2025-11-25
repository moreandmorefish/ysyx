module PC(
    input clk, reset,
    input halt,
    input [31:0] next_pc,
    input jump_en,
    output reg [31:0] pc
);
    initial begin
        pc = 32'h80000000;
    end
    always @(posedge clk or posedge reset) begin
        if (reset)
            pc <= 32'h80000000;
        else if (!halt) begin
            if (jump_en)
                pc <= next_pc;
            else
                pc <= pc + 4;
        end
    end
endmodule


