/*  @file               symqtop_onout.pc                                             
*   @file_type          online program
*   @brief              tops <==> EI 취급요청 및 취급응답  program
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
답 
*
*
*
*
*
*
*   @generated at 2023/06/02 09:30
*   @history
*
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*
*
*/

/* -------------------------- include files  -------------------------------------- */
#include <sytcpgwhead.h>
#include <syscom.h>
#include <mqi00001f.h>
#include <cmqc.h>                   /* includes for MQ */
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
#include <fxmsg.h>
/* --------------------------constant, macro definitions -------------------------- */
#define EXMQPARM                    (&ctx->exmqparm)

#define REQ_DATA                    (SESSION_DATA->req_data)
#define REQ_INDATA                  (REQ_DATA->indata)
#define REQ_HCMIHEAD                (&REQ_DATA->hcmihead) 


//#define MQMSG001                    (&ctx->exmqmsg_001)
//#define MQ_INFO                     (&ctx->mq_info)
/* --------------------------structure definitions -------------------------------- */
typedef struct session_data_s session_data_t;
struct session_data_s {
    CTX_T               ctxt;
    commbuff_t          cb;   
    hcmihead_t          hcmihead;  
    char                *req_data;
    long                req_len;
    char                *as_data;
    char                log_flag;   
    char                term_id[99];
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

typedef struct symqtop_onout_ctx_s symqtop_onout_ctx_t; 
struct symqtop_onout_ctx_s {
    exmqparm_t          exmqparm;
    exmqmsg_001_t       exmqmsg_001;
    commbuff_t          *cb;
    commbuff_t          _cb;   
    session_t           *session;
    mq_info_t           mqinfo;
    long                is_mq_connected;
    long                mqparam_load_flag;
    MQHCONN             mqhconn; 
    MQHOBJ              mqhobj;
    char                ilog_jrn_no[LEN_JRN_NO+1];          /* image LOG JRN_NO */
    char                jrn_no[LEN_JRN_NO+1];               /* 27자리            */
    char                corr_id[LEN_EXMQMSG_001_CORR_ID+1]; /* 24자리    */
    char                log_flag;
};

/* Global 구조체 */
typedef struct symqtop_onout_global_s symqtop_onout_global_t; 
struct symqtop_onout_global_s {
    long                session_num;
    char                svc_name[LEN_EXPARAM_AP_SVC_NAME + 1];
};

/* --------------exported global variables declarations ---------------------------*/
    char                g_svc_name[32];                 /* G/W 서비스명          */
    char                g_chnl_code[3 + 2];             /* CHANNEL CODE        */
    char                g_appl_code[2 + 1];             /* APPL_CODE           */
    int                 g_call_seq_no;                  /* 일련번호 채번         */
    int                 g_temp_len;                     /* 임시 사용 buffer에 저장한 데이터 길이  */
    int                 g_db_connect;
    char                g_temp_buf[8192];               /* 임시 사용 buffer */
    int                 g_head_len;                     /* 통신헤더 길이 */

commbuff_t              g_commbuff;
symqtop_onout_ctx_t     _ctx; 
symqtop_onout_ctx_t     *ctx = &_ctx;

symqtop_onout_global_t  g = {0};

/* ----------------------- exported function declarations ------------------------- */
int                     symqtop_onout(commbuff_t *commbuff);
int                     symqtop_onoutr(commbuff_t *commbuff);
int                     apsvrinit(int argc, char *argv[]);
int                     apsvrdone();
/* mq setting */
static int  a000_initial(int argc, char argv[]);
static int  a100_parse_custom_args(int argc, char argv[]);
static int  b100_mqparm_load(symqtop_onout_ctx_t        *ctx);
static int  d100_init_mqcon(symqtop_onout_ctx_t         *ctx);

/* 취급요청 */
static int  e100_check_req_data(symqtop_onout_ctx_t     *ctx);
static int  e200_set_session(symqtop_onout_ctx_t        *ctx);
static int  e210_session_err_proc(symqtop_onout_ctx_t   *ctx);
static int  e300_set_commbuff(symqtop_onout_ctx_t       *ctx);
static int  f000_call_svc_proc();
int         f100_tmax_reply_msg(symqtop_onout_ctx_t     *ctx);
static int  f200_check_reply_msg(symqtop_onout_ctx_t    *ctx);
static int  f300_make_res_data_proc(symqtop_onout_ctx_t *ctx);
static int  f400_put_mqmsg(symqtop_onout_ctx_t          *ctx);
static void f500_reply_to_gw_proc(symqtop_onout_ctx_t   *ctx);

/* log insert */
static int  g100_insert_exmqgwlog(symqtop_onout_ctx_t   *ctx);
static int  g110_insert_exmqgwlog(symqtop_onout_ctx_t   *ctx);
static int  g200_get_tpctx(symqtop_onout_ctx_t          *ctx);

/* 취급응답 */
static int  j100_set_session(symqtop_onout_ctx_t        *ctx);
static int  j200_make_req_data(symqtop_onout_ctx_t      *ctx);
static int  k100_put_mqmsg(symqtop_onout_ctx_t          *ctx);

/* 오류처리 */  
static int  x000_error_proc(symqtop_onout_ctx_t         *ctx);
static int  x100_db_connect(symqtop_onout_ctx_t         *ctx);
static int  x200_log_req_data(symqtop_onout_ctx_t       *ctx);
static void x300_error_proc(symqtop_onout_ctx_t         *ctx);


static int  y000_timeout_proc(symqtop_onout_ctx_t       *ctx);
static int  z000_free_session(symqtop_onout_ctx_t       *ctx);

/* 내부 정의 함수 */
static int el_data_chg(exmsg1200_t *exmsg1200, exparam_t *exparam);
int host_data_conv(int type, char *data, int len, int msg_type);
static int  term_id_conv(char *term_id);

/* -------------------------------------------------------------------------------- */
int apsvrinit(int argc, char *argv)
{
    int         rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(symqtop_onin_ctx_t));



    SYS_DBG("\n\n 취급요청 요청/응답 Program Ignition STart ");

    rc = x100_db_connect(ctx);
    if (rc == ERR_ERR)
        return rc;

    rc = a000_initial(argc, argv);
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
int symqtop_onout(commbuff_t    *commbuff)
{
    int                 rc       = ERR_NONE;
    sysiouth_t          sysiouth = {0};
    symqtop_onout_ctx_t _ctx     = {0};
    symqtop_onout_ctx_t *ctx     = &_ctx;  

    SYS_TRSF;

    /* set commbuff */
    ctx->cb = &ctx->cb;
    ctx->cb = commbuff;

    //SYSICOMM->intl_tx_flag = 0; //대문자 C 동작안함 
    SYS_DBG("\n\n============================ 취급요청 start ============================");

    /* 입력데이터 검증 */
    if (SYSIOUTH == NULL )
    {
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "INPUT DATA ERR");
        SYS_DBG("INPUT DATA ERROR");
        sysocbfb(ctx->cb);
        return ERR_ERR;
    }

    SYS_TRY(e100_check_req_data(ctx));

    SYS_TRY(e200_make_req_data(ctx));

    SYS_TRY(e300_set_commbuff(ctx));

    SYS_DBG("\n\n============================ 취급요청 end ============================");

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    SYS_HSTERR("err[%d][%s]", sys_err_code(), sys_err_msg());

    x200_log_req_data(ctx);
    x000_error_proc(ctx);

    SYS_TREF;

    return ERR_ERR;

}
/* -------------------------------------------------------------------------------- */
int symqtop_onoutr(commbuff_t   *commbuff)
{

    int                   rc  = ERR_NONE;
    sysiouth_t            sysiouth = {0};

    SYS_TRSF;
    /* 에러코드 초기화 */
    sys_err_init();

    /* set commbuff */
    ctx->cb = commbuff;

    SYS_DBG("\n\n============================ 취급응답 start ============================");

    /* 입력데이터 검증 */
    if (SYSGWINFO == NULL )
    {
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "INPUT DATA ERR ");
        sysocbfb(ctx->cb);
        return ERR_ERR;
    }

    PRINT_SYSGWINFO(SYSGWINFO);

    /* 회선관련 명령어 검증 */
    if (SYSGWINFO->conn_type > 0)
        return ERR_NONE;
    
    /* 입력데이터 검증 */
    if (HOSTSENDDATA == NULL){
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "INPUT DATA ERR");
        return ERR_ERR;
    }


#ifdef  _DEBUG
    if (SYSGWINFO->msg_type == SYSGWINFO_MSG_1200)
        el_data_chg((exmsg1200_t) sysocbfb(ctx->cb, IDX_HOSTSENDDATA), (exparam_t *) sysocbgp(ctx->cb, IDX_EXPARM));
#endif

    /* 2nc MQ Initialization (한번 init 후 플래그 확인후 이상없으면 안해줌 )*/
    rc = d100_init_mqcon(ctx);
    if (rc == ERR_ERR){
        SYS_DBG("d100_init_mqcon failed STEP 1");
        ex_syslog(LOG_FATAL, "[APPL_DM] d100_init_mqcon (2nd) STEP1 FAIL [해결방안] 시스템 당당자 call" );
        ctx->is_mq_connected = 0;
        sysocbfb(ctx->cb);
        return ERR_ERR;
    }

    /* set session */
    SYS_TRY(j100_get_commbuff(ctx));

    /* make req data */
    SYS_TRY(j200_make_req_data(Ctx));

    /* MQPUT & logging */
    SYS_TRY(k100_put_mqmsg(ctx));

    /* get context */
    SYS_TRY(g200_get_tpctx(ctx));


    /*if (SYSIOUTH == NULL) {
        rc = sysocbsi(commbuff, IDX_SYSIOUTH, (char *)&sysiouth, sizeof(sysiouth_t));
        sysiouth.err_code = 0000000;
        if (rc != ERR_NONE) {
            SYS_HSTERR(SYS_LC, SYS_GENERR, "SYSIOUTH COMMBUFF 설정 에러 ");
        }
    }
    */

    z000_free_session(ctx);

    SYS_DBG("\n\n============================ 취급응답 end ============================");

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    SYS_DBG("\n\n====================== 취급응답 enc (SYS_CATCH) =======================");

    x300_error_proc();
    z000_free_session();

    SYS_TREF;

    return ERR_ERR;

}
/* -------------------------------------------------------------------------------- */
int usermain(int argc, char argv[])
{

    int rc = ERR_NONE;

    tpregcb(f100_tmax_reply_msg);
    SYS_DBG("============================ usermain Start =========================");

    set_max_session(g.session_num);

    //set_session_tmo_callback(y000_timeout_proc, g.session_tmo);

    while(1){

        rc  = x100_db_connect(ctx);
        if (rc == ERR_ERR){
            tpschedule(60);
            return rc;
        }

        rc = tpschedule(TMOCHK_INTERVAL); //1초에 한번씩 
        if (rc < 0){
            if (tperrno = TPETIME){

            }else{

                SYS_HSTERR(SYS_LN, 8600, "tpschedule error tperrno[%d]", tperrno);
            }
            continue;
        }
        f000_call_svc_proc();
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
    /* -------------------------------------------------------------- */
    SYS_DBG("a000_initial: header_len = [%d]", g_head_len);
    /* -------------------------------------------------------------- */   

    /* callback function register to tpcall response */
    //tpregcb(f100_tmax_reply_msg);

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
            SYS_DBG("g_svc_name     = [%s]", g_svc_name);
            break;

        case 'c':
            strcpy(g_svc_name, optarg);
            SYS_DBG("g_svc_name     = [%s]", g_svc_name);
            break;

        case 'a':
            strcpy(g_svc_name, optarg);
            SYS_DBG("g_svc_name     = [%s]", g_svc_name);
            break;

        case 'M':
            strcpy(g_svc_name, optarg);
            SYS_DBG("g_svc_name     = [%s]", g_svc_name);
            break;

        case '?':
            SYS_DBG("unrecognized option %c %s", optarg,argv[optind]);
            return ERR_ERR;
        }
    }

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
    mqi0001f_t   mqi0001f;


    if (ctx->mqparam_load_flag == 1){
        SYS_DBG("Alread exmqparam loaded, mqparam_load_flag:[%ld]", ctx->mqparam_load_flag);
        return ERR_NONE;
    }

    SYS TRSF;

    memset (&mqi0001f, 0x00, sizeof (mq10001f_t));

    memcpy(EXMQPARM->chnl_code, g_chnl_code, LEN_EXMQPARM_CHNL_CODE);
    memcpy(EXMQPARM->appl_code, g_appl_code, LEN_EXMQPARM_APPL_CODE);
    
    mqi0001f.in.exmqparm = EXMQPARM;    //포인터 지정 
    rc = mq_exmqparm_select(&mqi0001f);
    if (rc == ERR_ERR) {
        SYS_HSTERR(SYS_LCF, SYS_GENERR, "[FAIL]EXMQPARAM select failed chnl[%s] err[%d][%s]",g_chnl_code, ERR_ERR, g_chnl_code);
        g_db_connect = 0;
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

/* -------------------------------------------------------------------------------- */
static int f000_call_svc_proc()
{
    int          rc                 = ERR_NONE;
    int          cd                 = 0;
    symqtop_onout_ctx_t  _ctx       = {0};
    symqtop_onout_ctx_t  *ctx       = &_ctx; 
    session_t    *session           = session_global.head_session;
    session_t    *session2          = 0x00; 

    char         *dp;

    SYS_TRSF;

    while(session != 0x00){

        if (session->cb == SESSION_READY_CD){

            SESSION = session; 
            /* Set call service */
            memcpy(g.svc_name, ((sysicomm_t *) sysocbgp(&SESSION_DATA->cb, IDX_SYSICOMM))->call_svc_name, sizeof(g.svc_name));
            SYS_DBG("HOST FLAG [%s]", ((exparam_t *) sysocbgp(&SESSION_DATA->cb, IDX_EXPARM))->host_rspn_flag);
            //strcpy(g.svc_name, g_call_svc_name);

            /* service call */
            cd =  sys_tpacall(g.svc_name, &SESSION_DATA->cb, 0);
            if (cd < 0){
                SYS_HSTERR(SYS_LN, SYS_GENERR, "SVC(%s) call failed [%d]jrn_no[%s]", g.svc_name, tperrno, SESSION->ilog_jrn_no);

                session2 = session->next;

                //x000_error_proc(ctx); //추후 살림 

                session = session2;

                continue;
            }

            SESSION->cd = cd;  

            SYS_DBG("after sys_tpacall SVC(%s) cd=[%d] jrn_no[%s]", g.svc_name, cd, SESSION->ilog_jrn_no);
        }

        session = session->next;
    }

    SYS_TREF;

    return ERR_NONE;

}
/* ---------------------------------------------------------------- */
static int( d100_init_mqcon(symqtops_onin_ctx_t  *ctx)
{
    int   rc = ERR_NONE;

    SYS_TRSF;

    if ( ctx->is_mq_connected == 1){
        SYS_DBG("Alread exmqparam loaded, mqparam_load_flag:[%ld]", ctx->mqparam_load_flag);
        return ERR_NONE;
    }

    if (ctx->mqhconn != 0){
        SYS_DBG("mqhconn ERR, ctx->mqhconn:[%ld]", ctx->mqhconn);
        ex_mq_disconnect(&ctx->mqhconn);
        ctx->mqhconn = 0;
    }

    /* queue Manager Connect*/
    rc = ex_mq_connect(&ctx->mqhconn, EXMQPARAM->mq_mngr);
    if (rc == MQ_ERR_SYS){
        ex_syslog(LOG_ERROR, "[CLIENT DM] %s d100_init_mqcon MQ CONNECT failed g_chnl_code[%s]mq_mngr[%s]",__FILE__, g_chnl_code,EXMQPARM->mq_mngr);
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

    /* -------------------------------------------------------- */
    SYS_DBG("CONN Handler         = [%d][%08x]",   ctx->mqhconn, ctx->mqhconn);
    SYS_DBG("GET Queue Handler    = [%d][%08x]",   ctx->mqhobj , ctx->mqhobj );
    /* ------------------------------------------------------- */

    ctx->is_mq_connected = 1;

    SYS_TREF;

    return ERR_NONE;

}
/* ---------------------------------------------------------------- */
static int e100_check_req_data(symqtop_onout_ctx_t  *ctx)
{
    int          rc                 = ERR_NONE;
    int          len;
    char         *dp;
    //char      buff[200];
    //char      buff2[200];
    hcmihead    *hcmihead;
    char        jrn_no[LEN_JRN_NO+1];

    SYS_TRSF;

    sys_err_init();

    //head 전문 변환 
    dp = (char *) sysocbgp(ctx->cb, IDX_SYSIOUTQ);
    SYS_DBG("SYSIOUTQ : [%s]", dp);

    // TOP로 전송할 24바이트 corr_id조립 (20바이트 앞에 0000을 추가 )
    hcmihead = (hcmihead_t *) dp;

    memset(ctx->corr_id,   0x00, sizeof(ctx->corr_id));
    memcpy(ctx->corr_id, "0000", 4);
    memcpy(&ctx->corr_id[4], hcmihead->queue_name, LEN_HCMIHEAD_QUEUE_NAME);

    SYS_DBG("ctx->corr_id : [%s]", ctx->corr_id);









    len = utoa2in(hcmihead->data_len, LEN_HCMIHEAD_DATA_LEN); //1200
    /* -------------------------------------------------------- */
    SYS_DBG("hcmihead->data_len [%d]",  hcmihead->data_len);
    /* ------------------------------------------------------- */

    /* jrn_no 저널번호 채번 */
    utoclck(jrn_no);
    memcpy(ctx->ilog_jrn_no, jrn_no, LEN_JRN_NO);
    memcpy(ctx->jrn_no     , jrn_no, LEN_JRN_NO);

    SYS_TREF;

    return ERR_NONE;

}

/* ---------------------------------------------------------------- */

static int e200_set_session(symqtop_onout_ctx_t *ctx)
{
    int          rc                 = ERR_NONE;

    SYS_TRSF;

    /* set session to list */
    SESSION = set_session_to_list(ctx->ilog_jrn_no, SESSION_DATA_SIZE);
    if (SESSION == 0x00){
        /* case of max session */
        e210_session_err_proc(ctx);
        return ERR_ERR;
    }

    memcpy(SESSION->jrn_no,  ctx->jrn_no,  LEN_JRN_NO);

    SYS_TREF;

    return ERR_NONE;

}

/* ---------------------------------------------------------------- */
static int e210_session_err_proc(symqtop_onout_ctx_t    *ctx)
{
    
    int                 rc       = ERR_NONE;
    int                 size     = 0;
    req_data_t          *as_data = 0;
    int                 req_len  = 0;

    SYS_TRSF;

    size = sizeof(session_t);
    SESSION = malloc(size);
    if (SESSION =0x00) {
        SYS_HSTERR(SYS_LN, SYS_GENERR, "malloc failed size[%d] ilog_jrn_no[%s]", size, ctx->ilog_jrn_no);
        return ERR_ERR;
    }

    memset(SESSION, 0x00, size);

    SESSION->data = malloc(SESSION_DATA_SIZE);
    if (SESSION_DATA = 0x00){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "malloc failed size[%d] ilog_jrn_no[%s]", SESSION_DATA_SIZE, ctx->ilog_jrn_no);

    }

    memset(SESSION_DATA, 0x00, SESSION_DATA_SIZE);

    SESSION->cd = SESSION_INIT_CD;
    memcpy(SESSION->ilog_jrn_no,  ctx->ilog_jrn_no, LEN_JRN_NO);
    memcpy(SESSION->jrn_no     ,  ctx->jrn_no     , LEN_JRN_NO);
    time(&SESSION->tval);

    SESSION_DATA->log_flag = 1;

    SYS_TREF;

    return ERR_NONE;

}

/* ---------------------------------------------------------------- */
static int e300_set_commbuff(symqtop_onout_ctx_t    *ctx)
{
    int                 rc                 = ERR_NONE;
    int                 len, len2;
    sysicomm_t          sysicomm           = {0};
    req_data_t          *req_data          = 0;
    req_data_t          *as_data           = 0;
    exi0212_t           exi0212;
    exmsg1200_comm_t    *exmsg1200;
    sysgwinfo_t         sysgwinfo;
    int                 req_len            = 0;
    int                 data_len           = 0;
    char                *dp;
    char                buff[256 + 1];
    char                call_svc_name[LEN_EXPARAM_AP_SVC_NAME + 1];

    SYS_TRSF;

    /* error code init */
    sys_err_init();

    /* SYSIOUTQ 값 저장 */
    req_data = SYSIOUTQ;
    req_len  = SYSIOUTQ_SIZE;

    /* ------------- */
    /* SYSGWINFO     */
    /* ------------- */
    memset(&sysgwinfo, 0x00, sizeof(sysgwinfo_t));
    sysgwinfo.sys_type     = SYSGWINFO_SYS_CORE_BANK;
    sysgwinfo.gw_rspn_send = SYSGWINFO_GW_REPLY_SEND_NO;
    sysgwinfo.msg_type     = SYSGWINFO_MSG_1200;

    rc = sysocbsi(&SESSION_DATA->cb, IDX_SYSGWINFO, &sysgwinfo, LEN_SYSGWINFO);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s e300_set_commbuff():"
                             "COMMBUFF ERR(SYSGWINFO) [해결방안] 시스템 담당자 call ",
                             __FILE__);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "COMMBUFF SET ERR");
        return ERR_ERR;
    }

    as_data = malloc(req_len+1);
    if (as_data = 0x00){
        SYS_HSTERR(SYS_NN, SYS_GENERR, "malloc failed. jrn_no[%s]", SESSION->ilog_jrn_no);
        return ERR_ERR;
    }

    memset(as_data, 0x00, req_len+1);
    memset(as_data, 0x20, req_len);

    SESSION_DATA->as_data = as_data;

    as_data->hcmihead = req_data->hcmihead;

    SESSION_DATA->log_flag = 1;

    //as_data->hcmihead = req_data
    SYS_DBG(" == SYSIOUTQ_SIZE [%d]", SYSIOUTQ_SIZE);
    /* check data length */
    data_len = utoa2ln(as_data->hcmihead.data_len, LEN_HCMIHEAD_DATA_LEN);
    SYS_DBG("as_data->hcmihead.data_len: [%d]", data_len);
    /*
    if (as_data != SYSIOUTQ_SIZE - LEN_HCMIHEAD){
        SYS_HSTERR(SYS_NN, SYS_GENERR, "as_data->hcmihead.data_len[%s]", SESSION->ilog_jrn_no);
        return ERR_ERR;
    }
    */
    SESSION_DATA->hcmihead = as_data->hcmihead;
    /* dp 헤더 와 실제데이터 저장 */
    dp = &as_data->hcmihead;

    memcpy(as_data->indata, req_data->indata, (req_len - LEN_HCMIHEAD));
    memcpy(&dp[g_head_len],  as_data->indata, (req_len - LEN_HCMIHEAD));
    utohexdp((char *)dp, 1272);

    /* 거래 파라미터 정보 */
    memset(&exi0212, 0x00, sizeof(exi0212_t));
    exi0212.in.func_code = 2;

    exmsg1200 = (exmsg1200_comm_t) &dp[g_head_len];
    memcpy(exi0212.in.appl_code, exmsg1200->appl_code, LEN_APPL_CODE);
    memcpy(exi0212.in.tx_code  , exmsg1200->tx_code  , LEN_TX_CODE  );

    /* -------------------------------------------------------- */
    SYS_DBG("e300_set_commbuff appl_code = [%s]", exi0212.in.appl_code);
    SYS_DBG("e300_set_commbuff tx_code   = [%s]", exi0212.in.tx_code  );
    /* ------------------------------------------------------- */

}
