module Ram(
    input clk,
    input [31:0] Addr_byte,      // address
    input [2:0]  MemOp,          // memory operation
    input [31:0] data_in,        // data to be written
    input        Wr_en,          // write enable
    output reg [31:0] data_out   // data read out
);
    // Word-aligned addressing
    wire [31:0] Addr = Addr_byte >> 2;
    reg [31:0] data_memory [0:65535];

    // 写操作（上升沿）
    always @(posedge clk) begin
        if (Wr_en) begin
            case (MemOp)
                3'b000: data_memory[Addr][7:0]   <= data_in[7:0];
                3'b001: data_memory[Addr][15:0]  <= data_in[15:0];
                3'b010: data_memory[Addr]        <= data_in;
                default: ;
            endcase
        end
    end

    // 读操作（组合逻辑）
    always @(*) begin
        case (MemOp)
            3'b000: data_out = {{24{data_memory[Addr][7]}},  data_memory[Addr][7:0]};
            3'b001: data_out = {{16{data_memory[Addr][15]}}, data_memory[Addr][15:0]};
            3'b010: data_out = data_memory[Addr];
            3'b100: data_out = {24'b0, data_memory[Addr][7:0]};
            3'b101: data_out = {16'b0, data_memory[Addr][15:0]};
            default: data_out = data_memory[Addr];
        endcase
    end

endmodule
