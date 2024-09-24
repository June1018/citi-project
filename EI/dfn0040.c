
/*  @file               dfn0040.pc
*   @file_type          pc source program
*   @brief              온라인 일괄전송 프로그램 (UCS) 
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
#include <sysconst.h>
#include <sytcpgwhead.h>
#include <usrinc/atmi.h>
#include <usrinc/ucs.h>
#include <sqlca.h>
#include <mqi0001f.h>
#include <cmqc.h>
#include <exmq.h>
#include <exmqparm.h>
#include <exmqmsg_001.h>
#include <exmqsvc.h>

#include <exmqmsg_df.h>
#include <unistd.h>
#include <excommon.h>

#include <hcmihead.h>
#include <eierrhead.h>
#include <exmsg11000.h>
#include <dfi0001x.h>
#include <exi0251x.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXMQPARM                (&ctx->exmqparam)
#define MQMSG                   (&ctx->exmqpmsg_001)
#define EXMQMSG_DF              (&ctx->exmqmsg_df)

#define EXMSG11000              (ctx->exmsg11000)
#define LEN_KTIHEAD             300
#define FCY_APPL_CODE           "073"
/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct dfn0040_ctx_s dfn0040_ctx_t;
struct dfn0040_ctx_s {
    exmqparm_t          exmqparm;
    exmqmsg_df_t        exmqmsg_df;
    exmqmsg_001_t       exmqmsg_001;

    commbuff_t   *cb;
    commbuff_t   _cb;

    exmsg11000_t   _exmsg11000;     /* host send 용 structure  */
    exmsg11000_t   *exmsg11000;


    char corr_id            [LEN_HCMIHEAD_QUEUE_NAME    + 1];
    char ext_recv_data      [LEN_EXMSG11000_DETL_AREA   + 1];
    int  ext_recv_len;    
    int  recv_len;
};

/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int a000_intial(int argc , char *argv[]);
static int a100_parse_custom_args(int argc , char *argv[]);

static int get_exmqmsg(dfn0040_ctx_t  *ctx);
static int update_exmqmsg_status(exmqmsg_df_t  *exmqmsg_df, long proc_type);
static int kti_msg_send(dfn0040_ctx_t  *ctx);
/* ----------------------------------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------------------------------- */
int dfn0040(commbuff_t  *commbuff)
{
    int             rc = ERR_NONE;
    int             i;
    char            *cp;
    char            *hp;

    SYS_TRSF;

    SYS_DBG("====================== DFN0040 START ====================================")
    /* MQ Initialization 초기화 처리              */
    SYS_DBG(" call #1 ------------------------------------------------------");
    PRINT_EXMQPARM(g_mqi000f.in.exmqparam);


    SYS_DBG("====================== DFN0040   END ====================================")
    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    SYS_DBG("SYS_CATCH");

    SYS_TREF;
    return ERR_ERR;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int a000_intial(int argc , char *argv[]);
{
    int                 rc  = ERR_NONE;

    SYS_TRSF;

    SYS_TRY(a100_parse_custom_args(argc, argv));
    SYS_TREF;

    return ERR_NONE;
SYS_CATCH:

    SYS_DBG("SYS_CATCH");

    SYS_TREF;
    return ERR_ERR;    
}
/* ----------------------------------------------------------------------------------------------------------- */
static int a100_parse_custom_args(int argc , char *argv[])
{
    int                 c;
    
    g_sleep_sec         = 1;

    memset(g_chnl_code, 0x00, sizeof(g_chnl_code));
    memset(g_appl_code, 0x00, sizeof(g_appl_code));

    while((c = getopt(argc, argv, "s:c:a: ")) != EOF){
        /* ---------------------------------------------------- */
        SYS_DBG("GETOPT: -%c/n",   c);
        /* ---------------------------------------------------- */

        switch(c){
        case 'c':
            strcpy(g_chnl_code, optarg);
            break;

        case 'a':
            strcpy(g_appl_code, optarg);
            break;

        case '?':
            SYS_DBG("unrecognized option : - %c %s ",optopt, argv[optind]);
            return ERR_ERR;
        }
    }

    /* ----------------------------------------------------------------------------- */
    SYS_DBG("g_chnl_code[%s]", g_chnl_code);
    SYS_DBG("g_appl_code[%s]", g_appl_code);
    SYS_DBG("g_sleep_sec[%d]", g_sleep_sec);
    /* ----------------------------------------------------------------------------- */

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
int usermain(int argc, char *argv[])
{
    int                 rc  = ERR_NONE;
    int                 cnt = 0;
    dfn0040_ctx_t       _ctx = {0};
    dfn0040_ctx_t       *ctx = &_ctx;
    exmsg11000_t        exmsg11000;
    
    ctx->cb = &ctx->_cb;
    ctx->exmsg11000 = &ctx->_exmsg11000;
    memset(ctx->exmsg11000, 0x20, sizeof(exmsg11000_t));

    rc = a000_data_init(argc, argv);
    if (rc == ERR_ERR)
        tpsvrdown();

SYS_DBG("=========================== USER MAIN START ================================================= ");

    while(1){
        if (get_exmqmsg(ctx) == ERR_NONE){
            tpschedule(10);     /* 10sec 한번씩 체크    */
        }else if (get_exmqmsg(ctx) == ERR_ERR){
            tpschedule(60);     /* 60sec 한번씩 체크    */
            continue;

            if (g_db_err == 0){
               tpschedule(g_sleep_sec);
            }

            if (cnt++ > 100){
                SYS_DBG("no data sleep");
                cnt = 0;
            }
        }
    }
SYS_DBG("=========================== USER MAIN  END  ================================================= ");

    return ERR_NONE;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int get_exmqmsg(dfn0040_ctx_t  *ctx)
{
    int                 rc  = ERR_NONE;
    int                 rec_cnt  = 0;
    int                 max_size = 0;

    SYS_DB_ERRORCLEAR;
    g_db_err = 0;

    /* DB와 연결이 끊어진 경우 재 연결       */
    rc = db_connect("");
    if (rc == ERR_ERR){
        ex_syslog(LOG_FATAL, "[APPL_DM] %s [FAIL] DB connect failed chnl_code][%s]appl_code[%s]", __FILE__, g_chnl_code, g_appl_code);
        SYS_DBG("sleep 60sec ");
        g_db_err = 1;
        return ERR_ERR;
    }

    EXEC SQL DECLARE MQMSG_CURSOR FOR 
            SELECT PROC_DATE,
                   CHNL_CODE,
                   APPL_CODE,
                   IO_TYPE,
                   MSG_ID,
                   CORR_ID,
                   RSPN_FLAG,
                   ILOG_JRN_NO,
                   MQLEN,
                   MQMSG
            FROM  EXMQMSG_DF
           WHERE  PROC_DATE IN (TO_CHAR(SYSDATE - 1, 'YYYYMMDD'), TO_CHAR(SYSDATE, 'YYYYMMDD'))
             AND  CHNL_CODE = :g_chnl_code
             AND  APPL_CODE = :g_appl_code
             AND  IO_TYPE   = 3     /* default : 3  */
             AND  PROC_TYPE = 0     /* 0: 미처리      */
        ORDER BY PROC_DATE , MSG_ID ;
        
    EXEC SQL OPEN MQMSG_CURSOR;

    if (SYS_DB_CHK_FAIL) {
        ex_syslog(LOG_FATAL, "[APPL_DM] %.7s dfn0040(KTI)-EXMQMSG_DF SELECT ERROR %d [해결방안] Host GW담당자 call "
                             , __FILE__, tperrno);
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s [FAIL]EXMQMSG_DF cusor open failed CHNL_CODE[%s]db_err[%d]",__FILE__,g_chnl_code, db_errno(), db_errstr());
        /* DB장애시 1분 sleep */
        /* NO DATA FOUND 아닌 경우   */
        if (SYS_DB_CHK_FETCHERROR){
            SYS_DBG("sleep(60)");
            //tpschedule(60);
            g_db_err = 1;
        }
        return ERR_ERR;
    }

    while(1){
        /* 에러코드 초기화 */
        sys_err_init();

        memset(EXMQMSGDF, 0x00, sizeof(exmqmsg_df_t));

        EXEC SQL FETCH MQMSG_CURSOR INTO :EXMQMSGDF->proc_date,
                                         :EXMQMSGDF->chnl_code,
                                         :EXMQMSGDF->appl_code,
                                         :EXMQMSGDF->io_type,
                                         :EXMQMSGDF->msg_id,
                                         :EXMQMSGDF->corr_id, 
                                         :EXMQMSGDF->rspn_flag,
                                         :EXMQMSGDF->ilog_jrn_no,
                                         :EXMQMSGDF->mqlen,
                                         :EXMQMSGDF->mqmsg;
        if (SYS_DB_CHK_NOTFOUND){
            break;
        }
        else if (SYS_DB_CHK_FAIL){
            db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);

            ex_syslog(LOG_FATAL, "[APPL_DM] %s :[FAIL]EXMQMSGDF FETCH chnl[%s]err[%d][%s]",__FILE__,g_chnl_code, db_errno(), db_errstr());
            EXEC SQL CLOSE MQMSG_CURSOR;
            SYS_DBG("sleep(60)");
            //tpschedule(60);
            g_db_err = 1;
            return ERR_ERR;
        }

        memcpy(ctx->ext_recv_data,  EXMQMSGDF->mqmsg, EXMQMSGDF->mqlen);
        max_size = 11000;
        SYS_DBG("get exmqmsg : max_size[%d]EXMQMSGDF->mqlen[%d]", max_size, EXMQMSGDF->mqlen);

        if (EXMQMSGDF->mqlen < 0 || EXMQMSGDF->mqlen > max_size ){
            ex_syslog(LOG_ERROR, "[APPL_DM] %s :[FAIL] EXMQMSGDF.ilog_jrn_no[%s]mqlen[%d]is vaild ", __FILE__, EXMQMSGDF->ilog_jrn_no, EXMQMSGDF->mqlen);
            update_exmqmsg_status(EXMQMSGDF, 8L);
            continue;
        }

        memcpy(ctx->corr_id, EXMQMSGDF->corr_id, LEN_HCMIHEAD_QUEUE_NAME);
        ctx->ext_recv_len = EXMQMSGDF->mqlen;
        SYS_DBG("get exmqmsg : ctx->ext_recv_len[%d]", ctx->ext_recv_len);

        /* TPCALL mq 호출  */
        kti_msg_send(ctx);      //KTI mq tpcall

        rec_cnt++;
    }

    EXEC SQL CLOSE MQMSG_CURSOR;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int kti_msg_send(dfn0040_ctx_t  *ctx)
{
    int                 rc  = ERR_NONE;
    int                 len;
    char                data_len[LEN_HCMIHEAD_DATA_LEN];
    char                detl_rec_size[LEN_EXMSG1500_DETL_REC_SIZE];
    char                msg_type[LEN_MSG_TYPE];
    char                proc_code[LEN_KFTC_TX_CODE];

    commbuff_t          dcb;
    exi0251x_t          exi0251x;
    hcmihead_t          hcmihead;
    sysgwinfo_t         *sysgwinfo;
    dfi0001x_t          dfi0001x;

    SYS_TRSF;

    memset(&dfi0001x,   0x20, sizeof(dfi0001x_t));
    memset(&hcmihead,   0x20, sizeof(hcmihead_t));
    memset(data_len,    0x00, sizeof(data_len));
    memset(msg_type,    0x00, sizeof(msg_type));
    memset(proc_code,   0x00, sizeof(proc_code));

    dfi0001x.in.exmsg11000 = EXMSG11000;
    df_proc_exmsg11000_init(&dfi0001x);

    memcpy(&EXMSG11000->detl_area,  ctx->ext_recv_data, ctx->ext_recv_len);
    memcpy(data_len,      &EXMSG11000->detl_area[4] , LEN_HCMIHEAD_DATA_LEN);
    memcpy(detl_rec_size, &EXMSG11000->detl_area[5] , LEN_EXMSG1500_DETL_REC_SIZE);
    
    memcpy(msg_type,      &EXMSG11000->detl_area[12], LEN_MSG_TYPE);
    memcpy(proc_code,     &EXMSG11000->detl_area[16], LEN_KFTC_TX_CODE);
    SYS_DBG("ex_tran_code_convrt : msg_type[%s]proc_code[%s]",msg_type , proc_code);

    memset(&exi0251x, 0x00, sizeof(exi0251x_t));
    exi0251x.in.tx_flag[0] = '7';
    exi0251x.in_conv_flag  = 'K';
    memcpy(exi0251x.in.appl_code   , FCY_APPL_CODE , LEN_APPL_CODE);
    memcpy(exi0251x.in.msg_type    , msg_type      , LEN_MSG_TYPE );
    memcpy(exi0251x.in.kftc_tx_code, proc_code     , LEN_KFTC_TX_CODE);

    exi0251x.in.msg_type_len      = 4;
    exi0251x.in.kftc_tx_code_len  = 6;
    exi0251x.in.ext_recv_data     = EXMSG11000->detl_area;    /* 대외수신데이터  set   */

    rc = ex_tran_code_convrt(&exi0251x);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_tran_code_conv(): "
                             "거래코드 변환 ERROR : tx_code[%s] kftc_tx_code[%s]"
                             "code/msgp[%d][%s]",
                             __FILE__, ctx->msg_type, ctx->kftc_tx_code,
                             sys_error_code(), sys_error_msg());
        return ERR_ERR;
    }
    
    SYS_DBG("[kti_msg_send]hcmihead : tx_code[%s]tx_name[%d]", exi0251x.out.tx_code, exi0251x.out.tx_name);
    memcpy(hcmihead.queue_name, ctx->corr_id        , LEN_HCMIHEAD_QUEUE_NAME);
    memcpy(hcmihead.tx_code   , exi0251x.out.tx_code, LEN_HCMIHEAD_TX_CODE);
    memset(data_len, 0x20, sizeof(data_len));
    sprintf(data_len, "%5d", ctx->ext_recv_len + LEN_KTIHEAD + LEN_EIERRHEAD);
    SYS_DBG("[kti_msg_send]hcmihead : data_len[%s]ext_recv_len[%d]",data_len ,ctx->ext_recv_len);

    memcpy(hcmihead.data_len, data_len,  LEN_HCMIHEAD_DATA_LEN);

#ifdef  _DEBUG
    PRINT_HCMIHEAD(&hcmihead);
#endif

    memcpy(EXMSG11000->tx_code, exi0251x.tx_code, LEN_EXMSG11000_TX_CODE);
    memcpy(&EXMSG11000->detl_area, ctx->ext_recv_data, ctx->ext_recv_len);
    memcpy(dfi0001x.in.exmsg11000->detl_area, EXMSG11000->detl_area, ctx->ext_recv_len);

    rc = sysocbsi(ctx->cb, IDX_TCPHEAD, &hcmihead,  sizeof(hcmihead_t));
    rc = sysocbsi(ctx->cb, IDX_HOSTSENDDATA, dfi0001x.exmsg11000, ctx->ext_recv_len + LEN_KTIHEAD + LEN_EIERRHEAD + LEN_HCMIHEAD);  /* 전제 전송길이 : hcmihead (72) EIERRHEAD (200) KTIHEAD (300) 전문데이터(ext_recv_len) */
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s kti_msg_send(): "
                           , "COMMBUFF(HOSTSENDDATA) set error"
                           ,"[해결방안]일괄전송 담당자 call" ,
                           __FILE__);
        return ERR_ERR;
    }

    memset(&dcb, 0x00, sizeof(commbuff_t));
    rc = sysocbdb(ctx->cb, &dcb);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s dfn0040: kti msg send(dcb)"
                             "COMMBUFF BACK UP ERROR "
                             "[해결방안] TMAX 담당자 call", __FILE__);
        sysocbfb(&dcb);
        return ERR_ERR;
    }

    /* SYMQSEND_DFK 또는 KTI 필요한 값 체크         */
    sysgwinfo = (sysgwinfo_t *) sysocbgp(&dcb, IDX_SYSGWINFO);
    if (sysgwinfo != NULL){
        sysgwinfo->time_val     =  60;
        sysgwinfo->msg_type     = SYSGWINFO_MSG_1500;
        sysgwinfo->call_type    = SYSGWINFO_CALL_TYPE_IR;
        sysgwinfo->rspn_flag    = SYSGWINFO_SVC_REPLY;
    }

    SYS_DBG("HOSTSENDDATA[%d][%.*s]", sysocbgs(ctx->cb, IDX_HOSTSENDDATA), sysocbgs(ctx->cb, IDX_HOSTSENDDATA),  HOSTSENDDATA);

    rc = sys_tpcall("SYMQSEND_DFK", &dcb, TPNOTRAN);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s dfn0040 kti msg send()"
                             "ERROR %d [해결방안] HOST G/W 담당자call",
                             __FILE__, tperrno);
        sysocbfb(&dcb);
        //return ERR_ERR;
    }
    update_exmqmsg_status(EXMQMSGDF, 9L);

    sysocbfb(&dcb);
    
    SYS_TREF;

    return ERR_NONE;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int update_exmqmsg_status(exmqmsg_df_t  *exmqmsg_df, long proc_type);
{

    int                 rc  = ERR_NONE;
    int                 len;
    char                proc_time[10] = {0};

    SYS_TRSF;

    if (proc_type == 9L){
        exmqmsg_df->err_code = 0;
        memset(exmqmsg_df->err_msg, 0x00, sizeof(exmqmsg_df->err_msg));
    }else{
        //ERROR가 아니어도 최종상태의 에러를 MQMSG에 보관하기 위해
        exmqmsg_df->err_code = sys_error_code();
        len = MIN(strlen(sys_error_msg()), LEN_EXMQMSG_DF_ERR_MSG);
        memset(exmqmsg_df->err_msg, 0x00, sizeof(exmqmsg_df->err_msg));
        memcpy(exmqmsg_df->err_msg, sys_error_msg(), len);
    }

    utotime1(proc_time);

    /*
     * TRANSACTION미처리 상태의 경우 proc_time을 그대로 저장 
     */
    utodate1(exmqmsg_df->mq_date);
    utotime1(exmqmsg_df->mq_time);
    exmqmsg_df->proc_type = proc_type;
    EXEC SQL UPDATE EXMQMSG_DF
                SET PROC_TYPE = :exmqmsg_df->proc_type,
                    PROC_TIME = :proc_time,
                    MQ_DATE   = :exmqmsg_df->mq_date,
                    MQ_TIME   = :exmqmsg_df->mq_time,
                    ERR_CODE  = :exmqmsg_df->err_code,
                    ERR_MSG   = :exmqmsg_df->err_msg,
                    ILOG_JRN_NO = :exmqmsg_df->ilog_jrn_no
             WHERE  PROC_DATE = :exmqmsg_df->proc_date
               AND  CHNL_CODE = :exmqmsg_df->chnl_code
               AND  APPL_CODE = :exmqmsg_df->appl_code
               AND  IO_TYPE   = :exmqmsg_df->io_type
               AND  MSG_ID    = :exmqmsg_df->msg_id;

    if (SYS_DB_CHK_FAIL) {
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM]%s MQMS변경 에러 EXMQMSG_DF[%d]", __FILE__, sqlca.sqlcode);
        db_rollback();
        return ERR_ERR;
    }

    db_commit();

    SYS_TREF;
    
    return ERR_NONE;

}
/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */