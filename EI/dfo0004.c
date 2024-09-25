
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
    int                 int  len;




    
    SYS_TRSF;

    utotrim(dfi0004f->in.brn_no);
    /*-----------------------------------------------------------*/
    /* EXBRN    SELECT                                           */
    /*-----------------------------------------------------------*/        
    EXEC SQL SELECT ICG_BRN_FLAG
                INTO :dfi0004f->out.kti_flag
                FROM EXBRN
               WHERE INST_NO  = '027'
                 AND BRN_NO   = :dfi0004f->in.brn_no
                 ;
    if (SYS_DB_CHK_NOTFOUND) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s EXBRN SELECT %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);        
        memcpy(dfi0004f->out.kti_flag,  "0", LEN_DFI0003F_KTI_FLAG);
        return ERR_NONE;
    }else if (SYS_DB_CHK_FAIL){
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s EXBRN SELECT %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);        
        ex_syslog(LOG_FATAL, "[APPL_DM] %s EXBRN SELECT %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);        
        SYS_HSTERR(SYS_NN, SYS_GENERR, "EXBRN SELECT ERROR");
        //return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;


}
/* ----------------------------------------------------------------------------------------------------------- */
static int c200_isdigit_char(dfi0004f_t  *dfi0004f, dfo0004f_ctx_t  *ctx)
{
    int                 rc = ERR_NONE;
    char                ext_send_data[LEN_DFI0004F_SEND_DATA + 1];  /* 대외수신데이터 */

    dfi0004f_t          dfi0004f;

    SYS_TRSF;
    memset(&dfi0004f, 0x00, sizeof(dfi0004f_t));
    memset(ext_send_data, 0x00, sizeof(ext_send_data));

    memcpy(ext_send_data, dfi0004f->in.ext_send_data, LEN_DFI0003F_SEND_DATA);
    memcpy(dfi0004f.in.ext_send_data, ext_send_data , LEN_DFI0003F_SEND_DATA);

    rc = df_swift_parse(&dfi0004f);
    if (rc == ERR_ERR){
        ex_syslog(LOG_FATAL, "[APPL_DM] %s df_swift_parse(dfi0004f) ERR: ext_send_data[%s]", __FILE__, ext_send_data);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "DF_SWIFT_PARSE ERROR");
        //return ERR_ERR;
    }
    memcpy(ctx->currency,    dfi0004f.out.currency   , LEN_DFI0004F_CURRENCY);
    memcpy(ctx->fcy_acct_no ,dfi0004f.out.rcv_acct_no, LEN_DFI0004F_ACCT_NO );
    utotrim(ctx->fcy_acct_no);
    SYS_DBG("[b200]df_swift_parse #2 currency[%s]fcy_acct_no[%s]", ctx->currency, ctx->fcy_acct_no);

    SYS_TREF;

    return ERR_NONE;
}

/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */