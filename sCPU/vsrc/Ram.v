module Ram(
    input clk,
    input reset,

    input [31:0] Addr_byte,      // 字节地址
    input [2:0]  MemOp,          // 100=sb, 101=sh, 110=sw; 000=lb...
    input [31:0] data_in,        // store 数据
    input        Wr_en,
    input        Rd_en,
    output reg [31:0] data_out,

    // ---- 流水线传递信号 ----
    input [31:0] alu_out_in,
    input [4:0]  rd_in,
    input        reg_wr_in,
    input        memtoreg_in,
    input [31:0] pc_in,
    wire [2:0]   ram_branch,

    output reg [31:0] alu_out_out,
    output reg [4:0]  rd_out,
    output reg        reg_wr_out,
    output reg        memtoreg_out,
    output reg [31:0] pc_out,
    output reg [2:0]  branch_out,
    input instr_valid_in,
    output reg instr_valid
);

    import "DPI-C" function int pmem_read(input int raddr);
    import "DPI-C" function void pmem_write(input int waddr, input int wdata, input byte wmask);

    // ---------------------------
    // 地址对齐辅助逻辑
    // ---------------------------
    wire [1:0] addr_offset = Addr_byte[1:0];
    wire is_word_aligned = (addr_offset == 2'b00); // 4字节对齐

    // ---------------------------
    // [Fix] 写操作 (Store)
    // ---------------------------
    always @(posedge clk) begin
        if (Wr_en && !reset) begin
            case (MemOp)
                3'b100: begin // sb: 字节存储
                    // 将数据移位到对应的字节位置，生成单字节掩码
                    pmem_write(Addr_byte, data_in << (addr_offset * 8), 8'b0000_0001 << addr_offset);
                    $display("[DPI-WRITE] sb: addr=0x%h, data=0x%h (byte)", Addr_byte, data_in[7:0]);
                end
                3'b101: begin // sh: 半字存储
                    // [Fix] 增加边界检查：如果地址是 0x...11，则跨越了字边界，当前 DPI 模型不支持自动跨字写入
                    if (addr_offset == 2'b11) begin
                         $display("\033[1;31m[ERROR] Misaligned SH at 0x%h (Cross Page/Word Boundary)\033[0m", Addr_byte);
                    end else begin
                        pmem_write(Addr_byte, data_in << (addr_offset * 8), 8'b0000_0011 << addr_offset);
                        $display("[DPI-WRITE] sh: addr=0x%h, data=0x%h (half)", Addr_byte, data_in[15:0]);
                    end
                end
                3'b110: begin // sw: 字存储
                    if (is_word_aligned) begin
                        pmem_write(Addr_byte, data_in, 8'h0F);
                        $display("[DPI-WRITE] sw: addr=0x%h, data=0x%h (word)", Addr_byte, data_in);
                    end else begin
                        $display("\033[1;31m[ERROR] Address Misaligned: sw access to 0x%h\033[0m", Addr_byte);
                    end
                end
                default: ; // Do nothing
            endcase
        end
    end

    // ---------------------------
    // [Fix] 读操作 (Load)
    // ---------------------------
    always @(posedge clk) begin
        if (Rd_en && !reset) begin
            // 1. 读取对齐的字
            automatic int rdata_aligned = pmem_read(Addr_byte); // 使用 int 匹配 C 返回值
            // 2. 逻辑右移，将目标字节移到低位
            automatic logic [31:0] rdata_shifted = rdata_aligned >> (addr_offset * 8);
            // 3. 临时变量用于计算最终值，确保 $display 打印正确
            automatic logic [31:0] final_val;

            case (MemOp)
                3'b000: final_val = {{24{rdata_shifted[7]}}, rdata_shifted[7:0]};   // lb
                3'b001: final_val = {{16{rdata_shifted[15]}}, rdata_shifted[15:0]}; // lh
                3'b010: begin // lw
                    if (is_word_aligned) final_val = rdata_aligned;
                    else begin
                        $display("\033[1;31m[ERROR] Address Misaligned: lw access to 0x%h\033[0m", Addr_byte);
                        final_val = 32'hDEAD_BEEF; // 错误标记值
                    end
                end
                3'b011: final_val = {24'b0, rdata_shifted[7:0]};  // lbu
                3'b111: final_val = {16'b0, rdata_shifted[15:0]}; // lhu
                default: final_val = 32'b0;
            endcase

            // 更新寄存器
            data_out <= final_val;
            
            // [Fix] 打印 logic 变量 final_val，而不是寄存器 data_out
            $display("[DPI-READ] Op=%b addr=0x%h, raw_word=0x%h, final_val=0x%h", 
                     MemOp, Addr_byte, rdata_aligned, final_val);

        end else if (reset) begin
            data_out <= 32'b0;
        end
    end

    // ---------------------------
    // WB 阶段信号传递
    // ---------------------------
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            alu_out_out   <= 32'b0;
            rd_out        <= 5'b0;
            reg_wr_out    <= 1'b0;
            memtoreg_out  <= 1'b0;
            pc_out        <= 32'b0;
            branch_out    <= 3'b000;
            instr_valid    <= 1'b0;
        end else begin
            alu_out_out   <= alu_out_in;
            rd_out        <= rd_in;
            reg_wr_out    <= reg_wr_in;
            memtoreg_out  <= memtoreg_in;
            pc_out        <= pc_in;
            branch_out    <= ram_branch;
            instr_valid    <= instr_valid_in;
        end
    end

endmodule