
//`#start header` -- edit after this line, do not edit this line
// ========================================
//
// Proper 8+2-bit DitherPWM, derived from PWM component v3.30
// Werner Johansson, wj@unifiedengineering.se
//
// ========================================
`include "cypress.v"
//`#end` -- edit above this line, do not edit this line
// Generated on 01/29/2016 at 19:06
// Component: DitherPWM
module DitherPWM (
	output  pwm,
	input   clock,
	input   reset
);

//`#start body` -- edit after this line, do not edit this line

    /**************************************************************************/
    /* Parameters                                                             */
    /**************************************************************************/
    /* localparams */

    localparam PWM_DITHER_OFFSET0  = 2'd0;
    localparam PWM_DITHER_OFFSET25 = 2'd1;
    localparam PWM_DITHER_OFFSET50 = 2'd2;
    localparam PWM_DITHER_OFFSET75 = 2'd3;

    /* Control Register Bits*/
    localparam PWM_CTRL_ENABLE      = 8'h7; /* Enable the PWM                 */
    localparam PWM_CTRL_DITHER_1    = 8'h1; /* Dither bit 1                   */
    localparam PWM_CTRL_DITHER_0    = 8'h0; /* Dither bit 0                   */

    /* Datapath Implementation */
    localparam RESET_PERIOD_SHIFT_OP    = `CS_ALU_OP_PASS;
    localparam RESET_PERIOD_SRC_B       = `CS_SRCB_D0;
    localparam RESET_PERIOD_A0_SRC      = `CS_A0_SRC___F0;

    parameter dpconfig0 = {
        `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
        `CS_SHFT_OP_PASS, `CS_A0_SRC__ALU, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM0:    Idle */
        RESET_PERIOD_SHIFT_OP, `CS_SRCA_A0, RESET_PERIOD_SRC_B,
        `CS_SHFT_OP_PASS, RESET_PERIOD_A0_SRC, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM1:    Reset Period */
        `CS_ALU_OP__DEC, `CS_SRCA_A0, `CS_SRCB_D0,
        `CS_SHFT_OP_PASS, `CS_A0_SRC__ALU, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM2:    Dec A0 (A0 = A0 - 1) */
        RESET_PERIOD_SHIFT_OP, `CS_SRCA_A0, RESET_PERIOD_SRC_B,
        `CS_SHFT_OP_PASS, RESET_PERIOD_A0_SRC, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM3:    Reset Period */
        RESET_PERIOD_SHIFT_OP, `CS_SRCA_A0, RESET_PERIOD_SRC_B,
        `CS_SHFT_OP_PASS, RESET_PERIOD_A0_SRC, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM4:    Reset Period*/
        RESET_PERIOD_SHIFT_OP, `CS_SRCA_A0, RESET_PERIOD_SRC_B,
        `CS_SHFT_OP_PASS, RESET_PERIOD_A0_SRC, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM5:    Reset Period */
        RESET_PERIOD_SHIFT_OP, `CS_SRCA_A0, RESET_PERIOD_SRC_B,
        `CS_SHFT_OP_PASS, RESET_PERIOD_A0_SRC, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM6:    Reset Period */
        RESET_PERIOD_SHIFT_OP, `CS_SRCA_A0, RESET_PERIOD_SRC_B,
        `CS_SHFT_OP_PASS, RESET_PERIOD_A0_SRC, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM7:    Reset Period */
          8'hFF, 8'h00, /*CFG9:     */
          8'hFF, 8'hFF, /*CFG11-10:     */
        `SC_CMPB_A0_D1, `SC_CMPA_A0_D1, `SC_CI_B_ARITH,
        `SC_CI_A_ARITH, `SC_C1_MASK_DSBL, `SC_C0_MASK_DSBL,
        `SC_A_MASK_DSBL, `SC_DEF_SI_0, `SC_SI_B_DEFSI,
        `SC_SI_A_DEFSI, /*CFG13-12:     */
        `SC_A0_SRC_ACC, `SC_SHIFT_SL, 1'h0,
        1'h0, `SC_FIFO1__A0, `SC_FIFO0_BUS,
        `SC_MSB_DSBL, `SC_MSB_BIT0, `SC_MSB_NOCHN,
        `SC_FB_NOCHN, `SC_CMP1_NOCHN,
        `SC_CMP0_NOCHN, /*CFG15-14:     */
         10'h00, `SC_FIFO_CLK__DP,`SC_FIFO_CAP_FX,
        `SC_FIFO__EDGE,`SC_FIFO__SYNC,`SC_EXTCRC_DSBL,
        `SC_WRK16CAT_DSBL /*CFG17-16:     */
    };

    /* Internal signals */
    wire       cmp1_eq;         /* A0 is equal to compare value 1         */
    wire       cmp1_less;       /* A0 is less than to compare value 1     */

    /* Clock Enable block signal */
    wire       ClockOutFromEnBlock;

    /* wires for output registering logic */
    wire       tc_i;
    reg        tc_i_reg;
    wire       pwm_i;
    reg        pwm_i_reg;

    /* Control Register Output        */
    wire [7:0] control;

    /* Control Signals */
    wire       ctrl_enable;
    wire [1:0] dithOff;

    /* Triggers */
    wire       final_enable;

    /* Dither Impl */
    wire       dith_sel;
    reg [1:0]  dith_count;

    /* PWM output(s) */
    wire       pwm_temp;

    /* Run Mode */
    reg        runmode_enable;

    /* Datapath Implementation */
    wire [2:0] cs_addr;

    /* Clock Enable Block Component instance */
    cy_psoc3_udb_clock_enable_v1_0 #(.sync_mode(`TRUE))
    clock_enable_block (
                        /* output */.clock_out(ClockOutFromEnBlock),
                        /* input */ .clock_in(clock),
                        /* input */ .enable(1'b1)
                        );


    /**************************************************************************/
    /* Control Register Implementation                                        */
    /**************************************************************************/
    generate
        cy_psoc3_control #(.cy_force_order(`TRUE), .cy_ctrl_mode_1(8'h00), .cy_ctrl_mode_0(8'hFF))
        ctrlreg(
            /* input          */ .clock(ClockOutFromEnBlock),
            /* output [07:00] */ .control(control)
        );

        assign ctrl_enable    = control[PWM_CTRL_ENABLE];
        assign dithOff        = {control[PWM_CTRL_DITHER_1], control[PWM_CTRL_DITHER_0]};
    endgenerate
    

    /**************************************************************************/
    /* Run Mode Block Implementations                                         */
    /**************************************************************************/
    generate
        always @(posedge ClockOutFromEnBlock or posedge reset)
        begin
            if(reset)
                runmode_enable <= 1'b0;
            else
                runmode_enable <= ctrl_enable;
        end

        assign final_enable = runmode_enable;
    endgenerate


    /**************************************************************************/
    /* Dither implementation                                                  */
    /**************************************************************************/
    always @(posedge ClockOutFromEnBlock or posedge reset)
    begin
        if(reset)
        begin
            dith_count <= 0;
        end
        else
        begin
            if(tc_i)
            begin
                dith_count <= dith_count + 1;
            end
        end
    end

    /* SUJA modified */
    assign dith_sel = (dithOff[1:0] == PWM_DITHER_OFFSET0) ? 1'b0 :
                      (dithOff[1:0] == PWM_DITHER_OFFSET25) ? ((dith_count == 0) ? 1'b1 : 1'b0) :
                      (dithOff[1:0] == PWM_DITHER_OFFSET50) ? ((dith_count == 0 || dith_count == 2) ? 1'b1 : 1'b0) :
                      /*(DitherOffset == PWM_DITHER_OFFSET75) ? */((dith_count == 3) ? 1'b0 : 1'b1) ;



    /**************************************************************************/
    /* Datapath Implementation                                                */
    /**************************************************************************/
    generate
        assign cs_addr = {tc_i,final_enable,reset};
    endgenerate

    generate
            cy_psoc3_dp8 #(.cy_dpconfig_a(dpconfig0)) pwmdp(
            /*  input                   */  .clk(ClockOutFromEnBlock),
            /*  input   [02:00]         */  .cs_addr(cs_addr),
            /*  input                   */  .route_si(1'b0),
            /*  input                   */  .route_ci(1'b0),
            /*  input                   */  .f0_load(1'b0),
            /*  input                   */  .f1_load(1'b0),
            /*  input                   */  .d0_load(1'b0),
            /*  input                   */  .d1_load(1'b0),
            /*  output                  */  .ce0(cmp1_eq),     /* Compare1 ( A0 == D0 )*/
            /*  output                  */  .cl0(cmp1_less),   /* Compare1 ( A0 < D0 ) */
            /*  output                  */  .z0(tc_i),           /* tc ( A0 == 0 )       */
            /*  output                  */  .ff0(),
            /*  output                  */  .ce1(),
            /*  output                  */  .cl1(),
            /*  output                  */  .z1(),
            /*  output                  */  .ff1(),
            /*  output                  */  .ov_msb(),
            /*  output                  */  .co_msb(),
            /*  output                  */  .cmsb(),
            /*  output                  */  .so(),
            /*  output                  */  .f0_bus_stat(),
            /*  output                  */  .f0_blk_stat(),
            /* TODO: Can't use this because we exceed the number of outputs allowed from one Datapath. */
            /*  output                  */  .f1_bus_stat(),
            /*  output                  */  .f1_blk_stat()
            );
    endgenerate


    /**************************************************************************/
    /* Pulse Width output(s) implementation                                   */
    /**************************************************************************/

    /* Register the pwm output to avoid glitches on the output terminal*/
    always @(posedge ClockOutFromEnBlock)
    begin
        pwm_i_reg <= pwm_i ;

        /* Register the tc to avoid glitches on the output terminal */
        tc_i_reg <= tc_i & final_enable;
    end

    assign pwm = pwm_i_reg;
    assign pwm_i = pwm_temp & final_enable;

    assign pwm_temp = (!dith_sel) ? cmp1_less : (cmp1_less | cmp1_eq);
    
//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
