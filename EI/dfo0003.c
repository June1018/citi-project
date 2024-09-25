
/*  @file               dfo0003.pc
*   @file_type          pc source program
*   @brief              외화자금이체 routing module 
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
#include <dfi0003f.h>
#include <dfi0004f.h>
#include <exi0321f.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define IN                      (dfi0003f->in) 
#define OUT                     (dfi0003f->out) 

/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct dfo0003f_ctx_s  dfo0003f_ctx_t;
struct dfo0003f_ctx_s {
    exmsg11000_t        _exmsg11000; /* host send 용 structure   */
    exmsg11000_t        *exmsg11000;

    char fcy_acct_no  [LEN_DFI0003F_ACCT_NO + 1];
    char currency     [LEN_DFI0003F_CURRENCY +1];
};

/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int a000_data_init(dfo0003f_ctx_t  *ctx, dfi0003f_t  *dfi0003f);
static int b000_df_exbrn_sel(dfi0003f_t  *dfi0003f);
static int b200_df_swift_parse(dfo0003f_ctx_t  *ctx, dfi0003f_t  *dfi0003f);
static int c000_df_canc_proc(dfi0003f_t  *dfi0003f);

/* ----------------------------------------------------------------------------------------------------------- */
int df_route_proc(dfi0003f_t   *dfi0003f)
{
    int                 rc = ERR_NONE;
    dfo0003f_ctx_t      _ctx;
    dfo0003f_ctx_t      *ctx = &_ctx;
    exi0321f_t          exi0321f;

    SYS_TRSF;

    memset(&exi0321f,   0x00, sizeof(exi0321f_t));

    /*---------------------------------------------------------------*/
    SYS_DBG("INPUT :     proc_date[%s]", dfi0003f->in.proc_date   );
    SYS_DBG("INPUT :      msg_type[%s]", dfi0003f->in.msg_type    );
    SYS_DBG("INPUT :        msg_no[%s]", dfi0003f->in.msg_no      );        
    SYS_DBG("INPUT :     proc_code[%s]", dfi0003f->in.proc_code   );
    SYS_DBG("INPUT :        brn_no[%s]", dfi0003f->in.brn_no      );
    SYS_DBG("INPUT :      trace_no[%s]", dfi0003f->in.trace_no    );
    SYS_DBG("INPUT :acct_tran_code[%s]", dfi0003f->in.acct_tran_code);

    /*---------------------------------------------------------------*/
    /* DATA초기회         */
    SYS_TRY(a000_data_init(ctx, dfi0003f));

    /* 입금거래 처리       */
    memset(msg_type, 0x00, sizeof(msg_type));
    memcpy(msg_type, dfi0003f->in.msg_type, 2);
    SYS_DBG("init msg_type[%s]", msg_type );
    if (memcmp(msg_type, "02", 2) == 0){
        /* 계좌송금 (AN)    */
        if (memcmp(dfi0003f->in.acct_tran_code, "AN", 2) == 0){
            /* 계좌거래 - swift parsing     */
            SYS_TRY(b200_df_swift_parse(ctx, dfi0003f));
            SYS_DBG("[b200]fcy_acct_no[%s]currency[%s]", ctx->fcy_acct_no, ctx->currency );
            /* FOREX21/KTI routing (ex_icg_gcg_acct)    */
            memcpy(exi0321f.in.acct_no,     ctx->fcy_acct_no, LEN_EXI0321F_ACCT_NO);
            memcpy(exi0321f.in.curr_code,   ctx->currency   , LEN_EXI0321F_CURR_CODE);
            ex_icg_gcg_acct(&exi0321f);     /* acct_type(계좌종류 구분필드 )판단 모듈 호출 */
            SYS_DBG("ex_icg_gcg_acct: acct_no[%s]curr_code[%s]acct_type[%d]", exi0321f.in.acct_no, exi0321f.in.curr_code, exi0321f.acct_type);
            //계좌종류 구분 : 4:ICG가상계좌 , 5:ICG계좌 
            if (exi0321f.out.acct_type == 4 || exi0321f.out.acct_type == 5){
                memcpy(dfi0003f->out.kti_flag, "1", LEN_DFI0003F_KTI_FLAG);     //KTI
            }else{
                memcpy(dfi0003f->out.kti_flag, "0", LEN_DFI0003F_KTI_FLAG);     //FOREX21
            }
        }
        /* 무계좌송금 (BN)*/
        else if (memcmp(dfi0003f->in.acct_tran_code, "BN", 2) == 0){
            SYS_TRY(b000_df_exbrn_sel(dfi0003f));   //무계좌거래 점번호 조건으로 EXBRN.icg_brn get
        }
        memcpy(dfi0003f->out.rcv_acct_no, ctx->fcy_acct_no,  LEN_EXI0321F_ACCT_NO);
        memcpy(dfi0003f->out.currency   , ctx->currency   ,  LEN_EXI0321F_CURR_CODE);

    } /* 입금취소 전문처리 */
    else if (memcmp (dfi0003f->in.msg_type, "0400", 4) == 0){
        SYS_TRY(c000_df_canc_proc(dfi0003f));
        SYS_DBG("c000_df_canc_proc : 거래고유번호[%s]",dfi0003f->in.msg_no );
        SYS_DBG("c000_df_canc_proc : kti_flag [%s]",dfi0003f->out.kti_flag );
    }else if (memcmp(dfi0003f->in.msg_type, "0800", 4) == 0){
           SYS_DBG("df_route_proc : 0800 skip ");
    }
    

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return rc;

}

/* ----------------------------------------------------------------------------------------------------------- */
static int a000_data_init(dfo0003f_ctx_t  *ctx, dfi0003f_t  *dfi0003f)
{
    int                 rc = ERR_NONE;

    SYS_TRSF;
    /*-----------------------------------------------------------*/
    /* 변수화 초기화                                                */
    /*-----------------------------------------------------------*/
    memset((char *)ctx, 0x00, sizeof(dfo0003f_ctx_t));

    SYS_TREF;

    return ERR_NONE;



}

/* ----------------------------------------------------------------------------------------------------------- */
static int b000_df_exbrn_sel(dfi0003f_t  *dfi0003f)
{
    SYS_TRSF;

    utotrim(dfi0003f->in.brn_no);
    /*-----------------------------------------------------------*/
    /* EXBRN    SELECT                                           */
    /*-----------------------------------------------------------*/        
    EXEC SQL SELECT ICG_BRN_FLAG
                INTO :dfi0003f->out.kti_flag
                FROM EXBRN
               WHERE INST_NO  = '027'
                 AND BRN_NO   = :dfi0003f->in.brn_no
                 ;
    if (SYS_DB_CHK_NOTFOUND) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s EXBRN SELECT %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);        
        memcpy(dfi0003f->out.kti_flag,  "0", LEN_DFI0003F_KTI_FLAG);
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
static int b200_df_swift_parse(dfo0003f_ctx_t  *ctx, dfi0003f_t  *dfi0003f)
{
    int                 rc = ERR_NONE;
    char                ext_send_data[LEN_DFI0004F_SEND_DATA + 1];  /* 대외수신데이터 */

    dfi0004f_t          dfi0004f;

    SYS_TRSF;
    memset(&dfi0004f, 0x00, sizeof(dfi0004f_t));
    memset(ext_send_data, 0x00, sizeof(ext_send_data));

    memcpy(ext_send_data, dfi0003f->in.ext_send_data, LEN_DFI0003F_SEND_DATA);
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
/* ----------------------------------------------------------------------------------------------------------- */
static int c000_df_canc_proc(dfi0003f_t  *dfi0003f)
{

    SYS_TRSF;

    SYS_DBG("c000_df_canc_proc: 거래고유번호[%s]", dfi0003f->in.msg_no);
    /*-----------------------------------------------------------*/
    /* DFJRN SELECT :거래일자+(#12)거래고유번호 key찾기                */
    /*-----------------------------------------------------------*/
    EXEC SQL SELECT KTI_FLAG
               INTO :dfi0003f->out.kti_flag
               FROM DFJRN
              WHERE PROC_DATE = :dfi0003f->in.proc_date 
                AND MSG_NO    = :dfi0003f->in.msg_no        /* #12:거래고유번호     */
                 ;
    SYS_DBG("c000_df_canc_proc: kti_flag[%c]", dfi0003f->out.kti_flag);

    if (SYS_DB_CHK_NOTFOUND) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFJRN SELECT %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);        
        memcpy(dfi0003f->out.kti_flag,  "0", LEN_DFI0003F_KTI_FLAG);
        return ERR_NONE;
    }else if (SYS_DB_CHK_FAIL){
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFJRN SELECT %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);        
        ex_syslog(LOG_FATAL, "[APPL_DM] %s DFJRN SELECT %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);        
        SYS_HSTERR(SYS_NN, SYS_GENERR, "DFJRN SELECT ERROR");
        //return ERR_ERR;
    }

    SYS_TREF;
}

/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */