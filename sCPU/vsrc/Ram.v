module Ram(
    input clk,
    input reset,

    input [31:0] Addr_byte,      // 字节地址
    input [2:0]  MemOp,          // 100=sb,101=sh,110=sw; 000=lb,001=lh,010=lw...
    input [31:0] data_in,        // store 数据
    input        Wr_en,
    input        Rd_en,
    output reg [31:0] data_out,

    // ---- 流水线传递信号 (保持不变) ----
    input [31:0] alu_out_in,
    input [4:0]  rd_in,
    input        reg_wr_in,
    input        memtoreg_in,
    input [31:0] pc_in,
    wire [2:0]  ram_branch,

    output reg [31:0] alu_out_out,
    output reg [4:0]  rd_out,
    output reg        reg_wr_out,
    output reg        memtoreg_out,
    output reg [31:0] pc_out,
    output reg [2:0]  branch_out
);

    import "DPI-C" function int pmem_read(input int raddr);
    import "DPI-C" function void pmem_write(input int waddr, input int wdata, input byte wmask);

    // ---------------------------
    // 辅助逻辑：地址低两位用于对齐处理
    // ---------------------------
    wire [1:0] addr_offset = Addr_byte[1:0];

    // ---------------------------
    // 写操作修正
    // ---------------------------
    always @(posedge clk) begin
        if (Wr_en && !reset) begin
            case (MemOp)
                3'b100: begin // sb
                    // [Fix]: 将常量改为 8'b 以匹配 input byte wmask 宽度，消除 Verilator 警告
                    // 逻辑：将数据移位到对应的字节通道，并生成对应字节的掩码 (1, 2, 4, 8)
                    pmem_write(Addr_byte, data_in << (addr_offset * 8), 8'b0000_0001 << addr_offset);
                    $display("[DPI-WRITE] sb: addr=0x%h, raw_data=0x%h", Addr_byte, data_in);
                end
                3'b101: begin // sh
                    // [Fix]: 将常量改为 8'b 以匹配 input byte wmask 宽度，消除 Verilator 警告
                    // 逻辑：将数据移位到对应的半字通道，并生成对应双字节的掩码 (3, 6, 12)
                    pmem_write(Addr_byte, data_in << (addr_offset * 8), 8'b0000_0011 << addr_offset);
                    $display("[DPI-WRITE] sh: addr=0x%h, raw_data=0x%h", Addr_byte, data_in);
                end
                3'b110: begin // sw
                    // sw 写入完整4字节，掩码为 0x0F
                    pmem_write(Addr_byte, data_in, 8'h0F);
                    $display("[DPI-WRITE] sw: addr=0x%h, raw_data=0x%h", Addr_byte, data_in);
                end
                default: begin
                   // Error handling
                end
            endcase
        end
    end

    // ---------------------------
    // 读操作修正
    // ---------------------------
    always @(posedge clk) begin
        if (Rd_en && !reset) begin
            // 1. 读取完整的 32 位对齐字
            // pmem_read 返回的是该地址所在的 4 字节对齐数据
            automatic logic [31:0] rdata_aligned = pmem_read(Addr_byte);
            
            // 2. 将目标数据移位到最低位 (逻辑右移)
            // 如果地址是 ...01 (offset=1)，我们将数据右移 8 位，使目标字节处于 LSB
            automatic logic [31:0] rdata_shifted = rdata_aligned >> (addr_offset * 8);

            case (MemOp)
                3'b000: begin // lb: 截取低8位 + 符号扩展
                    data_out <= {{24{rdata_shifted[7]}}, rdata_shifted[7:0]};
                    $display("[DPI-READ] lb: addr=0x%h, word=0x%h, val=0x%h", Addr_byte, rdata_aligned, {{24{rdata_shifted[7]}}, rdata_shifted[7:0]});
                end
                3'b001: begin // lh: 截取低16位 + 符号扩展
                    data_out <= {{16{rdata_shifted[15]}}, rdata_shifted[15:0]};
                    $display("[DPI-READ] lh: addr=0x%h, word=0x%h, val=0x%h", Addr_byte, rdata_aligned, {{16{rdata_shifted[15]}}, rdata_shifted[15:0]});
                end
                3'b010: begin // lw: 直接取 (假设是对齐访问)
                    data_out <= rdata_aligned;
                    $display("[DPI-READ] lw: addr=0x%h, val=0x%h", Addr_byte, rdata_aligned);
                end
                3'b011: begin // lbu: 截取低8位 + 零扩展
                    data_out <= {24'b0, rdata_shifted[7:0]};
                    $display("[DPI-READ] lbu: addr=0x%h, val=0x%h", Addr_byte, {24'b0, rdata_shifted[7:0]});
                end
                3'b111: begin // lhu: 截取低16位 + 零扩展
                    data_out <= {16'b0, rdata_shifted[15:0]};
                    $display("[DPI-READ] lhu: addr=0x%h, val=0x%h", Addr_byte, {16'b0, rdata_shifted[15:0]});
                end
                default: data_out <= 32'b0;
            endcase
        end else if (reset) begin
            data_out <= 32'b0;
        end
    end

    // ---------------------------
    // WB 阶段信号传递 (保持不变)
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