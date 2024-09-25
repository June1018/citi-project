
/*  @file               dfo0002f.pc
*   @file_type          pc source program
*   @brief              DFJRN INSERT (생성) UPDATE module (취급/개설)
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
#include <dfi0002f.h>
#include <dfjrn.h>
#include <sqlca.h>
/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define IN                      (dfi0002f->in) 
#define OUT                     (dfi0002f->out) 

/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct dfo0002f_ctx_s  dfo0002f_ctx_t;
struct dfo0002f_ctx_s {
    long dummy;
    char proc_date  [LEN_DFI0002F_PROC_DATE + 1];
};

/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int a000_data_init(dfo0002f_ctx_t  *ctx, dfi0002f_t  *dfi0002f);
static int b000_df_jrn_ins(dfo0002f_ctx_t  *ctx, dfi0002f_t  *dfi0002f);
static int b100_df_jrn_sel(dfo0002f_ctx_t  *ctx, dfi0002f_t  *dfi0002f);
static int c000_df_jrn_upd(dfo0002f_ctx_t  *ctx, dfi0002f_t  *dfi0002f);
static int c100_df_jrn_canc(dfo0002f_ctx_t  *ctx, dfi0002f_t  *dfi0002f);

/* ----------------------------------------------------------------------------------------------------------- */
int b000_df_jrn_insupd(dfi0002f_t   *dfi0002f)
{
    int                 rc = ERR_NONE;
    dfo0002f_ctx_t      _ctx;
    dfo0002f_ctx_t      *ctx = &_ctx;

    SYS_TRSF;

    /* DATA초기회         */
    SYS_TRY(a000_data_init(ctx, dfi0002f));
    /* dfjrn INSERT     */
    SYS_TRY(b000_df_jrn_ins(ctx, dfi0002f));

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return rc;

}

/* ----------------------------------------------------------------------------------------------------------- */
int df_jrn_upd(dfi0002f_t   *dfi0002f)
{
    int                 rc = ERR_NONE;
    dfo0002f_ctx_t      _ctx;
    dfo0002f_ctx_t      *ctx = &_ctx;

    SYS_TRSF;

    SYS_DBG("df_jrn_upd :proc_date[%s]msg_no[%s]corr_id[%s]", dfi0002f->in.proc_date, dfi0002f->in.msg_no, dfi0002f->in.corr_id);
    memcpy(ctx->proc_date,  dfi0002f->in.proc_date,     LEN_DFI0002F_PROC_DATE);
    /* DATA초기회         */
    SYS_TRY(a000_data_init(ctx, dfi0002f));
    /* 이체 재거래 요청시  update dfjrn     */
    SYS_TRY(c000_df_jrn_upd(ctx, dfi0002f));

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return rc;

}

/* ----------------------------------------------------------------------------------------------------------- */
int df_jrn_canc_upd(dfi0002f_t  *dfi0002f)
{

    int                 rc = ERR_NONE;
    dfo0002f_ctx_t      _ctx;
    dfo0002f_ctx_t      *ctx = &_ctx;

    SYS_TRSF;
    SYS_DBG("df_jrn_canc_upd :proc_date[%s]msg_no[%s]corr_id[%s]", dfi0002f->in.proc_date, dfi0002f->in.msg_no, dfi0002f->in.corr_id);
    memcpy(ctx->proc_date,  dfi0002f->in.proc_date,     LEN_DFI0002F_PROC_DATE);
    /* DATA초기회         */
    SYS_TRY(a000_data_init(ctx, dfi0002f));
    /* 취소거래 요청시 update       */
    SYS_TRY(c100_df_jrn_canc(ctx, dfi0002f));

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return rc;

}

/* ----------------------------------------------------------------------------------------------------------- */
int df_jrn_sel(dfi0002f_t  *dfi0002f)
{

    int                 rc = ERR_NONE;
    dfo0002f_ctx_t      _ctx;
    dfo0002f_ctx_t      *ctx = &_ctx;

    SYS_TRSF;

    /* DATA초기회         */
    SYS_TRY(a000_data_init(ctx, dfi0002f));
    /* dfjrn select     */
    SYS_TRY(b100_df_jrn_sel(ctx, dfi0002f));

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return rc;

}

/* ----------------------------------------------------------------------------------------------------------- */
static int a000_data_init(dfo0002f_ctx_t  *ctx, dfi0002f_t  *dfi0002f)
{

    int                 rc = ERR_NONE;

    SYS_TRSF;
    /*-----------------------------------------------------------*/
    /* 변수화 초기화                                                */
    /*-----------------------------------------------------------*/
    memset((char *)ctx, 0x00, sizeof(dfo0002f_ctx_t));

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int b000_df_jrn_ins(dfo0002f_ctx_t  *ctx, dfi0002f_t  *dfi0002f)
{
    int                 rc = ERR_NONE;
    dfjrn_t             dfjrn;

    SYS_TRSF;

    /* 저널초기화   */
    memset(&dfjrn,     0x00, sizeof(dfjrn_t));
    SYS_DBG("df_jrn_insupd: msg_no[%s]", dfi0002f->in.msg_no);
    utodate1(dfjrn.proc_date);
    utotime1(dfjrn.proc_time);
    memcpy(dfjrn.tx_code,       dfi0002f->in.tx_code        , LEN_DFJRN_TX_CODE);
    memcpy(dfjrn.msg_type,      dfi0002f->in.msg_type       , LEN_DFJRN_MSG_TYPE);
    memcpy(dfjrn.proc_code,     dfi0002f->in.proc_code      , LEN_DFJRN_PROC_CODE);
    memcpy(dfjrn.msg_no,        dfi0002f->in.msg_no         , LEN_DFJRN_MSG_NO);
    memcpy(dfjrn.trace_no,      dfi0002f->in.trace_no       , LEN_DFJRN_TRACE_NO);
    memcpy(dfjrn.corr_id,       dfi0002f->in.corr_id        , LEN_DFJRN_CORR_ID);
    dfjrn.io_flag[0]  = dfi0002f->in.io_flag[0];
    dfjrn.kti_flag[0] = dfi0002f->in.kti_flag[0];
    SYS_DBG("dfjrn.msg_type [%s]", dfjrn.msg_type);
    if (memcmp(dfjrn.msg_type, "0400", 4) == 0){
        dfjrn.canc_type[0] = '1';
        memcpy(dfjrn.canc_trace_no , dfi0002f->in.trace_no  , LEN_DFJRN_TRACE_NO);
    }else{
        dfjrn.canc_type[0] = '0';
    }
    /*-----------------------------------------------------------*/
    PRINT_DFJRN(&dfjrn);
    /*-----------------------------------------------------------*/
    EXEC SQL INSERT INTO DFJRN (
                        PROC_DATE       /* [8] 처리일자     */
                      , PROC_TIEM       /* [6] 처리시간     */
                      , TX_CODE         /*[10] 거래코드     */
                      , MSG_TYPE        /* [4] 전문종별코드  */
                      , PROC_CODE       /* [6] 거래구분코드  */
                      , IO_FLAG         /* [1] IN/OUT구분  */
                      , MSG_NO          /*[20] 거래고유번호  */
                      , TRACE_NO        /* [8] 전문추적번호  */
                      , RSPN_CODE       /* [3] 처리결과코드  */
                      , CANC_TYPE       /* [1] 취소여부     */
                      , CANC_RSPN_CODE  /* [3] 취소처리결과코드 */
                      , CANC_TRACE_NO   /* [8] 취소전문추적번호 */
                      , CORR_ID         /*[20] CORR_ID      */
                      , KTI_FLAG        /* [1] KTI FLAG     */
                ) VALUES (
                      :dfjrn.proc_date 
                    , :dfjrn.proc_time 
                    , :dfjrn.tx_code 
                    , :dfjrn.msg_type 
                    , :dfjrn.proc_code
                    , :dfjrn.io_flag 
                    , :dfjrn.msg_no 
                    , :dfjrn.trace_no
                    , :dfjrn.rspn_code 
                    , :dfjrn.canc_type 
                    , :dfjrn.canc_rspn_code 
                    , :dfjrn.canc_trace_no
                    , :dfjrn.corr_id 
                    , :dfjrn.kti_flag
                );

    if (SYS_DB_CHK_SUCCESS) {
        return ERR_NONE;
    }else if (SYS_DB_CHK_DUPKEY){
        SYS_DBG("b000_df_jrn_ins #SYS_DB_CHK_DUPKEY msg_type[%s]", dfjrn.msg_type);
        if (memcmp(dfjrn.msg_type, "0400", 4) == 0) {
            c100_df_jrn_canc_upd(ctx, dfi0002f);
        }else{
            c000_df_jrn_upd(ctx, dfi0002f);
        }
    }else{
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_FATAL, "[APPL_DM] %s DFJRN INSERT %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFJRN INSERT %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "DFJRN INSERT ERROR");
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int b100_df_jrn_sel(dfo0002f_ctx_t  *ctx, dfi0002f_t  *dfi0002f)
{

    int                 rc = ERR_NONE;
    dfjrn_t             dfjrn;
    char                proc_date[LEN_DFJRN_PROC_DATE + 1];
    char                msg_no[LEN_DFJRN_MSG_NO + 1];

    /* DFJRN OUT Data assign        */
    dfjrn = &OUT.dfjrn;

    /* 저널 초기화 */
    memset(dfjrn ,    0x00, sizeof(dfjrn_t));
    memset(proc_date, 0x00, sizeof(proc_date));
    memset(msg_no   , 0x00, sizeof(msg_no));

    memcpy(proc_date, IN.proc_date , LEN_DFJRN_PROC_DATE);
    memcpy(msg_no   , IN.msg_no    , LEN_DFJRN_MSG_NO   );

    /*-----------------------------------------------------------*/
    /* DFJRN SELECT                                              */
    /*-----------------------------------------------------------*/
    EXEC SQL SELECT 
                  PROC_DATE       /* [8] 처리일자     */
                , PROC_TIEM       /* [6] 처리시간     */
                , TX_CODE         /*[10] 거래코드     */
                , MSG_TYPE        /* [4] 전문종별코드  */
                , PROC_CODE       /* [6] 거래구분코드  */
                , IO_FLAG         /* [1] IN/OUT구분  */
                , MSG_NO          /*[20] 거래고유번호  */
                , TRACE_NO        /* [8] 전문추적번호  */
                , RSPN_CODE       /* [3] 처리결과코드  */
                , CANC_TYPE       /* [1] 취소여부     */
                , CANC_RSPN_CODE  /* [3] 취소처리결과코드 */
                , CANC_TRACE_NO   /* [8] 취소전문추적번호 */
                , CORR_ID         /*[20] CORR_ID      */
                , KTI_FLAG        /* [1] KTI FLAG     */
            INTO 
                  :dfjrn.proc_date 
                , :dfjrn.proc_time 
                , :dfjrn.tx_code 
                , :dfjrn.msg_type 
                , :dfjrn.proc_code
                , :dfjrn.io_flag 
                , :dfjrn.msg_no 
                , :dfjrn.trace_no
                , :dfjrn.rspn_code 
                , :dfjrn.canc_type 
                , :dfjrn.canc_rspn_code 
                , :dfjrn.canc_trace_no
                , :dfjrn.corr_id 
                , :dfjrn.kti_flag
          WHERE DFJRN
            AND PROC_DATE       = :proc_date
            AND TRIM(MSG_NO)    = :msg_no 
            AND ROWNUM          = 1;

    /*-----------------------------------------------------------*/
    SYS_DBG("b100 df_jrn_sel : proc_date[%s]msg_no[%s]", proc_date, msg_no);
    PRINT_DFJRN(dfjrn);
    /*-----------------------------------------------------------*/
    if (SYS_DB_CHK_NOTFOUND){
        return ERR_NONE;
    }else if (SYS_DB_CHK_FAIL){
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_FATAL, "[APPL_DM] %s DFJRN SELECT %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFJRN SELECT %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "DFJRN SELECT ERROR");
        return ERR_ERR;

    }else{

        return ERR_NONE;
    }

SYS_CATCH:

    return rc;

}

/* ----------------------------------------------------------------------------------------------------------- */
static int c000_df_jrn_upd(dfo0002f_ctx_t  *ctx, dfi0002f_t  *dfi0002f)
{

    int                 rc = ERR_NONE;

    SYS_TRSF;
    SYS_DBG("c000_df_jrn_upd #1 ======================================= ");
    SYS_DBG("c000_df_jrn_upd : proc_date[%s]", dfi0002f->in.proc_date    );
    SYS_DBG("c000_df_jrn_upd : msg_no   [%s]", dfi0002f->in.msg_no       );
    SYS_DBG("c000_df_jrn_upd #2 ======================================= ");
    SYS_DBG("c000_df_jrn_upd : tx_code  [%s]", dfi0002f->in.tx_code      );
    SYS_DBG("c000_df_jrn_upd : msg_type [%s]", dfi0002f->in.msg_type     );
    SYS_DBG("c000_df_jrn_upd : proc_code[%s]", dfi0002f->in.proc_code    );
    SYS_DBG("c000_df_jrn_upd : io_flag  [%s]", dfi0002f->in.io_flag      );
    SYS_DBG("c000_df_jrn_upd : trace_no [%s]", dfi0002f->in.trace_no     );
    SYS_DBG("c000_df_jrn_upd : rspn_code[%s]", dfi0002f->in.rspn_coe     );
    SYS_DBG("c000_df_jrn_upd : corr_id  [%s]", dfi0002f->in.corr_id      );

    /*-------------------------------------------------------------------*/
    utotime1(dfi0002f->in.proc_time);
    SYS_DBG("c000_df_jrn_upd: proc_time[%s]", dfi0002f->in.proc_time );

    /*-------------------------------------------------------------------*/
    /* DFJRN UPDATE                                                      */
    /*-------------------------------------------------------------------*/
    EXEC SQL UPDATE DFJRN 
                SET IO_FLAG      = :dfi0002f->in.io_flag 
                  , TRACE_NO     = :dfi0002f->in.trace_no 
                  , RSPN_CODE    = :dfi0002f->in.rspn_code 
                  , CORR_ID      = :dfi0002f->in.corr_id
                  , KTI_FLAG     = :dfi0002f->in.kti_flag
                  , PROC_TIME    = :dfi0002f->in.proc_time
              WHERE PROC_DATE    = :dfi0002f->in.proc_date 
                AND MSG_NO       = :dfi0002f->in.msg_no 
            ;
    if (SYS_DB_CHK_FAIL){

        if (SYS_DB_CHK_DUPKEY){
            SYS_DBG("[c000]UPDATE DFJRN DUPKEY: msg_type[%s]", dfi0002f->in.msg_type);
            return ERR_NONE;
        }
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_FATAL, "[APPL_DM] %s DFJRN UPDATE %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFJRN UPDATE %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "DFJRN UPDATE ERROR");
        return ERR_ERR;

    }
    SYS_TREF;

    return ERR_NONE;
    
}
/* ----------------------------------------------------------------------------------------------------------- */
static int c100_df_jrn_canc(dfo0002f_ctx_t  *ctx, dfi0002f_t  *dfi0002f)
{

    int                 rc = ERR_NONE;
    SYS_TRSF;
    SYS_DBG("c100_df_jrn_canc_upd #1 ======================================= ");
    SYS_DBG("c100_df_jrn_canc_upd : proc_date[%s]", dfi0002f->in.proc_date    );
    SYS_DBG("c100_df_jrn_canc_upd : msg_no   [%s]", dfi0002f->in.msg_no       );
    SYS_DBG("c100_df_jrn_canc_upd #2 ======================================= ");
    SYS_DBG("c100_df_jrn_canc_upd :      rspn_code[%s]", dfi0002f->in.rspn_code);
    SYS_DBG("c100_df_jrn_canc_upd :canc_rspn_code [%s]", dfi0002f->in.canc_rspn_code);
    SYS_DBG("c100_df_jrn_canc_upd :  canc_trace_no[%s]", dfi0002f->in.canc_trace_no );
    SYS_DBG("c100_df_jrn_canc_upd :        corr_id[%s]", dfi0002f->in.corr_id      );

    /*-------------------------------------------------------------------*/
    /* DFJRN UPDATE  취소거래요청                                           */
    /*-------------------------------------------------------------------*/
    EXEC SQL UPDATE DFJRN 
                SET CANC_TYPE       = '1'
                  , CANC_RSPN_CODE  = :dfi0002f->in.rspn_code                 
                  , CANC_TRACE_NO   = :dfi0002f->in.trace_no 
                  , CORR_ID         = :dfi0002f->in.corr_id
              WHERE PROC_DATE    = :dfi0002f->in.proc_date 
                AND MSG_NO       = :dfi0002f->in.msg_no 
            ;
    if (SYS_DB_CHK_FAIL){

        if (SYS_DB_CHK_DUPKEY){
            SYS_DBG("[c100]UPDATE DFJRN DUPKEY: msg_type[%s]", dfi0002f->in.msg_type);
            return ERR_NONE;
        }
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_FATAL, "[APPL_DM] %s DFJRN CANC UPDATE %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFJRN CANC UPDATE %d "
                             "[해결방안] 외화자금이체 담당자 call MSG[%s]"
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "DFJRN CANC UPDATE ERROR");
        return ERR_ERR;

    }

    SYS_TREF;

    return ERR_NONE;

}
/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */