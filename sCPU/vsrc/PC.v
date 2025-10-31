module PC(
    input clk, reset,
    input halt,
    input [31:0] next_pc,
    output reg [31:0] pc
);
    always @(posedge clk or posedge reset) begin
        if (reset)
            pc <= 32'h0000_0000;
        else begin
            if (!halt)
                pc <= next_pc;
            else
                pc <= pc;
        end
    end
endmodule


