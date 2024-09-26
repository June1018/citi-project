/*  @file               dfn0020.pc
*   @file_type          pc source program
*   @brief              외화자금이체 (FCY_DFT) 취급응답 main
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
#include <common_define.h>
#include <exmsg11000.h>
#include <hcmihead.h>
#include <exi0331x.h>
#include <dfi0001x.h>
#include <dfi0002f.h>
#include <dfi0003f.h>
#include <dfi3100f.h>
#include <sqlca.h>
#include <dfjrn.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXMSG11000              (ctx->exmsg11000)
#define FCY_CODE                "073"
#define LEN_KTIHEAD             500

/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct dfn0020_ctx_s dfn0020_ctx_t;
struct dfn0020_ctx_s {
    commbuff_t          *cb;

    exmsg11000_t        _exmsg11000; /* host send 용 structure   */
    exmsg11000_t        *exmsg11000;

    char                msg_no         [LEN_DFI0002F_MSG_NO        + 1];   /* 거래고유번호      */
    char                msg_type       [LEN_DFI0002F_MSG_TYPE      + 1];   /* 전문종별코드      */
    char                proc_code      [LEN_DFI0002F_PROC_CODE     + 1];   /* 거래구분코드      */
    char                corr_id        [LEN_DFI0002F_CORR_ID       + 1];   /* corr_id        */
    char                proc_date      [LEN_DFI0002F_PROC_DATE     + 1];   /* 처리일자         */
    char                trace_no       [LEN_DFI0002F_TRACE_NO      + 1];   /* 전문추적번호      */
    char                ei_tx_code     [LEN_DFI0002F_TX_CODE       + 1];   /* tx_code        */
    char                tx_code        [LEN_DFI0002F_TX_CODE       + 1];   /* tx_code        */
    char                kti_tx_code    [LEN_DFI0002F_TX_CODE       + 1];   /* tx_code        */    
    char                rspn_code      [LEN_DFI0002F_RSPN_CODE     + 1];   /* 응답코드         */
    char                io_flag;                                           /* 시스템 구분 0:GCG 1: KTI  */
    char                kti_flag [ 1+ 1];
    char                mgr_flag [ 1+ 1];
    int                 recv_len;                                          /* 수신길이         */
};
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int a000_data_receive(dfn0020_ctx_t  *ctx, commbuff_t  *commbuff);
static int b000_init_proc(dfn0020_ctx_t *ctx);
static int c000_sel_orig_msg_proc(dfn0020_ctx_t *ctx);
static int e000_exparmkti_load(dfn0020_ctx_t *ctx);
static int h000_host_msg_send(dfn0020_ctx_t *ctx);
static int k100_kti_msg_send(dfn0020_ctx_t *ctx);
static int m000_upd_jrn_proc(dfn0020_ctx_t *ctx);
static int z100_log_insert(dfn0020_ctx_t *ctx), char *log_data, int size, char io_flag, char sr_flag);
static int z200_tmp_sel_exparmkti(dfn0020_ctx_t *ctx);
/* ----------------------------------------------------------------------------------------------------------- */
/* host recveive 부터 */
/* ----------------------------------------------------------------------------------------------------------- */
int dfn0020(commbuff_t  *commbuff)
{
    int                 rc = ERR_NONE;
    dfn0020_ctx_t   _ctx; 
    dfn0020_ctx_t   *ctx = &_ctx;

    SYS_TRSF;
    /* 데이터 수신  */
    SYS_TRY(a000_data_receive(ctx, commbuff));
    /* 초기화 처리  */
    SYS_TRY(b000_init_proc(ctx));
    /* orig msg select  */
    SYS_TRY(c000_sel_orig_msg_proc(ctx));

    if (ctx->kti_flag[0] == '0'){
        /* Forex mq send gw 호출 */
        SYS_TRY(k000_host_msg_send(ctx));
    }else if (ctx->kti_flag[0] == '1'){
        /* 취급응답 거래코드 get + hcmihead set     */
        SYS_TRY(e000_exparmkti_load(ctx));
        /* KTI mq send gw 호출  */
        SYS_TRY(k100_kti_msg_send(ctx));
    }

    /* 관리전문/수취계조회 X */
    if (ctx->mgr_flag[0] != '1'){
        /* DFJRN update     */
        SYS_TRY(m000_upd_jrn_proc(ctx));
    }
        
    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:


    SYS_TREF;

    return ERR_ERR;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int a000_data_receive(dfn0020_ctx_t  *ctx, commbuff_t  *commbuff)
{

    int                 rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(dfn0010_ctx_t));
    ctx->cb = commbuff;

    SYS_ASSERT(EXTRECVDATA);

    /* input channel clear */
    SYSICOMM->intl_tx_flag = 0;
    memset(SYSICOMM->call_svc_name, 0, sizeof(SYSICOMM->call_svc_name));
    memcpy(SYSICOMM->call_svc_name, "DFN0020", 7);

    /* 11000메세지          */
    ctx->exmqmsg11000 = &ctx->_exmsg11000;

    SYS_TREF;    

    return ERR_NONE;

}

/* ----------------------------------------------------------------------------------------------------------- */
static int b000_init_proc(dfn0020_ctx_t *ctx)
{

    int                 rc = ERR_NONE;
    char                temp_str[LEN_EXMSG11000_DETL_REC_SIZE + 1];

    SYS_TRSF;

    /* 전문길이 세팅    */
    memset(temp_str, 0x00, LEN_EXMSG11000_DETL_REC_SIZE + 1);
    utol2an( (int)sysocbgs(ctx->cb, IDX_EXTRECVDATA), LEN_EXMSG11000_DETL_REC_SIZE, temp_str);

    memcpy(EXMSG11000->detl_rec_size, temp_str      , LEN_EXMSG11000_DETL_REC_SIZE);
    memcpy(&EXMSG11000->detl_area   , EXTRECVDATA   , sysocbgs(ctx->cb, IDX_EXTRECVDATA) );

    memcpy(ctx->msg_type    , &EXMSG11000->detl_area[12], LEN_DFI0002F_MSG_TYPE );   /* msg_type     */
    memcpy(ctx->proc_code   , &EXMSG11000->detl_area[16], LEN_DFI0002F_PROC_CODE);   /* proc_code    */
    memcpy(ctx->rspn_code   , &EXMSG11000->detl_area[26], LEN_DFI0002F_RSPN_CODE);   /* rspn_code    */
    memcpy(ctx->trace_no    , &EXMSG11000->detl_area[46], LEN_DFI0002F_TRACE_NO );   /* trace_no     */
    memcpy(ctx->msg_no      , &EXMSG11000->detl_area[64], LEN_DFI0002F_MSG_NO   );   /* msg_no       */
    ctx->io_flag = 'O';

    /*---------------------------------------------------------------------------*/
    SYS_DBG("b000_init_proc:    msg_type[%s]",     ctx->msg_type  );
    SYS_DBG("b000_init_proc:   proc_code[%s]",     ctx->proc_code );
    SYS_DBG("b000_init_proc:   rspn_code[%s]",     ctx->rspn_code );
    SYS_DBG("b000_init_proc:    trace_no[%s]",     ctx->trace_no  );
    SYS_DBG("b000_init_proc:      msg_no[%s]",     ctx->msg_no    );
    /*---------------------------------------------------------------------------*/
    //취급응답일 경우 0으로 set
    exmqmsg11000->rspn_flag[0] = '0';

    if (memcmp (ctx->msg_type, "0800", LEN_DFI0002F_MSG_TYPE) == 0 ||
        memcmp (ctx->msg_type, "0810", LEN_DFI0002F_MSG_TYPE) == 0 ) {
        ctx->mgr_flag[0] = '1';                         /* 관리전문 1 set           */
        memset(ctx->msg_no, 0x00, LEN_DFI0002F_MSG_NO); /* 관리전문 msg_no 초기회     */
        SYS_DBG("[b000] 관리전문 [%s]msg_type[%s]", ctx->mgr_flag , ctx->msg_type);
    }

    rc = z100_log_insert(ctx, (char *)EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), 'O', '3');   /* 취급응답 io_flag:O sr_flag:3 [KTI->EI]  */

    SYS_TREF;

    return ERR_NONE;

}

/* ----------------------------------------------------------------------------------------------------------- */
static int c000_sel_orig_msg_proc(dfn0020_ctx_t *ctx)
{

    int                 rc = ERR_NONE;
    char                buff[LEN_TCP_HEAD + 1];
    dflog_t             dflog;

    SYS_TRSF;

    memset(&dflog,  0x00, sizeof(dflog_t));
    //거래일자 
    utodate1(dflog.proc_date);
    //전문추적번호 
    memcpy(dflog.trace_no,  ctx->trace_no,  LEN_DFI0002F_TRACE_NO);
    /*---------------------------------------------------------------------------*/
    SYS_DBG("c000_sel_orig_msg_proc:   proc_date[%s]", dflog.proc_date);
    SYS_DBG("c000_sel_orig_msg_proc:    trace_no[%s]", dflog.trace_no );
    /*---------------------------------------------------------------------------*/
    // rownum = 1을 준이유는 호스트에서 전송한후 에러 발생으로 응답을 못받고 
    // 동일한 전문으로 재전송할 경우 두건 이상 나올수 있으므로 .....
    //최근 TCP헤더 정보를 조회한다. 
    if (ctx->msg_type[0] == '0' ||
        memcmp(ctx->msg_type = "920", 3) == 0 ||
        memcmp(ctx->msg_type = "940", 3) == 0 ||
        memcmp(ctx->msg_type = "980", 3) == 0 ) { //취급응답 포맷오류 '920', '980', '940'
        SYS_DBG("c000_sel_orig_msg_proc: #1  msg_type[%s]", ctx->msg_type);

        EXEC SQL 
            SELECT SUBSTR(NVL(TCP_HEAD, ' '), 1, 72),
                ,  kti_flag
              INTO :dflog.tcp_head, 
                   :dflog.kti_flag
              FROM DFLOG 
             WHERE PROC_DATE      = :dflog.proc_date 
               AND TRIM(TRACE_NO) = TRIM(:dflog.trace_no)
               AND IO_FLAG        = 'O'     //취급거래 
               AND SR_FLAG        = '1'     // 1:pp->EI 취급요청 
               AND ROWNUM         =  1
             ORDER BY PROC_TIME DESC;

        if (SYS_DB_CHK_FAIL) {
            if (SYS_DB_CHK_NOTFOUND){
                ex_syslog(LOG_FATAL, "[APPL_DM] %.7s c000_sel_orig_msg_proc : SR_FLAG[1](1403)proc_date[%s]trace_no"
                         __FILE__, dflog.proc_date, dflog.trace_no);
                //return ERR_ERR;
            }else{
                db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
                ex_syslog(LOG_FATAL, "[APPL_DM] %.7s c000_sel_orig_msg_proc : 원거래없음 proc_date[%s]trace_no"
                         __FILE__, dflog.proc_date, dflog.trace_no);
                SYS_TREF;
                //return ERR_ERR;
            }
        }
    }else if ( memcmp(ctx->msg_type = "921", 3) == 0 ||
               memcmp(ctx->msg_type = "941", 3) == 0 ||
               memcmp(ctx->msg_type = "981", 3) == 0 ) { //취급응답 포맷오류 '921', '981', '941'
        SYS_DBG("c000_sel_orig_msg_proc: #2  msg_type[%s]", ctx->msg_type);
        EXEC SQL 
            SELECT SUBSTR(NVL(TCP_HEAD, ' '), 1, 72),
                ,  kti_flag
              INTO :dflog.tcp_head, 
                   :dflog.kti_flag
              FROM DFLOG 
             WHERE PROC_DATE      = :dflog.proc_date 
               AND TRIM(TRACE_NO) = TRIM(:dflog.trace_no)
               AND IO_FLAG        = 'I'     // 개설거래 
               AND SR_FLAG        = '3'     // 3:pp->EI 취급요청 
               AND ROWNUM         =  1
             ORDER BY PROC_TIME DESC;

        if (SYS_DB_CHK_FAIL) {
            if (SYS_DB_CHK_NOTFOUND){
                ex_syslog(LOG_FATAL, "[APPL_DM] %.7s c000_sel_orig_msg_proc : SR_FLAG[1](1403)proc_date[%s]trace_no"
                         __FILE__, dflog.proc_date, dflog.trace_no);
                //return ERR_ERR;
            }else{
                db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
                ex_syslog(LOG_FATAL, "[APPL_DM] %.7s c000_sel_orig_msg_proc : 원거래없음 proc_date[%s]trace_no"
                         __FILE__, dflog.proc_date, dflog.trace_no);
                SYS_TREF;
                //return ERR_ERR;
            }
        }
    }
    /* TCP/IP Header info   */
    utotrim(dflog.tcp_head);

    if (strlen(dflog.tcp_head) > 0 ){
        memset(buff, 0x20, sizeof(buff));
        memcpy(buff         , dflog.tcp_head     , strlen(dflog.tcp_head));
        memcpy(ctx->tx_code , dflog.tcp_head     , LEN_DFI0002F_TX_CODE   );    //[10]tx_code
        memcpy(ctx->corr_id , dflog.tcp_head+25  , LEN_HCMIHEAD_QUEUE_NAME);    //[20]corr_id
        ctx->kti_flag[0] = dflog.kti_flag[0];
        SYS_DBG("tx_code[%s]kti_flag[%s]corr_id[%s]", ctx->msg_type, ctx->kti_flag, ctx->corr_id);
        rc = sysocbsi(ctx->cb, IDX_TCPHEAD, buff, TCP_HEAD);
        if (rc == ERR_ERR){
            ex_syslog(LOG_FATAL, "[APPL_DM] %.7s c000_sel_orig_msg_proc:IDX_TCPHEAD tx_code[%s]corr_id[%s]"
                                ,  __FILE__, ctx->tx_code, ctx->corr_id );
            ex_syslog(LOG_ERROR, "[APPL_DM] %.7s c000_sel_orig_msg_proc:IDX_TCPHEAD tx_code[%s]corr_id[%s]"
                                ,  __FILE__, ctx->tx_code, ctx->corr_id );
            //return ERR_ERR;
        }
    }

    SYS_TREF;

    return ERR_NONE;
}

/* ----------------------------------------------------------------------------------------------------------- */
static int e000_exparmkti_load(dfn0020_ctx_t *ctx)
{

    int                 rc = ERR_NONE;
    exi0331_t           exi0331;
    hcmihead_t          hcmihead;
    char                *hp;
    char                *dp;
    char                exmsgcomm[300+1];
    char                data_len[LEN_HCMIHEAD_DATA_LEN];
    int                 len, len2; 

    SYS_TRSF;
    SYS_DBG("e000_exparmkti_load :tx_code[%s]", ctx->tx_code);
    memcpy(ctx->ei_tx_code,     ctx->tx_code    , LEN_DFI0002F_TX_CODE );

    memset(&exi0331,    0x00, sizeof(exi0331_t));
    exi0331.in.proc_type = '6';         //6:mq send info select EI Parameter to KTI Parameter
    memcpy(exi0331.in.appl_code     , FCY_APPL_CODE             , sizeof(exi0331.in.appl_code));
    memcpy(exi0331.in.ei_tx_code    , ctx->ei_tx_code           , sizeof(exi0331.in.appl_code));
    memcpy(exi0331.in.ap_svc_name   , SYSICOMM->call_svc_name   , sizeof(SYSICOMM->call_svc_name));

    rc = ex_exchange_kti_tx_code(&exi0331);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "ex_exchange_kti_tx_code[%s][%s] not Found ", exi0331.in.ex_tx_code, exi0331.in.kti_tx_code)
    }

    if (exi0331.in.kti_tx_code[0] == 0x00){
        SYS_DBG("kti_tx_code = ei_tx_code[%s]", ctx->ei_tx_code);
        memcpy(ctx->kti_tx_code, ctx->ei_tx_code, LEN_DFI0002F_TX_CODE);
    }else{
        memcpy(ctx->kti_tx_code, exi0331.in.kti_tx_code, LEN_DFI0002F_TX_CODE);
    }
    SYS_DBG("e000_exparmkti_load : ei_tx_code[%s]kti_tx_code", ctx->ei_tx_code, ctx->kti_tx_code);

    //tx_code set
    memcpy(EXMSG11000->tx_code,     ctx->kti_tx_code, LEN_DFI11000_TX_CODE);

    SYS_TREF;

    return ERR_NONE;

}

/* ----------------------------------------------------------------------------------------------------------- */
static int h000_host_msg_send(dfn0020_ctx_t *ctx)
{

    int                 rc  = ERR_NONE;
    hcmihead_t          hcmihead;

    /* init hcmihead    */
    memset(&hcmihead,   0x20, sizeof(hcmihead_t));
    /* set hcmihead     */
    memcpy(hcmihead.queue_name, ctx->corr_id       , LEN_HCMIHEAD_QUEUE_NAME);
    memcpy(hcmihead.tx_code   , ctx->host_tx_code  , LEN_HCMIHEAD_TX_CODE   );
    memcpy(hcmihead.data_len  , EXMSG11000->detl_rec_size, LEN_HCMIHEAD_DATA_LEN);
    memcpy(hcmihead.resp_code , "000"              , LEN_HCMIHEAD_RESP_CODE );
#ifdef   _DEBUG
    //PRINT_HCMIHEAD(&hcmihead);
#endif

    /* set tcp head */
    rc = sysocbsi(ctx->cb, IDX_TCPHEAD, &hcmihead, sizeof(hcmihead_t));
    ctx->recv_len = utoa2in(EXMSG11000->detl_rec_size, LEN_HCMIHEAD_DATA_LEN);    
    rc = sysocbsi(ctx->cb, IDX_HOSTSENDDATA, EXMSG11000->detl_area, ctx->recv_len + LEN_HCMIHEAD);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "COMMBUFF SET ERR");
        return ERR_ERR;
    }
     SYS_DBG("[Forex] HOSTSENDDATA [%d][%.*s]", sysocbgs(ctx->cb, IDX_HOSTSENDDATA), sysocbgs(ctx->cb, IDX_HOSTSENDDATA), EXMSG11000->detl_area);

    SYSGWINFO->time_val     = SYSGWINFO_SAF_DFLT_TIMEOUT;
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_IR;
    SYSGWINFO->rspn_flag    = SYSGWINFO_SVC_REPLY;    

    /* Forex MQ MSG SEND    */
    rc - sys_tpcall("SYMQSEND_DFF", ctx->cb, TPNOTRAN);
    if ( rc == ERR_ERR){
        ex_syslog(LOG_FATAL, "[APPL_DM] %.7s dfn0020(FOREX21)- MQ MSG SEND ERROR %d [해결방안]HOST G/W 담당자 call",
                            __FILE__, tperrno);
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s dfn0020(FOREX21)- k000_host_msg_send ERROR %d [해결방안]HOST G/W 담당자 call",
                            __FILE__, tperrno);
        //return ERR_ERR;
    }

    rc = z100_log_insert(ctx, (char *)EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), 'O', '4');   /* 취급응답 :O sr_flag: 4[EI->PP]   */
 

    SYS_TREF;

    return ERR_NONE;

}

/* ----------------------------------------------------------------------------------------------------------- */
static int k100_kti_msg_send(dfn0020_ctx_t *ctx)
{

    int                 rc  = ERR_NONE;
    char                corr_id[LEN_HCMIHEAD_QUEUE_NAME + 1];
    char                data_len[LEN_HCMIHEAD_DATA_LEN];
    int                 len, len2;
    hcmihead_t          hcmihead;
    dfi0001x_t          dfi0001x;

    memset(&dfi0001x,   0x20, sizeof(dfi0001x_t));
    dfi0001x.in.exmsg11000 = EXMSG11000;
    df_proc_exmsg11000_init(&dfi0001x);

    /* init hcmihead    */
    /* set hcmihead     */
    memset(&hcmihead,    0x20, sizeof(hcmihead_t));
    memcpy(hcmihead.queue_name, ctx->corr_id      , LEN_HCMIHEAD_QUEUE_NAME);
    memcpy(hcmihead.tx_code   , ctx->host_tx_code , LEN_HCMIHEAD_TX_CODE   );

    memcpy(hcmihead.data_len  , EXMSG11000->detl_rec_size , LEN_HCMIHEAD_DATA_LEN );
    memcpy(hcmihead.resp_code , "000" , LEN_HCMIHEAD_RESP_CODE );

#ifdef  _DEBUG
    //PRINT_HCMIHEAD(&hcmihead);
#endif

    ctx->recv_len = utoa2in(exmqmsg11000->detl_rec_size, LEN_HCMIHEAD_DATA_LEN);
    memcpy(dfi0001x.in.exmsg11000->detl_area, EXMSG11000->detl_area, ctx->recv_len );

    rc = sysocbsi(ctx->cb, IDX_TCPHEAD, &hcmihead, sizeof(hcmihead_t));
    rc = sysocbsi(ctx->cb, IDX_HOSTSENDDATA, dfi0001x.in.exmqmsg11000, ctx->recv_len + LEN_KTIHEAD);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "COMMBUFF SET ERR");
        return ERR_ERR;
    }

    SYS_DBG("[KTI]HOSTSENDDATA[%d][%.*s]",  sysocbgs(ctx->cb, IDX_HOSTSENDDATA), sysocbgs(ctx->cb, IDX_HOSTSENDDATA), HOSTSENDDATA);

    SYSGWINFO->time_val     = SYSGWINFO_SAF_DFLT_TIMEOUT;
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_IR;
    SYSGWINFO->rspn_flag    = SYSGWINFO_SVC_REPLY;    

    /* KTI MQ MSG SEND    */
    rc - sys_tpcall("SYMQSEND_DFK", ctx->cb, TPNOTRAN);
    if ( rc == ERR_ERR){
        ex_syslog(LOG_FATAL, "[APPL_DM] %.7s dfn0020(KTI)- MQ MSG SEND ERROR %d [해결방안]HOST G/W 담당자 call",
                            __FILE__, tperrno);
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s dfn0020(KTI)- k100_kti_msg_send() ERROR %d [해결방안]HOST G/W 담당자 call",
                            __FILE__, tperrno);
        //return ERR_ERR;
    }

        rc = z100_log_insert(ctx, (char *)EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), 'O', '4');   /* 취급응답 :O sr_flag: 4[EI->PP]   */
    
    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int m000_upd_jrn_proc(dfn0020_ctx_t *ctx)
{
    int                 rc = ERR_NONE;
    dfi0002f_t          dfi0002f;

    SYS_TRSF;

    memset(&dfi0002f, 0x00, sizeof(dfi0002f_t));
    utodate1(ctx->proc_date);       //조회조건일자 
    memcpy(dfi0002f.in.proc_date       ,  ctx->proc_date    , LEN_DFI0002F_PROC_DATE );
    memcpy(dfi0002f.in.msg_no           , ctx->msg_no       , LEN_DFI0002F_MSG_NO    );     /* 거래고유번호     */
    memcpy(dfi0002f.in.msg_type         , ctx->msg_type     , LEN_DFI0002F_MSG_TYPE  );     /* msg_type      */

    memcpy(dfi0002f.in.tx_code          , ctx->tx_code      , LEN_DFI0002F_TX_CODE   );     /* tx_code       */    
    memcpy(dfi0002f.in.proc_code        , ctx->proc_code    , LEN_DFI0002F_PROC_CODE );     /* 거래구분코드     */    
    dfi0002f.in.io_flag[0]  = ctx->io_flag;
    dfi0002f.in.kti_flag[0] = ctx->kti_flag[0];
    /*---------------------------------------------------------------------------*/
    SYS_DBG("m000_upd_jrn_proc: proc_date[%s]", dfi0002f.in.proc_date);
    SYS_DBG("m000_upd_jrn_proc:    msg_no[%s]", dfi0002f.in.msg_no   );
    SYS_DBG("m000_upd_jrn_proc:  msg_type[%s]", dfi0002f.in.msg_type );
    SYS_DBG("m000_upd_jrn_proc:   tx_code[%s]", dfi0002f.in.tx_code  );
    SYS_DBG("m000_upd_jrn_proc: proc_code[%s]", dfi0002f.in.proc_code  );
    /*---------------------------------------------------------------------------*/

    if (memcmp(ctx->msg_type, "0400", LEN_DFI0002F_MSG_TYPE) ==0 ||
        memcmp(ctx->msg_type, "0410", LEN_DFI0002F_MSG_TYPE) ==0 ) {
        memcpy(dfi0002f.in.canc_rspn_code  ,  ctx->rspn_code    , LEN_DFI0002F_RSPN_CODE );
        memcpy(dfi0002f.in.canc_trace_no   ,  ctx->trace_no     , LEN_DFI0002F_TRACE_NO  );
        dfi0002f.in.canc_type[0] = '1';
        SYS_DBG("m000_upd_jrn_proc:   canc_rspn_code[%s]", dfi0002f.in.canc_rspn_code  );
        SYS_DBG("m000_upd_jrn_proc:    canc_trace_no[%s]", dfi0002f.in.canc_trace_no   );

        rc = df_jrn_canc_upd(&dfi0002f);
        if (rc == ERR_ERR){
            ex_syslog(LOG_FATAL, "[APPL_DM] %s m000_upd_jrn_proc() :", __FILE__);
            return ERR_ERR;
        }
    }else{
        memcpy(dfi0002f.in.trace_no         , ctx->trace_no     , LEN_DFI0002F_TRACE_NO  );     /* 전문추적번호     */
        memcpy(dfi0002f.in.corr_id          , ctx->corr_id      , LEN_DFI0002F_CORR_ID   );     /* corr_id       */        
        memcpy(dfi0002f.in.rspn_code       ,  ctx->rspn_code    , LEN_DFI0002F_RSPN_CODE );
        SYS_DBG("m000_upd_jrn_proc:    trace_no[%s]", dfi0002f.in.trace_no   );
        SYS_DBG("m000_upd_jrn_proc:     corr_id[%s]", dfi0002f.in.corr_id    );
        SYS_DBG("m000_upd_jrn_proc:   rspn_code[%s]", dfi0002f.in.rspn_code  );

        rc = df_jrn_upd(&dfi0002f);
        if (rc == ERR_ERR) {
            ex_syslog(LOG_FATAL, "[APPL_DM] %s m000_upd_jrn_proc() :", __FILE__);
            return ERR_ERR;
        }
    }

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int z100_log_insert(dfn0020_ctx_t *ctx), char *log_data, int size, char io_flag, char sr_flag)
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
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0020: z100_log_insert() DFLOG sysocbdb ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }

    rc = sysocbsi(&dcb, IDX_EXMSG1200, &dfi3100f sizeof(dfi3100f_t));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0020: z100_log_insert() DFLOG sysocbsi ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        sysocbfb(&dcb);
        return ERR_NONE;
    }

    rc = sys_tpacall("DFN3100F", &dcb, TPNOREPLY | TPNOTRAN);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0020: z100_log_insert() DFLOG sys_tpacall ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }
    
    sysocbfb(&dcb);

    SYS_TREF;

    return ERR_NONE;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int z200_tmp_sel_exparmkti(dfn0020_ctx_t *ctx)
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
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0020: z100_log_insert() DFLOG sysocbdb ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }

    rc = sysocbsi(&dcb, IDX_EXMSG1200, &dfi3100f sizeof(dfi3100f_t));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0020: z100_log_insert() DFLOG sysocbsi ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        sysocbfb(&dcb);
        return ERR_NONE;
    }

    rc = sys_tpacall("DFN3100F", &dcb, TPNOREPLY | TPNOTRAN);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s DFN0020: z100_log_insert() DFLOG sys_tpacall ERR log_type %d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }
    
    sysocbfb(&dcb);

    SYS_TREF;

    return ERR_NONE;
}
/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */