module top(
    input clk,
    input reset,
    input handshake
);

    // =======================================================
    // 全局时钟与 PC / 指令 总线
    // =======================================================
    wire [31:0] pc_current;   // 当前 PC（来自 PC 模块）
    wire [31:0] next_pc;      // 下一条 PC（送回 PC 模块）
    wire [31:0] instr;        // 取出的指令（来自 instr_mem）

    wire clk_cpu = clk;       // 目前直接使用外部 clk，后续可做 halt/gate 处理

    // =======================================================
    // IF → ID：IF_ID 流水寄存器输出（送入 ID 阶段）
    // =======================================================
    wire [31:0] if_id_pc;       // IF_ID 输出的 PC，作为 ID 阶段的 pc_in
    wire [31:0] if_id_instr;    // IF_ID 输出的指令，作为 ID 阶段的 instr_in

    // =======================================================
    // ID 阶段：译码 + 从寄存器堆读数
    // =======================================================
    // 这些是 ID 阶段产生的控制信号和操作数，送往 ID_EX 寄存器

    wire [31:0] id_imm;        // ID 阶段生成的立即数
    wire [31:0] id_busA;       // ID 阶段从寄存器堆读出的 rs1 数据
    wire [31:0] id_busB;       // ID 阶段从寄存器堆读出的 rs2 数据

    wire [3:0]  id_ALUctr;     // ID 阶段生成的 ALU 控制信号
    wire        id_ALUBsrc;    // ALU 第二操作数来源：0=寄存器，1=立即数
    wire        id_RegWr;      // ID 阶段解析得到的寄存器写使能

    wire [2:0]  id_branch;     // 分支/跳转类型（送 EX 的 Branch 单元）
    wire [2:0]  id_MemOp;      // 访存操作类型（byte/half/word 等）

    wire        id_MemWr;      // 存储器写使能
    wire        id_MemRd;      // 存储器读使能
    wire        id_MemtoReg;   // 写回选择：1=从内存，0=从 ALU

    wire [4:0]  id_rs1;        // 源寄存器地址 1
    wire [4:0]  id_rs2;        // 源寄存器地址 2
    wire [4:0]  id_rd;         // 目的寄存器地址

    wire        id_legal_instr;  // ID 阶段给出的“合法/非法指令”标志（原名不改，只是从 id 输出）

    // 寄存器堆中 x10(a0) 的值，用于调试
    wire [31:0] reg_a0;

    // =======================================================
    // ID_EX 流水寄存器输出（送入 EX 阶段）
    // =======================================================
    // 这些信号是从 ID_EX 寄存器出来，作为 EX 阶段的实际输入

    wire [31:0] id_ex_imm;       // 立即数传递到 EX 阶段
    wire [31:0] id_ex_busA;      // rs1 操作数传递到 EX 阶段
    wire [31:0] id_ex_busB;      // rs2 操作数传递到 EX 阶段

    wire [3:0]  id_ex_ALUctr;    // EX 阶段使用的 ALU 控制信号
    wire        id_ex_ALUBsrc;   // EX 阶段使用的第二操作数选择
    wire        id_ex_RegWr;     // EX 阶段之后是否需要写回寄存器

    wire [2:0]  id_ex_branch;    // 分支类型，送 EX 阶段的 Branch 单元
    wire [2:0]  id_ex_MemOp;     // MEM 阶段需要的访存操作类型

    wire        id_ex_MemWr;     // MEM 阶段写使能
    wire        id_ex_MemRd;     // MEM 阶段读使能
    wire        id_ex_MemtoReg;  // MEM/WB 阶段写回来源选择

    wire [4:0]  id_ex_rs1;       // 传给后面做前递/冲突检测（目前暂未使用）
    wire [4:0]  id_ex_rs2;
    wire [4:0]  id_ex_rd;        // 写回目标寄存器号

    wire [31:0] id_ex_pc;        // EX 阶段看到的 PC（用于分支基址、JAL 写回等）

    wire        id_ex_illegal_instr; // 从 ID_EX 传出的“非法指令标志”（用于后续异常处理或统计）
    wire [31:0] id_ex_a0;            // a0 的流水传递（目前只做传递，未使用）

    // =======================================================
    // EX 阶段内部信号
    // =======================================================
    wire [31:0] ex_alu_in2;       // 送入 ALU 的第二操作数（来自 rs2 或 imm）
    wire [31:0] ex_alu_out;       // ALU 计算结果

    wire        ex_less;          // ALU 比较结果：A < B
    wire        ex_zero;          // ALU 比较结果：A == B
    wire        ex_of;            // 溢出标志（overflow）
    wire        ex_cf;            // 进位标志（carry）

    wire [31:0] ex_branch_target; // Branch 单元计算出的分支/跳转目标地址
    wire        ex_jump_en;       // EX 阶段给出的“是否跳转”信号，直接驱动 PC 选择 & 冲刷流水线

    // =======================================================
    // EX_MEM 流水寄存器输出（送入 MEM 阶段）
    // =======================================================
    wire [31:0] ex_mem_alu_out;    // 送入 MEM 阶段的地址/ALU 结果（例如 lw/sw 的地址）
    wire [31:0] ex_mem_rs2_val;    // 送入 MEM 阶段的写数据（来自 rs2）

    wire [2:0]  ex_mem_MemOp;      // MEM 阶段需要的访存类型
    wire        ex_mem_MemWr;      // MEM 阶段写使能
    wire        ex_mem_MemRd;      // MEM 阶段读使能

    wire        ex_mem_RegWr;      // 之后是否需要写回寄存器
    wire        ex_mem_MemtoReg;   // 写回时是否选择内存

    wire [4:0]  ex_mem_rd;         // 目标寄存器号
    wire [31:0] ex_mem_pc;         // 经过 EX_MEM 的 PC（用于调试 & 部分跳转写回）

    // =======================================================
    // MEM 阶段（Ram 模块输出，送入 MEM_WB）
    // =======================================================
    wire [31:0] mem_mem_data;     // 从内存读出的数据（Ram.data_out）
    wire [31:0] mem_alu_out;      // 透传的 ALU 结果（Ram.alu_out_out）
    wire [4:0]  mem_rd;           // 透传的写回寄存器号
    wire        mem_RegWr;        // 透传的写回使能
    wire        mem_MemtoReg;     // 透传的写回来源选择
    wire [31:0] mem_pc;           // 透传的 PC（MEM 阶段看到的）

    // =======================================================
    // MEM_WB 流水寄存器输出（最终 WB 阶段）
    // =======================================================
    wire [31:0] mem_wb_mem_data;   // 送入 WB 阶段的“内存读数据”
    wire [31:0] mem_wb_alu_out;    // 送入 WB 阶段的“ALU 结果”
    wire [4:0]  mem_wb_rd;         // WB 阶段最终要写入的寄存器号
    wire        mem_wb_RegWr;      // WB 阶段是否实际写回寄存器
    wire        mem_wb_MemtoReg;   // WB 阶段最终写回来源：1=mem_data，0=alu_out
    wire [31:0] mem_wb_pc;         // WB 阶段携带的 PC（用于 JAL/JALR 写回）
    wire [2:0]  mem_wb_branch;     // WB 阶段看到的分支类型（用来识别 JAL/JALR）

    // =======================================================
    // 最终写回数据（WB → RegisterFile）
    // =======================================================
    wire [31:0] wb_write_data;     // 选择好来源后的“最终写回寄存器的数据”

    // =======================================================
    // PC 模块
    // =======================================================
    // PC 只负责：在时钟上升沿，根据 next_pc 更新 pc_current
    // jump_en 作为一个辅助信号，在该设计中传入便于 PC 内部后续扩展（例如停止更新等）
    PC my_pc(
        .clk     (clk_cpu),
        .halt    (),            // 目前未使用
        .reset   (reset),
        .next_pc (next_pc),     // 来自 EX/Branch 的下一条 PC 选择
        .pc      (pc_current),  // 当前 PC 输出
        .jump_en (ex_jump_en)   // 直接使用 EX 阶段输出的跳转使能
    );

    // =======================================================
    // 指令存储器：根据 PC 取指
    // =======================================================
    wire [31:0] instr_mem_pc_addr;
    instr_mem my_instr_mem(
        .clk   (clk_cpu),
        .pc    (pc_current),
        .flush (ex_jump_en),
        .instr (instr),
        .pc_addr(instr_mem_pc_addr)
    );

    // =======================================================
    // IF/ID 流水寄存器（EX 决定跳转时需要 flush）
    // =======================================================
    // flush = ex_jump_en 时，IF_ID 会把输出清零（相当于插入一个 bubble）
    IF_ID my_if_id(
        .clk       (clk_cpu),
        .reset     (reset),
        .pc_in     (instr_mem_pc_addr),
        .instr_in  (instr),
        .pc_out    (if_id_pc),       // 送入 ID 的 PC
        .instr_out (if_id_instr),    // 送入 ID 的指令
        .flush     (ex_jump_en)      // EX 决定跳转时清空 IF/ID
    );

    // =======================================================
    // ID 阶段（译码 + 控制信号生成）
    // =======================================================
    id my_decoder(
        .instr_in      (if_id_instr),   // 从 IF_ID 过来的指令
        .imm           (id_imm),        // 生成的立即数
        .ALUctr        (id_ALUctr),     // ALU 控制
        .ALUBsrc       (id_ALUBsrc),    // ALU 第二操作数来源
        .RegWr         (id_RegWr),      // 之后是否写回寄存器
        .branch        (id_branch),     // 分支类型
        .MemOp         (id_MemOp),      // 访存类型
        .MemWr         (id_MemWr),      // 存储器写使能
        .MemRd         (id_MemRd),      // 存储器读使能
        .MemtoReg      (id_MemtoReg),   // 写回是否来自内存
        .rs1           (id_rs1),        // 源寄存器 1
        .rs2           (id_rs2),        // 源寄存器 2
        .rd            (id_rd),         // 目的寄存器
        .illegal_instr (id_legal_instr) // 原信号名保留，语义可由你在 id 内部定义
    );

    // =======================================================
    // 寄存器堆（RegisterFile）
    // =======================================================
    // 写端：来自 WB 阶段（即 MEM_WB 寄存器输出）
    // 读端：使用 ID 阶段解析出的 rs1 / rs2
    RegisterFile my_regfile(
        .clk   (clk_cpu),
        .RegWr (mem_wb_RegWr),      // 最终写回使能
        .Rw    (mem_wb_rd),         // 最终写回寄存器号
        .busW  (wb_write_data),     // 最终写回数据

        .Ra    (id_rs1),            // 读地址 1
        .Rb    (id_rs2),            // 读地址 2

        .pc_WB (mem_wb_pc),         // WB 阶段的 PC（用于调试）
        .busA  (id_busA),           // 读数据 1（送往 ID_EX，再到 EX）
        .busB  (id_busB),           // 读数据 2（送往 ID_EX，再到 EX）
        .a0    (reg_a0)             // 特别输出 x10(a0) 用于调试
    );

    // =======================================================
    // ID_EX 流水寄存器（EX 阶段的所有输入在此锁存）
    // =======================================================
    ID_EX my_id_ex(
        .clk              (clk_cpu),
        .reset            (reset),

        .imm_in           (id_imm),
        .busA_in          (id_busA),
        .busB_in          (id_busB),
        .ALUctr_in        (id_ALUctr),
        .ALUBsrc_in       (id_ALUBsrc),
        .RegWr_in         (id_RegWr),
        .branch_in        (id_branch),
        .MemOp_in         (id_MemOp),
        .MemWr_in         (id_MemWr),
        .MemRd_in         (id_MemRd),
        .MemtoReg_in      (id_MemtoReg),
        .rs1_in           (id_rs1),
        .rs2_in           (id_rs2),
        .rd_in            (id_rd),
        .pc_in            (if_id_pc),

        .imm              (id_ex_imm),
        .busA_out         (id_ex_busA),
        .busB_out         (id_ex_busB),
        .ALUctr           (id_ex_ALUctr),
        .ALUBsrc          (id_ex_ALUBsrc),
        .RegWr            (id_ex_RegWr),
        .branch           (id_ex_branch),
        .MemOp            (id_ex_MemOp),
        .MemWr            (id_ex_MemWr),
        .MemRd            (id_ex_MemRd),
        .MemtoReg         (id_ex_MemtoReg),
        .rs1              (id_ex_rs1),
        .rs2              (id_ex_rs2),
        .rd               (id_ex_rd),
        .pc_to_ex         (id_ex_pc),

        .flush            (ex_jump_en),          // EX 决定跳转时，清空 ID_EX，避免错误执行

        .ie_legal_instr_in(id_legal_instr),      // ID 阶段的非法/合法标志进入 ID_EX
        .ie_illegal_instr (id_ex_illegal_instr), // 从 ID_EX 输出（再送往 EX_MEM）
        .ie_a0_in         (reg_a0),              // a0 的值
        .ie_a0            (id_ex_a0)             // 流水传递 a0（目前未使用）
    );

    // =======================================================
    // EX 阶段
    // =======================================================
    // ALU 第二操作数选择：来自 rs2（id_ex_busB）或 imm（id_ex_imm）
    assign ex_alu_in2 = id_ex_ALUBsrc ? id_ex_imm : id_ex_busB;

    // ---------------------------
    // ALU：算术逻辑单元
    // ---------------------------
    ALU my_alu(
        .A       (id_ex_busA),     // 来自寄存器堆/ID_EX 的第一个操作数
        .B       (ex_alu_in2),     // 第二操作数（寄存器或立即数）
        .ALUctr  (id_ex_ALUctr),   // 控制信号
        .less    (ex_less),
        .zero    (ex_zero),
        .of      (ex_of),
        .cf      (ex_cf),
        .ALUout  (ex_alu_out)      // ALU 运算结果
    );

    // ---------------------------
    // Branch：分支/跳转决策
    // ---------------------------
    Branch my_branch(
        .A             (id_ex_busA),
        .B             (id_ex_busB),
        .branch        (id_ex_branch),      // 分支类型（来自 ID/ID_EX）
        .zero          (ex_zero),           // 来自 ALU 的比较结果
        .less          (ex_less),
        .pc_to_ex      (id_ex_pc),          // 当前 EX 阶段看到的 PC
        .imm           (id_ex_imm),         // 立即数（用于计算目标地址）
        .branch_target (ex_branch_target),  // 计算出的目标 PC
        .jump_en       (ex_jump_en)         // 是否跳转
    );

    // ---------------------------
    // PC 下一跳选择
    // ---------------------------
    // 如果 EX 决定跳转，则 PC = ex_branch_target
    // 否则 PC = PC + 4（顺序执行）
    //assign next_pc = ex_jump_en ? ex_branch_target : (pc_current + 4);
    assign next_pc = ex_branch_target; // 已经在 Branch 模块内处理跳转与顺序执行的选择
    wire [2:0] ex_mem_branch; 
    // =======================================================
    // EX_MEM 流水寄存器（锁存 EX 阶段结果，送入 MEM 阶段）
    // =======================================================
    EX_MEM my_ex_mem(
        .clk                 (clk_cpu),
        .reset               (reset),

        // 输入（来自 EX 阶段）
        .alu_out_in          (ex_alu_out),
        .rs2_val_in          (id_ex_busB),
        .mem_op_in           (id_ex_MemOp),
        .mem_wr_in           (id_ex_MemWr),
        .mem_rd_in           (id_ex_MemRd),
        .reg_wr_in           (id_ex_RegWr),
        .memtoreg_in         (id_ex_MemtoReg),
        .rd_in               (id_ex_rd),
        .branch_in           (id_ex_branch),
        .pc_in               (id_ex_pc),

        // 输出（到 MEM 阶段）
        .alu_out             (ex_mem_alu_out),
        .rs2_val             (ex_mem_rs2_val),
        .mem_op              (ex_mem_MemOp),
        .mem_wr              (ex_mem_MemWr),
        .mem_rd              (ex_mem_MemRd),
        .reg_wr              (ex_mem_RegWr),
        .memtoreg            (ex_mem_MemtoReg),
        .rd                  (ex_mem_rd),
        .branch              (ex_mem_branch),              
        .pc_out              (ex_mem_pc),
        .a0                  (id_ex_a0),      // 复用 id_ex_a0 这根线（原代码就是如此，保持不变）
        .em_illegal_instr_in (id_ex_illegal_instr)
    );

    // =======================================================
    // MEM 阶段：Ram 模块（读写内存 + 携带控制信号）
    // =======================================================
    wire [2:0] ram_branch;
    Ram my_ram(
        .clk          (clk_cpu),
        .reset        (reset),

        .Addr_byte    (ex_mem_alu_out),    // 访存地址（字节地址）
        .MemOp        (ex_mem_MemOp),      // 访存类型
        .data_in      (ex_mem_rs2_val),    // 要写入内存的数据（来自 rs2）
        .Wr_en        (ex_mem_MemWr),      // 写使能
        .Rd_en        (ex_mem_MemRd),      // 读使能

        // 下面这些是为了把控制信号也一并顺着 MEM 阶段往后带
        .alu_out_in   (ex_mem_alu_out),    // EX 的 ALU 结果输入
        .rd_in        (ex_mem_rd),         // 目标寄存器号
        .reg_wr_in    (ex_mem_RegWr),      // 写回使能
        .memtoreg_in  (ex_mem_MemtoReg),   // 写回来源选择
        .pc_in        (ex_mem_pc),         // PC 透传

        // 从 MEM 阶段输出（送入 MEM_WB）
        .data_out     (mem_mem_data),      // 从内存读出的数据
        .alu_out_out  (mem_alu_out),       // 透传的 ALU 结果
        .rd_out       (mem_rd),            // 透传的 rd
        .reg_wr_out   (mem_RegWr),         // 透传的 RegWr
        .memtoreg_out (mem_MemtoReg),      // 透传的 MemtoReg
        .pc_out       (mem_pc),            // 透传的 PC

        .ram_branch   (ex_mem_branch),      // 原代码是 ex_branch，这里对应 id_ex_branch（分支类型）
        .branch_out   (ram_branch)                   // 未使用
    );

    // =======================================================
    // MEM/WB 流水寄存器（锁存 MEM 阶段结果，送入 WB 阶段）
    // =======================================================
    MEM_WB my_mem_wb(
        .clk              (clk_cpu),
        .reset            (reset),

        .mw_mem_data_in   (mem_mem_data),      // MEM 阶段的“内存读数据”
        .mw_alu_out_in    (mem_alu_out),       // MEM 阶段的“ALU 结果”
        .mw_rd_in         (mem_rd),            // MEM 阶段的目标寄存器号
        .mw_reg_wr_in     (mem_RegWr),         // MEM 阶段的写回使能
        .mw_memtoreg_in   (mem_MemtoReg),      // MEM 阶段的写回来源选择
        .mw_pc_in         (mem_pc),            // MEM 阶段携带的 PC

        .mw_mem_data_out  (mem_wb_mem_data),   // WB 阶段看到的“内存读数据”
        .mw_alu_out_out   (mem_wb_alu_out),    // WB 阶段看到的“ALU 结果”
        .mw_rd_out        (mem_wb_rd),         // WB 阶段最终写回寄存器号
        .mw_reg_wr_out    (mem_wb_RegWr),      // WB 阶段最终写回使能
        .mw_memtoreg_out  (mem_wb_MemtoReg),   // WB 阶段最终写回来源选择
        .mw_pc_out        (mem_wb_pc),         // WB 阶段携带 PC

        .mw_branch        (ram_branch),      // 原代码连接 ex_branch，这里对应 id_ex_branch
        .mw_branch_out    (mem_wb_branch)      // WB 阶段看到的分支类型（用于识别 JAL/JALR）
    );

    // =======================================================
    // 写回阶段（WB）：选择写回数据并写入 RegisterFile
    // =======================================================
    // 利用 mem_wb_branch 判断是否是 JAL / JALR：
    //   对于 JAL / JALR：写回 PC+4
    //   对于普通指令：  根据 mem_wb_MemtoReg 决定写回 mem 或 alu
    wire is_jal  = (mem_wb_branch == 3'b001);
    wire is_jalr = (mem_wb_branch == 3'b010);

    assign wb_write_data =
        (is_jal || is_jalr) ? (mem_wb_pc + 32'd4) :
        (mem_wb_MemtoReg     ? mem_wb_mem_data     : mem_wb_alu_out);

    // =======================================================
    // 调试输出（DPI + 波形观察）
    // =======================================================
    import "DPI-C" function int pmem_read(input int raddr);

    always @(posedge clk_cpu) begin
        if (!reset) begin
            $display("=====================================");
            $display("PC=0x%h instr=0x%h jump=%b target=0x%h",
                     pc_current, instr, ex_jump_en, ex_branch_target);

            // ID 阶段
            $display("ID 阶段：rs1=R%0d, rs2=R%0d, rd=R%0d, imm=0x%h",
                     id_rs1, id_rs2, id_rd, id_imm);
            $display("ID 控制：RegWr=%b, MemOp=0x%0h, MemWr=%b, MemRd=%b, MemtoReg=%b, ALUBsrc=%b, branch=0x%0h",
                     id_RegWr, id_MemOp, id_MemWr, id_MemRd, id_MemtoReg, id_ALUBsrc, id_branch);

            // EX 阶段
            $display("EX 阶段：ALUctr=0x%0h, A=0x%h, B=0x%h, ALUout=0x%h, zero=%b, less=%b",
                     id_ex_ALUctr, id_ex_busA, ex_alu_in2, ex_alu_out, ex_zero, ex_less);
            $display("EX 分支：branch=0x%0h, pc_ex=0x%h, imm=0x%h",
                     id_ex_branch, id_ex_pc, id_ex_imm);

            // MEM 阶段
            $display("MEM 阶段：Addr_byte=0x%h, 写数据=0x%h, 读数据=0x%h, Wr_en=%b, Rd_en=%b",
                     ex_mem_alu_out, ex_mem_rs2_val, mem_mem_data, ex_mem_MemWr, ex_mem_MemRd);

            // WB 阶段
            $display("WB 阶段：写回 R%0d, 写回数据=0x%h, RegWr=%b, MemtoReg=%b, branch=0x%0h",
                     mem_wb_rd, wb_write_data, mem_wb_RegWr, mem_wb_MemtoReg, mem_wb_branch);

            // 内存关键地址观察：0x80000020 ~ 0x80000023
            //$display("内存状态：0x20=0x%02h, 0x21=0x%02h, 0x22=0x%02h, 0x23=0x%02h",
            //         8'( pmem_read(32'h80000020)        & 32'hFF),
            //         8'((pmem_read(32'h80000020) >> 8)  & 32'hFF),
            //         8'((pmem_read(32'h80000020) >> 16) & 32'hFF),
            //         8'((pmem_read(32'h80000020) >> 24) & 32'hFF));
        end
    end

    // 单独观察 a0（x10）寄存器的变化
    always @(posedge clk_cpu) begin
        $display("a0=0x%h", reg_a0);
    end

endmodule