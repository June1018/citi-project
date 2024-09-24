
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
static int d000_get_corr_id(dfn0040_ctx_t *ctx)
{
    int                 rc  = ERR_NONE;
    char                corr_id[LEN_HCMIHEAD_QUEUE_NAME + 1], buff[LEN_HCMIHEAD_DATA_LEN + 1];
    hcmihead_t          hcmihead;

    SYS_TRSF;

    /* kti 전송용 corr_id 생성      */
    memset(corr_id, 0x00, sizeof(corr_id));

    utoick(corr_id);

    SYS_DBG("corr_id[%s]", corr_id);
    memcpy(ctx->corr_id, corr_id, LEN_HCMIHEAD_QUEUE_NAME);

    /* init hcmihead    */
    /* MQ에서 TCPHEAD = NULL 방지하기 위해 공백을 초기화  */
    memset(&hcmihead, 0x20, sizeof(hcmihead));

    hcmihead.comm_type = 'A';       /* COMMUNICATION 구분 : Sync, Async     */
    memcpy(hcmihead.queue_name,           corr_id , LEN_HCMIHEAD_QUEUE_NAME);
    memcpy(hcmihead.tx_code   , ctx->host_tx_code , LEN_EXMSG11000_TX_CODE );

#ifdef _DEBUGS
    PRINT_HCMIHEAD(&hcmihead);
#endif

    /* set tcp_head     */
    rc = sysocbsi(ctx->cb, IDX_TCPHEAD, &hcmihead, sizeof(hcmihead_t));

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int e000_msgdf_insert(dfn0040_ctx_t *ctx)
{
    int                 rc  = ERR_NONE;
    char                buff[LEN_SIZE+1];
    exmqmsg_df_t        exmqmsg_df;
    dfi0003f_t          dfi0003f;

    SYS_TRSF;

    /* init  */
    memset(&exmqmsg_df, 0x00, sizeof(exmqmsg_df_t));

    utodate1(exmqmsg_df.proc_date);
    utotime1(exmqmsg_df.proc_time);
    utotime2(exmqmsg_df.sys_mtime);
    utoclck(exmqmsg_df.ilog_jrn_no);

    memcpy(exmqmsg_df.chnl_code , "007", LEN_EXMQMSG_DF_CHNL_CODE);
    memcpy(exmqmsg_df.appl_code , "06" , LEN_EXMQMSG_DF_APPL_CODE); //EXMQPARM의 APPL_CODE 05[KTI->EI]06[EI->KTI]

    exmqmsg_df.io_type = 3;     /* default :3 */
    memcpy(exmqmsg_df.msg_id  , ctx->corr_id, LEN_EXMQMSG_DF_CORR_ID);
    memcpy(exmqmsg_df.corr_id , ctx->corr_id, LEN_EXMQMSG_DF_CORR_ID);

    utodate1(exmqmsg_df.mq_date);
    utotime1(exmqmsg_df.mq_time);

    exmqmsg_df.proc_type = 0; // insert초기값 0 mq put후에 9로 업데이트 
    memcpy(buff, &EXTRECVDATA[4], 5);
    ctx->len = atoi(buff);

    exmqmsg_df.mqlen = ctx->len + LEN_DFI0030_HDR_DATA; /* swift헤더 9자리 THDR12345        */
    memcpy(exmqmsg_df.mqmsg, EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA));

    /* ------------------------------------------------------ */
    PRINT_EXMQMSG_DF(&exmqmsg_df);
    /* ------------------------------------------------------ */

    /* ------------------------------------------------------ */
    /* EXMQMSG_DF INSERT                                      */
    /* ------------------------------------------------------ */
    EXEC SQL INSERT INTO EXMQMSG_DF (
                        PROC_DATE                 /*  [8] 처리일자    */
                      , CHNL_CODE                 /*  [3] 채널코드    */
                      , APPL_CODE                 /*  [2] APPL_CODE */
                      , IO_TYPE                   /*  [1] IO_TYPE   */
                      , MSG_ID                    /* [24]메세지ID    */
                      , CORR_ID                   /* [20]CORR ID   */
                      , PROC_TIME                 /*  [6]처리시간    */
                      , RSPN_FLAG                 /*  [1]처리결과    */
                      , ILOG_JRN_NO               /* [27]ILOGJRNNO */
                      , SYS_MTIME                 /*  [6]SYS시간    */
                      , PROC_TYPE                 /*  [1]처리구분    */
                      , ERR_CODE                  /*  [7]에러코드    */
                      , ERR_MSG                   /*[100]에러메세지   */
                      , MQ_DATE                   /*  [8] MQ일자     */
                      , MQ_TIME                   /*  [8] MQ시간     */
                      , MQLEN                     /*  [5] MQ길이     */
                      , MQMSG                     /* CLOB:MQ메세지   */
                    )VALUES (
                        :exmqmsg_df.proc_date
                       ,:exmqmsg_df.chnl_code
                       ,:exmqmsg_df.appl_code
                       ,:exmqmsg_df.io_type 
                       ,:exmqmsg_df.msg_id
                       ,:exmqmsg_df.corr_id
                       ,:exmqmsg_df.proc_time
                       ,:exmqmsg_df.rspn_flag 
                       ,:exmqmsg_df.ilog_jrn_no
                       ,:exmqmsg_df.sys_mtime
                       ,:exmqmsg_df.proc_type
                       ,:exmqmsg_df.err_code
                       ,:exmqmsg_df.err_msg
                       ,:exmqmsg_df.mq_date
                       ,:exmqmsg_df.mq_time
                       ,:exmqmsg_df.mqlen
                       ,:exmqmsg_df.mqmsg
                    );
    if (SYS_DB_CHK_SUCCESS){
        return ERR_NONE;
    }
    else if (SYS_DB_CHK_DUPKEY){
        return ERR_NONE;
    }else{
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s EXMQMSG_DF(): INSERT [%d]"
                             "해결방안 : 외화자금이체 담당자   call:msg[%s]",
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "EXMQMSG_DF INSERT ERROR");
        return ERR_ERR;
    }

    SYS_TREF;

SYS_CATCH:

    return rc;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int f000_host_msg_send(dfn0040_ctx_t *ctx)
{
    int                 rc  = ERR_NONE;
    char                log_data[5000+1];
    dfi0030x_t          dfi0030x;

    SYS_TRSF;

    /* kftc 보내는 응답 메세지       */
    memset(&dfi0030x, 0x20, sizeof(dfi0030x_t));

    memcpy(dfi0030x.hdr_data        ,&EXTRECVDATA[0]   , LEN_DFI0030_HDR_DATA   );
    memcpy(dfi0030x.system_id       ,&EXTRECVDATA[9]   , LEN_DFI0030_SYSTEM_ID  );
    memcpy(dfi0030x.msg_type        , "0210"           , LEN_DFI0030_MSG_TYPE   );
    memcpy(dfi0030x.proc_code       , ctx->proc_code   , LEN_DFI0030_PROC_CODE  );
    memcpy(dfi0030x.io_flag         , "2"              , LEN_DFI0030_IO_FLAG    );
    memcpy(dfi0030x.status_code     , ctx->status_code , LEN_DFI0030_STATUS_CODE);
    memcpy(dfi0030x.rspn_code       , "000"            , LEN_DFI0030_RSPN_CODE  );
    memcpy(dfi0030x.rspn_bank_code  , "027"            , LEN_DFI0030_RSPN_BANK_CODE);
    utodate1(dfi0030x.proc_date);
    utotime1(dfi0030x.proc_time);
    memcpy(dfi0030x.trace_no        , ctx->trace_no    , LEN_DFI0030_TRACE_NO   );
    memcpy(dfi0030x.recv_filler     , "             "  , LEN_DFI0030_RECV_FILLER);  /* 예비정보 필드 10자리     */
    memcpy(dfi0030x.msg_no          , ctx->msg_no      , LEN_DFI0030_MSG_NO     );
    memcpy(dfi0030x.trd_data_nm     , ctx->trd_data_nm , LEN_DFI0030_TRD_DATA_NM);
    memcpy(dfi0030x.div_tot_cnt     , ctx->div_tot_cnt , LEN_DFI0030_DIV_TOT_CNT);
    memcpy(dfi0030x.div_sr_num      , ctx->div_sr_num  , LEN_DFI0030_DIV_SR_NUM );
    ctx->ext_recv_len = ctx->len + LEN_DFI0030_HDR_DATA;

    memcpy(&EXMSG11000->detl_area, &dfi0030x , ctx->ext_recv_len);

    SYS_DBG("EXTSENDDATA[%d][%.*s]", ctx->ext_recv_len, ctx->ext_recv_len, &EXMSG11000->detl_area);
    rc = sysocbsi(ctx->cb, IDX_EXTSENDDATA, &EXMSG11000->detl_area, ctx->ext_recv_len);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "COMMBUFF SET ERROR");
        return ERR_ERR;
    }

    /* 대외기관 G/W에 호출하는 방식을 전달하기 위한 값 set */
    strcpy(SYSGWINFO->func_name , "TCPDFC");                /* 대외기관 G/W[server]TCPDFS  [client]TCPDFC   */
    SYSGWINFO->time_val     = SYSGWINFO_SAF_DFLT_TIMEOUT;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_SAF;      /* SAF 방식 호출        */
    SYSGWINFO->rspn_flag    = SYSGWINFO_REPLY;              /* G/W 로 부터의 응답    */
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;           /* 1200전문외에는 etc전문구분 0=1200 전문 1:etc 기타 전문  */

    /* 대외기관 데이터 전송     */
    rc = sys_tpcall("SYSEXTGW_DF", ctx->cb, TPNOTRAN);
    if (rc == ERR_ERR) {
        SYS_HSTERR(SYS_LN, 0265200. "GW SYEXTGW_DF TPCALL ERROR");
        ex_syslog(LOG_ERROR, "[APPL_DF] %s dfn0030: m000_ext_msg_send() ERROR %d"
                             "해결방안 대외기관 G/W담당자 call ", __FILE__, tperrno );
        return ERR_ERR;
    }

    rc = z100_log_insert(ctx, (char *)EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), 'I', '4');       /* 개설응답 io_flag: I[개설거래]sr_flag:4[EI->KFTC]  */
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s dfn0030: DFLOG ERROR() [UNIX->HOST]", __FILE__);
    }

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    return ERR_ERR;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int z100_log_insert(dfn0040_ctx_t *ctx, char *log_data, int size, char io_flag, char sr_flag)
{
    int                 rc  = ERR_NONE;
    dfi3100f_t          dfi3100f;
    commbuff_t          dcb;
    char                *hp;

    SYS_TRSF;
    /* ------------------------------------------------------ */
    SYS_DBG("z100_log_insert: len[%d]"      ,size);
    SYS_DBG("z100_log_insert: msg[%s]"      ,log_data );
    SYS_DBG("z100_log_insert: trace_no[%s]" ,ctx->trace_no );
    SYS_DBG("z100_log_insert: msg_no[%s]"   ,ctx->msg_no   );
    /* ------------------------------------------------------ */

    memset(&dfi3100f, 0x00, sizeof(dfi3100f_t));
    dfi3100f.in.io_flag = io_flag;
    dfi3100f.in.sr_flag = sr_flag; 
    dfi3100f.in.log_len = size;
    dfi3100f.in.kti_flag = '1';
    memcpy(dfi3100f.in.msg_no   , ctx->msg_no   , LEN_DFI0003F_MSG_NO   );
    memcpy(dfi3100f.in.trace_no , ctx->trace_no , LEN_DFI0003F_TRACE_NO );
    memcpy(dfi3100f.in.log_data , log_data      , size);

    /* TCP/IP 통신헤더 정보     */
    hp = sysocbgs(ctx->cb, IDX_TCPHEAD);
    if (hp != NULL){
        SYS_DBG("TCP HEAD LOG IS OKAY");
        memcpy(dfi3100f.in.tcp_head,    hp, LEN_TCP_HEAD);
        SYS_DBG("TCP HEAD[%s]", dfi0003f.in.tcp_head);
    }

    SYS_DBG("z100_log_insert: in_tcphead[%d][%.*s]" strlen(dfi3100f.in.tcp_head), strlen(dfi3100f.in.tcp_head),dfi3100f.in.tcp_head);
    memset(&dcb,    0x00, sizeof(commbuff_t));
    rc = sysocbdb(ctx->cb, &dcb);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0030: z100_log_insert() DFLOG sysocbdb ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }

    rc = sysocbsi(&dcb, IDX_EXMSG1200, &dfi3100f sizeof(dfi3100f_t));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0030: z100_log_insert() DFLOG sysocbsi ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        sysocbfb(&dcb);
        return ERR_NONE;
    }

    rc = sys_tpacall("DFN3100F", &dcb, TPNOREPLY | TPNOTRAN);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0030: z100_log_insert() DFLOG sys_tpacall ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }
    
    sysocbfb(&dcb);

    SYS_TREF;

    return ERR_NONE;
}

/* ----------------------------------------------------------------------------------------------------------- */
static int y000_csf_data_insert(dfn0040_ctx_t *ctx)
{
    int                 rc     = ERR_NONE;
    long                urcode = 0;
    mqput_t             mqput  = {0};

    SYS_TRSF;

    memcpy(mqput.chnl_code  , "007", LEN_EXMQMSG_DF_CHNL_CODE);
    memcpy(mqput.appl_code  , "06" , LEN_EXMQMSG_DF_APPL_CODE);

    mqput.data_len  = ctx->ext_recv_len;
    mqput.data      = EXTRECVDATA;

    SYS_DBG("[y000]trace_no[%s]", ctx->trace_no);

    /* csf는 요청 Corr_id 를 고대로 돌려줘야 함     */
    EXEC SQL
        SELECT substr(trim(tcp_head), 1, 24)
          INTO :mqput.corr_id 
          FROM DFLOG 
         WHERE PROC_DATE = to_char(sysdate, 'YYYYMMDD')
           AND IO_FLAG   = 'O'
           AND SR_FLAG   = '1'
           AND TRIM(TRACE_NO) = TRIM(:ctx->trace_no)
           AND ROWNUM    = 1;

    if (SYS_DB_CHK_FAIL){
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s y000_csf_data_insert:원거래없음[%d][%s]", __FILE__, ctx->trace_no, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        SYS_TREF;
        return ERR_ERR;
    }

    SYS_DBG("mqput.corr_id[%s]mqput.data[%s]", mqput.corr_id, mqput.data);

    rc = tdlcall2("libeimqput", "eimqput", &mqput, &urcode, 0);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "tdlcall(eimqput) failed");
        return ERR_ERR;
    }
    if (urcode == ERR_ERR){
        return ERR_ERR;
    }

    rc = z100_log_insert(ctx, EXTRECVDATA, ctx->ext_recv_len, 'O', '4');
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s dfn0030: BB_LOG_ERR() [EI->CSF]", __FILE__);
    }

    SYS_TREF;
    return ERR_NONE;

}
/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */