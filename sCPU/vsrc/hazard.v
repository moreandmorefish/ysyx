module hazard(
    input [4:0] srcA_no,
    input [31:0] srcA_data,
    input [4:0] srcB_no,
    input [31:0] srcB_data,
    input [4:0] ex_mem_in_rd,
    input [31:0] ex_mem_in_data,
    input [4:0] ram_in_rd,
    input [31:0] ram_in_data,
    input [4:0] mem_wb_in_rd,
    input [31:0] mem_wb_in_data,
    input [4:0] wb_in_rd,
    input [31:0] wb_in_data,
    output reg [31:0] hazard_out_A,
    output reg [31:0] hazard_out_B
);
    always@(*)begin
        //假设没有冲突
        hazard_out_A = srcA_data;
        hazard_out_B = srcB_data;
        if(srcA_no == ex_mem_in_rd && ex_mem_in_rd !=5'b0) begin
            hazard_out_A = ex_mem_in_data;
        end
        else if(srcB_no == ex_mem_in_rd && ex_mem_in_rd !=5'b0)begin
            hazard_out_B = ex_mem_in_data;
        end
        else if(srcA_no == ram_in_rd && ram_in_rd !=5'b0) begin
            hazard_out_A = ram_in_data;
        end
        else if(srcB_no == ram_in_rd && ram_in_rd !=5'b0) begin
            hazard_out_B = ram_in_data;
        end
        
        else if(srcA_no == mem_wb_in_rd && mem_wb_in_rd !=5'b0) begin
            hazard_out_A = mem_wb_in_data;
        end
        else if(srcB_no == mem_wb_in_rd && mem_wb_in_rd !=5'b0) begin
            hazard_out_B = mem_wb_in_data;
        end
        else if(srcA_no == wb_in_rd && wb_in_rd !=5'b0) begin
            hazard_out_A = wb_in_data;
        end
        else if(srcB_no == wb_in_rd && wb_in_rd !=5'b0) begin
            hazard_out_B = wb_in_data;
        end
    end
endmodule
