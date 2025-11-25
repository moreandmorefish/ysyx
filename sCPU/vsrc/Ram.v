module Ram(
    input clk,
    input reset,

    input [31:0] Addr_byte,      // 输入：字节地址（直接传递给 DPI-C，无需转换）
    input [2:0]  MemOp,          // 修正后的MemOp编码：100=sb,101=sh,110=sw; 000=lb,001=lh,010=lw,011=lbu,111=lhu
    input [31:0] data_in,        // 输入：store 指令要写入的数据
    input        Wr_en,          // 输入：写使能（1=写内存）
    input        Rd_en,          // 输入：读使能（1=读内存）
    output reg [31:0] data_out,  // 输出：load 指令读出的数据

    // ---- 来自 MEM 阶段的输入（传递到 WB 阶段）----
    input [31:0] alu_out_in,     // ALU 运算结果
    input [4:0]  rd_in,          // 写回寄存器号
    input        reg_wr_in,      // 是否写回寄存器
    input        memtoreg_in,    // 写回数据选择信号
    input [31:0] pc_in,          // 对应指令的 PC 值
    wire [2:0]  ram_branch,      // 分支控制信号（未使用，仅传递）

    // ---- 输出到 WB 阶段 ----
    output reg [31:0] alu_out_out,
    output reg [4:0]  rd_out,
    output reg        reg_wr_out,
    output reg        memtoreg_out,
    output reg [31:0] pc_out,      // 写回阶段的 PC 值
    output reg [2:0]  branch_out   // 分支控制信号（未使用，仅传递）
);

    // DPI-C 函数声明（与你的 C 代码完全匹配：接收字节地址）
    import "DPI-C" function int pmem_read(input int raddr);  // raddr：字节地址
    import "DPI-C" function void pmem_write(input int waddr, input int wdata, input byte wmask);  // waddr：字节地址

    // ---------------------------
    // 写操作（时序逻辑）：修正MemOp编码，确保sw时pmem_write被调用
    // ---------------------------
    always @(posedge clk) begin
        if (Wr_en && !reset) begin  // 写使能有效且未复位
            case (MemOp)
                3'b100: begin  // sb：写 1 字节（修正后MemOp=100）
                    pmem_write(Addr_byte, data_in, 8'h01);  // 掩码 0x01：只写第 0 字节
                    $display("[DPI-WRITE] sb: addr=0x%h, data=0x%h, mask=0x01", Addr_byte, data_in);
                end
                3'b101: begin  // sh：写 2 字节（修正后MemOp=101）
                    pmem_write(Addr_byte, data_in, 8'h03);  // 掩码 0x03：写第 0+1 字节
                    $display("[DPI-WRITE] sh: addr=0x%h, data=0x%h, mask=0x03", Addr_byte, data_in);
                end
                3'b110: begin  // sw：写 4 字节（修正后MemOp=110）
                    pmem_write(Addr_byte, data_in, 8'h0F);  // 掩码 0x0F：写第 0+1+2+3 字节
                    $display("[DPI-WRITE] sw: addr=0x%h, data=0x%h, mask=0x0F", Addr_byte, data_in);
                end
                default: begin
                    $display("[DPI-WRITE] Error: invalid MemOp=0x%h", MemOp);
                end
            endcase
        end
    end

    // ---------------------------
    // 读操作（时序逻辑）：保持不变，只修正MemOp注释
    // ---------------------------
    always @(posedge clk) begin
        if (Rd_en && !reset) begin  // 读使能有效且未复位
            case (MemOp)
                3'b000: begin  // lb：读 1 字节 + 符号扩展
                    logic [7:0] byte_data;
                    byte_data <= 8'(pmem_read(Addr_byte) & 32'h000000FF);
                    data_out <= 32'({{24{byte_data[7]}}, byte_data});
                    $display("[DPI-READ] lb: addr=0x%h, data=0x%h", Addr_byte, data_out);
                end
                3'b001: begin  // lh：读 2 字节 + 符号扩展
                    logic [15:0] halfword_data;
                    halfword_data <= 16'(pmem_read(Addr_byte) & 32'h0000FFFF);
                    data_out <= 32'({{16{halfword_data[15]}}, halfword_data});
                    $display("[DPI-READ] lh: addr=0x%h, data=0x%h", Addr_byte, data_out);
                end
                3'b010: begin  // lw：读 4 字节
                    data_out <= 32'(pmem_read(Addr_byte));
                    $display("[DPI-READ] lw: addr=0x%h, data=0x%h", Addr_byte, data_out);
                end
                3'b011: begin  // lbu：读 1 字节 + 零扩展
                    data_out <= 32'({24'h000000, 8'(pmem_read(Addr_byte) & 32'h000000FF)});
                    $display("[DPI-READ] lbu: addr=0x%h, data=0x%h", Addr_byte, data_out);
                end
                3'b111: begin  // lhu：读 2 字节 + 零扩展
                    data_out <= 32'({16'h0000, 16'(pmem_read(Addr_byte) & 32'h0000FFFF)});
                    $display("[DPI-READ] lhu: addr=0x%h, data=0x%h", Addr_byte, data_out);
                end
                default: begin
                    data_out <= 32'b0;
                    $display("[DPI-READ] Error: invalid MemOp=0x%h", MemOp);
                end
            endcase
        end else if (reset) begin
            data_out <= 32'b0;
        end else begin
            data_out <= data_out;
        end
    end

    // ---------------------------
    // WB 阶段信号传递（保持不变）
    // ---------------------------
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            alu_out_out   <= 32'b0;
            rd_out        <= 5'b0;
            reg_wr_out    <= 1'b0;
            memtoreg_out  <= 1'b0;
            pc_out        <= 32'b0;
            branch_out    <= 3'b000;
        end else begin
            alu_out_out   <= alu_out_in;
            rd_out        <= rd_in;
            reg_wr_out    <= reg_wr_in;
            memtoreg_out  <= memtoreg_in;
            pc_out        <= pc_in;
            branch_out    <= ram_branch;
        end
    end

endmodule