
/*  @file               dfn0000.pc
*   @file_type          pc source program
*   @brief              외화자금이체 (FCY_DFT)취급요청 
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
#include <utodate.h>
#include <exdefine.h>
#include <exmsg11000.h>
#include <dfi0002f.h>
#include <dfi3100f.h>
#include <sqlca.h>
#include <hcmihead.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXMSG11000              (ctx->exmsg11000)
#define FCY_APPL_CODE           "073"
/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct dfn0000_ctx_s dfn0000_ctx_t;
struct dfn0000_ctx_s {
    commbuff_t          *cb;

    exmsg11000_t        _exmsg11000; /* host send 용 structure   */
    exmsg11000_t        *exmsg11000;

    char                msg_no         [LEN_DFI0002F_MSG_NO        + 1];   /* 거래고유번호      */
    char                msg_type       [LEN_DFI0002F_MSG_TYPE      + 1];   /* 전문종별코드      */
    char                proc_code      [LEN_DFI0002F_PROC_CODE     + 1];   /* 거래구분코드      */
    char                trace_no       [LEN_DFI0002F_TRACE_NO      + 1];   /* 전문추적번호      */
    char                tx_code        [LEN_DFI0002F_TX_CODE       + 1];   /* tx_code        */ 
    char                rspn_code      [LEN_DFI0002F_RSPN_CODE     + 1];   /* 응답코드         */
    char                io_flag;                                           /* 시스템 구분 0:GCG 1: KTI  */
    char                kti_flag;
    char                mgr_flag [ 1+ 1];
    char                host_send_msg  [11000 + 1];
    int                 recv_len;                                          /* 수신길이         */
    int                 send_len;                                          /* 대외기관 전송길이  */
};
/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int a000_data_receive(dfn0000_ctx_t  *ctx, commbuff_t  *commbuff);
static int b000_init_proc(dfn0000_ctx_t *ctx);
static int c000_jrn_insert(dfn0000_ctx_t *ctx);
static int m000_ext_msg_send(dfn0000_ctx_t *ctx);
static int z000_error_proc(dfn0000_ctx_t *ctx);
static int z100_log_insert(dfn0000_ctx_t *ctx), char *log_data, int size, char io_flag, char sr_flag);

/* ----------------------------------------------------------------------------------------------------------- */
int dfn0000(commbuff_t  *commbuff)
{
    int                 rc = ERR_NONE;
    dfn0000_ctx_t   _ctx; 
    dfn0000_ctx_t   *ctx = &_ctx;

    SYS_TRSF;
    /* 데이터 수신  */
    SYS_TRY(a000_data_receive(ctx, commbuff));
    /* 초기화 처리  */
    SYS_TRY(b000_init_proc(ctx));

    /* 관리전문 거래전문 전송 */
    if (ctx->mgr_flag[0] != '1' ){
        /* dfjrn Insert - 관리전문 제외     */
        SYS_TRY(c000_jrn_insert(ctx));
    }
    /* 대외기관 SEND    */    
    SYS_TRY(m000_ext_msg_send(ctx));
    SYSGWINFO->gw_rspn_send = SYSGWINFO_GW_REPLY_SEND_NO;   /* 호스트에 응답 전송 안함 */

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:
    switch(rc) {
        case GOB_ERR;
            SYS_TREF;
            /* host 무응답 */
            SYSGWINFO->gw_rspn_send = SYSGWINFO_GW_REPLY_SEND_NO;
            return ERR_ERR;
        //비밀번호 변경 전문일 경우 GW전송안하고, 처리한다. 
        case GOB_NRM:
            SYS_TREF;
            return ERR_NONE;
        default:
            z000_err_ext_send(ctx);
    }

    SYS_TREF;

    return ERR_ERR;
}

/* ----------------------------------------------------------------------------------------------------------- */
static int a000_data_receive(dfn0000_ctx_t   *ctx, commbuff_t *commbuff)
{
    int                 rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(dfn0000_ctx_t));
    ctx->cb = commbuff;

    SYS_ASSERT(HOSTRECVDATA);

    /* input channel clear */
    SYSICOMM->intl_tx_flag = 0;
    memset(SYSICOMM->call_svc_name, 0, sizeof(SYSICOMM->call_svc_name));
    memcpy(SYSICOMM->call_svc_name, "DFN0000", 7);
    /* 11000메세지          */
    ctx->exmqmsg11000 = &ctx->_exmsg11000;

    SYS_TREF;

    return ERR_NONE;
}

/* ----------------------------------------------------------------------------------------------------------- */
static int b000_init_proc(dfn0000_ctx_t *ctx)
{
    int                 rc = ERR_NONE;
    char                *hp; 
    char                msg_type[2];
    char                tmp_str[LEN_EXMSG11000_DETL_REC_SIZE + 1];
    char                detl_len[LEN_HCMIHEAD_DATA_LEN];
    hcmihead_t          hcmihead;
    exmqmsg11000_t      exmqmsg11000;

    SYS_TRSF;

    /* TCP/IP 통신헤더 정보     */
    hp = sysocbgp(ctx->cb, IDX_TCPHEAD);
    if (hp != NULL){
        memset(&hcmihead, 0x00, sizeof(hcmihead_t));
        memcpy(&hcmihead, hp  , sysocbgs(ctx->cb, IDX_TCPHEAD));
        memcpy(ctx->tx_code, hcmihead.tx_code   , LEN_HCMIHEAD_TX_CODE);
        memcpy(ctx->corr_id, hcmihead.queue_name, LEN_HCMIHEAD_QUEUE_NAME);
        memcpy(ctx->rspn_code, hcmihead.resp_code, LEN_HCMIHEAD_RESP_CODE);
        SYS_DBG("corr_id  : [%s]", ctx->corr_id );
        SYS_DBG("tx_code  : [%s]", ctx->tx_code );
        SYS_DBG("rspn_code: [%s]", ctx->rspn_code );

    }else{
        SYS_DBG("[b000]init_proc: TCPHEAD is NULL");
        return GOB_ERR;
    }

    memset(temp_str, 0x00, sizeof(temp_str));
    memset(datl_len, 0x00, sizeof(detl_len));
    /* 전문길이 세팅    */
    utol2an( (int)sysocbgs(ctx->cb, IDX_EXTRECVDATA), LEN_EXMSG11000_DETL_REC_SIZE, temp_str);
    memcpy(EXMSG11000->detl_rec_size, temp_str      , LEN_EXMSG11000_DETL_REC_SIZE);
    memcpy(&EXMSG11000->detl_area   , HOSTRECVDATA  , sysocbgs(ctx->cb, IDX_HOSTRECVDATA));

    /* Corebanking 취급 ---------------------------- */
    if (memcmp(ctx->tx_code, "6000100073", 10) == 0){
        /* Corebanking 취급 ---------------------------- */
        SYS_DBG("Corebanking 취급수신자료 !!! LEN[%d]DATA[%s]", ctx->recv_len, &EXMSG11000->detl_area);
        SYS_DBG("HOSTSENDDATA(%d)[%.*s]", sysocbgs(ctx->cb, IDX_HOSTRECVDATA), sysocbgs(ctx->cb, IDX_HOSTRECVDATA), HOSTRECVDATA);
        memcpy(ctx->msg_type    , &EXMSG11000->detl_area[12], LEN_DFI0002F_MSG_TYPE );   /* msg_type     */
        memcpy(ctx->proc_code   , &EXMSG11000->detl_area[16], LEN_DFI0002F_PROC_CODE);   /* proc_code    */
        memcpy(ctx->rspn_code   , &EXMSG11000->detl_area[26], LEN_DFI0002F_RSPN_CODE);   /* rspn_code    */
        memcpy(ctx->trace_no    , &EXMSG11000->detl_area[46], LEN_DFI0002F_TRACE_NO );   /* trace_no     */
        memcpy(ctx->msg_no      , &EXMSG11000->detl_area[64], LEN_DFI0002F_MSG_NO   );   /* msg_no       */
        ctx->io_flag = 'O';
        memcpy(detl_len,        , &EXMSG11000->detl_area[4] , LEN_HCMIHEAD_DATA_LEN );
        ctx->recv_len = utoa2in(detl_len, LEN_HCMIHEAD_DATA_LEN);
        ctx->send_len = ctx->recv_len + 9;  //THDR12345
        SYS_DBG("b000_init_proc[Forex]send_len[%d]", ctx->send_len);
        memcpy(ctx->host_send_msg , &EXMSG11000->detl_area , ctx->send_len );
        ctx->kti_flag = '0'
    }else{
        /* KTI 취급 -------------------------------------------------- */
        /* Host 수신 msg저장 KTI 72 + 200 + 300 +10700                  */
        /* ---------------------------------------------------------- */
        SYS_DBG("KTI 취급수신자료 !!! LEN[%d]DATA[%s]", ctx->recv_len, &EXMSG11000->detl_area);
        SYS_DBG("HOSTSENDDATA(%d)[%.*s]", sysocbgs(ctx->cb, IDX_HOSTRECVDATA), sysocbgs(ctx->cb, IDX_HOSTRECVDATA), HOSTRECVDATA);
        memcpy(ctx->msg_type    , &EXMSG11000->detl_area[512], LEN_DFI0002F_MSG_TYPE );   /* msg_type     */
        memcpy(ctx->proc_code   , &EXMSG11000->detl_area[516], LEN_DFI0002F_PROC_CODE);   /* proc_code    */
        memcpy(ctx->rspn_code   , &EXMSG11000->detl_area[526], LEN_DFI0002F_RSPN_CODE);   /* rspn_code    */
        memcpy(ctx->trace_no    , &EXMSG11000->detl_area[546], LEN_DFI0002F_TRACE_NO );   /* trace_no     */
        memcpy(ctx->msg_no      , &EXMSG11000->detl_area[564], LEN_DFI0002F_MSG_NO   );   /* msg_no       */
        ctx->io_flag = 'O';
        memcpy(detl_len,        , &EXMSG11000->detl_area[504], LEN_HCMIHEAD_DATA_LEN );
        ctx->recv_len = utoa2in(detl_len, LEN_HCMIHEAD_DATA_LEN);
        ctx->send_len = ctx->recv_len + 9;  //THDR12345
        memcpy(ctx->host_send_msg , &EXMSG11000->detl_area[500], ctx->send_len );
        ctx->kti_flag = '1'
    }
#ifdef  _DEBUG
    //PRINT_EXMSG11000(EXMSG11000);
#endif 
    /*---------------------------------------------------------------------------*/
    SYS_DBG("b000_init_proc:     msg_type[%s]", ctx->msg_type  );
    SYS_DBG("b000_init_proc:    proc_code[%s]", ctx->proc_code );
    SYS_DBG("b000_init_proc:    rspn_code[%s]", ctx->rspn_code );
    SYS_DBG("b000_init_proc:     trace_no[%s]", ctx->trace_no  );
    SYS_DBG("b000_init_proc:       msg_no[%s]", ctx->msg_no    );
    SYS_DBG("b000_init_proc:      io_flag[%c]", ctx->io_flag   );
    SYS_DBG("b000_init_proc:host_send_msg[%s]", ctx->host_send_msg);
    /*---------------------------------------------------------------------------*/

    /* 관리전문 -------------------------------------------------------------------*/
    if (memcmp (ctx->msg_type, "0800", LEN_DFI0002F_MSG_TYPE) == 0 ||
        memcmp (ctx->msg_type, "0810", LEN_DFI0002F_MSG_TYPE) == 0 ) {
        ctx->mgr_flag[0] = '1';                         /* 관리전문 1 set           */
        memset(ctx->msg_no, 0x00, LEN_DFI0002F_MSG_NO); /* 관리전문 msg_no 초기회     */
    }
    
    SYS_DBG("[b000]HOSTRECVDATA[%d][%.*s]",  sysocbgs(ctx->cb, IDX_HOSTRECVDATA), sysocbgs(ctx->cb, IDX_HOSTRECVDATA), HOSTRECVDATA);    
    /* z100 log insert: trace_no(전문추적번호)NULL이면 처리 안됨    */
    rc = z100_log_insert(ctx, (char *)EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), 'O', '1');   /* 취급요청 o_flag:O sr_flag:1 [PP -> EI]  */

    SYS_TREF;

    return ERR_NONE;
}

/* ----------------------------------------------------------------------------------------------------------- */
static int c000_jrn_insert(dfn0000_ctx_t *ctx)
{
    int                 rc = ERR_NONE;
    dfi0002f_t          dfi0002f;

    SYS_TRSF;

    memset(&dfi0002f, 0x00, sizeof(dfi0002f_t));
    dfi0002f.in.exmsg11000 = EXMSG11000;
    /*--------------------------------------------------------------*/
    //PRINT_EXMSG11000(EXMSG11000);
    /*--------------------------------------------------------------*/
    utodate1(ctx->proc_date);       //조회조건일자 
    memcpy(dfi0002f.in.proc_date       ,  ctx->proc_date    , LEN_DFI0002F_PROC_DATE );
    memcpy(dfi0002f.in.tx_code          , ctx->tx_code      , LEN_DFI0002F_TX_CODE   );     /* tx_code       */
    memcpy(dfi0002f.in.msg_no           , ctx->msg_no       , LEN_DFI0002F_MSG_NO    );     /* 거래고유번호     */
    memcpy(dfi0002f.in.msg_type         , ctx->msg_type     , LEN_DFI0002F_MSG_TYPE  );     /* msg_type      */
    memcpy(dfi0002f.in.proc_code        , ctx->proc_code    , LEN_DFI0002F_PROC_CODE );     /* 거래구분코드     */
    memcpy(dfi0002f.in.corr_id          , ctx->corr_id      , LEN_DFI0002F_CORR_ID   );     /* corr_id       */
    dfi0002f.in.io_flag[0]  = ctx->io_flag;
    dfi0002f.in.kti_flag[0] = ctx->kti_flag[0];

    if (memcmp(ctx->msg_type, "0400", LEN_DFI0002F_MSG_TYPE) ==0 ||
        memcmp(ctx->msg_type, "0410", LEN_DFI0002F_MSG_TYPE) ==0 ) {
        memcpy(dfi0002f.in.canc_rspn_code  ,  ctx->rspn_code    , LEN_DFI0002F_RSPN_CODE );
        memcpy(dfi0002f.in.canc_trace_no   ,  ctx->trace_no     , LEN_DFI0002F_TRACE_NO  );
        dfi0002f.in.canc_type[0] = '1';
    }

    rc = df_jrn_insupd(&dfi0002f);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_jrn_insert :ERROR ", __FILE__);
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;

}

/* ----------------------------------------------------------------------------------------------------------- */
static int m000_ext_msg_send(dfn0000_ctx_t *ctx)
{
    int                 rc = ERR_NONE;

    SYS_TRSF;

    /* 데이터가 없는 경우 전송하지 않음     */
    if (HOSTRECVDATA == NULL)
        return ERR_NONE;

    SYS_DBG("[m000_ext_msg_send] ctx->host_send_msg[%s]", ctx->host_send_msg );
    rc = sysocbsi(ctx->cb, IDX_EXTSENDDATA, ctx->host_send_msg, ctx->send_len);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "COMMBUFF SET ERROR");
        return ERR_ERR;
    }

    /* 대외기관 G/W에 호출하는 방식을 전달하기 위한 값을 set    */
    SYSGWINFO->time_val     = SYSGWINFO_SAF_DFLT_TIMEOUT;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_SAF;
    SYSGWINFO->rspn_flag    = SYSGWINFO_SVC_REPLY;
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;

    /* 대외기관 데이터 전송     */
    strcpy(SYSGWINFO->func_name, "TCPDFC");    
    rc - sys_tpcall("SYEXTGW_DF", ctx->cb, TPNOTRAN);
    if ( rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "COMMBUFF SET ERR");
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0000- m000_ext_msg_send() ERROR %d [해결방안]HOST G/W 담당자 call",
                            __FILE__, tperrno);
        return ERR_ERR;
    }

    rc = z100_log_insert(ctx, (char *)HOSTRECVDATA, sysocbgs(ctx->cb, IDX_HOSTRECVDATA), 'O', '2');     /* 취급거래 io_flag:O sr_flag:2 [EI -> KFTC]  */
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0000 : DFLOG[EI->KFTC]", __FILE__ );
    }

    SYS_TREF;

    return ERR_NONE;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int z000_error_proc(dfn0000_ctx_t *ctx)
{

    int                 rc  = ERR_NONE;
    int                 len;
    char                err_pgm[20];

    SYS_TRSF;

    /* ------------------------------------------------------ */
    SYS_DBG("z000_error_proc: error_code[%d]" , sys_err_code());
    SYS_DBG("z000_error_proc: error_msg [%s]" , sys_err_msg());
    /* ------------------------------------------------------ */
    memcpy(EXMSG11000->tx_code  , ctx->tx_code,   LEN_DFI0002F_TX_CODE );
    memcpy(EXMSG11000->err_code , "999999"    ,   LEN_EXMSG11000_ERR_CODE);

    SET_11000ERROR_INFO(EXMSG11000);

    SYS_DBG("EXMSG11000->tx_code[%.10s]", EXMSG11000->tx_code );
    SYS_DBG("EXMSG11000->err_code[%.7s]", EXMSG11000->err_code);

    SYSGWINFO->gw_rspn_send = SYSGWINFO_GW_REPLY_SEND_NO;       /* 호스트에 응답전송  */
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;               /* 1200전문에는 etc 전문구분 : 0=1200 1=etc 전문   */

    rc =  z100_log_insert(ctx, HOSTRECVDATA, ctx->recv_len, '0', '2');  /* 취급거래 io_flag:O sr_flag:2 [EI -> KFTC]  */
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0000 : DFLOG[EI->KFTC]", __FILE__ );
    }

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int z100_log_insert(dfn0000_ctx_t *ctx), char *log_data, int size, char io_flag, char sr_flag)
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
    dfi3100f.in.kti_flag = ctx->kti_flag;
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
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0000: z100_log_insert() DFLOG sysocbdb ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }

    rc = sysocbsi(&dcb, IDX_EXMSG1200, &dfi3100f sizeof(dfi3100f_t));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0000: z100_log_insert() DFLOG sysocbsi ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        sysocbfb(&dcb);
        return ERR_NONE;
    }

    rc = sys_tpacall("DFN3100F", &dcb, TPNOREPLY | TPNOTRAN);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0000: z100_log_insert() DFLOG sys_tpacall ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }
    
    sysocbfb(&dcb);

    SYS_TREF;

    return ERR_NONE;
}
/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */