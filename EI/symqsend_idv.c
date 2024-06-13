/*  @file               symqsend_idv.pc
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
*   @generated at 2024/04/24 09:30
*   @history
*
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*
*/

/* ---------------------------------------------- include files ----------------------------------------------- */
#include <syscom.h>
#include <syscommbuff.h>
#include <sytcpgwhead.h>
#include <usrinc/atmi.h>
#include <usrinc/ucs.h>
#include <mqi0001f.h>
#include <cmqc.h>
#include <exmq.h>
#include <exmqparam.h>
#include <exmqmsg_001.h>
#include <exmqmsg_002.h>
#include <mqimsg002.h>
#include <sqlca.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXMQPARM                            (&ctx->exmqparm)
#define MQMSG                               (&ctx->exmqmsg_001)

/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct symqsend_idv_ctx_s  symqsend_idv_ctx_t;
struct symqsend_idv_ctx_s {
    exmqparm_t      exmqparm;
    mq_info_t       mqinfo;
    long            is_mq_connected;
    MQHCONN         mqhconn;
    MQHOBJ          mqhobj;
    exmqmsg_001_t   exmqmsg_001;
    commbuff_t      _cb;
    commbuff_t      *cb;
};

/* ------------------------------------- exported global variables definitions -------------------------------- */
char                g_svc_name[32];                     /* G/W 서비스명         */
char                g_chnl_code[3 + 1];                 /* CHANNEL CODE       */
char                g_appl_code[2 + 1];                 /* APPL_CODE          */
char                g_pid[6 + 1];                       /* process_id         */

mqi0001f_t    g_mqi000f;

symqsend_idv_ctx_t  _ctx;
symqsend_idv_ctx_t  *ctx = &_ctx;  

/* ------------------------------------------ exported function  declarations --------------------------------- */
int                 symqsend_idv(commbuff_t    *commbuff);
int                 apsvrinit(int argc,   char argv[]);
int                 apsvrdone();

static int          a000_initial(int argc,  char *argv[]);
static int          a100_parse_custom_args(int argc,  char *argv[]);
static int          b100_mqparm_load(symqsend_idv_ctx_t    *ctx);
static int          d100_init_mqcon(symqsend_idv_ctx_t     *ctx);
static int          e100_get_sendmsg(symqsend_idv_ctx_t    *ctx);
static int          f100_put_mqmsg(symqsend_idv_ctx_t      *ctx);
static int          g100_update_proc_code(symqsend_idv_ctx_t   *ctx);

/* ------------------------------------------------------------------------------------------------------------ */
int apsvrinit(int argc, char argv[])
{

    int                 rc = ERR_NONE;
    //symqsend_idv_ctx_t _ctx = {0};
    //symqsend_idv_ctx_t *ctx = &_ctx;

    SYS_TRSF;
    memset(&_ctx,   0, sizeof(mqnsend01_ctx_t));
    ctx = &_ctx;

    SYS_DBG("SYMQSEND_IDV Initialize ");

    rc = a000_initial(argc, argv);
    if (rc == ERR_ERR)
        return rc;

    //get channel info 한번 load하면 다음번에는 안하게 되어있으니 상관없음 
    rc = b100_mqparm_load(ctx)
    if (rc !=  ERR_NONE) {
        SYS_DBG("b100_mqparm_load failed ##################### ");
        ex_syslog(LOG_FATAL, "[APPL_DM] b100_mqparm_load() FAIL [해결방안]시스템담당자 call" );
        return rc;

    }

    rc = d100_init_mqcon(ctx);
    if (rc ==  ERR_NONE) {
        SYS_DBG("d100_init_mqcon failed STEP 1 ");
        ex_syslog(LOG_FATAL, "[APPL_DM] d100_init_mqcon() STEP 1 FAIL [해결방안]시스템담당자 call" );
        return rc;
    }

    //get Process ID
    utoi2an(getpid(), LEN_EXMQMSG_002_PID, g_pid);

    /* DB연결이 끊어진 경우 재연결     */
    if (db_connect("") != ERR_NONE) {
        ex_syslog(LOG_ERROR, "apsvrinit db_connect re connecting " );

    }

    SYS_TREF;

    return rc;

}
/* ------------------------------------------------------------------------------------------------------------ */
static int          a000_initial(int argc,  char *argv[])
{

    int                 rc = ERR_NONE;
    int                 i;

    SYS_TRSF;

    /* command argument 처리 */
    rc = a100_parse_custom_args(argc, argv);
    if (rc == ERR_ERR){
        SYS_DBG("parsing custom argument error ");
        return rc;
    }

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
    
    char      c;

    SYS_TRSF;

    g_sleep_sec = 0.1;

    while((c = getopt(argc, argv, "s:c:a")) != EOF ){
        /* -------------------------------------------------------------------------------- */
        SYS_DBG("GETOPT:%c\n", c);
        /* -------------------------------------------------------------------------------- */        

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

    SYS_TREF;
    
    return ERR_NONE;
}


/* ------------------------------------------------------------------------------------------------------------ */
int  symqsend_tax(commbuff_t    *commbuff)
{
    int                 rc = ERR_NONE;
    int                 i;
    char                *cp;
    char                *hp;
    //symqsend_idv_ctx_t     _ctx = {0};
    //symqsend_idv_ctx_t     *ctx = &ctx;  

    SYS_TRSF;

    SYS_DBG(" ================SYMQSEND_TAX START =================== ");

    /* MQ connect Initialization -> 연결안될 경우 , 연결        */
    SYS_TRY(d100_init_mqcon(ctx));
    if (rc == ERR_ERR) {
        SYS_DBG("d100_init_mqcon failed STEP 2");
        ex_syslog(LOG_FATAL, "[APPL_DM] d100_init_mqcon() STEP 2 FAIL [해결방안]시스템담당자 call" );
        ctx->is_mq_connected = 0;
        return rc; 
    }

    ctx->cb = commbuff;

    cp = sysocbgp(ctx->cb, IDX_HOSTSENDDATA);
    if (cp == NULL) {
        SYS_DBG("HOSTSENDDATA is NULL");
        return ERR_ERR;
    }

    if (HOSTSENDDATA == NULL){
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "INPUT DATA ERR");
        SYS_DBG("INPUT DATA ERR");
        return ERR_ERR;
    }

    SYS_DBG("HOSTSENDDATA[%s]", (char *)HOSTSENDDATA);
    /* MQ Initialization    */
    SYS_DBG("호출 #1 ------------------------------------------------- ");
    PRINT_EXMQPARM(g_mqi000f.in.exmqparm);
    //g_mqi000f.in.exmqparm = EXMQPARM;
    /******************************************************************/

    rc = e100_get_sendmsg(ctx);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "get msg ERR");
        SYS_DBG("get msg ERR");
        return ERR_ERR;
    }


    SYS_DBG(" ================SYMQSEND_TAX END   =================== ");
    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return ERR_ERR;

}
/* ------------------------------------------------------------------------------------------------------------ */
static int   b100_mqparm_load(symqsend_idv_ctx_t   *ctx)
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

    /***********************************************************************************/
    PRINT_EXMQPARM(mqi0001f.in.exmqparm);
    /***********************************************************************************/

    utotrim(EXMQPARM->mq_mngr);
    if (strlen(EXMQPARM->mq_mngr) == 0) {
        SYS_HSTERR(SYS_LF, SYS_GENERR, "[FAIL]EXMQPARM [%s/%s/%s].mq_mngr is null", g_chnl_code, g_appl_code, EXMQPARM->mq_mngr);
        return ERR_ERR;
    }

    utotrim(EXMQPARM->putq_name);
    if (strlen(EXMQPARM->putq_name) == 0) {
        SYS_HSTERR(SYS_LF, SYS_GENERR, "[FAIL]EXMQPARM [%s/%s/%s].getq_name is null", g_chnl_code, g_appl_code, EXMQPARM->putq_name);
        return ERR_ERR;
    }

    mqparam_load_flag = 1;

    memcpy(&g_mqi000f,  &mqi0001f,  sizeof(mqi0001f));

    SYS_TREF;

    return ERR_NONE;
}

/* ------------------------------------------------------------------------------------------------------------ */
static int   d100_init_mqcon(symqsend_idv_ctx_t    *ctx)
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
    //
    //
    /* ----------------------------------------------------------------------------------- */

    ctx->is_mq_connected = 1;

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int   e100_get_sendmsg(symqsend_idv_ctx_t   *ctx)
{

    int                 rc          = ERR_NONE;
    int                 len         = 0;
    int                 g_len       = 0;
    char                *jrn_no[LEN_SESSION_JRN_NO + 1] = {0};
    char                *cp;
    char                *hp;
    char                *ep;
    MQBYTE24            msgid[LEN_MQ_MSGID + 1];
    char                tmp_len[LEN_HCMIHEAD_DATA_LEN + 1];

    mq_info_t           mqinfo = ctx->mqinfo;
    mqimg001_t          mqimg001;
    eierrhead_t         eierrhead;

    SYS_TRSF;

    /* DB연결이 끊어진 경우 재 연결   */
    if ( db_connect("") != ERR_NONE){
        ex_syslog(LOG_ERROR, "e100_get_sendmsg db_connect re connecting ");
        return ERR_ERR;
    }

    /* TCPHEAD: ===================================== */
    SYS_DBG("TCPHEAD=============");
    hp = sysocbgp(ctx->cb, IDX_TCPHEAD);
    if (hp == NULL){
        SYS_DBG("TCPHEAD is null");
        return ERR_ERR;
    }

    SYS_DBG("TCPHEAD (hp): %.*s", LEN_HCMIHEAD, hp);
    SYS_DBG("%.*s", LEN_HCMIHEAD, (char *)TCPHEAD);
    SYS_DBG("corr_id %s", ((hcmihead_t *)TCPHEAD)->queue_name);

    /* --------------------------------------------------------------- */
    PRINT_HCMIHEAD((hcmihead_t *)hp);
    /* --------------------------------------------------------------- */

    /* 에러코드 초기화  */
    memset(MQMSG,   0x00,   sizeof(exmqmsg_001_t));
    memset(&eierrhead,  0x20, sizeof(eierrhead_t));
    
    /* MQMSG set */
    memcpy(MQMSG->chnl_code, g_chnl_code,    LEN_EXMQMSG_001_CHNL_CODE);
    memcpy(MQMSG->appl_code, g_appl_code,    LEN_EXMQMSG_001_APPL_CODE);
    memcpy(MQMSG->corr_id,  ((hcmihead_t *)TCPHEAD)->queue_name,  LEN_HCMIHEAD_QUEUE_NAME);
    utocick(MQMSG->msg_id);


    /* TCPHEAD 부 data length set */
    //sprintf(tmp_len, "%.*s", LEN_HCMIHEAD_DATA_LEN, ((hcmihead_t)TCPHEAD)->data_len);
    //len = atoi(tmp_len);

    len = sysocbgs(ctx->cb, IDX_HOSTSENDDATA);    
    SYS_DBG("# data len length [%d]", len);

    MQMSG->mqlen = len + LEN_HCMIHEAD = LEN_EIERRHEAD;

    //g_len =sysocbgs(ctx->cb, IDX_HOSTSENDDATA);
    //SYS_DBG("#2 get full data len length [%d]", g_len);

    /* TCP_HEADER Length set  */
    memset(tmp_len, 0x20, sizeof(tmp_len));
    sprintf(tmp_len, "%05d", len + LEN_EIERRHEAD);
    SYS_DBG("#2 data len length[%d]", tmp_len);

    memcpy(((hcmihead_t *)TCPHEAD)->data_len, tmp_len, LEN_HCMIHEAD_DATA_LEN);

    /* HOSTSEND DATA pointer get    */
    cp = sysocbgp(ctx->cb, IDX_HOSTSENDDATA);
    if (cp == NULL) {
        SYS_DBG("HOSTSENDDATA is NULL");
        return ERR_ERR;
    }

    /* EXTRECV DATA Pointer get  */
    ep = sysocbgp(ctx->cb, IDX_EXTRECVDATA);
    if (ep == NULL){
        SYS_DBG("EXTRECVDATA is NULL");
        return ERR_ERR;
    }

    g_len = sysocbgs(ctx->cb, IDX_EXTRECVDATA);
    SYS_DBG("#3 data len length[%d]", g_len);

    memcpy(eierrhead.err_code, "0000000", LEN_EIERRHEAD_ERR_CODE);
#if 0
    memcpy(MQMSG->mqmsg, cp , len);
#else 
    memcpy(MQMSG->mqmsg, hp, LEN_HCMIHEAD);
    memcpy(&MQMSG->mqmsg[LEN_HCMIHEAD], &eierrhead, LEN_EIERRHEAD);
    memcpy(&MQMSG->mqmsg[LEN_HCMIHEAD+LEN_EIERRHEAD], cp , len);
#endif    
    
    /* -------------------------------- */
    PRINT_EXMQMSG(&MQMSG);
    /* -------------------------------- */

    
    //MQPUT
    if (f100_put_mqmsg(ctx) == ERR_ERR){
        SYS_DBG("CALL f100_put_mqmsg !!!!!!!!");
        return ERR_ERR;
    }

    memset(&mqimsg001, 0x00, sizeof(mqimsg001_t));
    mqimg001.in.mqmsg_001 = MQMSG;
    memcpy(mqimg001.in.job_proc_type, "4" , LEN_MQIMG001_PROC_TYPE);
    rc = mqomsg001(&mqimg001);
    if (rc == ERR_ERR){
        EXEC SQL ROLLBACK WORK;
        return ERR_ERR;
    }

    EXEC SQL COMMIT WORK;

    return ERR_NONE;
}
/* ------------------------------------------------------------------------------------------------------------ */
static int   f100_put_mqmsg(symqsend_idv_ctx_t *ctx)
{
    
    int                 rc     = ERR_NONE;
    mq_info_t           mqinfo = {0};
    MQMD                md     = {MQMD_DEFAULT};



    SYS_TRSF;

    /* MQPUT을 위한 mq_info_t Set : MQIHEAD + INPDATA   */
    mqinfo.hobj             = &ctx->mqhobj;         /* PUT QUEUE OBJECT */
    mqinfo.hcon             = ctx->mqhconn;         /* QM Connection    */
    mqinfo.coded_charset_id = EXMQPARM->charset_id  /* CodedCharSetID   */

    /* ----------------------------------------------------------------------------------- */
    SYS_DBG("CONN Handler       :[%d][%08x]", ctx->mqhconn, ctx->mqhconn);
    SYS_DBG("GET QUEUE Handler  :[%d][%08x]", ctx->mqhobj , ctx->mqhobj );
    /* ----------------------------------------------------------------------------------- */
    
    memcpy(mqinfo.msg_id,       MQMSG->msg_id,  LEN_EXMQMSG_001_MSG_ID);
    memcpy(mqinfo.corrid,       MQMSG->corr_id,  strlen(MQMSG->corr_id));
    memcpy(mqinfo.msgbuf,       MQMSG->mqmsg,    MQMSG->mqlen);
    
    if (MQMSG->mqlen < sizeof(MQMSG->mqlen)) {
        SYS_DBG("MQMSG TEST FAIL !!!");
        return ERR_ERR;
    }

    rc = ex_mq_put_with_opts(&mqinfo, &md, 0);
    if (rc == ERR_NONE){
        rc = ex_mq_commit(&ctx->mqhconn);

        memcpy(MQMSG->mq_date,   md.PutDate, LEN_EXMQMSG_001_MQ_DATE);
        memcpy(MQMSG->mq_time,   md.PutTime, LEN_EXMQMSG_001_MQ_TIME);

    } else {

        /* reset flag for mq initialize */
        ctx->is_mq_connected = 0;

        SYS_DBG("CALL ex_mq_put FAIL !!!");
        ex_syslog(LOG_FATAL, "[APPL_DM] CALL ex_mq_put FAIL [해결방안]시스템 담당자  CALL");
        //ex_mq_rollback(&ctx->mqhconn);
        ex_mq_commit(&ctx->mqhconn);
    }

    SYS_TREF;

    return ERR_NONE;
}

/* ------------------------------------------------------------------------------------------------------------ */
int   apsvrdone()
{
    
    int           i;  

    SYS_DBG("MQSEND complete =============================== ");
    
    return ERR_NONE;
}
/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */