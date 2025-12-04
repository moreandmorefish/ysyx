module IF_ID(
    input clk, reset, flush,
    input stall,
    input [31:0] pc_in,
    input [31:0] instr_in,
    output reg [31:0] pc_out,
    output reg [31:0] instr_out,
    input instr_valid_in,
    output reg instr_valid
);
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            pc_out <= 0;
            instr_out <= 0;
            instr_valid <= 1'b0;
        end else if (flush) begin
            instr_out <= 32'h00000013; // NOP 指令
            instr_valid <= 1'b0;
        end else if (stall) begin
            pc_out <= pc_out;
            instr_out <= instr_out;
            instr_valid <= 1'b1;
        end else begin
            pc_out <= pc_in;
            instr_out <= instr_in;
            instr_valid <= instr_valid_in;
        end
    end
endmodule