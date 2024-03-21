/*  @file               symqtop_onin.pc                                             
*   @file_type          onine program
*   @brief              server shell program
*   @warn
*   @author
*   @version 
*
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
*   @generated at 2023/05/22 09:30
*   @history
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*
*/
/* -------------------------- include files  -------------------------------------- */
#include <sytcpgwhead.h>
#include <syscom.h>
#include <mqi00001f.h>
#include <cmqc.h>
#include <exmq.h>
#include <exmqparm.h>
#include <exmqpmsg_001.h>
#include <exmqsvcc.h>
#include <mqimsg001.h>
#include <usrinc/atmi.h>
#include <usrinc/tx.h>
#include <sqlca.h>
#include <sessionlist.h>
#include <exi0280.h>
#include <exi0212.h>
/* --------------------------constant, macro definitions -------------------------- */
#define EXCEPTION_SLEEP_INTV        300

#define EXMQPARM                    (&ctx->exmqparm)
#define MQMSG001                    (&ctx->exmqmsg_001)
#define MQ_INFO                     (&ctx->mq_info)

#define REQ_DATA                    (SESSION_DATA->req_data)    //ctx->session->req_data
#define REQ_INDATA                  (REQ_DATA->indata)
#define REQ_HCMIHEAD                (&REQ_DATA->hcmihead)

#define LEN_TCP_HEAD                72
#define LEN_COMMON_HEAD             300
#define LEN_MQ_CHNL_TYPE            2
/* --------------------------structure definitions -------------------------------- */
typedef struct session_data_s session_data_t;
struct session_data_s {
    CTX_T               ctxt;
    commbuff_t          cb;   
    char                *req_data;
    long                req_len;
    char                *as_data;
};

/* 요청전문 : HCMIHEAD(72) + INDATA(XX) */
typedef struct res_data_s res_data_t;
struct req_data_s {
    hcmihead_t          hcmihead;  
    char                indata[1];
};

/* 요청전문 : HCMIHEAD(72) + OUTDATA(XX) */
typedef struct res_data_s res_data_t;
struct req_data_s {
    hcmihead_t          hcmihead;  
    char                outdata[1];
};

typedef struct symqtop_onin_ctx_s symqtop_onin_ctx_t; 
struct symqtop_onin_ctx_s {
    exmqparm_t          exmqparm;
    exmqmsg_001_t       exmqmsg_001;
    commbuff_t          *cb;
    commbuff_t          _cb;   
    session_t           *session;
    mq_info_t           mqinfo;
    long                is_mq_connected;
    MQHCONN             mqhconn; 
    MQHOBJ              mqhobj;
    char                ilog_jrn_no[LEN_JRN_NO+1];          /* image LOG JRN_NO */
    char                jrn_no[LEN_JRN_NO+1];               /* 27자리            */
    char                corr_id[LEN_EXMQMSG_001_CORR_ID+1]; /* 24자리    */
    char                log_flag;
};

typedef struct symqtop_onin_global_s symqtop_onin_global_t; 
struct symqtop_onin_global_s {
    char                mq_chnl_id[LEN_MQ_CHNL_TYPE+1]; /* MQ CHNL D            */
    long                is_mq_connected;
    MQHCONN             mqhconn;
    MQHOBJ              mqhobj;
    long                session_num;
    long                session_tmo;
    long                min_req_len;
};

/* --------------exported global variables declarations ---------------------------*/
    char                g_svc_name[32];                 /* G/W 서비스명          */
    char                g_chnl_code[3 + 2];             /* CHANNEL CODE        */
    char                g_appl_code[2 + 1];             /* APPL_CODE           */
    int                 g_session_num = 100;            /* 최대 session 수      */
    int                 g_session_tmo = 10;             /* default time out   */
    int                 g_call_seq_no;                  /* 일련번호 채번         */

    int                 g_temp_len;                     /* 임시 사용 buffer에 저장한 데이터 길이  */
    char                g_temp_buf[8192];               /* 임시 사용 buffer */
    int                 g_head_len;                     /* 통신헤더 길이 */

symqtop_onin_ctx_t      _ctx; 
symqtop_onin_ctx_t      *ctx = &_ctx;

symqtop_onin_global_t   g = {0};

/* ----------------------- exported function declarations ------------------------- */
int                     symqtop_onin(commbuff_t *commbuff);
int                     symqtop_oninr(commbuff_t *commbuff);
int                     apsvrinit(int argc, char *argv[]);
int                     apsvrdone();
/* 개설요청 */
static int  a000_initial(int argc, char argv[]);
static int  a100_parse_custom_args(int argc, char argv[]);
static int  b100_mqparm_load(symqtop_onin_ctx_t    *ctx);
static int  d100_init_mqcon(symqtop_onin_ctx_t     *ctx);
static int  e100_set_session(symqtop_onin_ctx_t    *ctx);
static int  e200_make_req_data(symqtop_onin_ctx_t  *ctx);
static int  f100_put_mqmsg(symqtop_onin_ctx_t      *ctx);

static int  f300_insert_exmqgwlog(symqtop_onin_ctx_t *ctx);    
static int  g100_get_tpctx(symqtop_onin_ctx_t       *ctx);
/* 개설응답 */
static int  j100_get_commbuff(symqtop_onin_ctx_t    *ctx);
static int  j200_reply_res_data(symqtop_onin_ctx_t  *ctx);
/* 오류처리 */
static int  x000_error_proc(symqtop_onin_ctx_t      *ctx);
static int  y000_timeout_proc(symqtop_onin_ctx_t    *ctx);
static int  z000_free_session(symqtop_onin_ctx_t    *ctx);

static int  term_id_conv(char *term_id);
int host_data_conv(int type, char *data, int len, int msg_type);


/* -------------------------- function prototypes --------------------------------- */
/* -------------------------------------------------------------------------------- */

int apsvrinit(int argc, char *argv)
{
    int         rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(symqtop_onin_ctx_t));
    SYS_DBG("Initialize");

    rc = a000_svc_init(argc, argv);
    if (rc == ERR_ERR)
        return rc;

    rc = b100_mqparm_load(ctx);
    if (rc != ERR_NONE){
        SYS_DBG("b100 mqparm_load failed +++++++++++++++++++++");
        ex_syslog(LOG_FATAL, "[APPL_DM] b100_mqparm_load() FAIL [해결방안]시스템 담당자 call");
        return rc; 
    }
    rc = d100_init_mqcon(ctx);
    if (rc != ERR_NONE){
        SYS_DBG("d100_init_mqcon failed +++++++++++++++++++++");
        ex_syslog(LOG_FATAL, "[APPL_DM] d100_init_mqcon() FAIL [해결방안]시스템 담당자 call");
        return rc; 
    }

    SYS_TREF;

    return rc;



}


/* -------------------------------------------------------------------------------- */
int symqtop_onin(commbuff, *commbuff)
{

    int         rc = ERR_NONE;
    sysiouth_t  sysiouth = {0};

    SYS_TRSF;
    /* 에러코드 초기화 */
    sys_err_init();

    /* set commbuff */
    ctx->cb = commbuff;

    SYS_DBG("================= 개설요청 symqtop_onin Start =========================");
    /* 입력데이터 검증 */
    if (SYSGWINFO == NULL) {
        SYS_HSTERR(SYS_NM, ERR_SVC_SNDERR, "INPUT DATA ERR");
        sysocbfb(ctx->cb);
        return ERR_ERR;
    }

    PRINT_SYSGWINFO(SYSGWINFO);

    /* 회선 관련 명령어 검증 */
    if (SYSGWINFO->conn_type > 0)
        return ERR_NONE;
    
    /* 입력데이터 검증 */
    if (SYSGWINFO == NULL) {
        SYS_HSTERR(SYS_NM, ERR_SVC_SNDERR, "INPUT_DATA ERR");
        return ERR_ERR;
    }

    #ifdef _CORE_TEST
        if (SYSGWINFO->msg_type == SYSGWINFO_MSG_1200)
            el_data_chg((exmsg1200_t *) sysocbgp(ctx->cb, IDX_HOSTSENDDATA), (exmqparm_t *) sysocbgp(ctx->cb,IDX_EXPARM));
    #endif

    /* 2nc MQ Initialization 한번 init후 플래그 확인후 이상없으면 안해줌 */
    rc = d100_init_mqcon(ctx);
    if (rc == ERR_ERR){
        SYS_DBG("d100_init_mqcon failed STEP1 ");

        ex_syslog(LOG_FATAL, "[APPL_DM]d100_init_mqcon 2nd STEP1 FAIL [해결방안]시스템 담당자 call");
        ctx->is_mq_connected = 0;
        sysocbfb(ctx->cb);

        return ERR_ERR;
    }

    /* set session */
    SYS_TRY(e100_set_session(ctx));

    /* make req data */
    SYS_TRY(e200_make_req_data(ctx));

    /* MQPUT & logging */
    SYS_TRY(f100_put_mqmsg(ctx));

    /* get context */
    SYS_TRY(g100_get_tpctx(ctx));

    sysocbfb(ctx->cb); /* commbuff free */
    if (SYSIOUTH == NULL) {
        rc = sysocbsi(commbuff, IDX_SYSIOUTH, (char *)&sysiouth, sizeof(sysiouth_t));
        sysiouth.err_code = 0000000;
        if (rc != ERR_NONE) {
            SYS_HSTERR(SYS_LC, SYS_GENERR, "SYSIOUTH COMMBUFF 설정 에러 ");
        }
    }

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:
    z000_free_session(ctx);

    SYS_TREF;
    return ERR_ERR;

}

/* -------------------------------------------------------------------------------- */
int symqtop_oninr(commbuff_t *commbuff)
{

    int                 rc          = ERR_NONE;
    sysiouth_t          sysiouth    = {0};
    symqtop_onin_ctx_t  _ctx        = {0};
    symqtop_onin_ctx_t  *ctx        = &_ctx;

    SYS_TRSF;
    /* 에러코드 초기화  */
    sys_err_init();
    /* set commbuff */
    ctx->cb  = &ctx->cb;    
    ctx->cb  = commbuff;

    SYSICOMM->intl_tx_flag = 0; /* 대문자 C 동작안함 */


    SYS_DBG("================= 개설응답 symqtop_oninr Start =========================");

    /* 입력데이터 검증 */
    if (SYSIOUTQ == NULL) {
        SYS_HSTERR(SYS_NM, ERR_SVC_SNDERR, "INPUT DATA ERR");
        sysocbfb(ctx->cb);
        return ERR_ERR;
    }

    SYS_DBG("symqtop_oninr SYSIOUTQ[%s]", SYSIOUTQ);

    SYS_TRY(j100_get_commbuff(ctx));

    SYS_TRY(j200_reply_res_data(ctx));

    SYS_DBG("================= 개설응답 symqtop_oninr End =========================");
    
    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:
    SYS_DBG("======== 개설 응답 END (SYS_CATCH)  ======== ");
    x000_error_proc();
    z000_free_session();

    SYS_TREF;

    return ERR_ERR;

}
/* -------------------------------------------------------------------------------- */
int usermain(int argc, char *argv[])
{

    int rc = ERR_NONE;

    SYS_DBG("============================ usermain Start =========================");
    set_max_session(g.session_num);

    set_session_tmo_callback(y000_timeout_proc, g.session_tmo);

    while(1){
        if (db_connect("") != ERR_NONE){
            SYS_DBG("sleep(60)");
            tpschedule(60);
            continue;
        }

        rc = tpschedule(TMOCHK_INTERVAL); //1초에 한번씩 

        if (rc < 0){
            if (tperrno = TPETIME){

                session_tmo_check();

            }else{

                SYS_HSTERR(SYS_LN, 8600, "tpschedule error tperrno[%d]", tperrno);
            }
            continue;
        }
        session_tmo_check();

    }

    return ERR_NONE;
}


/* -------------------------------------------------------------------------------- */
static int a000_initial(int argc, char *argv[])
{


    int rc = ERR_NONE;
    int i;

    SYS_TRSF;
    /* command argument 처리 */
    SYS_TRY(a100_parse_custom_args(argc, argv));
    strcpy(g_arch_head.svc_name, g_svc_name);
    memset(g_arch_head.eff_file_name, 0x00, g_arch_head.eff_file_name);

    /* 통신 헤더 */
    g_head_len = sizeof(hcmihead_t);
    
    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;
    return ERR_ERR;
}
/* -------------------------------------------------------------------------------- */
static int a100_parse_custom_args(int argc, char *argv[])
{

    int  c;  
    
    while((c = getopt(argc, argv, "s:c:a:M:T:")) != EOF ){

        /*************************************************/
        SYS_DBG("GETOPT: -%c\n", c);
        /*************************************************/
        
        switch(c){
        case 's':
            strcpy(g_svc_name, optarg);
            break;

        case 'c':
            strcpy(g_svc_name, optarg);
            break;

        case 'a':
            strcpy(g_svc_name, optarg);
            break;

        case 'M':
            strcpy(g_svc_name, optarg);
            break;

        case 'T':
            strcpy(g_svc_name, optarg);
            break;

        case '?':
            SYS_DBG("unrecognized option %c %s", optarg,argv[optind]);
            return ERR_ERR;
        }
    }


   /* -------------------------------------------------------- */
    SYS_DBG("g_svc_name           = [%s]",   g_svc_name);
    SYS_DBG("g_chnl_code          = [%s]",   g_chnl_code);
    SYS_DBG("g_appl_code          = [%s]",   g_appl_code);
    SYS_DBG("session_num          = [%d]",   g.session_num);
    SYS_DBG("session_tmo          = [%d]",   g.session_tmo);
    /* ------------------------------------------------------- */
    /* 서비스명 검증 */
    if (strlen(g_svc_name) == 0 ) ||
       (g_svc_name[0]      == 0x20))
        return ERR_ERR;

    return ERR_NONE;

}

/* ---------------------------------------------------------------- */
static int b100_mqparam_load(symqtops_onin_ctx_t  *ctx)
{   
    int          rc                 = ERR_NONE;
    static int   mqparam_load_flag  = 0;
    mqi0001f_t   mqi0001f;

    SYS TRSF;

    memset (&mqi0001f, 0x00, sizeof (mq10001f_t));

    memcpy(EXMQPARM->chnl_code, g_chnl_code, LEN_EXMQPARM_CHNL_CODE);
    memcpy(EXMQPARM->appl_code, g_appl_code, LEN_EXMQPARM_APPL_CODE);
    
    mqi0001f.in.exmqparm = EXMQPARM;
    rc = mq_exmqparm_select(&mqi0001f);
    if (rc == ERR_ERR) {
        SYS_HSTERR(SYS_LCF, SYS_GENERR, "[FAIL]EXMQPARAM select failed chnl[%s] err[%d][%s]",g_chnl_code, ERR_ERR, g_chnl_code);
        return ERR_ERR;
    }

    utotrim(EXMQPARAM->mq_mngr);
    if(strlen(EXMQPARM->mq_mngr) == 0){
        SYS_HSTERR(SYS_LCF, SYS_GENERR, "[FAIL]EXMQPARAM [%s/%s/%s]mq_mngr is null ",g_chnl_code, ERR_ERR, g_chnl_code);
        return ERR_ERR;
    }

    utotrim(EXMQPARAM->putq_name);
    if(strlen(EXMQPARM->putq_name) == 0){
        SYS_HSTERR(SYS_LCF, SYS_GENERR, "[FAIL]EXMQPARAM [%s/%s/%s] putq_name is null",g_chnl_code, ERR_ERR, g_chnl_code);
        return ERR_ERR;
    }

    mqparam_load_flag = 1;
    
    SYS_TREF;

    return ERR_NONE;
}


/* ---------------------------------------------------------------- */
static int( d100_init_mqcon(symqtops_onin_ctx_t  *ctx)
{
    int   rc = ERR_NONE;
    SYS_TRSF;

    if ( ctx->is_mq_connected == 1){
        return ERR_NONE;
    }

    if (ctx->mqhconn != 0){
        ex_mq_disconnect(&ctx->mqhconn);
        ctx->mqhconn = 0;

    }

    /* queue Manager Connect*/
    rc = ex_mq_connect(&ctx->mqhconn, EXMQPARAM->mq_mngr);
    if (rc == MQ_ERR_SYS){
        ex_syslog(LOG_ERROR, "[CLIENT DM] %s d100_init_mqcon MQ CONNECT failed");
        return ERR_ERR;
    }

    /*
     * mq_con_type : Queue Connect Type
     * ------------------------------------
     * 1 = onlu GET, 2 = only PUT, 3 = BOTH
     */
    /* FWR Queue OPEN EIS.KR.KTI_01 LISTEN */
    rc = ex_mq_open(ctx->mqhconn, &ctx->mqhobj, EXMQPARAM->putq_name, MQ_OPEN_OUTPUT);
    if (rc == MQ_ERR_SYS) {
        ex_syslog(LOG_ERROR, "[CLIENT DM] %s d100_init_mqcon MQ CONNECT failed g_chnl_code");
        ex_mq_disconnect(&ctx->mqhconn);
        ctx->mqhconn = 0;
        return ERR_ERR;
    }

    ctx->is_mq_connected = 1;

    SYS_TREF;

    return ERR_NONE;
}
 


/* ---------------------------------------------------------------- */
static int e100_set_session(symqtops_onin_ctx_t  *ctx)
{

    int     rc                             = ERR_NONE;
    char    jrn_no[LEN_JRN_NO+1]           = {0};
    char    corr_id[LEN_SESSION_CORR_ID+1] = {0};

    SYS_TRSF;

    /* 채널번호, corr_id, msg_id 채번 */
    utoclck(jrn_no);
    utocick(corr_id);

    memcpy(ctx->ilog_jrn_no, jrn_no+1, LEN_JRN_NO);
    memcpy(ctx->jrn_no  ,    jrn_no+1, LEN_JRN_NO);
    
    /* set session to list */
    SESSION = set_session_to_list(ctx->ilog_jrn_no, SESSION_DATA_SIZE);
    SYS_DBG("SESSION1[%p]", SESSION);
    SYS_DBG("SESSION2[%p]", SESSION);


    if (SESSION == 0x00){
        SYS_HSTERR(SYS_LN, sys_error_code(), "%s", sys_error_msg());
        return ERR_ERR;
    }

    memcpy(SESSION->jrn_no, ctx->jrn_no, LEN_JRN_NO);


    /*memcpy(SESSION->corr_id, "20230607153012345678", LEN_HCMIHEAD_QUEUE_NAME);
    TOPS로 전송할때 24바이트 corr_id조립(20바이트 앞에 0000을 추가) */


    memset( SESSION->corr_id   , 0x00   , sizeof(SESSION->corr_id));
    memcpy( SESSION->corr_id   , "0000" , 4);
    memcpy(&SESSION->corr_id[4], corr_id, LEN_HCMIHEAD_QUEUE_NAME);

    SYS_DBG("SESSION->corr_id:[%s]", SESSION->corr_id);




    if(EXPARM){
        SESSION->wait_sec = atoi(EXPARM->time_val);
    }

    SYS_DBG("SESSION DATA %s", SESSION_DATA->cb);

    /* save SVC commbuff */
    rc = sysocbdb(ctx->cb, &SESSION_DATA->cb);
    if (rc = ERR_ERR){
        SYS_HSTERR(SYS_LN, 8600, "sysocbdb failed jrno[%.27s]", ctx->ilog_jrn_no);
    }

    SYS_TREF;

    return ERR_NONE;
} 




/* ---------------------------------------------------------------- */
static int e200_make_req_data(symqtops_onin_ctx_t  *ctx)
{
    long               rc       = ERR_NONE
    long               data_len = 0;
    long               req_len  = 0;
    int                size;
    int                len;
    char               *hp, *dp, *dp2, *dp3, buff[10];
    req_data_t         *req_data = 0;
    exi0280_t          exi0280;
    hcmihead_t         *hcmihead;


    SYS_TRSF;

    memset(g_temp_buf, 0x00 , sizeof(g_temp_buf));
    dp = g_temp_buf;

    /* 송신 길이 검증 */
    len = sysocbgs(ctx->cb, IDX_HOSTSENDDATA);
    if (len <= 0){
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "DATA NOT FOUND");
        return ERR_ERR;
    }
    /* ---------------------------------------------- */
    SYS_DBG("e200_make_req_data: data_len = [%d]", len);
    /* ---------------------------------------------- */

    /* 송신할 데이터 */
    dp2 = (char *) sysocbgp(ctx->cb, IDX_HOSTSENDDATA);
    if(dp2 == NULL){
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "DATA NOT FOUND");
        return ERR_ERR;
    }
    SYS_DBG("dp2[%s]", dp2);

    SYSGWINFO->sys_type = SYSGWINFO_SYS_CORE_BANK;

    /* 1200 message인경우 NULL TRIM */
    if (SYSGWINFO->msg_type == SYSGWINFO_MSG_1200){
        SYS_DBG("CASE - >(SYSGWINFO->msg_type == SYSGWINFO_MSG_1200 : 1200전문");
        memset(&exi0280, 0x00, sizeof(exi0280_t));
        exi0280.in.type = 2;
        memcpy(exi0280.in.data, dp2, LEN_EXMQMSG1200);
        exmsg1200_null_proc(&exi0280);

        /* 전송데이터 복사 */
        memcpy(&dp[g_header_len], exi0280.out.data, LEN_EXMQMSG1200_COMM);
        g_temp_len = LEN_EXMQMSG1200_COMM;

        /* 기타 데이터 추가 되어 있는 경우 1304 보다 클때 */
        if (len > LEN_EXMQMSG1200){
            memcpy(&dp[g_header_len + g_temp_len], &dp2[LEN_EXMQMSG1200], (len - LEN_EXMQMSG1200));
            g_temp_len += (len - LEN_EXMQMSG1200);
        }
    }
    else{

        /* 전송 데이터 복사 */
        memcpy(&dp[g_head_len], dp2, len); 
        g_temp_len = len;
    }
    SYS_DBG(" 전송 데이터 [%d][%.*s]", g_temp_len, g_temp_len, &dp[g_head_len]);

    /*
     HCMIHEAD
    */

    /* -------------------------------  기본 저장 ------------------------------- */


    /* 통신 헤더 생성 */
    memset(dp, 0x20, g_head_len);

    hcmihead = (hcmihead_t *) dp;
    /* 응답으로 호스트계에 전송하는 경로 */
    hp = sysocbgp(ctx->cb, IDX_TCPHEAD);
    if (hp != NULL){
        memcpy(dp, hp, g_head_len);
    }

    
    if(SYSGWINFO->msg_type == SYSGWINFO_MSG_1200){
        memcpy(hcmihead->tx_code, &dp2[10], LEN_TX_CODE);
    }else{
        memcpy(hcmihead->tx_code, &dp2[9] , LEN_TCP_CODE);
    }
    hcmihead->comm_type = 'S';

    sprintf(buff, "%3d", SYSGWINFO->time_val);
    memcpy(hcmihead->wait_time, buff, LEN_HCMIHEAD_WAIT_TIME);
    hcmihead->cont_type = CONT_TYPE_END;

    sprintf(buff, "%5d", g_temp_len);
    memcpy(hcmihead->data_len, buff, LEN_HCMIHEAD_DATA_LEN);
    hcmihead->cnvt_type = '0';

    memcpy(hcmihead->tran_id, dp2, LEN_HCMIHEAD_TRAN_ID);
    memcpy(hcmihead->resp_code, '0', LEN_HCMIHEAD_RESP_CODE);

    //queue name 저장
    memcpy(hcmihead->queue_name, SESSION->corr_id, LEN_HCMIHEAD_QUEUE_NAME);


    /* 일련번호 채번 : 호스트 응답을 받는 경우 */
    if (IS_SYSGWINFO_SVC_REPLAY){
        g_call_seq_no++;
        if (g_call_seq_no > 5999999)
            g_call_seq_no = 1;
            sprintf(buff, "%7d", g_call_seq_no);
            memcpy(hcmihead->mint_rtk, buff, 7);
    }

    /* ---------------------------------------------------------- */
    SYS_DBG("e200_make_seq_data: input len[%d]", len);
    SYS_DBG("e200_make_seq_data: tx_code  [%10.10s] tran_id[%-4.4s]"
    , hcmihead->tx_code, hcmihead->tran_id);

    PRINT_HCMIHEAD(hcmihead);

    /* --------------session save start ---------------------------*/

    req_len  = g_head_len + g_temp_len + 256;

    req_data = malloc(req_len + 100);
    id(req_data == NULL){
        SYS_HSTERR(SYS_LN, SYS_GENERR, " sys_tpcall failed [%d]jrn_no[%s]"len, SESSION->ilog_jrn_no);
        return ERR_ERR;
    }

    memset(req_data, 0x00, req_len + 100);
    memset(req_data, 0x20, req_len);

    if( SYSGWINFO->msg_type == SYSGWINFO_MSG_1200){
        memcpy(req_data->hcmihead.tx_code, &dp2[10], LEN_TX_CODE);
    }else{
        memcpy(req_data->hcmihead.tx_code, &dp2[9] , LEN_TX_CODE);
    }
    req_data->hcmihead.comm_type = 'S';

    /* --------------session save -end --------------------------*/

    /* 전문변환 */
    size = g_temp_len + g_head_len +256;
    dp3 = (char *) malloc(size);
    if (dp3 == NULL){
        ex_syslog(LOG_FATAL, "[APPL_DM] syonhtsend :메모리 설정 에러
                              해결방안 시스템 담당자 CALL" );
        ex_syslog(LOG_FATAL, "[APPL_DM] %s syonhtsend :메모리 설정 에러: %d"
                           __FILE__, size);
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "MEMORY ALLOC ERR");
        return ERR_ERR;
    }

    /* ---------------------------------------------------------- */
    //SYS_DBG
    //SYS_DBG

    /* ---------------------------------------------------------- */
    //SYS_DBG
    //SYS_DBG
    //SYS_DBG

    /* ---------------------------------------------------------- */

    memcpy(req_data->indata, g_temp_buf, (g_head_len + g_temp_len));
    SESSION_DATA->reg_data = reg_data;

    SESSION_DATA->req_len  = g_temp_len;

    //전문전 로그 기록
    if ( f300_insert_exmqwlog(ctx) == ERR_ERR){
        ex_syslog(LOG_ERROR, "f300_insert_exmqwlog() FAIL CALL system admin");
        sysocbfb(ctx->cb);
        return ERR_ERR;
    }

    memcpy(dp3, g_temp_buf, g_head_len);
    SYS_DBG("host_data_conv 변환후 dp3[%.*s]", g_head_len, dp3);
    /* 데이터 코드 변환 */
    utoae2eb((unsigned char *)g_temp_buf, g_head_len, (unsigned char*)dp3);
    host_data_conv(AS_TO_BE, g_temp_buf, (g_temp_len + g_head_len));
    len = g_head_len + g_temp_len;

    /* 변환 후 데이터 로그 */
    /* ------------------------------------------------------------ */
    SYS_DBG("host_data_conv 변환 후 hcmihead data [%.*s]", sizeof(hcmihead_t), g_temp_buf);


#ifdef _DEBUG
    utohexdp(dp3, len);
    //memcpy(req_data->indata, dp3, len);

#endif

    SYS_TREF;

    return ERR_NONE;
} 

/* ---------------------------------------------------------------- */
static int f100_put_mqmsg(symqtops_onin_ctx_t  *ctx)
{
    int           rc        = ERR_NONE;
    mq_info_t     mq10001f  = {0};
    char          *hp;
    MQMD          md        = {MQMD_DEFAULT};
    int           size, len;
    int           g_temp_len;
    char          *dp, *dp2;
    char          corridTemp[20];
    char          l_head_len[ 4 + 1];
    char          l_head_buf[72 + 1];
    char          l_temp_buf[ 5000 ];

    SYS_TRSF;

    /* MQ 기본 정보 셋팅 */
    mqinfo                  = ctx->mqinfo;
    mqinfo.hobj             = &ctx->mqhobj;
    mqinfo.hcon             = ctx->mqhconn;
    mqinfo.coded_charset_id = EXMQPARM->charset_id;
    memcpy(mqinfo.msgid,   SESSION->corr_id,  LEN_EXMQMSG_001_MSG);
    memcpy(mqinfo.corrid,  SESSION->corr_id,  LEN_EXMQMSG_001_CORR_ID);
    /* ---------------------------------------------------------- */
    //SYS_DBG

    /* ---------------------------------------------------------- */
    //SYS_DBG

    /* ---------------------------------------------------------- */
    SYS_DBG("ilog_jrn_no[%s], corr_id[%s]", ctx->ilog_jrn_no, mqinfo.corr_id);

    mqinfo.msglen = SESSION_DATA->req_len + g_head_len;
    SYS_DBG("mqinfo.msglen put직전 [%d]", mqinfo.msglen);

    memcpy(mqinfo.msgbuf,  ((req_data_t *) REQ_DATA)->indata, mqinfo.msglen);

    rc = ex_mq_put_with_opts(&mqinfo, &md, 0);

    SYS_DBG("put mqmsg req_data->indata [%s]",((req_data_t *) REQ_DATA)->indata);

    if(rc = ERR_NONE){
        rc = ex_mq_commit(&ctx->mqhconn);
    }else{
        /* reset flag for mq_put FAIL !!! */
        ctx->is_mq_connected = 0;

        SYS_DBG("CALL ex_mq_put FAIL");
        ex_syslog(LOG_FATAL, "[APPL_DM] Call ex mq put FAIL [해결방안 ] 시스템 담당자 CALL");

        rc = ex_mq_commit(&ctx->mqhconn);

        return ERR_ERR;

    }

    SYS_TREF;

    return ERR_NONE;
}

/* ---------------------------------------------------------------- */
static int f300_insert_exmqwlog(symqtops_onin_ctx_t  *ctx)
{
    int             db_rc       = ERR_NONE;
    mq_info_t       mq_info     = {0};
    MQMD            md          = {MQMD_DEFAULT};
    exmqmsg_001_t   exmqmsg_001;
    mqi0001f_t      mqimsg001;

    SYS_TRSF;

    /* ------------------MQ 기본 정보 세팅 ----------------------------*/
    mqinfo          = ctx->mqinfo;

    utocick(mqinfo.msgid);
    memcpy(mqinfo.corr_id, SESSION->corr_id, LEN_EXMQMSG_001_CORR_ID);

    SYS_DBG("req_data ASCII: [%s]", ((req_data_t) REQ_DATA)->indata);

    mqinfo.msglen = SESSION_DATA->req_len + g_head_len;
    SYS_DBG("reqinfo.msglen: [%d]", reqinfo.msglen);  
    memcpy(mqinfo.msgbuf, ((req_data_t *)REQ_DATA)->indata, mqinfo.msglen);

    /* ------------------mqinfo_001 세팅 ----------------------------*/
    memset(&mqmsg_001, 0x00, sizeof(exmqmsg_001_t));
    memcpy(mqmsg_001.chnl_code, g_chnl_code , LEN_EXMQPARM_CHNL_CODE);
    memcpy(mqmsg_001.appl_code, g_appl_code , LEN_EXMQPARM_APPL_CODE);
    memcpy(mqmsg_001.msg_id   , mqinfo.msg_id ,  LEN_EXMQPARM_MSG_ID);
    memcpy(mqmsg_001.corr_id  , mqinfo.corr_id, LEN_EXMQPARM_CORR_ID);
    mqmsg_001.mqlen = mqinfo.msglen;
    memcpy(mqmsg_001.mqmsg   , mqinfo.msgbuf   , mqmsg_001.mqlen);
    mqmsg_001.io_type = 1;   //0이 get 1이 put
    memcpy(mqmsg_001.ilog_jrn_no, SESSION->ilog_jrn_no, LEN_EXMQMSG_001_ILOG_JRN_NO);
    

    /* ------------------MQ 로그 처리    ----------------------------*/
    memset(&mqmsg_001, 0x00, sizeof(exmqmsg_001_t));
    mqmsg_001.mqmsg_001 = &mqmsg_001;

    memcpy(mqimsg001.in.job_proc_type, "4", LEN_MQIMSG001_PROC_TYPE);

    db_rc = mqomsg001(&mqimsg001);
    if (db_rc == ERR_ERR){
        EXEC SQL ROLLBACK WORK;
        ex_syslog(LOG_ERROR, "[APPL_DM] %s f300_insert_exmqwlog: MQMSG001 INSERT ERROR 시스템 담당자 CALL" );
        return ERR_ERR;
    }


    SYS_TREF;

    return  ERR_NONE;

}

/* ---------------------------------------------------------------- */
static int g100_get_tpctx(symqtops_onin_ctx_t  *ctx)
{

    int     rc          = ERR_NONE;

    SYS_TRSF;
 
    /* get tmax context to relay svc */
    if(tpgetctx(&SESSION->ctx) < 0){
        SYS_HSTERR(SYS_LN, 782900, "tpgetctx failed jrn_no[%s]", SESSION->ilog_jrn_no);
        return ERR_ERR;        
    }

    // 응답필요없는 것 tprelay
    //service return 
    if (!IS_SYSGWINFO_SVC_REPLAY){
        sys_tprelay(“EXTRELAY”, &SESSION_DATA->cb, 0, &SESSION_DATA->ctxt);
        if(rc == ERR_ERR){
            SYS_HSTERR(SYS_LN, SYS_GENERR, "sys_tprelay failed jrn_no[%s]", SESSION->ilog_jrn_no);
            return ERR_ERR;            
        }

        return GOB_NRM;
    }

    SESSION->cd = SESSION_READY_CD;

    SYS_TREF;

    return ERR_NONE;
}
/* ---------------------------------------------------------------- */
static int j100_get_commbuff(symqtop_onin_ctx_t *ctx)
{

    int                 rc      = ERR_NONE;
    int                 len; 
    char                corr_id[LEN_SESSION_CORR_ID+1] = {0};
    char                *dp;
    char                buff[200];
    char                buff2[200];
    hcmihead_t          *hcmihead;


    SYS_TRSF;
    
    sys_err_init();

    //head 전문
    dp = (char *) sysocbgp(ctx->cb, IDX_SYSIOUTQ);
    SYS_DBG("dp:[%s]", dp);

    
    hcmihead = (hcmihead_t *)dp;
    memset(buff,    0x00            , sizeof(buff));
    memcpy(buff, hcmihead->data_len , LEN_HCMIHEAD_DATA_LEN);

    memset(buff2,   0x00              , sizeof(buff2));
    memcpy(buff2, hcmihead->queue_name, LEN_HCMIHEAD_QUEUE_NAME);

    /* ------------------------------------------------------- */

    SYS_DBG("data_len=[%s]", buff);
    SYS_DBG("queue_name=[%s]", buff2);
    /* ------------------------------------------------------- */

    len = utoa2in(buff, LEN_HCMIHEAD_DATA_LEN); /* 1200 전문 */

    memcpy(corr_id, "0000", 4);  /* session corr_id 같아야함 */
    memcpy(&corr_id[4], buff2, LEN_HCMIHEAD_QUEUE_NAME);

    SYS_DBG("corr_id[%s]", corr_id);

    SESSION = seek_session_from_list_by_corr_id(corr_id);  // ctx->session
    if (SESSION = 0x00){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "get session_from_by_list_corr_id failed. invaild corr_id[%s] or timeout", corr_id);


        // 중복 로깅을 방지하기 위해 log_flag set
        ctx->log_flag = 1;
        
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;
}
/* ---------------------------------------------------------------- */
static int j200_reply_res_data(symqtop_onin_ctx_t   *ctx)
{

    int                 rc       = ERR_NONE;
    int                 err_code = 0; 
    int                 data_len = 0;
    int                 out_len  = 0;
    int                 res_len  = 0;
    int                 len,len2 = 0; //EI용
    res_data_t         *res_data = 0;
    res_data_t         *as_data  = 0;
    hcmihead_t          hcmihead  ={0};
    char                buff[256] ={0};
    exmsg1200_t        *exmsg1200;
    exi0280_t           exi0280;
    char               *dp;


    SYS_TRSF;

    res_data = SYSIOUTQ;
    res_len  = SYSIOUTQ_SIZE;   //1272 or 4472

    //서비스 전문설정 
    if (memcmp(g_svc_name, "SYMQTOP", 7) == 0){
        //SYSGWINFO->msg_type = SYSGWINFO_MSG_1500;
    }

    SYS_DBG("res_data=[%s]", res_data);
    SYS_DBG("res_data->outdat00=[%s]", res_data->outdata);

    SYS_DBG("SESSION_DATA->req_len  [%d]", SESSION_DATA->req_len);
    SYS_DBG("SESSION_DATA->req_data [%s]", SESSION_DATA->req_data);

    exmsg1200 = (exmsg1200_t *) sysocbgp(&SESSION_DATA->cb, IDX_SYSIOUTQ);

    rc = sysocbsi(ctx->cb, IDX_SYSGWINFO, ((sysgwinfo_t *) sysocbgp(&SESSION_DATA->cb, IDX_SYSGWINFO)) , sizeof(sysgwinfo_t));
    if (rc == ERR_ERR) {
        SYS_HSTERR(SYS_LN, IDX_GENERR, "sysocbsi(IDX_SYSGWINFO failed. jrn_no[%s])", SESSION->ilog_jrn_no);
        return ERR_ERR;
    }
    SYS_DBG("요청전문 SYSGWINFO->msg_type (0:1200전문 1:1500전문 ):[%d]", SYSGWINFO->msg_type);
    SYS_DBG("SESSION_DATA->req_data: %s", SESSION_DATA->req_data);

    as_data = malloc(res_len + 1);
    if (as_data == NULL){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "sys_tpalloc failed [%d]", res_len);
        return ERR_ERR;
    }
    memset(as_data, 0x00, res_len+1);
    memset(as_data, 0x00, res_len);

    SESSION_DATA->as_data = as_data;

    as_data->hcmihead =res_data->hcmihead;
    /*------------------------------------------------------------- */
    PRINT_HCMIHEAD(&as_data->hcmihead);
    /*------------------------------------------------------------- */
    
    /* check data length */
    len = utoa2in(as_data->hcmihead.data_len, LEN_HCMIHEAD_DATA_LEN) + g_head_len; //1272 data sum 
    
    out_len = res_len - LEN_HCMIHEAD;   //out_len : 1272 - 72 = 1200
    if (out_len > 0){
        if (as_data->hcmihead.cnvt_type == '0')
        {
            memcpy(as_data->outdata, res_data->out_data, out_len);
            //1200전문시 
            if (SYSGWINFO->msg_type == SYSGWINFO_MSG_1200){
                // skip 1200 size target value ...
            }

        }else{

                //utoeb2as(res_data->outdata, out_len, as_data->outdata);
        }

        SYS_DBG("out_len : [%d]", out_len); // 1304-72
        SYS_DBG("as_data->outdata [%s]", as_data->outdata);

        //dp header 실데이터 저장
        dp = &as_data->hcmihead;

        memcpy(&dp[g_head_len], as_data->out_data, res_len );

#ifdef _DEBUG
        /* -------------------------------------------------- */
        SYS_DBG("<====헤더 + 전문 ====>");
        len2 = (len > 2500) ? 2500 : len;   //len2는 2500이하면 1272 
        utohexdp((char *)dp, len2);         //헤더 + 실데이터 
        /* -------------------------------------------------- */
#endif 
    } //(out_len) end if 


    /* check receive length */
    len2 = len - g_head_len -1; //1273 - 72 - 1500전문 4472 - 72 = 4400
    if (len2 <= 0){


        SYS_HSTERR(SYS_LN, SYS_GENERR, "Failed (len2 <= 0 jrn_no[%s])", SESSION->ilog_jrn_no);
        z000_free_session();
        return ERR_ERR;
    }

    /* 전문구분 */
    //#define SYSGWINFO_MSG_1200        0
    //#define SYSGWINFO_MSG_1500        0
    //#define SYSGWINFO_MSG_ETC         0
    //#define SYSGWINFO_MSG_1200_EN     0
    //#define SYSGWINFO  ((sysgwinfo_t *)(ctx->cb->item[IDX_SYSGWINFO].buf_ptr)) 


    SYS_DBG("헤더의 실데이터 [%.*s]", len2, &dp[g_head_len]);    //len2=1200

    /* 1200 message인 경우 NULL ADD */
    if (SYSGWINFO->msg_type == SYSGWINFO_MSG_1200){
        memset(&exi0280,    0x00, sizeof(exi0280_t));
        exi0280.in.type = 1;
        memcpy(exi0280.in.data , &dp[g_head_len], (len - g_head_len));
        exmsg1200_null_proc(&exi0280);

        /* 전송데이터 복사 */
        memcpy(g_temp_buf, exi0280.out.data, LEN_EXMQMSG1200);
        g_temp_len = LEN_EXMQMSG1200; //1200

        /* 1200 데이터 추가 되어 있는경우 1200보다 클경우 , */
        if(len2 > LEN_EXMQMSG1200_COMM) {
            SYS_DBG("추가데이터 = [%d][%.*s]", (len2 - LEN_EXMQMSG1200_COMM), (len2 - LEN_EXMQMSG1200_COMM),
                                                 &dp[g_head_len+LEN_EXMQMSG1200] );
            memcpy(&g_temp_buf[LEN_EXMQMSG1200], &dp[g_head_len+LEN_EXMQMSG1200_COMM], (len2 - LEN_EXMQMSG1200_COMM) );
            g_temp_len += (len2 - LEN_EXMQMSG1200);
        }

    }else{

        /* 전송데이터 복사 */
        memcpy(g_temp_buf, &dp[g_head_len], (len - g_head_len));
        g_temp_len = len - g_head_len;

    }


    rc = sysocbsi(&SESSION_DATA->cb, IDX_SYSIOUTQ, g_temp_buf, g_temp_len);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "sysocbsi(IDX_SYSIOUTQ) failed. jrn_no[%s]", SESSION->ilog_jrn_no);
        return ERR_ERR;

    }

#ifdef  _DEBUG
    SYS_DBG("j200_reply_res_data: g_temp_len (1200msg 전문경우 null추가된 길이 )=[%d]", g_temp_len);    
    exmsg1200 = (exmsg1200_t *) sysocbgp(&SESSION_DATA->cb, IDX_SYSIOUTQ);  //4400 msg도 가능 
    SYS_DBGG("j200_reply_res_data: recv_data (exmsg1200->tx_id)=[%.*s]", g_temp_len,exmsg1200->tx_id);  //1200 data
#endif


    /* 아래 sys_tprelay 오류시 중복 로깅 방지 위해 log_flag 설정 */
    ctx->log_flag = 1;





    if (SESSION->cb == SESSION_READY_CD){
        SYS_DBG("[sys_tprelay]SESSION_DATA->req_data[%s]SESSION_DATA->cb[%s],ctxt[%s]", SESSION_DATA->req_data, SESSION_DATA->cb, SESSION_DATA->ctxt);
        rc = sys_tprelay("EXTRELAY", &SESSION_DATA->cb, 0, &SESSION_DATA->ctxt);
        if (rc == ERR_ERR){
            SYS_HSTERR(SYS_LN, SYS_GENERR, "sys_tprelay failed. cd[%d], jrn_no[%s]"m SESSION->cd, SESSION->ilog_jrn_no);
            return ERR_ERR;
        }
    }
    SYS_DBG("[j200_reply_res_data]ctx-cb", ctx->cb);
    z000_free_session();

    SYS_TREF;

    return ERR_NONE;

}
/* ---------------------------------------------------------------- */
static int x000_error_proc(symqtops_onin_ctx_t *ctx)
{

     int rc = ERR_NONE;

     SYS_TRSF;

     if(session = 0x00){

         SYS_HSTERR(SYS_LN, SYS_GENERR, “Session is NULL. err , sys_err_msg());

    } else {

         SYS_HSTERR(SYS_LN, sys_error_code, “%s”, sys_err_msg());

        //service return
        if(SESSION->cd == SESSION_READY_CD){
        rc = tprelay(“EXTRELAY”, &SESSION_DATA->cb, 0, &SESSION_DATA->ctxt);
        if (rc==ERR_ERR){
            SYS_HSTERR(SYS_LN, SYS_GENERR, “x000_error_proc(): sys_tprelay(EXTRELAY) failed jrn_no[%s]”, SESSION->ilog_jrn_no);
            }
        }
    }

    z000_free_session();

    SYS_TREF;

    return ERR_NONE;
}
/* -------------------------------------------------------------------------------- */
static void y000_timeout_proc(session_t *session)
{
    symqtop_onin_ctx_t     _ctx = {0};
    symqtop_onin_ctx_t     *ctx = {0};

    SYS_TRSF;

    ctx->cb = &ctx->cb;

    ex_syslog(LOG_FATAL, "[APPL_DM] y000_timeout_proc() 발생 [해결방안 ] 시스템 담당자 call " );

    SESSION = session;

    x000_error_proc();

    SYS_TREF;

    return;

}

/* -------------------------------------------------------------------------------- */
static int z000_free_session(symqtop_onin_ctx_t *ctx)
{

    SYS_TRSF;


    if(SESSION){

        del_session_from_list(SESSION);

        if (SESSION->data) {
            sysocbfb(&SESSION_DATA->cb);

            if (SESSION_DATA->req_data) {
                free(SESSION_DATA->req_data);
                SESSION_DATA->req_data = 0x00;
            }

            SESSION_DATA->req_len = 0;

            if (SESSION_DATA->as_data) {
                free(SESSION_DATA->as_data);

                SESSION_DATA->as_data = 0x00;
            }

            free(SESSION->data);
            SESSION->data = 0x00;
        }

        free(SESSION);
        SESSION = 0x00;  
    }
SYS_DBG("[z000_free_session]ctx->cb [%s]", ctx->cb);
    if (ctx->cb){
        sysocbfb(ctx->cb);
        ctx->cb = 0x00;
    }

    SYS_TREF;

    return ERR_NONE;
}
/* -------------------------------------------------------------------------------- */
int host_data_conv(int type, char *data, int len, int msg_type)
{
    char                *dp;
    char                pswd[50];
    char                term_id[50;]
    exmsg1200_comm_t    *excomm;
    exmsg1500_t         *exmsg1500;

    SYS_TRSF;

    dp   = &data[g_head_len];
    len -= g_head_len;

    excomm      = (exmsg1200_comm_t *) dp;
    exmsg1500   = (exmsg1500_t *) dp;

    /* 1200 전문의 암호화된 비밀번호 단말ID 변화하면 않됨. */
    if (msg_type == SYSGWINFO_MSG_1200) {
        memset(pswd,    0x00, sizeof(pswd));
        memcpy(pswd,    excomm->pswd_1,    LEN_EXMQMSG1200_PSWD_1);
        memcpy(term_id, excomm->term_id,   LEN_EXMQMSG1200_TERM_ID);
        if (type == EB_TO_AS) {
            memset(excomm->pswd_1 , 0x40, LEN_EXMQMSG1200_PSWD_1);
            memset(excomm->term_id, 0x40, LEN_EXMQMSG1200_TERM_ID );
        }else{
            memset(excomm->pswd_1 , 0x20, LEN_EXMQMSG1200_PSWD_1);
            memset(excomm->term_id, 0x20, LEN_EXMQMSG1200_TERM_ID );            
        }
    }
    
    /* 1500 전문의 단말 ID 변환하면 안됨 */
    else if (msg_type == SYSGWINFO_MSG_1500) {
        memcpy(term_id, exmsg1500->term_id,   LEN_EXMQMSG1500_TERM_ID);
        if (type == EB_TO_AS) {
            memset(excomm->exmsg1500, 0x40, LEN_EXMQMSG1500_TERM_ID );
        }else{
            memset(excomm->exmsg1500, 0x20, LEN_EXMQMSG1500_TERM_ID );            
        }
    }

    /* 1200 전문의 비밀번호, 단말 ID 변환하면 안됨 */
    if (msg_type == SYSGWINFO_MSG_1200) {
        memset(pswd,    0x00, sizeof(pswd));

        if (type == EB_TO_AS) {
                if ((pswd[0] == 0x40) && (pswd[1] == 0x40) &&
                    (pswd[2] == 0x40) && (pswd[3] == 0x40) &&
                    (pswd[4] == 0x40) && (pswd[5] == 0x40))
                {
                    memset(excomm->pswd_1,  0x20, LEN_EXMQMSG1200_PSWD_1);
                }else{
                    memcpy(excomm->pswd_1,  pswd, LEN_EXMQMSG1200_PSWD_1);
                    memcpy(excomm->term_id, term_id, LEN_EXMQMSG1200_TERM_ID);
                }
        }else{
            if (utohksp(pswd, LEN_EXMQMSG1200_PSWD_1) == SYS_TRUE)
                memset(excomm->pswd_1, 0x40, LEN_EXMQMSG1200_PSWD_1);
            else 
                memcpy(excomm->pswd_1, pswd, LEN_EXMQMSG1200_PSWD_1);

                /* 개설 거래 변환 함 */
                term_id_conv(term_id);
                memcpy(excomm->term_id, term_id, LEN_EXMQMSG1200_TERM_ID);
            }
        }
    
    /* 1500 전문의 단말 ID 변환하면 안됨 */
    else if (msg_type == SYSGWINFO_MSG_1500) {
        if (type == EB_TO_AS) {
            memset(exmsg1500->term_id, term_id, LEN_EXMQMSG1500_TERM_ID );
        }else{
            /* 개설 거래 변환 함 */
            term_id_conv(term_id);
            memset(exmsg1500->term_id, term_id, LEN_EXMQMSG1500_TERM_ID );            
        }
    }
        
    SYS_TREF;

    return ERR_NONE;

}
/* -------------------------------------------------------------------------------- */
static int term_id_conv(char *term_id)
{
    int           i; 
    char   buff[20];

    for (i = 0; < LEN_EXMQMSG1200_TERM_ID; i++)
    {
        buff[i] = ascii2ebcdic((unsinged char) term_id[i]);
    }

    memcpy(term_id, buff, LEN_EXMQMSG1200_TERM_ID);

    return 1;
}

/* -------------------------------------------------------------------------------- */
int apsvrdone()
{
    int           i;  

    SYS_DBG("Server End ");

    return ERR_NONE;

}
/* --------------------------------  Program End  --------------------------------- */
