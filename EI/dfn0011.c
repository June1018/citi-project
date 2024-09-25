
/*  @file               dfn0011.pc
*   @file_type          pc source program
*   @brief              외화자금이체 (FCY_DFT)개설응답 
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
#include <exparm.h>
#include <dfi0002f.h>
#include <dfi3100f.h>
#include <sqlca.h>
#include <bssess_stat.h>
#include <hcmihead.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXMSG11000              (ctx->exmsg11000)
#define EXMSG11000DETL          (ctx->exmsg11000->detl_area)
#define LEN_KTIHEAD             500
/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct dfn0011_ctx_s dfn0011_ctx_t;
struct dfn0011_ctx_s {
    commbuff_t          *cb;

    exmsg11000_t        _exmsg11000; /* host send 용 structure   */
    exmsg11000_t        *exmsg11000;

    char                proc_date      [LEN_DFI0002F_PROC_DATE     + 1];   /* 처리일자         */
    char                proc_time      [LEN_DFI0002F_PROC_TIME     + 1];   /* 처리시간         */
    char                msg_type       [LEN_DFI0002F_MSG_TYPE      + 1];   /* 전문종별코드      */
    char                msg_no         [LEN_DFI0002F_MSG_NO        + 1];   /* 거래고유번호      */
    char                trace_no       [LEN_DFI0002F_TRACE_NO      + 1];   /* 전문추적번호      */
    char                proc_code      [LEN_DFI0002F_PROC_CODE     + 1];   /* 거래구분코드      */
    char                corr_id        [LEN_DFI0002F_CORR_ID       + 1];   /* corr_id        */
    char                rspn_code      [LEN_DFI0002F_RSPN_CODE     + 1];   /* 응답코드         */
    char                tx_code        [LEN_DFI0002F_TX_CODE       + 1];   /* tx_code        */ 
    char                io_flag;                                           /* 시스템 구분 0:GCG 1: KTI  */
    char                kti_flag;
    char                mgr_flag [ 1+ 1];
    char                host_send_msg  [10000 + 1];
    int                 recv_len;                                          /* 수신길이         */
    int                 send_len;                                          /* 대외기관 전송길이  */
};
/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int a000_data_receive(dfn0011_ctx_t  *ctx, commbuff_t  *commbuff);
static int b000_init_proc(dfn0011_ctx_t *ctx);
static int d000_upd_jrn_proc(dfn0011_ctx_t *ctx);
static int e000_ext_msg_send(dfn0011_ctx_t *ctx);
static int z100_log_insert(dfn0011_ctx_t *ctx), char *log_data, int size, char io_flag, char sr_flag);

/* ----------------------------------------------------------------------------------------------------------- */
int dfn0011(commbuff_t  *commbuff)
{
    int                 rc = ERR_NONE;
    dfn0011_ctx_t   _ctx; 
    dfn0011_ctx_t   *ctx = &_ctx;

    SYS_TRSF;
    /* 데이터 수신  */
    SYS_TRY(a000_data_receive(ctx, commbuff));
    /* 초기화 처리  */
    SYS_TRY(b000_init_proc(ctx));
    /* 대외기관 SEND    */
    SYS_TRY(e000_ext_msg_send(ctx));
    /* 관리전문 거래전문 전송 - 한쪽 gw 문제가 있어도 다른 gw영향 없는지 */
    if (ctx->mgr_flag[0] != '1' ){
        /* dfjrn UPDATE - 관리전문 제외     */
        SYS_TRY(d000_upd_jrn_proc(ctx));
    }

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:
    if (rc == GOB_NRM) {
        SYS_DBG("RETURN ERR_NONE");
        return ERR_NONE;
    }

    SYS_TREF;

    return ERR_ERR;
}

/* ----------------------------------------------------------------------------------------------------------- */
static int a000_data_receive(dfn0011_ctx_t   *ctx, commbuff_t *commbuff)
{
    int                 rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(dfn0011_ctx_t));
    ctx->cb = commbuff;

    SYS_ASSERT(HOSTRECVDATA);

    /* input channel clear */
    SYSICOMM->intl_tx_flag = 0;
    memset(SYSICOMM->call_svc_name, 0, sizeof(SYSICOMM->call_svc_name));
    memcpy(SYSICOMM->call_svc_name, "DFN0011", 7);
    /* 11000메세지          */
    ctx->exmqmsg11000 = &ctx->_exmsg11000;

    SYS_TREF;    

    return ERR_NONE;
}

/* ----------------------------------------------------------------------------------------------------------- */
static int b000_init_proc(dfn0011_ctx_t *ctx)
{
    int                 rc = ERR_NONE;
    char                *hp; 
    char                temp_str[LEN_EXMSG11000_DETL_REC_SIZE + 1];
    char                detl_len[LEN_HCMIHEAD_DATA_LEN];
    char                data_len[LEN_HCMIHEAD_DATA_LEN];
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
    }else{
        SYS_DBG("[b000]init_proc: TCPHEAD is NULL");
        return GOB_ERR;
    }

    memset(temp_str, 0x00, LEN_EXMSG11000_DETL_REC_SIZE + 1);
    /* 전문길이 세팅    */
    utol2an( (int)sysocbgs(ctx->cb, IDX_EXTRECVDATA), LEN_EXMSG11000_DETL_REC_SIZE, temp_str);
    memcpy(EXMSG11000->detl_rec_size, temp_str      , LEN_EXMSG11000_DETL_REC_SIZE);
    memcpy(&EXMSG11000->detl_area   , HOSTRECVDATA  , sysocbgs(ctx->cb, IDX_HOSTRECVDATA));

    memset(detl_len, 0x00, sizeof(detl_len));
    memset(data_len, 0x00, sizeof(data_len));

    if (memcmp(ctx->tx_code, "6000100073", 10) == 0){
        memcpy(ctx->msg_type    , &EXMSG11000->detl_area[12], LEN_DFI0002F_MSG_TYPE );   /* msg_type     */
        memcpy(ctx->proc_code   , &EXMSG11000->detl_area[16], LEN_DFI0002F_PROC_CODE);   /* proc_code    */
        memcpy(ctx->rspn_code   , &EXMSG11000->detl_area[26], LEN_DFI0002F_RSPN_CODE);   /* rspn_code    */
        memcpy(ctx->trace_no    , &EXMSG11000->detl_area[46], LEN_DFI0002F_TRACE_NO );   /* trace_no     */
        memcpy(ctx->msg_no      , &EXMSG11000->detl_area[64], LEN_DFI0002F_MSG_NO   );   /* msg_no       */
        ctx->io_flag = 'I';
        memcpy(detl_len,        , &EXMSG11000->detl_area[4] , LEN_HCMIHEAD_DATA_LEN );
        ctx->recv_len = utoa2in(detl_len, LEN_HCMIHEAD_DATA_LEN);
        ctx->send_len = ctx->recv_len + 9;  //THDR12345
        memcpy(ctx->host_send_msg , &EXMSG11000->detl_area , ctx->send_len );
        ctx->kti_flag = '0'
    }else{
        memcpy(ctx->msg_type    , &EXMSG11000->detl_area[512], LEN_DFI0002F_MSG_TYPE );   /* msg_type     */
        memcpy(ctx->proc_code   , &EXMSG11000->detl_area[516], LEN_DFI0002F_PROC_CODE);   /* proc_code    */
        memcpy(ctx->rspn_code   , &EXMSG11000->detl_area[526], LEN_DFI0002F_RSPN_CODE);   /* rspn_code    */
        memcpy(ctx->trace_no    , &EXMSG11000->detl_area[546], LEN_DFI0002F_TRACE_NO );   /* trace_no     */
        memcpy(ctx->msg_no      , &EXMSG11000->detl_area[564], LEN_DFI0002F_MSG_NO   );   /* msg_no       */
        ctx->io_flag = 'I';
        memcpy(detl_len,        , &EXMSG11000->detl_area[504], LEN_HCMIHEAD_DATA_LEN );
        ctx->recv_len = utoa2in(detl_len, LEN_HCMIHEAD_DATA_LEN);
        ctx->send_len = ctx->recv_len + 9;  //THDR12345
        memcpy(ctx->host_send_msg , &EXMSG11000->detl_area , ctx->send_len );
        ctx->kti_flag = '1'
    }
    /* 관리전문 set     */
    if (memcmp (ctx->msg_type, "0800", LEN_DFI0002F_MSG_TYPE) == 0 ||
        memcmp (ctx->msg_type, "0810", LEN_DFI0002F_MSG_TYPE) == 0 ) {
        ctx->mgr_flag[0] = '1';                         /* 관리전문 1 set           */
        memset(ctx->msg_no, 0x00, LEN_DFI0002F_MSG_NO); /* 관리전문 msg_no 초기회     */
    }
    /* z100 log insert: trace_no(전문추적번호)NULL이면 처리 안됨    */
    rc = z100_log_insert(ctx, (char *)EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), 'I', '3');   /* 개설요청 io_flag:I sr_flag:3 [PP -> EI]  */

    SYS_TREF;

    return ERR_NONE;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int e000_ext_msg_send(dfn0011_ctx_t *ctx)
{
    int                 rc = ERR_NONE;

    SYS_TRSF;

    /* 데이터가 없는 경우 전송하지 않음     */
    if (HOSTRECVDATA == NULL)
        return ERR_NONE;

    /* -------------------------------------------------------------------------------------------------- */
    SYS_DBG("e000_ext_msg_send send_len[%s]host_send_msg[%s]", ctx->send_len, ctx->host_send_msg );
    /* -------------------------------------------------------------------------------------------------- */
    rc = sysocbsi(ctx->cb, IDX_EXTSENDDATA, ctx->host_send_msg, ctx->send_len);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "COMMBUFF SET ERROR");
        return ERR_ERR;
    }

    /* 대외기관 G/W에 호출하는 방식을 전달하기 위한 값을 set    */
    strcpy(SYSGWINFO->func_name, "TCPDFC");
    SYSGWINFO->time_val     = SYSGWINFO_SAF_DFLT_TIMEOUT;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_SAF;
    SYSGWINFO->rspn_flag    = SYSGWINFO_SVC_REPLY;
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;

    /* 대외기관 데이터 전송     */
    rc - sys_tpcall("SYMQSEND_DFK", ctx->cb, TPNOTRAN);
    if ( rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s dfn0010(KTI)- e000_ext_msg_send() ERROR %d [해결방안]HOST G/W 담당자 call",
                            __FILE__, tperrno);
        return ERR_ERR;
    }

    rc = z100_log_insert(ctx, (char *)HOSTRECVDATA, sysocbgs(ctx->cb, IDX_HOSTRECVDATA), 'I', '4');     /* 개설요청 io_flag:I sr_flag:4 [EI -> KFTC]  */

    SYS_TREF;

    return ERR_NONE;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int d000_upd_jrn_proc(dfn0011_ctx_t *ctx)
{
    int                 rc = ERR_NONE;
    dfi0002f_t          dfi0002f;

    SYS_TRSF;

    memset(&dfi0002f, 0x00, sizeof(dfi0002f_t));
    dfi0002f.in.exmsg11000 = EXMSG11000;
    /*--------------------------------------------------------------*/
    //PRINT_EXMSG11000(EXMSG11000);
    /*--------------------------------------------------------------*/
    memcpy(dfi0002f.in.tx_code          , ctx->tx_code      , LEN_DFI0002F_TX_CODE   );     /* tx_code       */
    memcpy(dfi0002f.in.msg_no           , ctx->msg_no       , LEN_DFI0002F_MSG_NO    );     /* 거래고유번호     */
    memcpy(dfi0002f.in.msg_type         , ctx->msg_type     , LEN_DFI0002F_MSG_TYPE  );     /* msg_type      */
    memcpy(dfi0002f.in.proc_code        , ctx->proc_code    , LEN_DFI0002F_PROC_CODE );     /* 거래구분코드     */
    memcpy(dfi0002f.in.corr_id          , ctx->corr_id      , LEN_DFI0002F_CORR_ID   );     /* corr_id       */
    dfi0002f.in.io_flag[0]  = ctx->io_flag;
    dfi0002f.in.kti_flag[0] = ctx->kti_flag[0];

    if (memcmp(ctx->msg_type, "0400", LEN_DFI0002F_MSG_TYPE) ==0 ||
        memcmp(ctx->msg_type, "0410", LEN_DFI0002F_MSG_TYPE) ==0 ) {
        utodate1(ctx->proc_date);       //조회조건일자 
        memcpy(dfi0002f.in.proc_date       ,  ctx->proc_date    , LEN_DFI0002F_PROC_DATE );
        memcpy(dfi0002f.in.canc_rspn_code  ,  ctx->rspn_code    , LEN_DFI0002F_RSPN_CODE );
        memcpy(dfi0002f.in.canc_trace_no   ,  ctx->trace_no     , LEN_DFI0002F_TRACE_NO  );
        rc = df_jrn_canc_upd(&dfi0002f);
        if (rc == ERR_ERR){
            /* 취소 개설 거래시 [SIT TEST CASE 052] CANC_TYPE(df_jrn_canc_upd):[1] default canc_trace_no, corr_id */
            ex_syslog(LOG_FATAL, "[APPL_DM] % d000_upd_jrn_proc() :msg_type[%s],canc_trace_no[%s]corr_id[%s]KTI_FLAG[%s]", __FILE__, dfi0002f.in.msg_type, dfi0002f.in.canc_trace_no, dfi0002f.in.corr_id, dfi0002f.in.kti_flag);
            return ERR_ERR;
        }
    }else{
        memcpy(dfi0002f.in.trace_no         , ctx->trace_no     , LEN_DFI0002F_TRACE_NO  );     /* 전문추적번호     */
        memcpy(dfi0002f.in.rspn_code       ,  ctx->rspn_code    , LEN_DFI0002F_RSPN_CODE );
        rc = df_jrn_insupd(&dfi0002f);
        if (rc == ERR_ERR) {
            ex_syslog(LOG_ERROR, "[APPL_DM] %s d000_jrn_insert :msg_type[%s],canc_trace_no[%s]corr_id[%s]KTI_FLAG[%s]", __FILE__, dfi0002f.in.msg_type, dfi0002f.in.canc_trace_no, dfi0002f.in.corr_id, dfi0002f.in.kti_flag);
            dfi0002f.in.kti_flag[0] = '1'; /* KTI flag(1)   */
            //return ERR_ERR;
        }
    }

    SYS_TREF;

    return ERR_NONE;

}

/* ----------------------------------------------------------------------------------------------------------- */
static int z100_log_insert(dfn0011_ctx_t *ctx), char *log_data, int size, char io_flag, char sr_flag)
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
/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */