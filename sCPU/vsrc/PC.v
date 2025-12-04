module PC(
    input clk, reset,
    input [31:0] next_pc,
    input jump_en,
    input stall,
    output reg [31:0] pc,
    output reg instr_valid
);
    initial begin
        pc = 32'h80000000-32'h4;
    end
    always @(posedge clk or posedge reset) begin
        if (reset)begin
            pc <= 32'h00000000;
            instr_valid <= 1'b1;
        end
        else if (jump_en) begin
            pc <= next_pc;
            instr_valid <= 1'b1;
        end
        else if (stall) begin
            pc <= pc;
            instr_valid <= 1'b1;
        end
        else begin
            pc <= pc+4;
            instr_valid <= 1'b1;
        end
    end
endmodule


