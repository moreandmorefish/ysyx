module PC(
    input clk, reset,
    input [31:0] next_pc,
    input jump_en,
    input stall,
    output reg [31:0] pc
);
    initial begin
        pc = 32'h80000000;
    end
    always @(posedge clk or posedge reset) begin
        if (reset)
            pc <= 32'h80000000;
        else if (jump_en) begin
            pc <= next_pc;
        end
        else if (stall) begin
            pc <= pc;
        end
        else begin
            pc <= pc+4;
        end
    end
endmodule


