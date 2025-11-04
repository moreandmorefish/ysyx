module PC(
    input clk, reset,
    input halt,
    input [31:0] next_pc,
    input jump_en,
    output reg [31:0] pc
);
    always @(posedge clk or posedge reset) begin
        if (reset)
            pc <= 32'h0000_0000;
        else begin
            if (!halt && jump_en)
                pc <= next_pc;
            else if(!halt && !jump_en)
                pc <= pc+4;
            else pc <= pc;
        end
    end
endmodule


