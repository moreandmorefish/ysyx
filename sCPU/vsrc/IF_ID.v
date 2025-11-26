module IF_ID(
    input clk, reset, flush,
    input stall,
    input [31:0] pc_in,
    input [31:0] instr_in,
    output reg [31:0] pc_out,
    output reg [31:0] instr_out
);
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            pc_out <= 0;
            instr_out <= 0;
        end else if (flush) begin
            pc_out <= pc_out;
            instr_out <= 32'h00000013; // NOP 指令
        end else if (stall) begin
            pc_out <= pc_out;
            instr_out <= instr_out;
        end else begin
            pc_out <= pc_in;
            instr_out <= instr_in;
        end
    end
endmodule