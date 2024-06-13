
/*  @file               symqrecv_idv.pc
*   @file_type          pc source program
*   @brief              CKI Imaging ==> MQ ==> EI로 들어오는 PROCESS
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
*   @generated at 2024/04/23 09:30
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
#include <syscommbuff.h>
#include <sytcpgwhead.h>
#include <usrinc/atmi.h>
#include <usrinc/ucs.h>
#include <sqlca.h>
#include <mqi0001f.h>
#include <cmqc.h>
#include <exmq.h>
#include <exmqparam.h>
#include <exmqmsg_002.h>
#include <mqimsg002.h>
#include <hcmihead.h>
#include <exi0212.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXCEPTION_SLEEP_INTV                10                        /* EXCEPTION SLEEP Interval 10 sec        */

#define EXMQPARM                            (&ctx->exmqparm)
#define MQ_INFO                             (&ctx->mq_info)

#define TMOCHK_INTERVAL                     60

#define AP_SVC_NMAE                         (ctx->ap_svc_name)

/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct symqrecv_idv_ctx_s  symqrecv_idv_ctx_t;
struct symqrecv_idv_ctx_s {
    exmqparm_t      exmqparm;
    char            ap_svc_name[LEN_EXPARM_AP_SVC_NAME + 1];
    long            is_mq_connected;
    MQHCONN         mqhconn;
    MQHOBJ          mqhobj;
    mq_info_t       mq_info;
    commbuff_t      _cb;
    commbuff_t      *cb;
};

/* ------------------------------------- exported global variables definitions -------------------------------- */
char                g_svc_name[32];                     /* G/W 서비스명         */
char                g_chnl_code[3 + 1];                 /* CHANNEL CODE       */
char                g_appl_code[2 + 1];                 /* APPL_CODE          */
char                g_pid[6 + 1];                       /* process_id         */

exmqrecv_idv_ctx_t  _ctx;
exmqrecv_idv_ctx_t  *ctx = &_ctx;  

/* ------------------------------------------ exported function  declarations --------------------------------- */
int                 symqrecv_idv(commbuff_t    *commbuff);
static int          a000_initial(int argc,  char *argv[]);
static int          a100_parse_custom_args(int argc,  char *argv[]);
static int          b100_mqparm_load(symqrecv_idv_ctx_t    *ctx);
static int          c100_check_available_time(symqrecv_idv_ctx_t    *ctx);
static int          d100_init_mqcon(symqrecv_idv_ctx_t     *ctx);
static int          e100_get_mqmsg(symqrecv_idv_ctx_t    *ctx);
static int          f100_mqmsg_proc(symqrecv_idv_ctx_t      *ctx);
static int          f200_set_exparm(symqrecv_idv_ctx_t      *ctx);
static int          g000_call_svc(symqrecv_idv_ctx_t   *ctx);
static int          z200_mqmsg_update(symqrecv_idv_ctx_t   *ctx);

int                 apsvrdone();

/* ------------------------------------------------------------------------------------------------------------ */
int
apsvrinit(int argc, char *argv[])
{
    int                 rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(symqrecv_idv_ctx_t));

    SYS_DBG("Initialize");

    rc = a000_data_init(argc, argv);
    if (rc == ERR_ERR){
        return rc;
    }

    rc = b100_mqparm_load(ctx);
    if ( rc != ERR_NONE){
        SYS_DBG("b100_mqparm_load failed  +++++++++++++++++");
        ex_syslog(LOG_FATAL, "[APPL_DM] b100_mqparm_load() FAIL [해결방안]시스템 담당자 CALL ");
        cxt->is_mqparm_loaded = 0;
        return rc;
    }

    /* MQ connect Initialization -> 연결안된 경우, 연결    */
    rc = d100_init_mqcon(ctx);
    if (rc == ERR_ERR){
        SYS_DBG("d100_init_mqcon failed STEP 1");
        ex_syslog(LOG_FATAL, "[APPL_DM] d100_init_mqcon ():STEP 1 FAIL [해결방안] 시스템 담당자 CALL");
        ctx->is_mq_connected = 0;
        return rc;
    }

    //get Process ID
    utoi2an(getpid(), LEN_EXMQMSG_002_PID, g_pid);

    SYS_TREF;

    return rc;

}
/* ------------------------------------------------------------------------------------------------------------ */

static int          
a000_initial(int argc,  char *argv[])
{

    int                 rc = ERR_NONE;
    int                 i;

    SYS_TRSF;

    /* command argument 처리 */
    SYS_TRY(a100_parse_custom_args(argc, argv));

    strcpy(g_arch_head.svc_name,    g_svc_name);


    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;
    return ERR_ERR;
}

/* ------------------------------------------------------------------------------------------------------------ */
static int          a100_parse_custom_args((int argc,  char *argv[])
{
    int      c;

    g_sleep_sec = 0.1;

    while((c = getopt(argc, argv, "s:c:a")) != EOF ){
    /* ----------------------------------------------------------------------------------- */
    SYS_DBG("GETOPT:%c\n", c);
    /* ----------------------------------------------------------------------------------- */
        switch(c){
        case 's' :
            strcpy(g_svc_name, optargs);
            break;

        case 'c' :
            strcpy(g_chnl_code, optargs);
            break;

        case 'a' :
            strcpy(g_appl_code, optargs);
            break;

        case '?' :
            SYS_DBG("unreconized option: -%c %s", optopt, argv[optind]);
            return ERR_ERR;
            
        }
    }

    /* ----------------------------------------------------------------------------------- */
    SYS_DBG("g_svc_name         :[%s]", g_svc_name);
    SYS_DBG("g_chnl_code        :[%s]", g_chnl_code);
    SYS_DBG("g_appl_code        :[%s]", g_appl_code);
    /* ----------------------------------------------------------------------------------- */

    /* 서비스명 검증 */
    if ((strlen(g_svc_name) == 0) ||
        (g_svc_name[0])     == 0x20))
        return ERR_ERR;

    return ERR_NONE;
}


/* ------------------------------------------------------------------------------------------------------------ */
int usermain(int argc,  char *argv[])
{
    int                 rc = ERR_NONE;
    symqrecv_idv_ctx_t     _ctx = {0};
    symqrecv_idv_ctx_t     *ctx = &ctx;  

    /* initial */
    rc = a000_initial(argc, argv);
    if (rc == ERR_ERR)
        //tpsvrdown();

    /* tmboot시 업무서비스 boot할 시간을 줌 5sec */
    tpschedule(5);

    rc = db_connect("");
    if (rc == ERR_ERR){
        SYS_DBG("데이터베이스 연결실패 ");
        SYS_HSTERR(SYS_LCF, SYS_GENERR, "[CRITICAL] DB connection failed :%d", sqlca.sqlcode);
        SYS_DBG("sleep 60");
        tpschedule(60);
        return ERR_NONE;
    }

    /* never return    */
    while(1){

        if ( db_connect("") != ERR_NONE){
            SYS_DBG("sleep 60");
            tpschedule(60);
            continue;
        }

        //get channel info 한번 load 하면 다음번에는 안하게 되어있으니까 상관없음. 
        if (b100_mqparam_load(ctx) ==  ERR_NONE) {
            SYS_DBG("b100_mqparam_load failed tpschedule(%d)", EXCEPTION_SLEEP_INTV);
            tpschedule(EXCEPTION_SLEEP_INTV);
            continue;
        }

        /* check_available_time    */
        if (c100_check_available_time(ctx0) ==  ERR_NONE) {
            SYS_DBG("c100_check_available_time failed tpschedule(60) ");
            tpschedule(60);
            continue;
        }

        /* MQ Initialization    */
        if (d100_init_mqcon(ctx) ==  ERR_NONE) {
            SYS_DBG("d100_init_mqcon failed tpschedule(%d)", EXCEPTION_SLEEP_INTV);
            tpschedule(EXCEPTION_SLEEP_INTV);
            continue();

        }

        /* get MQ & insert ktmqmsg  */
        if (e100_get_mqmsg(ctx) ==  ERR_NONE) {
            SYS_DBG("e100_get_mqmsg failed tpschedule(%d)", EXCEPTION_SLEEP_INTV);
            tpschedule(EXCEPTION_SLEEP_INTV);
            continue();
        }


        //sleep(60);

        /* check event */
        tpschedule(-1);

    }  /* end while loop */
}


/* ------------------------------------------------------------------------------------------------------------ */
static int   b100_mqparm_load(symqrecv_idv_ctx_t   *ctx)
{

    int                 rc  = ERR_NONE;
    static int          mqparam_load_flag = 0;
    mqi0001f_t          mqi0001f;

    if (mqparam_load_flag == 1){
        return ERR_NONE;
    }

    SYS_TRSF;

    memset(&mqi0001f,   0x00, sizeof(mqi0001f_t));

    memcpy(EXMQPARM->chnl_code, g_chnl_code,    LEN_EXMQPARM_CHNL_CODE);
    memcpy(EXMQPARM->appl_code, g_appl_code,    LEN_EXMQPARM_APPL_CODE);

    mqi0001f.in.exmqparm = EXMQPARM;
    rc = mq_exmqparm_select(&mqi0001f);
    if (rc == ERR_ERR) {
        SYS_HSTERR(SYS_LF, 8600, "[FAIL]EXMQPARM select failed chnl[%s] err[%d][%s]", g_chnl_code, db_errno(), db_errstr());
        return ERR_ERR;
    }

    utotrim(EXMQPARM->mq_mngr);
    if (strlen(EXMQPARM->mq_mngr) == 0) {
        SYS_HSTERR(SYS_LF, SYS_GENERR, "[FAIL]EXMQPARM [%s/%s/%s].mq_mngr is null", g_chnl_code, g_appl_code, EXMQPARM->mq_mngr);
        return ERR_ERR;
    }

    utotrim(EXMQPARM->getq_name);
    if (strlen(EXMQPARM->getq_name) == 0) {
        SYS_HSTERR(SYS_LF, SYS_GENERR, "[FAIL]EXMQPARM [%s/%s/%s].getq_name is null", g_chnl_code, g_appl_code, EXMQPARM->getq_name);
        return ERR_ERR;
    }

    //
    //PRINT_EXMQPARM(EXMQPARM);
    //

    mqparam_load_flag = 1;

    SYS_TREF;

    return ERR_NONE;
}

/* ------------------------------------------------------------------------------------------------------------ */
static int  c100_check_available_time(symqrecv_idv_ctx_t    *ctx)
{
    int                 rc = ERR_NONE;
    char                curtime[LEN_TIME + 1];

    utotime1(curtime);
    if ((memcmp(curtime, EXMQPARM->mq_start_time,    6) < 0) ||
        (memcmp(curtime, EXMQPARM->mq_end_time  ,    6) > 0)){

            SYS_DBG(" --------------------------------------------------------------------------------- ");
            SYS_DBG(" Service available time : %s ~ %s ", EXMQPARM->mq_start_time, EXMQPARM->mq_end_time );
            SYS_DBG(" --------------------------------------------------------------------------------- ");
            return ERR_ERR;
        }

        return ERR_NONE;
}

/* ------------------------------------------------------------------------------------------------------------ */
static int   d100_init_mqcon(symqrecv_idv_ctx_t    *ctx)
{

    int                 rc  = ERR_NONE;

    if (ctx->is_mq_connected == 1){
        return ERR_NONE;
    }

    SYS_TRSF;

    if (ctx->mqhconn != 0 ){
        ex_mq_disconnect(&ctx->mqhconn);
        ctx->mqhconn = 0;
    }

    /* Queue Manager Connect */
    rc = ex_mq_connect(&ctx->mqhconn, EXMQPARM->mq_mngr);
    if (rc == MQ_ERR_SYS) {
        ex_syslog(LOG_ERROR, "[CLIENT DM] %s d100_init_mqcon MQ OPEN failed g_chnl_code[%s].mq_mngr[%s]", __FILE__, g_chnl_code, EXMQPARM->mq_mngr);
        ctx->is_mq_connected = 0;
        return ERR_ERR;
    }

    /* 
     * mq_con_type : Queue Connection Type 
     * -----------------------------------------
     * 1 = Only GET, 2 = Only PUT, 3 = BOTH
     */
     /* GET Queue Open */
    rc = ex_mq_open(&ctx->mqhconn, &ctx->mqhobj, EXMQPARM->putq_name, MQ_OPEN_OUTPUT);
    if (rc == MQ_ERR_SYS) {
        ex_syslog(LOG_ERROR, "[CLIENT DM] %s d100_init_mqcon MQ OPEN failed g_chnl_code[%s].mq_mngr[%s]", __FILE__, g_chnl_code, EXMQPARM->mq_mngr);
        ex_mq_disconnect(&ctx->mqhconn);
        ctx->mqhconn = 0;
        return ERR_ERR;
    }

    /* ----------------------------------------------------------------------------------- */
    SYS_DBG("CONN Handler       :[%d][%08x]", ctx->mqhconn, ctx->mqhconn);
    SYS_DBG("GET QUEUE Handler  :[%d][%08x]", ctx->mqhobj , ctx->mqhobj );
    /* ----------------------------------------------------------------------------------- */

    ctx->is_mq_connected = 1;

    SYS_TREF;

    return ERR_NONE;

}


/* ------------------------------------------------------------------------------------------------------------ */
static int   e100_get_mqmsg(symqrecv_idv_ctx_t   *ctx)
{

    int                 rc     = ERR_NONE;
    MQMD                md     = {MQMD_DEFAULT};

    sys_err_init();

    SYS_DB_ERRORCLEAR;

    memset(MQ_INFO, 0x00,   sizeof(mq_info_t));
    /* MQPUT을 위한 mq_info_t Set : MQIHEAD + INPDATA   */
    MQ_INFO->hobj             = &ctx->mqhobj;         /* PUT QUEUE OBJECT */
    MQ_INFO->hcon             = ctx->mqhconn;         /* QM Connection    */
    MQ_INFO->coded_charset_id = EXMQPARM->schedule_ms * 1000;  /* MQ Get Timeout milliseconds   */

    /********************************************************************************************/
    /* coded_charset_id : default 819으로 설정함                                                   */
    /********************************************************************************************/
    MQ_INFO->coded_charset_id   = EXMQPARM->charset_id;             /* CodedCharSetID           */

    /* get mq data      */
    rc = ex_mq_put_with_opts(MQ_INFO,   &mqmd, EXMQPARM->mq_get_opts);
    if (rc != ERR_NONE){
        switch(rc){
        case MQ_ERR_NO_MSG:                                 /* QUEUE에 쌓인 메시지가 없는 경우        */
            //SYS_DBG("NO MSG AVAILABLE");
            return ERR_NONE;

        case MQ_ERR_EMPTY_BUF:                              /* QUEUE에 메시지가 buffer 비어있는 경우  */ 
            SYS_DBG("EMPTY BUFFER");
            return ERR_NONE;

        default:
            ex_syslog(LOG_FATAL, "[CLIENT DM] %s e100_get_mqmsg ex_mq_get failed rc[%d] g_chnl_code[%s]mq_mngr[%s]", __FILE__, rc, g_chnl_code, EXMQPARM->mq_mngr);

            /* rest flag for mq initialize    */
            ctx->is_mq_connected = 0;
            return ERR_ERR;
        }
    } 

    /********************************************************************/
    SYS_DBG("msgid[%s] corrid[%s]" , MQ_INFO->msgid,    MQ_INFO->corrid);
    /********************************************************************/

    /* MQ message는 가져오는 순간 메세지가 큐에서 없어지기 때문에 commit/rollback의 의미가 없음  */
    rc = f100_mqmsg_proc(ctx, &mqmd);
    if (rc == ERR_NONE){
        rc = ex_mq_commit(&ctx->mqhconn);
    }else if (rc != ERR_NONE) {
        ex_mq_commit(&ctx->mqhconn);
        return ERR_ERR;
    }

    return rc;
}


/* ------------------------------------------------------------------------------------------------------------ */
static int   f100_mqmsg_proc(symqrecv_idv_ctx_t *ctx)
{
    
    int                 rc          = ERR_NONE;
    int                 db_rc;
    exmqmsg_002_t       mqmsg_002;
    mqimsg002_t         mqimsg002;
    hcmihead_t          hcmihead;
    sysgwinfo_t         sysgwinfo   = {0};
    sysicomm_t          sysicomm    = {0};

    ctx->cb = &ctx->_cb;

    SYS_TRSF;

    /* -------------------    hcmihead 세팅   ------------------------- */
    memset(&hcmihead,   0x00,   sizeof(hcmihead_t));
    memcpy(&hcmihead,   MQ_INFO->msgbuf,  LEN_HCMIHEAD);
    /* -------------------    hcmihead 세팅   ------------------------- */

    memset(&mqmsg_002,  0x00,   sizeof(exmqmsg_002_t));
    /* chnl_code */
    memcpy(mqmsg_002.chnl_code, g_chnl_code,    LEN_EXMQMSG_002_CHNL_CODE);
    /* appl_code */
    memcpy(mqmsg_002.mq_appl_code, g_appl_code,    LEN_EXMQMSG_002_MQ_APPL_CODE);
    /* io_type   */
    mqmsg_002.io_type = 1;      /*  취급요청,취급응답,개설요청,개설응답, 취급콜백 1/2/3/4/5  */
    /* pid       */
    memcpy(mqmsg_002.pid    , g_pid        ,   LEN_EXMQMSG_002_PID);
    /* msg_id    */
    utocick(mqmsg_002.msg_id);
    /* corr_id   */
    memcpy(mqmsg_002.corr_id,  MQ_INFO->corr_id,  LEN_HCMIHEAD_QUEUE_NAME);
    /* appl_code */
    memcpy(mqmsg_002.appl_code,&MQ_INFO->msgbuf[91], LEN_EXMQMSG_002_APPL_CODE);
    /* tx_code */
    memcpy(mqmsg_002.tx_code  ,&MQ_INFO->msgbuf[81], LEN_EXMQMSG_002_TX_CODE);
    /* msg_no  */
    memcpy(mqmsg_002.msg_no, MQ_INFO->corr_id,  strlen(MQ_INFO->corrid));

    SYS_DBG("MQ_INFO->msglen [%d]", MQ_INFO->msglen);

    if (MQ_INFO->msglen < 1){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s f100_mqmsg_proc() message Length ERROR %d MQ_INFO->msglen[%d]", __FILE__, tperrno, MQ_INFO->msglen);
        return ERR_ERR;
    }

    /* msg_no  */
    memcpy(mqmsg_002.msg_no, MQ_INFO->msgbuf,  MQ_INFO->msglen);

    memset(&mqimsg002, 0x00, sizeof(mqimsg002_t));

    mqimsg002.in.mqimsg002 = &mqmsg_002;
    memcpy(mqimsg002.in.job_proc_type,  "1",    LEN_MQIMG002_PROC_TYPE);

    /* EXMQLOG 로그  insert  */
    db_rc = mqomsg002(&mqimg002);
    if (db_rc == ERR_ERR){
        EXEC SQL ROLLBACK WORK;
        ex_syslog(LOG_ERROR, "[APPL_DM] %s f100_mqmsg_proc() : EXMQLOG INSERT ERROR call mqomsg002", __FILE__);
        //return ERR_ERR;
    }

    EXEC SQL COMMIT WORK;

    /* -------------------------------- */
    PRINT_HCMIHEAD(&hcmihead);
    /* -------------------------------- */


    /* -----------------------------svc Name Set ------------------------------------- */
    SYS_TRY(f200_set_exparm(ctx, &hcmihead));

    /* -----------------------------TCP Head Set ------------------------------------- */
    rc = sysocbsi(ctx->cb, IDX_TCPHEAD, &hcmihead,  LEN_TCP_HEAD);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s f100_mqmsg_proc() : "
                             " COMMBUFF ERR (TCPHEAD) [해결방안]시스템 담당자 CALL",
                              __FILE__);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "COMMBUFF SET ERROR");
        return ERR_ERR;        
    }else{
        SYS_DBG("******* hp (TCPHEAD) [%s]", sysocbgp(ctx->cb, IDX->TCPHEAD));
    }

    /* -----------------------------MSG      Set ------------------------------------- */
    rc = sysocbsi(ctx->cb,  IDX_HOSTRECVDATA, MQ_INFO->msgbuf + LEN_TCP_HEAD + LEN_EIERRHEAD, MQ_INFO->msglen - LEN_TCP_HEAD);
    if (rc == ERR_ERR){
        ex_syslog(LOG_FATAL, "[APPL_DM] %s MSG TMAX SEND COMMBUFF(IDX_HOSTRECVDATA) SET ERROR ", __FILE__);
        return ERR_ERR;
    }

    /* ------ KTI 구별용 Flag 넣어줌 ----- */
    sysgwinfo.sys_type = SYSGWINFO_SYS_CORE_BANK;

    rc = sysocbsi(ctx->cb,  IDX_SYSGWINFO,  &sysgwinfo, LEN_SYSGWINFO);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, 8700, "sysocbsi(IDX_SYSGWINFO) failed - queue_name[%.20s]", hcmihead.queue_name);
    }

    strcpy(sysicomm.call_svc_name, g_svc_name); //SYMQRECV_IDV 저장  
    SYS_DBG("sysicomm.call_svc_name:[%s]", sysicomm.call_svc_name);

    sysocbsi(ctx->cb, IDX_SYSICOMM, &sysicomm, sizeof(sysicomm_t));
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "sysocbsi(SYSICOMM)failed ");
        return ERR_ERR;
    }

    /* call svc  */
    SYS_TRY(g000_call_svc(ctx));

    //SYS_TREF;

    return ERR_NONE;

SYS_CATCH:
    SYS_TREF;
    return ERR_ERR;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int  f200_set_exparm(symqrecv_idv_ctx_t  *ctx,   hcmihead_t         *hcmihead)
{
    int                 rc  = ERR_NONE;
    exi0331_t           exi0331 = {0};

    SYS_TRSF;

    //memcpy(AP_SVC_NAME,  "IGN0000",   LEN_EXPARM_AP_SVC_NAME);
    /* get ap svc name */
    exi0331.in.proc_type = 4;
    memcpy(exi0331.in.kti_tx_code,  hcmihead->tx_code,  LEN_EXPARMKTI_KTI_TX_CODE);
    rc = ex_exchange_kti_tx_code(&exi0331);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, 9000, "ex_exchange_kti_tx_code[%s] failed", exi0331.in.kti_tx_code);
        return ERR_ERR;
    }

    memcpy(AP_SVC_NMAE, exi0331.in.ap_svc_name, LEN_EXPARMKTI_AP_SVC_NAME);

    SYS_DBG("AP_SVC_NAME [%s]", AP_SVC_NMAE);

    utotrim(AP_SVC_NMAE);

    if (strlen(AP_SVC_NMAE) == 0){
        //SYS_HSTERR(SYS_LN, 9000, "null[%s] ", exi0331.in.kti_tx_code);
        return ERR_ERR;
    }

    rc = sysocbsi(ctx->cb,  IDX_EXPARM, MQ_INFO->msgbuf,    MQ_INFO->msglen);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, 8700, "sysocbsi (IDX_EXPARM) failed");
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int g000_call_svc(symqrecv_idv_ctx_t     *ctx)
{
    int           rc = ERR_NONE; 

    SYS_TRSF;

    SYS_DBG("***** TCPHEAD  /sysocbgp(ctx->cb, IDX_TCPHEAD) [%s]", sysocbgp(ctx->cb, IDX_TCPHEAD));
    utohexdp(TCPHEAD, 72);

    SYS_DBG("***** EXPARM  /sysocbgp(ctx->cb, IDX_EXPARM) [%s]", sysocbgp(ctx->cb, IDX_EXPARM));
    utohexdp(EXPARM,    sizeof(exparm_t));

    SYS_DBG("sys_tpcall[%s]",   AP_SVC_NMAE);
    rc = sys_tpcall(AP_SVC_NMAE,    ctx->cb, TPNOREPLY | TPNOTRAN);
    if (rc == ERR_ERR){
        ex_syslog(LOG_FATAL,    "[APPL_DM] %s SYMQSEND_TAX: g000_call_svc() : MSG TAX 서비스 호출 ERR[%d:%d]svc_name[%s]",
                                __FILE__, tperrno, sys_err_code(), AP_SVC_NMAE);
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
static int   z900_error_rspn_proc(symqrecv_idv_ctx_t    *ctx,   exmqmsg_001_t   *mqmsg_001)
{

    int                 rc  = ERR_NONE;
    int                 len;
    tfhlay_t            *tfhlay;
    mqimsg001_t         mqimsg001;

    memcpy(mqmsg_001.chnl_code, g_chnl_code,    LEN_EXMQMSG_001_CHNL_CODE);
    memcpy(mqmsg_001.appl_code, "01"       ,    LEN_EXMQMSG_001_APPL_CODE);
    mqmsg_001->io_type = 4; /*  EI->KTI Response    */

    memcpy(mqmsg_001->corr_id,  MQ_INFO->msgbuf+25, 20);
    mqmsg_001->err_code = 8888888;


    tfhlay = (tfhlay_t *)mqmsg_001->mqmsg;
    memcpy(tfhlay->err_code, "8888888", LEN_TFHLAY_ERR_CODE);
    
    memset(&mqimg001, 0x00, sizeof(mqimsg001_t));
    mqimg001.in.mqmsg_001 = mqmsg_001;

    memset(&mqimsg001.in.job_proc_type, "1" , LEN_MQIMG001_PROC_TYPE);
    rc = mqomsg001(&mqimg001);
    if (rc == ERR_ERR){
        return ERR_ERR;
    }

    EXEC SQL COMMIT WORK;


    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
int  symqrecv_idv(commbuff_t    *commbuff)
{

    int                 i; 

    return ERR_NONE;
}



/* ------------------------------------------------------------------------------------------------------------ */
int   apsvrdone()
{
    
    int           i;  


    return ERR_NONE;
}
/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */