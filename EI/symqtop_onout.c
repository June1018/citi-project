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
    int                 g_header_len;                     /* 통신헤더 길이 */

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
    g_header_len = sizeof(hcmihead_t);
    /* -------------------------------------------------------------- */
    SYS_DBG("a000_initial: header_len = [%d]", g_header_len);
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
    memcpy(&dp[g_header_len],  as_data->indata, (req_len - LEN_HCMIHEAD));
    utohexdp((char *)dp, 1272);

    /* 거래 파라미터 정보 */
    memset(&exi0212, 0x00, sizeof(exi0212_t));
    exi0212.in.func_code = 2;

    exmsg1200 = (exmsg1200_comm_t) &dp[g_header_len];
    memcpy(exi0212.in.appl_code, exmsg1200->appl_code, LEN_APPL_CODE);
    memcpy(exi0212.in.tx_code  , exmsg1200->tx_code  , LEN_TX_CODE  );

    /* -------------------------------------------------------- */
    SYS_DBG("e300_set_commbuff appl_code = [%s]", exi0212.in.appl_code);
    SYS_DBG("e300_set_commbuff tx_code   = [%s]", exi0212.in.tx_code  );
    /* ------------------------------------------------------- */

    rc = exparam_hash_algorism(&exi0212);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s e300_set_commbuff() :"
                             "TX_CODE NOT FOUND [해결방안 ]업무 담당자 call",
                             __FILE__);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "TX_CODE NOT FOUND");
        //z000_free_session(ctx);  --error는 catch문으로 
        return ERR_ERR;
    }

    /* 호스트 응답여부 */
    if ((exi0212.out.param.host_rspn_flag[0] == '1') ||
        (exparam->rspn_flag[0]               == '1'))
        exi0212.out.param.host_rspn_flag[0] = '1';
    else 
        exi0212.out.param.host_rspn_flag[1] = '0';

    //memcpy(linked->tx_code, exi0212.out.param.tx_code, LEN_TX_CODE);
    //memcpy(linked->svc_name, svc_name, MAX_LEN_SVC_NAME);
    time(&SESSION->tval);

    rc = sysocbsi(&SESSION_DATA->cb, IDX_EXPARM, &exi0212.out.param, sizeof(exparam_t));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s e300_set_commbuff() :"
                             "SET BUFFER ERROR [해결방안 ]업무 담당자 call",
                             __FILE__);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "SET BUFFER ERROR");
        return ERR_ERR; 
    }
    
    SYS_DBG("EXPARAM->host_rspn_flag [%s]", ((exparm_t *) sysocbgp(&SESSION_DATA->cb, IDX_EXPARM))->host_rspn_flag);

    /* ------------- */
    /* SYSICOMM      */
    /* ------------- */
    sysicomm.intl_tx_flag   = 0;
    sysicomm.xa_flag        = 0;
    memcpy(sysicomm.ilog_no, SESSION->ilog_jrn_no, LEN_JRN_NO);
    strcpy(sysicomm.ilog_no, SESSION->jrn_no     , LEN_JRN_NO);

    /* service call */
    memset(call_svc_name, 0x00, sizeof(call_svc_name));
    memcpy(call_svc_name, exi0212.out.exparam.ap_svc_name, LEN_EXPARAM_AP_SVC_NAME);
    utortrim(call_svc_name);
    strcpy(sysicomm.call_svc_name, call_svc_name);

    SYS_DBG("sysicomm.call_svc_name [%s]", sysicomm.call_svc_name);

#ifdef _DEBUG
    //PRINT_SYSICOMM(&sysicomm);
#endif 

    sysocbsi(&SESSION_DATA->cb, IDX_SYSICOMM, &sysicomm, sizeof(sysicomm_t));
    if (rc == ERR_ERR) {
        SYS_HSTERR(SYS_LN, SYS_GENERR, "sysocbsi(SYSICOMM) failed jrn_no[%s]", SESSION->ilog_jrn_no);
        return ERR_ERR;
    }

    /* set exmsg1200 commbuff */
    rc = sysocbsi(&SESSION_DATA->cb, IDX_EXMSG1200, &dp[g_header_len], (req_len - LEN_HCMIHEAD));
    if (rc == ERR_ERR) {

        ex_syslog(LOG_ERROR, "[APPL_DM] %s e300_set_commbuff():"
                             "COMMBUFF ERR [EXMSG1200] 업무 담당자 call", __FILE__ );
        SYS_HSTERR(SYS_NN, SYS_GENERR, "COMMBUFF SET ERR");
        return ERR_ERR;
    }

    rc = sysocbsi(&SESSION_DATA->cb, IDX_TCPHEAD, &dp, (req_len - g_header_len));
    if (rc == ERR_ERR) {

        ex_syslog(LOG_ERROR, "[APPL_DM] %s e300_set_commbuff():"
                             "COMMBUFF ERR [TCPHEAD] 업무 담당자 call", __FILE__ );
        SYS_HSTERR(SYS_NN, SYS_GENERR, "COMMBUFF SET ERR");
        return ERR_ERR;
    }

    SESSION_DATA->as_data = as_data; //헤더 포함 
    SESSION_DATA->req_len = SYSIOUTQ_SIZE;

    SESSION->cd = SESSION_READY_CD;

    SYS_TREF;

    return ERR_NONE;

}
/* ---------------------------------------------------------------- */
int f100_tmax_reply_msg(UCSMSGINFO *reply)
{
    int                 rc    , len,      len2;
    int                 msg_type;
    char                *dp;
    session_t           *session;
    linked_t            *linked;
    hcmihead_t          *hcmihead;
    exi0280_t           exi0280;
    fxmsg_t             *fxmsg;
    exmsg1500_t         *exmsg1500;
    exmsg1200_comm_t    *exmsg1200;

    SYS_TRSF;

    /* error code 초기화 */
    sys_err_init();
    SYS_DBG("============================ 취급요청 callback Start =========================");

    //memset(ctx,   0x00, sizeof(ctx));
    memset(&g_commbuff, 0x00, sizeof(g_commbuff));
    ctx->cb = &g_commbuff;

    SESSION = get_session_from_list_by(reply->cd);
    if (SESSION == 0x00) {
        SYS_HSTERR(SYS_LN, SYS_GENERR, "    SESSION = get_session_from_list_by(reply->cd[%s]", reply->cd);
        return ERR_ERR;

    }

    /* 응답데이터 있으면 commbuff 설정 */
    if (reply->len > 0){
        rc = sysocbsb(ctx->cb, reply->data, reply->len);
        if (rc = ERR_ERR) {
            return ERR_ERR;

        }
    }

    memcpy(ctx>jrn_no,      SESSION->jrn_no,      LEN_JRN_NO);
    memcpy(ctx>ilog_jrn_no, SESSION->ilog_jrn_no, LEN_JRN_NO);

    /* ----------------------------------------------------------- */
    SYS_DBG("f100_tmax_reply_msg cd [%d] errno[%d] urcode[%d]   len[%d]", reply->cd, reply->errcode, reply->urcode, reply->len);
    SYS_DBG("f100_tmax_reply_msg (&SESSION_DATA->cb):host_rspn_flag[%s]", ((exparm_t *) sysocbgp(&SESSION_DATA->cb, IDX_EXPARM))->host_rspn_flag);
    SYS_DBG("f100_tmax_reply_msg  (ctx->cb):host_rspn_flag[%s]", ((exparm_t *) sysocbgp(ctx->cb, IDX_EXPARM))->host_rspn_flag);
    /* ----------------------------------------------------------- */

    SYS_TRY(f200_check_reply_msg(ctx, reply));

    SYS_TRY(f300_make_res_data_proc(ctx));

    SYS_TREF;

    SYS_DBG("============================ 취급요청 callback End =========================");

    return ERR_NONE;

SYS_CATCH:

    SYS_DBG("============================ 취급요청 callback End =========================");

    x000_error_proc(ctx);

    return ERR_ERR;
}

/* ---------------------------------------------------------------- */
static int f200_check_reply_msg(symqtop_onout_ctx_t *ctx, UCSMSGINFO *reply)
{

    int                 rc    = ERR_NONE;
    int                 len, size;
    int                 msg_type;
    char                *dp;
    char                tran_id[50], buff[10];
    char                host_rspn_flag[2];
    session_t           *session;
    linked_t            *linked;
    hcmihead_t          *hcmihead;
    exi0280_t           exi0280;
    fxmsg_t             *fxmsg;
    exmsg1500_t         *exmsg1500;
    exmsg1200_comm_t    *exmsg1200;

    SYS_TRSF;

    if (reply->len > 0){

            /* G/W Commbuff 항목 검증 */
            if (SYSGWINFO == NULL) {
                SYS_HSTERR(SYS_LN, SYS_GENERR, "[%s SYSGWINFO is NULL, jrn_no[%s]", __FILE__, SESSION->ilog_jrn_no);
                return ERR_ERR;
            }
            /* --------------------------------------- */
            PRINT_SYSGWINFO(SYSGWINFO);
            /* --------------------------------------- */

        /* 호스트 응답여부 검증 gw_rspn_send가 1이면 SYSGWINFO_GW_REPLY_SEND_NO이며 응답안받는 거래 */
        if (IS_SYSGWINFO_GW_REPLY_SEND_NO){
            //z100_free_session(ctx); 밖에 기존재 
            SYS_DBG("IS_SYSGWINFO_GW_REPLY_SEND_NO is TRUE (Core에  callback 미전송 )");
            return ERR_ERR;
        }

        /* NULL을 trim해야 하는 응답 */
        if ((SYSIOUTQ != NULL) && (SYSGWINFO->msg_type == SYSGWINFO_MSG_1200)){
            memset(&exi0280, 0x00, sizeof(exi0280_t));
            exi0280.in.type = 2;
            memcpy(exi0280.in.data, SYSIOUTQ, sysocbgs(ctx->cb, IDX_SYSIOUTQ));
            exmsg1200_null_proc(&exi0280);

            rc = sysocbsi(ctx->cb, IDX_SYSIOUTQ, exi0280.out.data, LEN_EXMSG1200_COMM);
            if (rc == ERR_ERR){
                SYS_DBG("NULL 처리 실패" );
                sys_err_init();
                return ERR_ERR;
            }
        }

        SYS_DBG( "[f200_check_reply_msg] exi0280.out.data [%s]", exi0280.out.data);

        /* 임시버퍼에 전송할 데이터 보관 */
        g_temp_len(g_temp_buf, SYSIOUTQ, g_temp_len);
        memcpy(g_temp_buf, SYSIOUTQ, g_temp_len);
        msg_type = SYSGWINFO->msg_type;

    }else{

        /* check error: 서버 비정상 종료나 또는 호출 에러 */
        if (reply->errcode > 0){
            if(reply->errcode == TPESVRDOWN) {
                ex_syslog(LOG_FATAL, "[APPL_DM] SERVER ABNORMAL DOWN:" 
                                     "[해결방안]  업무담당자 call" );
                ex_syslog(LOG_FATAL, "[APPL_DM] %s SERVER ABNORMAL DOWN:" 
                                     "svc[%s][해결방안]  업무담당자 call" , __FILE__, g.svc_name);
            }else{
                ex_syslog(LOG_FATAL, "[APPL_DM] SERVER CALL ERR : TPERRNO[%d]" 
                                     "svc[%s][해결방안]  업무담당자 call"
                                     , __FILE__, reply->errcode, g.svc_name );
                
            }
        }

        /* 서비스 비정상 종료나 또는 서비스가 없는 경우 호스트에서 응답처리 */
        //SYS_HSTERR(SYS_NN, SYS_GENERR ,"SERVICE CALL ERR");
        memset(g_temp_buf, 0x00, sizeof(g_temp_buf));
        SYS_DBG("test 11[%s]", SESSION_DATA->as_data);
        SYS_DBG("test 11 req_len[%d]", SESSION_DATA->req_len);

        memcpy(&g_temp_buf, &SESSION_DATA0>as_data[g_header_len], (SESSION_DATA->req_len -g_header_len));
        SYS_DBG("처음 f200_check_reply_msg: g_temp_buf[%s]", g_temp_buf);

        g_temp_len = SESSION_DATA->req_len - g_header_len;

        SYS_DBG("g_temp_len[%d]", g_temp_len);

        exmsg1200 = (exmsg1200_comm_t *) g_temp_buf;
        if (memcmp(exmsg1200->appl_code, "001", LEN_APPL_CODE) == 0) {
            exmsg1500 = (exmsg1500_t *) g_temp_buf;
            SET_1500ERROR_INFO(exmsg1500);
            msg_type = SYSGWINFO_MSG_1500;
        }else if (memcmp(exmsg1200->appl_code, "020" , LEN_APPL_CODE) == 0 ){
            fxmsg = (fxmsg_t *) g_temp_buf;
            SET_ERROR_INFO_FX(fxmsg);
            msg_type = SYSGWINFO_MSG_ETC;
        }else {
            SET_ERROR_INFO(exmsg1200);
            msg_type = SYSGWINFO_MSG_1200;
        }
    }

    /* ----------------------------------------------------------- */
    SYS_DBG("f200_check_reply_msg err_code [%d] err_msg[%s]"    , sys_err_code(), sys_err_msg());
    SYS_DBG("f200_check_reply_msg send_len [%d]", g_temp_len);
    SYS_DBG("f200_check_reply_msg host_rspn_flag[%s]", ((exparm_t *) sysocbgp(ctx->cb, IDX_EXPARM))->host_rspn_flag);
    //SYS_DBG("f200_check_reply_msg send_len [%d]", g_temp_len);
    SYS_DBG("f200_check_reply_msg g_temp_buff [%s]", g_temp_buff);
    /* ----------------------------------------------------------- */

    if (SYSIOUTH) {
        g_arch_hear.err_code = SYSIOUTH->err_code;
        g_arch_head.err_type = SYSIOUTH->err_type;
        memcpy(g_arch_head.err_msg,         SYSIOUTH->err_msg,          MAX_LEN_ERR_MSG);
        memcpy(g_appl_code.err_file_name,   SYSIOUTH->eff_file_name,    MAX_LEN_ERR_MSG);
    }

    //memcpy(g_call_svc_name, ((sysicomm_t *) sysocbgp(&SESSION_DATA->cb, IDX_SYSICOMM))->call_svc_name, sizeof(g_call_svc_name));
    /* ----------------------------------------------------------- */
    SYS_DBG("f200_check_reply_msg err_code [%d] err_msg[%s]"    , sys_err_code(), sys_err_msg());
    SYS_DBG("f200_check_reply_msg send_len [%d]", g_temp_len);
    SYS_DBG("f200_check_reply_msg host_rspn_flag[%s]", ((exparm_t *) sysocbgp(ctx->cb, IDX_EXPARM))->host_rspn_flag);
    SYS_DBG("f200_check_reply_msg g_temp_buff [%s]", g_temp_buff);
    /* ----------------------------------------------------------- */

    strcpy(host_rspn_flag, ((exparam_t *) sysocbgp(&SESSION_DATA->cb, IDX_EXPARM))->host_rspn_flag);

    SYS_DBG("f200_check_reply_msg : host_rspn_flag [%s]", host_rspn_flag);

    /* 호스트에 응답여부 검증 */
    SYS_DBG("호스트에 응답여부 직전 host_rspn_flag 1이 아니면 에러 리턴 host_rspn_flag[%s]"m host_rspn_flag);
    if (host_rspn_flag[0] != '1'){
        SYS_DBG("f200_check_reply_msg : ((exparam_t *) sysocbgp(&SESSION_DATA->cb, IDX_EXPARM))-> host_rspn_flag [%s]", host_rspn_flag);
        sys_err_init();
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;

}

/* ---------------------------------------------------------------- */
int static f300_make_res_data_proc(symqtop_onout_ctx_t *ctx)
{
    int                 rc    = ERR_NONE;
    int                 data_len    = 0;
    int                 res_len     = 0;  
    int                 len, size;
    res_data_t          *res_data;
    hcmihead_t          *hcmihead;
    char                msg_id[20]  = {0};
    char                *dp, *dp2;

    SYS_TRSF;

    SYS_DBG("<========DATA(SYSIOUTQ) from SVC ========>")
#ifdef _DEBUG
    utohexdp(SYSIOUTQ, SYSIOUTQ_SIZE);
#endif

    if (sys_err_code() != ERR_NONE) {
        SYS_DBG("jrn_no[%s] err_code[%d] err_msg[%s]", SESSION->ilog_jrn_no, sys_err_code(), sys_err_msg());
    }

    res_len = LEN_HCMIHEAD + SYSIOUTQ_SIZE; // 72 + 전문 
    g_temp_len =SYSIOUTQ_SIZE;  //전문만

    res_data = malloc(res_len + 100);
    if (res_data == 0x00) {
        SYS_HSTERR(SYS_LC, SYS_GENERR, "malloc failed len [%d] jrn_no[%s]", res_len, SESSION->ilog_jrn_no);
        return ERR_ERR;
    }

    memset(res_data, 0x00, res_len + 100);
    memset(res_data, 0x20, res_len);

    memcpy(&res_data->hcmihead, &SESSION_DATA->hcmihead, LEN_HCMIHEAD);

    SYS_DBG("SESSION_DATA->hcmihead.data_len : [%d]", SESSION_DATA->hcmihead.data_len);

    //TOPS로 전송할 24바이트 corr_id조립 (20바이트 앞에 0000을 추가)
    memset(ctx->corr_id,   0x00, sizeof(ctx->corr_id));
    memcpy(ctx->corr_id, "0000", 4);
    memcpy(&ctx->corr_id[4], res_data->hcmihead.queue_name, LEN_HCMIHEAD_QUEUE_NAME);

    SYS_DBG("(ctx->corr_id: [%s]", (ctx->corr_id);

    if (SYSIOUTQ && SYSIOUTQ_SIZE > 0) {
        memcpy(res_data->outdata, SYSIOUTQ, SYSIOUTQ_SIZE);
        data_len += SYSIOUTQ_SIZE;
    }

    //utol2an(data_len, LEN_HCMIHEAD_QUEUE_NAME, res_data->hcmihead.data_len);
    memcpy(res_Data->hcmihead.resp_code, "000", LEN_HCMIHEAD_RESP_CODE);
    res_data->hcmihead.conn_type = CONT_TYPE_END;
    //res_data->hcmihead.conn_type = CONT_TYPE_END;

    /* 호스트 요청에 대한 응답시 거래코드는 항상 UTRY로 응답 */
    //memset(buff, 0x00, sizeof(buff));
    //memcpy(buff, HOST_RSPN_TRANS_ID, strlen(HOST_RSPN_TRANS_ID));

    //memcpy(res_data->hcmihead.tx_code, ((exparam_t *) sysocbgp(&SESSION_DATA->cb ,IDX_EXPARM))->tx_code);
    memcpy(res_data->hcmihead.tran_id, HOST_RSPN_TRANS_ID, LEN_HCMIHEAD_TRANS_ID);
    utocick(msg_id);
    //memcpyres_data->hcmihead.queue_name, corr_id, LEN_HCMIHEAD_QUEUE_NAME);
    // ((exparam_t *) sysocbgp(&SESSION_DATA->cb ,IDX_EXPARM))->host_rspn_flag
    /* --------------------------------------- */
    PRINT_HCMIHEAD(&res_data->hcmihead);
    /* --------------------------------------- */

    /* 전문 변환 */
    memset(&g_temp_buf, 0x00, sizeof(g_temp_buf));
    memset(&g_temp_buf, 0x20, g_header_len);

    //size = g_temp_len + g_header_len +256;
    size = ctx->mqinfo.msglen +256;
    dp = (char *) malloc(size);
    if (dp == NULL){
        ex_syslog(LOG_FATAL, "[APPL_DM] 메모리 할당 에러 :" 
                             "[해결방안]  시스템담당자 call" );
        ex_syslog(LOG_FATAL, "[APPL_DM] %s  메모리 할당 에러 %d:" 
                             "[해결방안]  시스템담당자 call" , __FILE__, size);
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "MEMORY ALLOC ERR");
        return ERR_ERR;
    }
    memset(dp, 0x00, sizeof(dp));

    memcpy(g_temp_buf, &res_data->hcmihead, g_header_len);
    //memcpy(&g_temp_buf[14], &res_data->hcmihead.cont_type  , 1);
    //memcpy(&g_temp_buf[15], &res_data->hcmihead.data_len   , LEN_HCMIHEAD_DATA_LEN);
    //memcpy(&g_temp_buf[21], &res_data->hcmihead.tran_id    , LEN_HCMIHEAD_TRAN_ID);
    //memcpy(&g_temp_buf[25], &res_data->hcmihead.queue_name , LEN_HCMIHEAD_QUEUE_NAME);
    //memcpy(&g_temp_buf[45], &res_data->hcmihead.resp_code  , LEN_HCMIHEAD_RESP_CODE);

    memcpy(&g_temp_buf[g_header_len], &res_data->outdata  , (ctx->mqinfo.msglen - g_header_len));

    //call UTRY 세팅 
    memcpy(&g_temp_buf[g_header_len], HOST_RSPN_TRANS_ID  , strlen(HOST_RSPN_TRANS_ID));

    SYS_DBG(" 변환전 full_data [%.*s]", ctx->mqinfo.msglen, (char *)g_temp_buf);

    //전문 변환전 로그 기록 
    if (g100_insert_exmqgwlog(ctx) == ERR_ERR){
        ex_syslog(LOG_ERROR, "g100_insert_exmqgwlog() FAIL [해결방안 ]시스템 담당자 call");
        sysocbfb(ctx->cb);
        if(dp)
        {
            free(dp)
            return ERR_ERR;
        }
        return ERR_ERR;
    }

    /* 데이터 코드 변환 */
    //utoas2eb((unsinged char *)g_temp_buf, g_header_len, (unsinged char *)dp);
    memcpy(dp, g_temp_buf, g_header_len);

    host_data_conv(AS_TO_BE, g_temp_buf, ctx->mqinfo.msglen, (int)SYSGWINFO->msg_type);
    memcpy(&dp[g_header_len], &g_temp_buf[g_header_len], g_temp_len);
    len = ctx->mqinfo.msglen;

    /* 변환후 데이티 로그 */
    /* ---------------------------------------------------------------------------- */
    SYS_DBG("변환후 full_datas[%.*s]", ctx->mqinfo.msglen, (char *)dp);

#ifdef _DEBUG
    /* ---------------------------------------------------------------------------- */
    utohexdp(dp, len);
    /* PRINT_HCMIHEAD(&req_data->hcmihead); */
    /* ---------------------------------------------------------------------------- */

#endif

    /* mq msgbuf set */
    memcpy(ctx->mqinfo.msgbuf, dp, ctx->mqinfo.msglen);

    /* mq put */
    if (f400_put_mqmsg(ctx, res_data) !== ERR_NONE) {
        SYS_DBG("put error");
        if (dp) {
            free(dp);
        }
        return ERR_ERR;
    }

    if(dp)
       free(dp);

    f500_reply_to_gw_proc(ctx, res_data, res_len);

    SYS_TREF;

    return ERR_NONE;
}

/* ---------------------------------------------------------------- */
static int f400_put_mqmsg(symqtop_onout_ctx_t *ctx, res_data_t *res_data)
{
    int                 rc    = ERR_NONE;
    mq_info_t           *mqinfo;    = {0};
    res_data_t          _res_data   = {0};
    MQMD                md          = {MQMD_DEFAULT};
    int                 len, size;
    int                 g_temp_len;
    char                *dp, *dp2;

    SYS_TRSF;

    SYS_DBG( " put method &ctx->mqhobj: [%s]", &ctx->mqhobj);
    SYS_DBG( " put method ctx->mqhconn: [%s]", ctx->mqhconn);
    SYS_DBG( " put method ctx->mqhconn: [%d]", EXMQPARAM->charset_id);

    /* MQ 기본정보 세팅 flag 1 이면 통과함 */
    rc = b100_mqparam_load(ctx);
    if (rc != ERR_ERR) {
        SYS_DBG("b100_mqparam_load faile +++++++++++++++++");
        ex_syslog(LOG_FATAL, "[APPL_DM] b100_mqparam_load() FAIL [해결방안 ]시스템 담당자 call");
        ctx->mqparam_load_flag = 0;
        return rc;
    }

    rc = d100_init_mqcon(ctx);
    if (rc == ERR_ERR) {
        SYS_DBG("d100_init_mqcon failed STEP 1");
        ex_syslog(LOG_FATAL, "[APPL_DM] d100_init_mqcon() STEP 1 FAIL [해결방안 ]시스템 담당자 call");
        ctx->is_mq_connected = 0;
        return rc;

    }

    mqinfo                  = ctx->mqinfo;
    mqinfo.hobj             = &ctx->mqhobj;             /* put queue object */
    mqinfo.hcon             = ctx->mqhconn;             /* QM Connectiopn   */
    mqinfo.coded_charset_id = EXMQPARAM->charset_id;    /* CodedcharsetId */

    //tops로 송신할 correlation ID 설정 (기존 queue_name 20바이트 앞에 0000을 추가해서 24바이트 설정 
    memcpy(mqinfo.corrid, ctx->corr_id, LEN_EXMQMSG_001_CORR_ID);

    SYS_DBG("mqinfo.corrid[%s]", mqinfo.corrid);
    SYS_DBG("ilog_jrn_no  [%s]", ctx->ilog_jrn_no);

    SYS_DBG("최종 mqinfo.msgbuf: [%s]", mqinfo.msgbuf);

    rc = ex_mq_put_with_opts(&mqinfo, &md, 0);
    if (rc == ERR_NONE) {
        rc = ex_mq_commit(&ctx->mqhconn);
    }else{
        /* rest flag for mq initialize */
        ctx->is_mq_connected = 0;

        SYS_DBG("CALL ex_mq_put FAIL !!!");
        ex_syslog(LOG_FATAL, "[APPL_DM] call ex_mq_put FAIL [해결방안 ]시스템 담당자 call");
        //ex_mq_rollback(&ctx->mqhconn);
        rc = ex_mq_commit(&ctx->mqhconn);

        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;
}
/* ---------------------------------------------------------------- */
static int f500_reply_to_gw_proc(symqtop_onout_ctx_t *ctx, char *out, int len)
{
    
    int                 rc    = ERR_NONE;

    SYS_TRSF;

    SYS_DBG("<======== MQ Put 후 commbuff 및 SESSION clear 시작  ========>");
    //utohexdp(out, len);

    if (SESSION = 0x00){
        SYS_DBG("SESSION is null");
        z000_free_session(ctx);

        if (out){
            free(out);
        }
        return
    }

    z000_free_session(ctx);

    if (out){
        free(out);
    }

    SYS_TREF;

}
/* ---------------------------------------------------------------- */
static int g100_insert_exmqgwlog(symqtop_onout_ctx_t *ctx)
{

    int             db_rc       = ERR_NONE;
    mq_info_t       mqinfo      = {0};
    MQMD            md          = {MQMD_DEFAULT};
    exmqmsg_001_t   mqmsg_001;
    mqimsg001_t     mqimsg001;

    SYS_TRSF;

    /* --------------------MQ 기본 정보 세팅 --------------------------*/
    mqinfo          = ctx->mqinfo;

    utocick(mqinfo.msgid);

    //log 저장할 correlation ID 설정  (기존 queue_name 20바이트 앞에 0000을 추가해서 24바이트 설정 
    memcpy(mqinfo.corrid, ctx->corr_id, LEN_EXMQMSG_001_CORR_ID);

    SYS_DBG("mqinfo.corrid[%s]", mqinfo.corrid);
    
    mqinfo.msglen = ctx->mqinfo.msglen;
    memcpy(mqinfo.msgbuf, g_temp_buf, mqinfo.msglen);

    /* ------------------ mqmsg_001 세팅 (로그정보 ) ----------------- */
    memset(&mqmsg_001, 0x00, sizeof(exmqmsg_001_t));
    memcpy(mqmsg_001.chnl_code, g_chnl_code, LEN_EXMQPARM_CHNL_CODE );
    memcpy(mqmsg_001.appl_code, g_appl_code, LEN_EXMQPARM_APPL_CODE );
    memcpy(mqmsg_001.msg_id  , mqinfo.msgid, LEN_EXMQMSG_001_MSG_ID );
    memcpy(mqmsg_001.corr_id, mqinfo.corrid, LEN_EXMQMSG_001_CORR_ID );
    mqmsg_001.mqlen = mqinfo.msglen;
    memcpy(mqmsg_001.mqmsg   , mqinfo.msgbuf, mqmsg_001.mqlen );
    mqmsg_001.io_type = 1;  /* 0이면 GET 1이면 PUT */
    memcpy(mqmsg_001.ilog_jrn_no, SESSION->ilog_jrn_no, LEN_EXMQMSG_001_ILOG_JRN_NO);

    /* ------------------------ MQ 로그 정리 ------------------------ */
    memset(&mqimsg001, 0x00, sizeof(mqimsg001_t));
    mqimsg001.in.mqmsg_001 = &mqmsg_001;    /* mqimsg001 구조체에 mqmsg_001로 저장  */

    memcpy(mqimsg001.in.job_proc_type, "4", LEN_EXMQMSG_001_PROC_TYPE);

    db_rc = mqomsg001(&mqimsg001);
    if (db_rc == ERR_ERR) {
        EXEC SQL ROLLBACK WORK;
        ex_syslog(LOG_FATAL, "[APPL_DM] %s g100_insert_exmqgwlog: MQMSG001 INSERT ERROR call mqomsg001", __FILE__);
        g_db_connect = 0;
        return ERR_ERR;
    }
    EXEC SQL COMMIT WORK;


    SYS_TREF;

    return ERR_NONE;

}
/* ---------------------------------------------------------------- */
static int g110_insert_exmqgwlog(symqtop_onout_ctx_t *ctx)
{

    int             db_rc       = ERR_NONE;
    mq_info_t       mqinfo      = {0};
    MQMD            md          = {MQMD_DEFAULT};
    exmqmsg_001_t   mqmsg_001;
    mqimsg001_t     mqimsg001;

    SYS_TRSF;


    /* --------------------MQ 기본 정보 세팅 --------------------------*/
    mqinfo          = ctx->mqinfo;

    utocick(mqinfo.msgid);

    //log 저장할 correlation ID 설정  (기존 queue_name 20바이트 앞에 0000을 추가해서 24바이트 설정 
    memcpy(mqinfo.corrid, ctx->corr_id, LEN_EXMQMSG_001_CORR_ID);

    SYS_DBG("mqinfo.corrid[%s]", mqinfo.corrid);

    SYS_DBG("req_data->indata (ASCII)[%s]", ((req_data_t *) REQ_DATA)->indata);
    
    mqinfo.msglen = SESSION_DATA->req_len;
    memcpy(mqinfo.msgbuf, ((req_data_t *) REQ_DATA)->indata, mqinfo.msglen);

    /* ------------------ mqmsg_001 세팅 (로그정보 ) ----------------- */
    memset(&mqmsg_001, 0x00, sizeof(exmqmsg_001_t));
    memcpy(mqmsg_001.chnl_code, g_chnl_code, LEN_EXMQPARM_CHNL_CODE );
    memcpy(mqmsg_001.appl_code, g_appl_code, LEN_EXMQPARM_APPL_CODE );
    memcpy(mqmsg_001.msg_id  , mqinfo.msgid, LEN_EXMQMSG_001_MSG_ID );
    memcpy(mqmsg_001.corr_id, mqinfo.corrid, LEN_EXMQMSG_001_CORR_ID );
    mqmsg_001.mqlen = mqinfo.msglen;
    memcpy(mqmsg_001.mqmsg   , mqinfo.msgbuf, mqmsg_001.mqlen );
    mqmsg_001.io_type = 1;  /* 0이면 GET 1이면 PUT */
    memcpy(mqmsg_001.ilog_jrn_no, SESSION->ilog_jrn_no, LEN_EXMQMSG_001_ILOG_JRN_NO);

    /* ------------------------ MQ 로그 정리 ------------------------ */
    memset(&mqimsg001, 0x00, sizeof(mqimsg001_t));
    mqimsg001.in.mqmsg_001 = &mqmsg_001;    /* mqimsg001 구조체에 mqmsg_001로 저장  */

    memcpy(mqimsg001.in.job_proc_type, "4", LEN_EXMQMSG_001_PROC_TYPE);

    db_rc = mqomsg001(&mqimsg001);
    if (db_rc == ERR_ERR) {
        EXEC SQL ROLLBACK WORK;
        ex_syslog(LOG_FATAL, "[APPL_DM] %s g100_insert_exmqgwlog: MQMSG001 INSERT ERROR call mqomsg001", __FILE__);
        g_db_connect = 0;
        return ERR_ERR;
    }
    EXEC SQL COMMIT WORK;


    SYS_TREF;


    return ERR_NONE; 

}
/* ---------------------------------------------------------------- */
static int g200_get_tpctx(symqtop_onout_ctx_t *ctx)
{

    int                 rc    = ERR_NONE;

    SYS_TRSF;

    /* get tmax context to relay svc */
    if (tpgetctx(&SESSION_DATA->ctx) < 0){
        SYS_HSTERR(SYS_LN, 792900, "tpgetctx failed jrn_no[%s]", SESSION->ilog_jrn_no);
        return ERR_ERR;
    }

    //SESSION->cd = SESSION_READY_CD;
    rc = sys_tprelay("EXTRELAY", &SESSION_DATA->cb, 0, &SESSION_DATA->ctxt);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "sys_tprelay failed cd[%d] jrn_no[%s]", SESSION->cd, SESSION->ilog_jrn_no);
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;
}

/* ---------------------------------------------------------------- */
static int j100_set_session(symqtop_onout_ctx_t *ctx)
{

    int                 rc                      = ERR_NONE;
    char                jrn_no[LEN_JRN_NO+1]    = {0};      //27 자리 

    SYS_TRSF;

    /* 저널 번호 및 corr_id , msg_id 채번  */
    utoclck(jrn_no);

    memcpy(ctx->ilog_jrn_no, jrn_no+1, LEN_JRN_NO);
    memcpy(ctx->jrn_no     , jrn_no+1, LEN_JRN_NO);

    SYS_DBG("jrn_no [%s]", jrn_no);
    SYS_DBG("ctx->ilog_jrn_no[%s]", ctx->ilog_jrn_no);

    /* set session to list */
    SESSION = set_session_to_list(ctx->ilog_jrn_no, SESSION_DATA_SIZE);
    SYS_DBG("SESSION[%p]", SESSION);
    if (SESSION = 0x00){
        SYS_HSTERR(SYS_LN, sys_err_code(), "%s", sys_err_msg());
        return ERR_ERR;
    }

    memcpy(SESSION->jrn_no, ctx->jrn_no, LEN_JRN_NO);

    if(EXPARM){
        SESSION->wait_sec = atoi(EXPARM->time_val);  //EXPARM->time_val 값이 없으면 파라미터 값을 따른다. 
        //SESSION->wait_sec = 5;
    }

    SYS_DBG("SESSION_DATA %s", SESSION_DATA); //SESSION_DATA ctx->session

    /* save SVC Commbuff */
    rc = syocbdb(ctx->cb, &SESSION_DATA->cd);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, 8600, "syocbdb  failed jrn_no[%.27s] ", ctx->ilog_jrn_no);
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;
}

/* ---------------------------------------------------------------- */
static int j200_make_req_data(symqtop_onout_ctx_t *ctx)
{

    long                rc          = ERR_NONE;
    long                data_len    = 0;
    long                req_len     = 0; 
    int                 len;
    int                 size;
    char                *hp, *dp, *dp2, *dp3, buff[10];
    res_data_t          _res_data   = {0};
    exi0280_t           exi0280;
    hcmihead_t          *hcmihead;
    hcmihead_t          *recv_hcmihead;

    SYS_TRSF;

    memset(g_temp_buf, 0x00, sizeof(g_temp_buf));
    dp = g_temp_buf;

    /* 송신길이 검증 */
    len = sysocbgs(ctx->cb, IDX_HOSTSENDDATA);
    if (len <=0 ){
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "DATA NOT FOUND");
        return ERR_ERR;
    }
    
    /* ----------------------------------------------- */
    SYS_DBG("e200_make_req_data :data_len [%d]", len);
    /* ----------------------------------------------- */

    /* 송신할 데이터 */
    dp2 = (char *) sysocbgs(ctx->cb, IDX_HOSTSENDDATA);
    if (dp2 == NULL){
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "DATA NOT FOUND");
        return ERR_ERR;
    }
    SYS_DBG("HOSTSENDDATA :dp2 [%s]", dp2);

    SYSGWINFO->sys_type = SYSGWINFO_SYS_CORE_BANK;

    /* 1200message 경우 NULL TRIM */
    if (SYSGWINFO->msg_type == SYSGWINFO_MSG_1200){
        SYS_DBG("1200전문 NULL처리 ");
        memset(&exi0280, 0x00, sizeof(exi0280_t));
        exi0280.in.type = 2;
        memcpy(exi0280,in.data, dp2, LEN_EXMSG1200);
        exmsg1200_null_proc(&exi0280);

        /* 전송데이터 복사 */
        memcpy(&dp[g_header_len], exi0280.out.data, LEN_EXMSG1200_COMM);
        g_temp_len = LEN_EXMSG1200_COMM;

        /* 기타 데이터가 추가 되어 있는 경우 1304 보다 클때  */
        if (len > LEN_EXMSG1200){
            SYS_DBG("기타데이터 감지 ");
            memcpy(&dp[g_header_len + g_temp_len], &dp2[LEN_EXMSG1200], (len - LEN_EXMSG1200));
            g_temp_len += (len - LEN_EXMSG1200);
        }
    }else{
        SYS_DBG("기타 전문:[%s]", dp2);
        /* 전송데이터 복사 */
        memcpy(&dp[g_header_len], dp2, len);
        g_temp_len = len;
    }

    SYS_DBG("전송할 데이터 [%d][%.*s]", g_temp_len, g_temp_len, &dp[g_header_len]);

    /* ----------------------------------------------- */
    /* hcmihead                                        */
    /* ctx->session->req_data->hcmihead                */
    /* ----------------------------------------------- */
    /* 통신헤더 설정 */
    memset(dp, 0x20, g_header_len);

    hcmihead = (hcmihead_t *)dp;

    /* 응답으로 호스트에 전송하는 경우  */
    hp = sysocbgp(ctx->cb, IDX_TCPHEAD);
    if (hp != NULL){
        memcpy(dp, hp, g_header_len);
    }
    SYS_DBG("hp[%s]", hp);

    //TOPS로 전송할 24바이트 corr_id조립 (20바이트 앞에 0000을 추가)
    recv_hcmihead = (hcmihead_t *) hp;
    
    memset(ctx->corr_id,   0x00, sizeof(ctx->corr_id));
    memcpy(ctx->corr_id, "0000", 4);
    memcpy(&ctx->corr_id[4], res_data->hcmihead.queue_name, LEN_HCMIHEAD_QUEUE_NAME);

    SYS_DBG("(ctx->corr_id: [%s]", (ctx->corr_id);

    if (SYSGWINFO->msg_type == SYSGWINFO_MSG_1200) {
        memcpy(hcmihead->tx_code, &dp2[10],  LEN_TX_CODE);
    }else{
        memcpy(hcmihead->tx_code, &dp2[9],  LEN_TX_CODE);
    }
    hcmihead->comm_type = 'S';

    sprint(buff, "%03d", SYSGWINFO->time_val);
    memcpy(hcmihead->wait_sec, buff, LEN_HCMIHEAD_WAIT_TIME);
    hcmihead->conn_type = CONT_TYPE_END;

    sprint(buff, "%05d", g_temp_len);
    memcpy(hcmihead->data_len, buff, LEN_HCMIHEAD_DATA_LEN);
    hcmihead->cnvt_type = 0;

    memcpy(hcmihead->tran_id  , dp2, LEN_HCMIHEAD_TRANS_ID );
    memcpy(hcmihead->resp_code, '0', LEN_HCMIHEAD_RESP_CODE);
    

    /* 일련번호 채번 : 호스트로부터 응답을 받는 경우 */
    if (IS_SYSGWINFO_GW_REPLY) {
        g_call_seq_no++;
        if (g_call_seq_no > 5999999)
            g_call_seq_no = 1;
        sprintf(buff, "%07d", g_call_seq_no);
        memcpy(hcmihead->mint_rfk , buff, 7);
    }

    /* --------------------------------------------------------- */
    SYS_DBG("b000_make_send_data: input len[%d]", len); 
    SYS_DBG("b000_make_send_data: tx_code=[%10.10s] tran_id[%-4.4s]"
                            , hcmihead->tx_code, hcmihead->tran_id);

    PRINT_HCMIHEAD(hcmihead);

    /* ----------------- session 저장 --------------------------- */

    size = g_temp_len + g_temp_len + 256;
    //req_data = sys_tpcall("CARRY", req_len + 100);
    req_data = malloc(req_len + 100);
    if (req_data == NULL){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "sys_tpcall failed [%d jrn_no[%s]", len, SESSION->ilog_jrn_no);
        return ERR_ERR;
    }

    memset(req_data, 0x00, req_len + 100);
    memset(req_data, 0x20, req_len);

    if (SYSGWINFO->msg_type, == SYSGWINFO_MSG_1200){
        memcpy(req_data->hcmihead.tx_code, &dp2[10], LEN_TX_CODE);
    }else{
        memcpy(req_data->hcmihead.tx_code, &dp2[9] , LEN_TX_CODE);
    }
    req_data->hcmihead.comm_type = 'S';

    /* ----------------- session 저장 끝   ------------------------- */

    /* 전문 변환 */
    size = g_temp_len + g_header_len + 256;
    dp3 = (char *)malloc(size);
    if (dp3 == NULL){
        ex_syslog(LOG_FATAL, "[APPL_DM] syonthsend(): 메모리 할당 에러 "
                             "[해결방안 ] 시스템 담당자 call " );
        ex_syslog(LOG_FATAL, "[APPL_DM] %s syonthsend(): 메모리 할당 에러: %d "
                             "[해결방안 ] 시스템 담당자 call "
                             __FILE__, size );
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "MEMORY ALLOC ERR");
        return ERR_ERR;
    }

    /* ------------------------------------------------------------ */
    SYS_DBG("sizeof(hcmihead_t)   : [%d]",   sizeof(hcmihead_t));
    SYS_DBG("sizeof(g_header_len) : [%d]",   sizeof(g_header_len));
    /* header code 변환 */
    /* ------------------------------------------------------------ */
    SYS_DBG("변환전 dp=[%.*s]",  (g_header_len + g_temp_len), (char *)dp );
    SYS_DBG("변환전 hcmihead data=[%.*s] : [%d]",   sizeof(hcmihead_t), (char *)g_temp_buf);
    SYS_DBG("변환전 full_data    =[%.*s]",   (g_header_len, g_temp_len), (char *)g_temp_buf);
    /* ------------------------------------------------------------ */
       
    memcpy(req_data->indata, g_temp_buf, (g_header_len + g_temp_len));
    SESSION_DATA->req_data = req_data;
    SESSION_DATA->req_len  = len;  

    //전문변환 전 로그 기록     
    if (g110_insert_exmqgwlog(ctx) == ERR_ERR){
        ex_syslog(LOG_ERROR, "f300_insert_exmqgwlog FAIL 해결방안 시스템 담당자 call");
        sysocbfb(ctx->cb);
        return ERR_ERR;
    }

    //uto2aseb((unsinged char *)g_temp_buf, g_header_len, (unsigned char *)dp3);
    memcpy(dp3, g_temp_buf, g_header_len);

    /* 데이터 코드 변환 */
    host_data_conv(AS_TO_BE, g_temp_buf, (g_temp_len + g_header_len), (int)SYSGWINFO->msg_type);
    memcpy(&dp3[g_header_len], &g_temp_buf[g_header_len], g_temp_len); 
    len = g_header_len + g_temp_len;

    /* ------------------------------------------------------------ */
    SYS_DBG("변환후 dp=[%.*s]",  sizeof(hcmihead_t), (char *)dp3 );
    SYS_DBG("변환후 full_data    =[%.*s]",   (g_header_len, g_temp_len), (char *)dp3);
    /* ------------------------------------------------------------ */
    
#ifdef _DEBUG
    /* ---------------------------------------------------------------------------- */
    utohexdp(dp, len);
    /* PRINT_HCMIHEAD(&req_data->hcmihead); */
    /* ---------------------------------------------------------------------------- */
#endif

    SYS_DBG("len = [%d]", len);

    memset(req_data, 0x00, req_len + 100);
    memcpy(req_data->indata, dp3, len);
    SESSION_DATA->req_len  = len;
    
    SYS_TREF;

    return ERR_NONE;
}

/* ---------------------------------------------------------------- */
static int k100_put_mqmsg(symqtop_onout_ctx_t *ctx)
{

    int                 rc          = ERR_NONE;
    mq_info_t           mq_info     = {0};
    MQMD                md          = {MQMD_DEFAULT};
    int                 size, len;
    int                 g_temp_len;
    char                *dp, *dp2;

    SYS_TRSF;

    rc = b100_mqparam_load(ctx);
    if (rc == ERR_ERR){}
        SYS_DBG("b100_mqparam_load failed ++++++++++++++++++++");
        ex_syslog(LOG_ERROR, "b100_mqparam_load FAIL 해결방안 시스템 담당자 call");
        return rc;
    }

    rc = d100_init_mqcon(ctx);
    if (rc == ERR_ERR){
        SYS_DBG("d100_init_mqcon failed ++++++++++++++++++++");
        ex_syslog(LOG_ERROR, "d100_init_mqcon FAIL 해결방안 시스템 담당자 call");
        return rc;
    }

    /* MQ 기본정보 세팅 */
    mqinfo                  = ctx->mqinfo;
    mqinfo.hobj             = &ctx->mqhobj;             /* PUT QUEUE OBJECT */
    mqinfo.hcon             = ctx->mqhconn;             /* QM Connection    */
    mqinfo.coded_charset_id = EXMQPARAM->charset_id;    /* CodedCharSetID   */

    memcpy(mqinfo.msgid, mqinfo.msgid, LEN_EXMQMSG_001_MSG_ID);

    //TOPS로 송신할 correlation ID설정 (기본 queue_name 20바이트 앞에 0000을 추가해서 24바이트 설정 )
    memcpy(mqinfo.corrid,   ctx->corr_id, LEN_EXMQMSG_001_CORR_ID);

    SYS_DBG("mqinfo.corrid  = [%s]", mqinfo.corrid);
    SYS_DBG("ilog_jrn_no    = [%s]", ctx->ilog_jrn_no);

    memset(mqinfo.msgbuf, 0x00, sizeof(mqinfo.msgbuf));

    mqinfo.msglen   = SESSION_DATA->req_len;
    memcpy(mqinfo.msgbuf, ((req_data_t *) REQ_DATA)->indata, mqinfo.msglen);

    SYS_DBG("mqinfo.msglen  = [%d]", mqinfo.msglen);
    SYS_DBG("((req_data_t) REQ_DATA)->indata = [%s]", ((req_data_t *) REQ_DATA)->indata);

    rc = ex_mq_put_with_opts(&mqinfo, &md, 0);
    if (rc == ERR_NONE){
        rc = ex_mq_commit(&ctx->mqhconn);
    }else{
        /* rest flag for mq put FAIL !!! */
        ctx->is_mq_connected = 0;

        SYS_DBG("CALL ex_mq_put FAIL !!!");
        ex_syslog(LOG_FATAL, "[APPL_DM] CALL ex_mq_put FAIL [해결방안] 시스템 담당자 CALL");
        //ex_mq_rollback(&ctx->mqhconn);
        rc = ex_mq_commit(&ctx->mqhconn);

        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;
}

/* ---------------------------------------------------------------- */
static x000_error_proc(symqtop_onout_t *ctx)
{
    int                 rc          = ERR_NONE;
    int                 len         = 0;
    int                 len2        = 0;
    char                buff[900]   = {0};
    res_data_t          *out        = 0;
    hcmihead_t          *hcmihead   = 0;

    SYS_TRSF;

    if (SESSION == 0x00){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "SESSION is NULL err [%s]", sys_err_msg());
    
    }else{




    }

    z000_free_session(ctx);

    SYS_TREF;

    return;
}

/* ---------------------------------------------------------------- */
static int x100_db_connect(symqtop_onout_ctx_t *ctx)
{
    int  rc = ERR_NONE;

    if (g_db_connect != 1){
        rc = db_connect("");

        if (rc == ERR_NONE){
            g_db_connect = 1;
        }else{
            g_db_connect = 0;
            SYS_DBG("DB Connect fail");

            return ERR_ERR;
        }
    }

    return ERR_NONE;
}
/* ---------------------------------------------------------------- */
static int x200_log_req_data(symqtop_onout_ctx_t *ctx)
{

    int                 rc          = ERR_NONE;
    char          *as_data          = 0;
    req_data_t    *req_data         = 0;
    long           req_len          = 0;


    SYS_TRSF;

    req_data    = SYSIOUTQ;
    req_len     = SYSIOUTQ_SIZE;

    if (req_len < 1){
        return ERR_NONE;
    }

    if (SESSION && (SESSION_DATA->log_flag == 0)){
        as_data = malloc(req_len + 1);
        if (as_data = 0x00){
            SYS_HSTERR(SYS_LC, SYS_GENERR, "malloc failed jrn_no[%s]", SESSION->ilog_jrn_no);
            return ERR_ERR;
        }

        free(as_data);
    }

    SYS_TREF;

    return ERR_NONE;
}

/* ---------------------------------------------------------------- */
static void x300_error_proc(symqtop_onout_t *ctx)
{

    int                 rc          = ERR_NONE;

    SYS_TRSF;

    if (SESSION == 0x00){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "SESSION is NULL. err [%s]", sys_err_msg());
    }
    else{
        SYS_HSTERR(SYS_LN, sys_err_code(), ""SESSION is NULL. err "[%s]", sys_err_msg());

        /* service return */
        if (SESSION->cd == SESSION_READY_CD){
            rc = sys_tprelay("EXTRELAY", &SESSION_DATA->cb, 0 ,&SESSION_DATA->ctx);
            if (rc = ERR_ERR){
                SYS_HSTERR(SYS_LN, SYS_GENERR, "x000_error_proc() :sys_tprelay(EXTRELAY) failed jrn_no [%s]", SESSION->ilog_jrn_no);
                //SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "INPUT DATA ERR");
            }
        }
    }

    z000_free_session(ctx);

    SYS_TREF;

    return ERR_NONE;
}

/* ---------------------------------------------------------------- */
static void y000_timeout_proc(session_t *session)
{

    symqtop_onout_ctx_t     _ctx    = {0};
    symqtop_onout_ctx_t     *ctx    = &CTX_T               ctxt;
    

    SYS_TRSF;

    ctx->cb = &ctx->commbuff_t _cb;

    ex_syslog(LOG_FATAL, "[APPL_DM] y000_timeout_proc() 발생 [해결방안] 시스템 담당자 call");

    SESSION = session;

    x000_error_proc(ctx);

    SYS_TREF;

    return;
}

/* ---------------------------------------------------------------- */
static int z000_free_session(symqtop_onout_t *ctx)
{
    sysiouth_t          sysiouth = {0};

    SYS_TRSF;

    if (SESSION) {

        del_session_from_list(SESSION);

        if (SESSION->data) {
            sysocbfb(&SESSION_DATA->cb);

            if (SESSION_DATA->as_data){
                free(SESSION_DATA->as_data);
                SESSION_DATA->as_data = 0x00;
            }

            free(SESSION->data);
            SESSION->data = 0x00;
        }

        free(SESSION);
        SESSION = 0x00;
    }

    if (ctx->cb) {
        sysocbfb(ctx->cb);
        ctx->cb = 0x00;
    }

    SYS_TREF;

    return ERR_NONE;
}
/* ---------------------------------------------------------------- */
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

    excomm      = (exmsg1200_comm_t *) dp;      /* 1200 message 통신전문 */
    exmsg1500   = (exmsg1500_t *) dp;           /* 1500 message 통신전문 */

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

/*
    if (type == EB_TO_AS)
        utoeb2as((unsigned char *)dp, len, (unsinged char *)dp);
    else
        utoeb2eb((unsigned char *)dp, len, (unsinged char *)dp);

*/

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

    //SYS_DBG("excomm->pswd_1[%s]", excomm->pswd_1);   


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
static int el_data_chg(exmsg1200_t *exmsg1200, exparm_t *exparm)
{
    int                 rc          = ERR_NONE;

    /* 전자금융 로깅 */
    if (memcmp(exmsg1200->appl_code, "094", LEN_APPL_CODE) != 0){
        return ERR_NONE;
    }

    if (memcmp(exparm->tot_info_flag_3[0])  != 0){
        return ERR_NONE;
    }

    /* ----------------------------------------- */
        PRINT_EXMSG1200(exmsg1200);
        PRINT_EXMSG1200_2(exmsg1200);
    /* ----------------------------------------- */

    memcpy(exmsg1200->offr_no, "777777", 6);

    /* ----------------------------------------- */
        PRINT_EXMSG1200(exmsg1200);
        PRINT_EXMSG1200_2(exmsg1200);
    /* ----------------------------------------- */

    return ERR_NONE;

}
/* -------------------------------------------------------------------------------- */
int apsvrdone()
{
    int           i;  

    SYS_DBG("Server End ");

    return ERR_NONE;

}