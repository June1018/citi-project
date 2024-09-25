
/*  @file               dfn0010.pc
*   @file_type          pc source program
*   @brief              외화자금이체 (FCY_DFT)개설요인 메인 
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
#include <exdefine.h>
#include <exi0251x.h>
#include <utodate.h>
#include <exparm.h>
#include <dfi0001x.h>
#include <dfi0002f.h>
#include <dfi0003f.h>
#include <dfi3100f.h>
#include <exi0212.h>    /* 거래파라미터 load */
#include <sqlca.h>
#include <bssess_stat.h>
#include <dfjrn.h>
#include <hcmihead.h>
#include <eierrhead.h>
#include <utoclck.h>
#include <exmsg11000.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXMSG11000              (ctx->exmsg11000)
#define LEN_KTIHEAD             300
#define FCY_CODE                "073"
#define LEN_MT103               11000

/* ---------------------------------------- structure definitions --------------------------------------------- */

typedef struct dfn0010_ctx_s dfn0010_ctx_t;
struct dfn0010_ctx_s {
    commbuff_t          *cb;

    exmsg11000_t        _exmsg11000; /* host send 용 structure   */
    exmsg11000_t        *exmsg11000;

    char                proc_date      [LEN_DFI0003F_PROC_DATE     + 1];   /* 처리일자         */
    char                msg_type       [LEN_DFI0003F_MSG_TYPE      + 1];   /* 전문종별코드      */
    char                msg_no         [LEN_DFI0003F_MSG_NO        + 1];   /* 거래고유번호      */
    char                proc_code      [LEN_DFI0003F_PROC_CODE     + 1];   /* 거래구분코드      */
    char                trace_no       [LEN_DFI0003F_TRACE_NO      + 1];   /* 전문추적번호      */
    char                brn_no         [LEN_DFI0003F_BRN_NO        + 1];   /* 의뢰기관점별코드   */
    char                corr_id        [LEN_DFI0003F_CORR_ID       + 1];   /* corr_id        */
    char                host_tx_code   [LEN_DFI0003F_TX_CODE       + 1];   /* kti_tx_code    */
    char                rcv_acct_no    [LEN_DFI0003F_ACCT_NO       + 1];   /* 수취계좌번호      */
    char                acct_tran_code [LEN_DFI0003F_ACCT_TRAN_CODE+ 1];   /* 계좌송금/수취인송금 코드 */
    char                rspn_code      [LEN_DFI0002F_RSPN_CODE     + 1];   /* 응답코드         */
    char                io_flag;                                           /* 시스템 구분 0:GCG 1: KTI  */
    char                kti_flag [ 1+ 1];
    char                mgr_flag [ 1+ 1];
    char                currency       [LEN_DFI0003F_CURRENCY      + 1];   /* 거래통화         */ 
    int                 recv_len;                                          /* 수신길이         */
    int                 send_len;                                          /* 대외기관 전송길이  */
    char                mt103_text     [LEN_MT103 + 1];                    /* swift mt103 최대길이 11000 */

/* ------------------------------------------ exported function  declarations --------------------------------- */
static int a000_data_receive(dfn0010_ctx_t  *ctx, commbuff_t  *commbuff);
static int b100_init_proc(dfn0010_ctx_t *ctx);
static int b200_routing_proc(dfn0010_ctx_t *ctx);
static int b300_tran_code_conv(dfn0010_ctx_t *ctx);
static int d000_jrn_insert(dfn0010_ctx_t *ctx);
static int k000_host_msg_send(dfn0010_ctx_t *ctx);
static int k100_kti_msg_send(dfn0010_ctx_t *ctx);
static int z100_log_insert(dfn0010_ctx_t *ctx), char *log_data, int size, char io_flag, char sr_flag);

/* ----------------------------------------------------------------------------------------------------------- */
int dfn0010(commbuff_t  *commbuff)
{
    int                 rc = ERR_NONE;
    dfn0010_ctx_t   _ctx; 
    dfn0010_ctx_t   *ctx = &_ctx;

    SYS_TRSF;
    /* 데이터 수신  */
    SYS_TRY(a000_data_receive(ctx, commbuff));
    /* 초기화 처리  */
    SYS_TRY(b100_init_proc(ctx));
    /* 관리전문 거래전문 전송 - 한쪽 gw 문제가 있어도 다른 gw영향 없는지 */
    if (ctx->mgr_flag[0] == '1' ){

        ctx->kti_flag[0] = '0';
        SYS_TRY(b300_tran_code_conv(ctx));      /* 대외기관 수신거래코드 --> Host거래코드 변환 후 개설서비스 호출   */
        SYS_TRY(k000_host_msg_send(ctx));       /* 개시전문 - Forex mq send gw call                      */

        SYS_TRY(b300_tran_code_conv(ctx));      /* 대외기관 수신거래코드 --> Host거래코드 변환 후 개설서비스 호출   */
        SYS_TRY(k100_kti_msg_send(ctx));        /* 개시전문 - KTI mq send gw call                        */
    }else{
        SYS_DBG("#1 mgr_flag[%s]", ctx->mgr_flag[0]);
        /* routing process - 관리전문 제외         */
        SYS_TRY(b200_routing_proc(ctx));
        /* 대외기관 수신거래코드 --> HOST 거래코드 변환 후 개설 서비스 호출    */
        SYS_TRY(b300_tran_code_conv(ctx));

        if (ctx->kti_flag[0] == '0') {
            SYS_TRY(k000_host_msg_send(ctx));       /* Forex mq send gw call    */
        }else if (ctx->kti_flag[0] == '1'){
            SYS_TRY(k100_kti_msg_send(ctx));        /* KTI mq send gw call      */
        }

        /* DFJRN Insert - 관리전문/수취계좌 제외 */
        if (memcmp(ctx->msg_type, "0200", 4 ) == 0 && memcmp(ctx->proc_code, "003000", 6) == 0 ){
            SYS_DBG("#수취계좌 : msg_type[%s]proc_code[%s]", ctx->msg_type, ctx->proc_code);
        }else{
            SYS_DBG("#DFJRN Insert : msg_type[%s]proc_code[%s]", ctx->msg_type, ctx->proc_code);
            SYS_TRY(d000_jrn_insert(ctx));
        }
    }

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:
    SYS_DBG("SYS_CATCH");

    SYS_TREF;

    return ERR_ERR;
}

/* ----------------------------------------------------------------------------------------------------------- */
static int a000_data_receive(dfn0010_ctx_t   *ctx, commbuff_t *commbuff)
{
    int                 rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(dfn0010_ctx_t));
    ctx->cb = commbuff;

    /* input channel clear */
    SYSICOMM->intl_tx_flag = 0;
    memset(SYSICOMM->call_svc_name, 0, sizeof(SYSICOMM->call_svc_name));
    memcpy(SYSICOMM->call_svc_name, "DFN0010", 7);

    /* 11000메세지          */
    ctx->exmqmsg11000 = &ctx->_exmsg11000;

    SYS_TREF;    

    return ERR_NONE;
}

/* ----------------------------------------------------------------------------------------------------------- */
static int b100_init_proc(dfn0010_ctx_t *ctx)
{
    int                 rc = ERR_NONE;
    char                *hp; 
    char                *ptr; 
    char                msg_type[2];
    char                temp_str[LEN_EXMSG11000_DETL_REC_SIZE + 1];
    exmqmsg11000_t      exmqmsg11000;

    SYS_TRSF;

    /* 전문길이 세팅    */
    memset(temp_str, 0x00, LEN_EXMSG11000_DETL_REC_SIZE + 1);
    utol2an( (int)sysocbgs(ctx->cb, IDX_EXTRECVDATA), LEN_EXMSG11000_DETL_REC_SIZE, temp_str);

    memcpy(EXMSG11000->detl_rec_size, temp_str      , LEN_EXMSG11000_DETL_REC_SIZE);
    memcpy(&EXMSG11000->detl_area   , EXTRECVDATA   , sysocbgs(ctx->cb, IDX_EXTRECVDATA) );
    ctx->recv_len = utol2an(temp_str, LEN_EXMSG11000_DETL_REC_SIZE);
    SYS_DBG("[b100]recv_len[%d]", ctx->recv_len);
    memcpy(ctx->msg_type    , &EXMSG11000->detl_area[12], LEN_DFI0002F_MSG_TYPE );   /* msg_type     */
    memcpy(ctx->proc_code   , &EXMSG11000->detl_area[16], LEN_DFI0002F_PROC_CODE);   /* proc_code    */
    memcpy(ctx->rspn_code   , &EXMSG11000->detl_area[26], LEN_DFI0002F_RSPN_CODE);   /* rspn_code    */
    memcpy(ctx->trace_no    , &EXMSG11000->detl_area[46], LEN_DFI0002F_TRACE_NO );   /* trace_no     */
    memcpy(ctx->msg_no      , &EXMSG11000->detl_area[64], LEN_DFI0002F_MSG_NO   );   /* msg_no       */
    memcpy(ctx->brn_no      , &EXMSG11000->detl_area[90], LEN_DFI0002F_BRN_NO   );   /* brn_no       */
    ctx->io_flag = 'I';

    if (memcmp (ctx->msg_type, "0800", LEN_DFI0002F_MSG_TYPE) == 0 ||
        memcmp (ctx->msg_type, "0810", LEN_DFI0002F_MSG_TYPE) == 0 ) {
        ctx->mgr_flag[0] = '1';                         /* 관리전문 1 set           */
        memset(ctx->msg_no, 0x00, LEN_DFI0002F_MSG_NO); /* 관리전문 msg_no 초기회     */
        SYS_DBG("[b100] 관리전문 [%s]", ctx->mgr_flag);
    }else{
        memcpy(ctx->acct_tran_code, &EXMSG11000->detl_area[94], 2   );   /* 계좌송금/수취인송금여부     */
    }

    rc = z100_log_insert(ctx, (char *)EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), 'I', '1');   /* 개설요청 io_flag:I sr_flag:1 [KFTC->EI]  */

    SYS_TREF;

    return ERR_NONE;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int b200_routing_proc(dfn0010_ctx_t *ctx)
{
    int                 rc = ERR_NONE;
    dfi0003f_t          dfi0003f;

    SYS_TRSF;

    memset(&dfi0003f, 0x00, sizeof(dfi0003f_t));

    utodate1(ctx->proc_date);
    memcpy(dfi0003f.in.proc_date       , ctx->proc_date     , LEN_DFI0003F_PROC_DATE    );
    memcpy(dfi0003f.in.msg_type        , ctx->msg_type      , LEN_DFI0003F_MSG_TYPE     );
    memcpy(dfi0003f.in.proc_code       , ctx->proc_code     , LEN_DFI0003F_PROC_CODE    );
    memcpy(dfi0003f.in.msg_no          , ctx->msg_no        , LEN_DFI0003F_MSG_NO       );
    memcpy(dfi0003f.in.brn_no          , ctx->proc_date     , LEN_DFI0003F_BRN_NO       );
    memcpy(dfi0003f.in.trace_no        , ctx->trace_no      , LEN_DFI0003F_TRACE_NO     );
    memcpy(dfi0003f.in.acct_tran_code  , ctx->acct_tran_code, LEN_DFI0003F_ACCT_TRAN_CODE);
    memcpy(&dfi0003f.in.ext_send_data  , EXTRECVDATA , sysocbgs(ctx->cb, IDX_EXTRECVDATA)); /* 대외수신 데이터 set  */

    dfi0003f.in.exmqmsg11000 = (exmsg11000_t *) EXMSG11000;

    rc = df_route_proc(&dfi0003f);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s b200_routing_proc ERROR ", __FILE__);
        return ERR_ERR;
    }
    /* -------------------------------------------------------------------------------------------------- */
    SYS_DBG("df_route_proc dfi0003f.out.kti_flag[%s]rcv_acct_no[%s]currency[%s]", dfi0003f.out.kti_flag, dfi0003f.out.rcv_acct_no, dfi0003f.out.currency);
    /* -------------------------------------------------------------------------------------------------- */
    memcpy(ctx->kti_flag    , dfi0003f.out.kti_flag     , LEN_DFI0003F_KTI_FLAG);
    memcpy(ctx->rcv_acct_no , dfi0003f.out.rcv_acct_no  , LEN_DFI0003F_ACCT_NO );   //수취계좌번호 
    memcpy(ctx->currency    , dfi0003f.out.currency     , LEN_DFI0003F_CURRENCY);   //거래통화 

    SYS_TREF;

    return ERR_NONE;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int b300_tran_code_conv(dfn0010_ctx_t *ctx)
{
    int                 rc = ERR_NONE;
    exi0251x_t          exi0251x;

    SYS_TRSF;

    memset(&exi0251x,   0x00, sizeof(exi0251x_t));

    SYS_DBG("[b300]tran_code_conv:msg_type[%s]proc_code[%s]kti_flag[%s]", ctx->msg_type, ctx->proc_code, ctx->kti_flag);
    /* ---------------------------------------  */
    /* kti_flag(1): 7 else corebank : 8         */
    if (ctx->kti_flag[0] == '1') {
        exi0251x.in.tx_flag[0]  = '7';
    }else{
        exi0251x.in.tx_flag[0]  = '6';
    }

    exi0251x.in_conv_flag  = 'K';
    memcpy(exi0251x.in.appl_code   , FCY_APPL_CODE , LEN_APPL_CODE);
    memcpy(exi0251x.in.msg_type    , ctx->msg_type , LEN_MSG_TYPE );
    memcpy(exi0251x.in.kftc_tx_code, ctx->proc_code, LEN_KFTC_TX_CODE);
    exi0251x.in.msg_type_len      = 4;
    exi0251x.in.kftc_tx_code_len  = 6;
    exi0251x.in.ext_recv_data     = EXTRECVDATA;    /* 대외수신데이터  set   */

    rc = ex_tran_code_convrt(&exi0251x);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s b300_tran_code_conv(): "
                             "거래코드 변환 ERROR : tx_code[%s] kftc_tx_code[%s]"
                             "code/msgp[%d][%s]",
                             __FILE__, ctx->msg_type, ctx->kftc_tx_code,
                             sys_error_code(), sys_error_msg());
        return ERR_ERR;
    }
    SYS_DBG("c000_tran_code_conv :tx_code[%s]tx_name[%s]", exi0251x.out.tx_code, utotrim(exi0251x.out.tx_name));
    memcpy(ctx->host_tx_code,   exi0251x.out.tx_code, LEN_DFI0003F_TX_CODE  );
    memcpy(EXMSG11000->tx_code, ctx->host_tx_code   , LEN_EXMSG11000_TX_CODE);

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int d000_jrn_insert(dfn0010_ctx_t *ctx)
{
    int                 rc = ERR_NONE;
    dfi0002f_t          dfi0002f;

    SYS_TRSF;

    memset(&dfi0002f, 0x00, sizeof(dfi0002f_t));

    utocick(ctx->corr_id);
    dfi0002f.in.exmsg11000 = EXMSG11000;

    memcpy(dfi0002f.in.proc_date        , ctx->proc_date    , LEN_DFI0002F_PROC_DATE );     /* 거래발생일자     */
    memcpy(dfi0002f.in.tx_code          , ctx->tx_code      , LEN_DFI0002F_TX_CODE   );     /* tx_code       */
    memcpy(dfi0002f.in.msg_type         , ctx->msg_type     , LEN_DFI0002F_MSG_TYPE  );     /* msg_type      */
    memcpy(dfi0002f.in.proc_code        , ctx->proc_code    , LEN_DFI0002F_PROC_CODE );     /* 거래구분코드     */
    memcpy(dfi0002f.in.trace_no         , ctx->trace_no     , LEN_DFI0002F_TRACE_NO  );     /* 전문추적번호     */
    memcpy(dfi0002f.in.msg_no           , ctx->msg_no       , LEN_DFI0002F_MSG_NO    );     /* 거래고유번호     */
    memcpy(dfi0002f.in.corr_id          , ctx->corr_id      , LEN_DFI0002F_CORR_ID   );     /* corr_id       */
    dfi0002f.in.io_flag[0]  = ctx->io_flag;
    dfi0002f.in.kti_flag[0] = ctx->kti_flag[0];

    if (memcmp(ctx->msg_type, "0400", LEN_DFI0002F_MSG_TYPE) ==0 ||
        memcmp(ctx->msg_type, "0410", LEN_DFI0002F_MSG_TYPE) ==0 ) {
        memcpy(dfi0002f.in.canc_rspn_code  ,  ctx->rspn_code    , LEN_DFI0002F_RSPN_CODE );
        memcpy(dfi0002f.in.canc_trace_no   ,  ctx->trace_no     , LEN_DFI0002F_TRACE_NO  );
        dfi0002f.in.canc_type[0] = '1';
    }else{
        memcpy(dfi0002f.in.rspn_code       ,  ctx->rspn_code    , LEN_DFI0002F_RSPN_CODE );
    }

    rc = df_jrn_insupd(&dfi0002f);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s d000_jrn_insert ", __FILE__ );
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int k000_host_msg_send(dfn0010_ctx_t *ctx)
{
    int                 rc  = ERR_NONE;
    int                 len = 0;
    char                corr_id[LEN_HCMIHEAD_QUEUE_NAME + 1];

    hcmihead_t          hcmihead;

    /* init hcmihead    */
    memset(corr_id, 0x00, sizeof(corr_id));

    utocick(corr_id);

    memcpy(ctx->corr_id, corr_id, LEN_HCMIHEAD_QUEUE_NAME);
    memset(&hcmihead,   0x20, sizeof(hcmihead_t));
    /* set hcmihead     */
    memcpy(hcmihead.queue_name, ctx->corr_id       , LEN_HCMIHEAD_QUEUE_NAME);
    memcpy(hcmihead.tx_code   , ctx->host_tx_code  , LEN_HCMIHEAD_TX_CODE   );
    memcpy(hcmihead.data_len  , EXMSG11000->detl_rec_size, LEN_HCMIHEAD_DATA_LEN);
    memcpy(hcmihead.resp_code , "000"              , LEN_HCMIHEAD_RESP_CODE );

    ctx->recv_len = utoa2in(EXMSG11000->detl_rec_size, LEN_HCMIHEAD_DATA_LEN);

    rc = sysocbsi(ctx->cb, IDX_TCPHEAD, &hcmihead, sizeof(hcmihead_t));
    rc = sysocbsi(ctx->cb, IDX_HOSTSENDDATA, EXMSG11000->detl_area, ctx->recv_len);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "COMMBUFF SET ERR");
        return ERR_ERR;
    }

    SYSGWINFO->time_val     = SYSGWINFO_SAF_DFLT_TIMEOUT;
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_IR;
    SYSGWINFO->rspn_flag    = SYSGWINFO_SVC_REPLY;    

    /* Forex MQ MSG SEND    */
    rc - sys_tpcall("SYMQSEND_DFF", ctx->cb, TPNOTRAN);
    if ( rc == ERR_ERR){
        ex_syslog(LOG_FATAL, "[APPL_DM] %.7s dfn0010(FOREX21)- MQ MSG SEND ERROR %d [해결방안]HOST G/W 담당자 call",
                            __FILE__, tperrno);
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s dfn0010(FOREX21)- k000_host_msg_send ERROR %d [해결방안]HOST G/W 담당자 call",
                            __FILE__, tperrno);
        if (ctx->mgr_flag[0] == '1'){
            /* 관리전문 KTI tx_code get하기위해서 kti_flag set      */
            memset(ctx->kti_flag, 0x00, sizeof(ctx->kti_flag));
            ctx->kti_flag[0] = '1';
            SYS_DBG("[k000]mgr_flag[%s]kti_flag[%s]",ctx->mgr_flag, ctx->kti_flag);
            //return ERR_ERR;   20240715 요청사항 관리전문 Forex21/KTI 한쪽 gw문제가 생겨도 다른쪽에 영향없어야 한다. 
        }else{
            return ERR_ERR;
        }
    }
    rc = z100_log_insert(ctx, (char *)EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), 'I', '2');   /* 개설요청 :I sr_flag: 2[EI->PP]   */

    if (ctx->mgr_flag[0] == '1'){
        /* 관리전문 KTI(tx_code) set :mgr_flag[%s]ctx->kti_flag[%s]*/
        memset(ctx->kti_flag,   0x00, sizeof(ctx->kti_flag));
        ctx->kti_flag[0] = '1';
        SYS_DBG("관리전문 KTI(tx_code) set :mgr_flag[%s]ctx->kti_flag[%s]", ctx->mgr_flag, ctx->kti_flag);
    }

    SYS_TREF;

    return ERR_NONE;
}

/* ----------------------------------------------------------------------------------------------------------- */
static int k100_kti_msg_send(dfn0010_ctx_t *ctx)
{
    int                 rc  = ERR_NONE;
    char                corr_id[LEN_HCMIHEAD_QUEUE_NAME + 1];
    char                data_len[LEN_HCMIHEAD_DATA_LEN];
    int                 len, len2;
    hcmihead_t          hcmihead;
    eierrhead_t         eierrhead;
    dfi0001x_t          dfi0001x;

    memset(&dfi0001x,   0x20, sizeof(dfi0001x_t));
    dfi0001x.in.exmsg11000 = EXMSG11000;
    df_proc_exmsg11000_init(&dfi0001x);

    /* init hcmihead    */
    memset(corr_id, 0x00, sizeof(corr_id));
    utocick(corr_id);
    memset(ctx->corr_id, 0x00, sizeof(corr_id));
    /* set hcmihead     */
    memset(&hcmihead,    0x20, sizeof(hcmihead_t));
    memcpy(hcmihead.queue_name, ctx->corr_id      , LEN_HCMIHEAD_QUEUE_NAME);

    memcpy(hcmihead.tx_code   , ctx->host_tx_code , LEN_HCMIHEAD_TX_CODE   );
    memcpy(EXMSG10000->tx_code, ctx->host_tx_code , LEN_HCMIHEAD_TX_CODE   );

    //len = LEN_HCMIHEAD + LEN_EIERRHEAD + ctx->recv_len;
    len = LEN_HCMIHEAD + ctx->recv_len;
    memset(data_len, 0x20, sizeof(data_len));
    sprintf(data_len, "%05d",   len);
    utotrim(data_len);
    SYS_DBG("k100_kti_msg_send #1 data_len[%s]recv_len[%d]", data_len, ctx->recv_len );

    memcpy(hcmihead.data_len,   data_len , LEN_HCMIHEAD_DATA_LEN );

#ifdef  _DEBUG
    //PRINT_HCMIHEAD(&hcmihead);
#endif
    SYS_DBG("EXMSG11000[KTI][%d][%s]",  sysocbgs(ctx->cb, IDX_HOSTSENDDATA), EXMSG11000);

    rc = sysocbsi(ctx->cb, IDX_TCPHEAD, &hcmihead, sizeof(hcmihead_t));
    rc = sysocbsi(ctx->cb, IDX_HOSTSENDDATA, EXMSG11000, len);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "COMMBUFF SET ERR");
        return ERR_ERR;
    }

    SYSGWINFO->time_val     = SYSGWINFO_SAF_DFLT_TIMEOUT;
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_IR;
    SYSGWINFO->rspn_flag    = SYSGWINFO_SVC_REPLY;    

    /* KTI MQ MSG SEND    */
    rc - sys_tpcall("SYMQSEND_DFK", ctx->cb, TPNOTRAN);
    if ( rc == ERR_ERR){
        ex_syslog(LOG_FATAL, "[APPL_DM] %.7s dfn0010(KTI)- MQ MSG SEND ERROR %d [해결방안]HOST G/W 담당자 call",
                            __FILE__, tperrno);
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s dfn0010(KTI)- k100_kti_msg_send() ERROR %d [해결방안]HOST G/W 담당자 call",
                            __FILE__, tperrno);

        if (ctx->mgr_flag[0] == '1'){
            SYS_DBG("[k100]mgr_flag[%s]",ctx->mgr_flag);
            //return ERR_ERR;   20240715 요청사항 관리전문 Forex21/KTI 한쪽 gw문제가 생겨도 다른쪽에 영향없어야 한다. 
        }else{
            return ERR_ERR;
        }
    }
    rc = z100_log_insert(ctx, (char *)EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), 'I', '2');   /* 개설요청 :I sr_flag: 2[EI->PP]   */
    
    SYS_TREF;

    return ERR_NONE;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int z100_log_insert(dfn0010_ctx_t *ctx), char *log_data, int size, char io_flag, char sr_flag)
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
/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */