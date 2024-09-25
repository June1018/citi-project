
/*  @file               dfo0004.pc
*   @file_type          pc source program
*   @brief              외화자금이체 parsing module 
*   @warn
*   @author
*   @version 
*   @dep_haeder
*   @dep_module
*   @dep_table
*   @dep_infile
*   @dep_outfile
*   @library
*   @usage
*
*
*
*
*
*
*   @generated at 2024/05/29 09:30
*   @history
*
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*
*/

/* ---------------------------------------------- include files ----------------------------------------------- */
#include <syscom.h>
#include <excommon.h>
#include <exmsg11000.h>
#include <sqlca.h>
#include <dfi0004f.h>
#include <dfi0004f.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define IN                      (dfi0004f->in) 
#define OUT                     (dfi0004f->out) 

/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct dfo0004f_ctx_s  dfo0004f_ctx_t;
struct dfo0004f_ctx_s {
    char opp_bank     [LEN_DFI0004F_OPP_BANK +1]
    char rcv_acct_no  [LEN_DFI0004F_ACCT_NO + 1];
    char temp_rcv_acct_no[LEN_DFI0004F_ACCT_NO + 1];
};

/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int a000_data_init(dfi0004f_t  *dfi0004f);
static int c000_data_send_proc(dfi0004f_t  *dfi0004f, dfo0004f_ctx_t  *ctx);
static int c200_isdigit_char(dfi0004f_t  *dfi0004f, dfo0004f_ctx_t  *ctx);

/* ----------------------------------------------------------------------------------------------------------- */
int df_route_proc(dfi0004f_t   *dfi0004f)
{
    int                 rc = ERR_NONE;
    dfo0004f_ctx_t      _ctx;
    dfo0004f_ctx_t      *ctx = &_ctx;

    SYS_TRSF;

    /* DATA초기회         */
    SYS_TRY(a000_data_init(ctx));

    SYS_TRY(c000_data_send_proc(dfi0004f, ctx));

    SYS_TRY(c200_isdigit_char(dfi0004f, ctx));
    
    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return ERR_ERR;

}

/* ----------------------------------------------------------------------------------------------------------- */
static int a000_data_init(dfo0004f_ctx_t  *ctx, dfi0004f_t  *dfi0004f)
{
    int                 rc = ERR_NONE;

    SYS_TRSF;
    /*-----------------------------------------------------------*/
    /* 변수화 초기화                                                */
    /*-----------------------------------------------------------*/
    memset((char *)ctx, 0x00, sizeof(dfo0004f_ctx_t));

    SYS_TREF;

    return ERR_NONE;



}

/* ----------------------------------------------------------------------------------------------------------- */
static int c000_data_send_proc(dfi0004f_t  *dfi0004f, dfo0004f_ctx_t  *ctx)
{
    
    int                 rc = ERR_NONE;
    char                mt103_text  [LEN_DFI0004F_SEND_DATA + 1]; //MT103 swift data 11000 size
    char                temp_buff   [LEN_DFI0004F_SEND_DATA + 1]; //MT103 swift data 11000 size
    char                temp_acct_no[LEN_DFI0004F_ACCT_NO   + 1]; //수취계좌번호 
    char                temp_currency[LEN_DFI0004F_CURRENCY + 1]; //거래통화 
    int                 len;
    int                 i = 0, j = 0;
    int                 chk = 0;
    char                *inpt_stream;
    char                *p;

    SYS_TRSF;

    memset(mt103_text,    0x00, sizeof(mt103_text));
    memset(temp_currency, 0x00, sizeof(temp_currency));
    memset(temp_acct_no , 0x00, sizeof(temp_acct_no))

    memcpy(mt103_text, dfi0004f->in.ext_send_data, LEN_DFI0004F_SEND_DATA);
    inpt_stream = mt103_text;

    for ( i = 0; i <= strlen(mt103_text); i++){
        temp_buff[j++] = inpt_stream[i];

        /* new line check */
        if (inpt_stream[i] == '\n'){
            
            if (memcmp(temp_buff, ":32A:", 5) == 0){
                SYS_DBG("c000_data_send_proc(32A) temp_buff[%s]", temp_buff);
                memcpy(temp_currency, temp_buff+11, LEN_DFI0004F_CURRENCY);     /* 6!3!a15d 결제일(YYYYMMDD),해단통화, 금액     */
                memset(dfi0004f->out.currency, 0x00, sizeof(dfi0004f->out.currency));
                memcpy(dfi0004f->out.currency, temp_currency, LEN_DFI0004F_CURRENCY);
                SYS_DBG("c000_data_send_proc #2 CURRENCY[%s]", dfi0004f->out.currency);
            }

            if (memcmp(temp_buff, ":59:/", 5) == 0){
                memcpy(temp_acct_no, temp_buff+5,   LEN_DFI0004F_ACCT_NO);
                memcpy(ctx->temp_rcv_acct_no, temp_acct_no, LEN_DFI0004F_ACCT_NO);
                SYS_DBG("c000_data_send_proc #2 acct_no[%s]", ctx->temp_rcv_acct_no);
            }

            memset(temp_buff,   0x00, sizeof(temp_buff));
            j++;
        }
    }

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int c200_isdigit_char(dfi0004f_t  *dfi0004f, dfo0004f_ctx_t  *ctx)
{
    char                *str;
    char                buff[20] = {0,};
    int                 i = 0;


    SYS_TRSF;
    
    strcpy(str,  ctx->temp_rcv_acct_no);

    while(*str)
    {
        if (isdigit(*str))
        {
            buff[i++] = *str;
        }
        str++;
    }

    SYS_DBG("c200_isdigit_char: buff[%s]", buff);

    memset(dfi0004f->out.rcv_acct_no, 0x00, LEN_DFI0004F_ACCT_NO);
    memcpy(dfi0004f->out.rcv_acct_no, buff, LEN_DFI0004F_ACCT_NO);

    SYS_DBG("c200_isdigit_char: dfi0004f->out.rcv_acct_no[%s]", dfi0004f->out.rcv_acct_no);

    SYS_TREF;

    return ERR_NONE;
}

/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */