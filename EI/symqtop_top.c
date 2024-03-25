/*  @file               symqtop_top.pc                                             
*   @file_type          proc source program
*   @brief              MQGET Common program
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
#include <cmqc.h>                   /* includes for MQI */
#include <exmq.h>
#include <exmqparm.h>
#include <exmqpmsg_001.h>
#include <exmqsvcc.h>
#include <mqimsg001.h>
#include <usrinc/atmi.h>
#include <usrinc/tx.h>
#include <sqlca.h>
#include <utoclck.h>
/* --------------------------constant, macro definitions -------------------------- */
#define EXCEPTION_SLEEP_INTV        300            /* Exception Sleep interval 5sec */

#define EXMQPARM                    (&ctx->exmqparm)
#define MQ_INFO                     (&ctx->mq_info)
#define MQMSG001                    (&ctx->exmqmsg_001)
//define EXMSG1200_DATA               (&ctx->exmsg1200)

#define TMOCHK_INTERVAL             60
#define LEN_TCP_HEAD                72
#define LEN_COMMON_HEAD             300

/* --------------------------structure definitions -------------------------------- */
typedef struct symqtop_top_ctx_s symqtop_top_ctx_t; 
struct symqtop_top_ctx_s {
    exmqparm_t          exmqparm;
    exmqmsg_001_t       exmqmsg_001;
    commbuff_t          *cb;
    commbuff_t          _cb;   
    mq_info_t           mqinfo;

    long                is_mq_connected;
    long                mqpget_opts;
    MQHCONN             mqhconn; 
    MQHOBJ              mqhobj;
};


/* --------------exported global variables declarations ---------------------------*/
    char                g_svc_name[32];                 /* G/W 서비스명          */
    char                g_chnl_code[3 + 2];             /* CHANNEL CODE        */
    char                g_appl_code[2 + 1];             /* APPL_CODE           */
    int                 g_call_svc_name[32];            /* 일련번호 채번         */
    int                 g_msg_type[8];                  /* 임시 사용 buffer에 저장한 데이터 길이  */
    char                g_temp_buf[5000];               /* 임시 사용 buffer */

symqtop_top_ctx_t     _ctx; 
symqtop_top_ctx_t     *ctx = &_ctx;

/* ----------------------- exported function declarations ------------------------- */
int                     symqtop_top(commbuff_t *commbuff);
int                     apsvrinit(int argc, char *argv[]);
int                     apsvrdone();
/* mq setting */
static int  a000_initial(int argc, char argv[]);
static int  a100_parse_custom_args(int argc, char argv[]);
static int  b100_mqparm_load(symqtop_top_ctx_t        *ctx);
static int  c100_check_available_time(symqtop_top_ctx_t  *ctx);
static int  d100_init_mqcon(symqtop_top_ctx_t         *ctx);
static int  e100_get_mqmsg(symqtop_top_ctx_t          *ctx);
static int  f100_mqmsg_proc(symqtop_top_ctx_t         *ctx);
/* log insert */
static int  g000_call_svc(symqtop_top_ctx_t           *ctx);
static int  z000_free_proc(symqtop_top_ctx_t          *ctx);
/* -------------------------------------------------------------------------------- */
int apsvrinit(int argc, char *argv)
{

    int         rc = ERR_NONE;


    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(symqtop_onin_ctx_t));
    //ctx->cb = &ctx->cb;

    SYS_DBG("Initialize");


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
int usermain(int argc, char argv[])
{

    int rc = ERR_NONE;

    /* tmboot시 업무서비스 boot할 시간을 줌 5sec */
    /********************/
    tpschedule(5);
    /********************/
    SYS_DBG("============================ usermain Start =========================");

    while(1){

        //check available time 항상 시간을 체크해야함 테스트앤 꺼름 
        if (c100_check_available_time(ctx) == ERR_ERR){
            SYS_DBG("c100_check_available_time failed tpschedule(60) ");
            tpschedule(60);
            continue;
        }

        //MQ Initialization */
        if (d100_init_mqcon(ctx) == ERR_ERR){
            SYS_DBG("d100_init_mqcon failed.  tpschedule(%d) ", EXCEPTION_SLEEP_INTV);
            tpschedule(EXCEPTION_SLEEP_INTV);
            //tpschedule(60);
            continue;    
        }

        //mq get & insert ktmqmsg */
        if (e100_check_req_data(ctx) == ERR_ERR){
            SYS_DBG("e100_check_req_data failed.  tpschedule(%d) ", EXCEPTION_SLEEP_INTV);
            tpschedule(EXCEPTION_SLEEP_INTV);
            //tpschedule(10);
            continue;    
        }

        if (z000_free_proc(ctx) == ERR_ERR){
            SYS_DBG("z000_free_proc failed.  tpschedule(%d) ", EXCEPTION_SLEEP_INTV);
            //tpschedule(EXCEPTION_SLEEP_INTV);
            //tpschedule(10);
            continue;    
        }

        //sleep(60)

        /* check event */
        tpschedule(-1);

    }


}
/* -------------------------------------------------------------------------------- */
int symqtop_top(commbuff_t    *commbuff)
{
    int                 rc       = ERR_NONE;
    sysiouth_t          sysiouth = {0};

    SYS_TRSF;

    /* set commbuff */
    ctx->cb = commbuff;

    return ERR_ERR;

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
    
    while((c = getopt(argc, argv, "s:c:a:t:o:")) != EOF ){
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

        case 't':
            strcpy(g_svc_name, optarg);
            break;

        case 'o':
            strcpy(g_msg_type, optarg);
            break;

        case '?':
            SYS_DBG("unrecognized option %c %s", optarg,argv[optind]);
            return ERR_ERR;
        }
    }

    /* ----------------------------------------------------------- */
    SYS_DBG("g_svc_name     = [%s]", g_svc_name);
    SYS_DBG("g_chnl_code    = [%s]", g_chnl_code);
    SYS_DBG("g_appl_code    = [%s]", g_appl_code);
    SYS_DBG("g_call_svc_name= [%s]", g_call_svc_name);
    SYS_DBG("g_msg_type     = [%s]", g_msg_type);
    /* ----------------------------------------------------------- */

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

    /* ------------------------------------------- */
    //PRINT_EXMQPARAM(EXMQPARAM);
    /* ------------------------------------------- */

    mqparam_load_flag = 1;
    
    SYS_TREF;

    return ERR_NONE;
}
/* -------------------------------------------------------------------------------- */
static int c100_check_available_time(symqtop_top_ctx_t  *ctx)
{
    int                 rc          = ERR_NONE;
    char                curtime[LEN_TIME+1];

    utotime1(curtime);      //HHMISS
    if ((memcmp(curtime, EXMQPARAM->mq_start_time, 6) < 0) ||
        (memcmp(curtime, EXMQPARAM->mq_end_time  , 6) > 0)) {

            SYS_DBG(" ----------------------------------------- ");
            SYS_DBG("Service available time : %s ~ %s", EXMQPARAM->mq_start_time, EXMQPARAM->mq_end_time)
            SYS_DBG(" ----------------------------------------- ");

            return ERR_ERR;
    }

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
        //SYS_DBG("d100_init_mqcon MQ_Connect failed g_chnl_code[%s]mq_mngr[%s], g_chnl_code, EXMQPARAM");
        //SYS_HSTERR(SYS_NN, SYS_GENERR, "%d d100_init_mqcon MQ Open failed g_chnl_code");
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
static int e100_get_mqmsg(symqtop_top_ctx_t  *ctx)
{
    int          rc                 = ERR_NONE;
    MQMD         mqmd               = {MQMD_DEFAULT};

    sys_err_init();

    SYS_DB_ERRORCLEAR;

    memset(MQ_INFO, 0x00, sizeof(mq_info_t));

    MQ_INFO->hcon             = ctx->mqhconn;             /* QM Connectiopn   */
    MQ_INFO->hobj             = &ctx->mqhobj;             /* put queue object */
    MQ_INFO->wait_intv      = EXMQPARAM->schedule_ms * 1000; /* MQ GET Time - milliseconds */

    /* -------------------------------------------------------- */
    /* coded_charset_id : default 970 또는 819                   */
    /* -------------------------------------------------------- */
    MQ_INFO->coded_charset_id = EXMQPARAM->charset_id;

    /* Get Queue Option Set */
    switch(EXMQPARAM->mq_get_opts){
    case 1: /* CharsetSet Convnersion Option */
        ctx->mqget_opts = MQGMO_ACCEPT_TRUNCATED_MSG | MQGMO_SYNCPOINT | MQGMO_WAIT | MQGMO_CONVERT;
        break;
    default : /* default */
        ctx->mqget_opts = MQGMO_ACCEPT_TRUNCATED_MSG | MQGMO_SYNCPOINT | MQGMO_WAIT ;
        break;
    }

    //SYS_DBG("MQ Option (ctx->mqget_opts) : [%ld]", ctx->mqget_opts);

    /* get mq data */
    rc = ex_mq_put_with_opts(MQ_INFO, &mqmd, ctx->mqget_opts);
    //rc = ex_mq_put_with_opts(MQ_INFO, &mqmd, MQ000_INPUT_AS_Q_DEF);
    if (rc != ERR_NONE){
        switch (rc){
        case MQ_ERR_NO_MSG;
            //SYS_DBG("NO MSG AVAILABLE");
            return ERR_NONE;
        
        case MQ_ERR_EMPTY_BUF;
            SYS_DBG("EMPTY BUFFER");
            return ERR_NONE;

        default:
            ex_syslog(LOG_ERROR, "[CLIENT DM] %s e100_get_mqmsg failed rc=[%d]g_chnl_code[%s]mq_mngr[%s] ",__FILE__, rc, g_chnl_code, EXMQPARAM->mq_mngr);

            /* reset flag for mq initialize */
            ctx->is_mq_connected = 0;
            return ERR_ERR;
        }
    }

    /* ----------------------------------------------------------------- */
    SYS_DBG("msgid[%s] corrid[%s]", MQ_INFO->msgid, MQ_INFO->corrid);
    /* ----------------------------------------------------------------- */

    rc = f100_mqmsg_proc(ctx, &mqmd);
    if (rc == ERR_NONE){
        rc = ex_mq_commit(&ctx->mqhconn);
    }else if (rc != ERR_NONE){
        rc = ex_mq_commit(&ctx->mqhconn);
        //ex_mq_rollback(&ctx->mqhconn);
        return ERR_ERR;
    }

    return rc;

}

/* ---------------------------------------------------------------- */
static int f100_mqmsg_proc(symqtop_top_ctx_t    *ctx, MQMD *mqmd)
{
    int          rc                 = ERR_NONE;
    long         len                = 0;
    long         max_msg_len        = 0;
    char         buff[256]          = {0};
    char         svc_name[32];
    char         temp_hcmihead[72];
    char         tcp_tmp_buff[LEN_TCP_HEAD + 1];
    char         cont_type[1];
    char         covt_type[1];
    //char       *hp, *dp;
    char         etc_data[900];
    commbuff_t   dcb;
    exmqmsg_001_t       exmqmsg_001;
    mqimsg001_t         mqimsg001;
    hcmihead_t          *hcmihead;
    sysgwinfo_t         sysgwinfo   = {0};
    long         data_len           = 0;
    long         err_code           = 0;

    ctx->cb = &ctx->cb;   

    SYS_TRSF;

    /* ----------------hcmihead의 EBCDIC to ASCII 전문 변환 ---------- */
    //EI-TOPS간 송수신에는 EBCDIC to ASCII 변환 사용안함 
    memset(g_temp_buf, 0x00, sizeof(g_temp_buf));
    memset(&mqmsg_001, 0x00, sizeof(exmqmsg_001_t));
    //memset(&dp, 0x20, LEN_HCMIHEAD);

    utocick(mqmsg_001.msg_id); //메세지아이디 처리할때 마다 채번 
    //utoeb2as(&MQ_INFO->msgbuf,LEN_HCMIHEAD );
    //memcpy(g_temp_buf, &dp ,LEN_HCMIHEAD );

    //memcpy(&dp, MQ_INFO->msgbuf, (MQ_INFO->msglen - LEN_HCMIHEAD));

    //옵션에서 corr_id를 가져와 queue name에 저장
    if (strlen(MQ_INFO->corrid) < 20){
        SYS_DBG("corr_id is NULL (취급요청거래 )");
        //memcpy(&g_temp_buf[25], "              ", (LEN_EXMQMSG_001_CORR_ID - 4));
        memcpy(mqmsg_001.corr_id, "              ", (LEN_EXMQMSG_001_CORR_ID - 4));
    }else{
        //memcpy(&g_temp_buf[25], MQ_INFO->corrid, (LEN_EXMQMSG_001_CORR_ID - 4));
        memcpy(mqmsg_001.corr_id, MQ_INFO->corrid, (LEN_EXMQMSG_001_CORR_ID));  
    }






































    

}

/* ---------------------------------------------------------------- */
static void y000_timeout_proc(session_t *session)
{

    symqtop_top_ctx_t     _ctx    = {0};
    symqtop_top_ctx_t     *ctx    = &CTX_T               ctxt;
    

    SYS_TRSF;

    ctx->cb = &ctx->commbuff_t _cb;

    ex_syslog(LOG_FATAL, "[APPL_DM] y000_timeout_proc() 발생 [해결방안] 시스템 담당자 call");

    SESSION = session;

    x000_error_proc(ctx);

    SYS_TREF;

    return;
}

/* ---------------------------------------------------------------- */
static int z000_free_session(symqtop_top_t *ctx)
{
    int                 rc          = ERR_NONE;

    SYS_TRSF;

    #ifdef  _DEBUG
       //utohexdp(TCPHEAD, 72);
       //utohexdp(TCPHEAD, 72);
    #endif 

    //g_call_svc_name =SYSMQTOP_ONINR / SYMQTOP_ONOUT
    rc = sys_tpacall(g_call_svc_name, ctx->cb, TPNOREPLY, | TPNOTRAN);
    if (rc < 0){
        SYS_HSTERR(SYS_LN, ERR_SVC_CALL, "SVC(%s) call failed [%d]", g_call_svc_name, tperrno);
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;
}
/* -------------------------------------------------------------------------------- */
int apsvrdone()
{
    int          i;

    sysocbfb(ctx->cb);

    SYS_DBG("Server End ");

    return ERR_NONE;
}
/* ----------------------- End of Program ------------------------------- */
