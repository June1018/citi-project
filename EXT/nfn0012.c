
/*  @file               nfn0012.pc
*   @file_type          c source program
*   @brief              국고 한국은행 개설응답 
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
*
*
*
*   @generated at 2023/06/02 09:30
*   @history
*
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*/
/*
 * main 
 * 1.   a000    CONTEXT 초기화 및 commbuff검증, 대외기관 수신전문 logging 
 * 2.   b000    1500전문 초기화 
 * 3.   c000    거래 parameter load
 * 4.   d000    관리전문 반영
 * 5.   f000    HOST 전문 송수신 
 * 6.   h000    대외기관 응답전문 전송
 */

 /* ---------------------------------------------- include files ----------------------------------------------- */
#include <syscom.h>
#include <sysconst.h>
#include <utodate.h>
#include <exdefine.h>
#include <exmsg1500.h>
#include <exparm.h>
#include <exi0212.h>
#include <nfi0001f.h>
#include <nfi0002f.h>
#include <nfi3100f.h>
#include <sqlca.h>
#include <bssess_stat.h>
#include <hcmihead.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXMSG1500           (ctx->exmsg1500)
#define EXMSG1500DETL       (ctx->exmsg1500->detl_area)

/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct nfn0012_ctx_s    nfn0012_ctx_t;
struct nfn0012_ctx_s {
    commbuff        *cb;  

    exmsg1500_t     _exmsg1500;
    exmsg1500_t     *exmsg1500;

    int             kti_recv_len;                       /* 수신 길이 */
    char            corr_id  [LEN_NFI0002F_CORR_ID + 1];
    char            tx_code  [LEN_TX_CODE + 1];
    char            msg_type [LEN_NFI0002F_MSG_TYPE + 1];
    char            msg_no   [LEN_NFI0002F_MSG_NO   + 1];
    char            trace_no [LEN_NFI0002F_TRACE_NO + 1];
    char            kti_flag;  /* KTI Flag 1:KTI 0:core    */
    char            proc_date[LEN_NFI0002F_PROC_DATE + 1];

};

/* ------------------------------------- exported global variables definitions -------------------------------- */
int      nf_on_session_flag = 0;    /* 0:초기화, 1:세션키교환중,  9:세션키교환완료   */


/* ------------------------------------------ exported function  declarations --------------------------------- */
static int  a000_data_receive(nfn0012_ctx_t *ctx, commbuff_t    *commbuff);
static int  b000_init_proc(nfn0012_ctx_t *ctx, int log_type);
static int  c000_sel_jrn_proc(nfn0012_ctx_t *ctx);
static int  d000_jrn_insert(nfn0012_ctx_t *ctx);
static int  f000_upd_jrn_proc(nfn0012_ctx_t *ctx);
static int  e000_ext_msg_send(nfn0012_ctx_t *ctx);
static int  x000_nf_err_log(nfn0012_ctx_t *ctx); 
/*static int  z000_error_proc(nfn0012_ctx_t *ctx);*/
static int  z100_log_insert(nfn0012_ctx_t *ctx,  char *log_data, int size, char io_flag, char sr_flag);

/* ------------------------------------------------------------------------------------------------------------ */
/* host recevie 부터 host(sna)send 까지 업무처리                                                                    */
/* ------------------------------------------------------------------------------------------------------------ */
int nfn0012(commbuff_t   commbuff)
{

    int                 rc = ERR_NONE;
    nfn0012_ctx_t       _ctx;  
    nfn0012_ctx_t       *ctx = &_ctx;


    SYS_TRSF;

    /* CONTEXT 초기화 */
    SYS_TRY(a000_data_receive(ctx, commbuff));

    /* 초기화 처리  */
    SYS_TRY(b000_init_proc(ctx));

    /* NFJRN Insert   */
    SYS_TRY(d000_jrn_insert(ctx));

    /* 대외기관 전문 전송  */
    SYS_TRY(f000_upd_jrn_proc(ctx));

    SYSGWINFO->gw_rspn_send = SYSGWINFO_GW_REPLY_SEND_NO;

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    switch(rc) {

        case GOB_ERR:
            SYS_TREF;

            /* 호스트에 무응답 */
            SYSGWINFO->gw_rspn_send = SYSGWINFO_GW_REPLY_SEND_NO;
            return ERR_ERR;

        /* 비밀번호 변경 전문일 경우에는 GW전송 안하고 처리한다. */
        case GOB_NRM:
            SYS_TREF;
            return ERR_NONE;
        
        default:
            z000_error_proc(ctx);
            break;

    }

    SYS_TREF;
    return ERR_ERR;
        

}
/* ------------------------------------------------------------------------------------------------------------ */
static int a000_data_receive(nfn0012_ctx_t  *ctx, commbuff_t  commbuff)
{
    int                 rc = ERR_NONE;
    char                tmp_rcv_data[100 + 1];

    SYS_TRSF;

    /* set commbuff  */
    memset((char *)ctx, 0x00, sizeof(nfn0012_ctx_t));
    ctx->cb = commbuff;
    


    /**
    memset(tmp_rcv_data, 0x00, sizeof(tmp_rcv_data));
    memcpy(tmp_rcv_data, &HOSTRCVDATA,[300 + 265], 100);

utohexdp(tmp_rcv_data, 100);


    SYS_DBG("tmp_rcv_data[%s]", tmp_rcv_data);
    printf("tmp_rcv_data[", sizeof(tmp_rcv_data));
    int tmp = 0;
    for (tmp = 0 ; tmp < sizeof(tmp_rcv_data); tmp ++){
        printf("%02X", tmp_rcv_data[tmp]);
    }
    printf("]"\n);
    **/

    SYS_TREF;

    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
static int b000_init_proc(nfn0012_ctx_t *ctx)
{

    int                 rc = ERR_NONE;
    hcmihead_t          hcmihead;
    char                *hp;

    SYS_TRSF;


    /* 입력 채널 clear */
    SYSICOMM->intl_tx_flag = 0;
    memset(SYSICOMM->call_svc_name, 0,  sizeof(SYSICOMM->call_svc_name));
    memcpy(SYSICOMM->call_svc_name, "nfn0012", 7);

    ctx->exmsg1500 = &ctx->_exmsg1500;

    /* host 수신 msg저장   */
    memcpy(ctx->exmsg1500, HOSTRECVDATA,    sysocbgs(ctx->cb, IDX_HOSTRECVDATA));
    memcpy(EXMSG1500->err_code,  "9999999"  , LEN_EXMSG1500_ERR_CODE);

#ifdef  _DEBUG
    /* --------------------------------------------------- */
    PRINT_EXMSG1500(EXMSG1500);
    /* --------------------------------------------------- */
#endif 

    //참가기관 정보 저장 후 BOK에 송신 하지 않고, 종료 처리 
    if (memcmp(&EXMSG1500->detl_area[15], "88", 2) == 0){
        rc = c000_sel_jrn_proc(ctx);
        if (rc == ERR_NONE){
            return GOB_NRM;
        }else{
            return ERR_ERR;
        }
    }

    EXEC SQL 
        SELECT TO_NUMBER(TRIM(MIR_CODE))
          INTO :nf_on_session_flag
          FROM excode
         WHERE maj_code = 'NF_ON_SESSION_FLAG';

    SYS_DBG("nf_on_session_flag[%d]", nf_on_session_flag);

    if (nf_on_session_flag != 9){
        SYS_HSTERR(SYS_LN, 9999999, "BEFORE SESSION KEY EXCHANGE ");
        SYS_TREF;
        return ERR_ERR;        
    }

    
    memcpy(ctx->msg_no      , &EXMSG1500->detl_area[15],  2);
    memcpy(&ctx->msg_no[2]  , &EXMSG1500->detl_area[32],  6);

    //전문관리 번호 추가 다른 전문일 경우에는  9900000로 세팅한다.
    if (memcmp(&EXMSG1500->detl_area[15], "TEJG00001", 9) == 0){
        memcpy(&ctx->msg_no[8], &EXMSG1500->detl_area[38],  7);
    }else{
        memcpy(&ctx->msg_no[8], "9900000",  7);
    }

    SYS_DBG("ctx->msg_no [%s]", ctx->msg_no);

    ctx->recv_len = utoa2in(EXMSG1500->detl_rec_size, LEN_EXMSG1500_DETL_REC_SIZE);
    SYS_DBG("ctx->recv_len[%d]", ctx->recv_len);

    SYS_DBG("SYSGWINFO->sys_type[%d]",  SYSGWINFO->sys_type);

    memset(ctx->corr_id, 0x00, LEN_NFI0002F_CORR_ID);


    /* KTI여부 (0:코어 2:KTI )   */
    if (SYSGWINFO->sys_type == 2){
        ctx->kti_flag = '1';
    
        /* TCP/IP 통신 헤더 정보 */
        hp = sysocbgp(ctx->cb, IDX_TCPHEAD);
        if (hp != NULL){
            memset(&hcmihead,   0x00, sizeof(hcmihead_t));
            memcpy(&hcmihead,   hp,   sysocbgs(ctx->cb, IDX_TCPHEAD));

            memcpy(ctx->corr_id,    hcmihead.queue_name, LEN_HCMIHEAD_QUEUE_NAME);
            SYS_DBG("corr_id [%s]", ctx->corr_id);
        }else {
            SYS_DBG("b000_init_proc :TCPHEAD ");
            return GOB_ERR;
        }
    }else {
        ctx->kti_flag = '0';
    }

    SYS_TREF;
    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int c000_sel_jrn_proc(nfn0012_ctx_t     *ctx)
{

    int                 rc = ERR_NONE;
    char                send_name [20 + 1];
    char                send_pswd [ 8 + 1];




    SYS_TRSF;

    memset(send_name,   0x00, sizeof(send_name));
    memset(send_pswd,   0x00, sizeof(send_pswd));

    memcpy(send_name,   &EXMSG1500->detl_area[114], 20);
    memcpy(send_pswd,   &EXMSG1500->detl_area[134],  8);

    SYS_DBG("send name [%s] send_pswd[%s]", send_name, send_pswd);

    EXEC SQL 
        UPDATE EXINSTSQ 
           SET send_name        = :send_name 
             , send_pswd        = :send_pswd
             , proc_date        = TO_CHAR(SYSDATE, 'YYYYMMDD')
         WHERE inst_no          = '926'
           AND appl_code        = '020';

    if (SYS_DB_CHK_FAIL){
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_sel_jrn_proc: UPDATE EXINSTSQ %d%d"
                             "[해결방안] 업무담당자 CALL ", __FILE__ , SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "update EXINSTSQ ERROR");
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    return ERR_ERR;
    
}

/* ------------------------------------------------------------------------------------------------------------ */
static int d000_jrn_insert(nfn0012_ctx_t    *ctx)
{

    int                 rc = ERR_NONE;
    

    nfi0002f_t          nfi0002f;

    SYS_TRSF;

    SYS_DBG("#1");
    memset(&nfi0002f,   0x00, sizeof(nfi0002f_t));

    SYS_DBG("#2");
    nfi0002f.in.exmsg1500 = EXMSG1500;

    SYS_DBG("#3");
    memcpy(nfi0002f.in.msg_no,  ctx->msg_no,    LEN_NFI0002F_MSG_NO);       /* msg_no     */
    memcpy(nfi0002f.in.corr_id, ctx->corr_id,   LEN_NFI0002F_CORR_ID);      /* corr_id    */
    nfi0002f.in.kti_flag[0] = ctx->kti_flag;


    SYS_DBG("jrn insert ====================== msg_no[%s]corr_id[%s]", nfi0002f.in.msg_no, nfi0002f.in.corr_id);

    rc = nf_jrn_insupd(&nfi0002f);

    if (rc == ERR_ERR){
        ex_syslog("[APPL_DM]%s :d000_jrn_insert(): nf_jrn_ins ERROR", __FILE__);
        return ERR_ERR;
    }

    SYS_TREF;
    return ERR_NONE;
}

/* ------------------------------------------------------------------------------------------------------------ */
static int f000_upd_jrn_proc(nfn0012_ctx_t      *ctx)
{
    int                 rc = ERR_NONE;


    SYS_TRSF;

/**
    if (EXMSG1500->rspn_flag[0] == '0){
        SYS_HSTERR(SYS_LN, 0000000, "NO RESPONSE MESSAGE");
        return GOB_NRM;
    }
**/


    rc = sysocbsi(ctx->cb, IDX_EXTSENDDATA, &HOSTRECVDATA[300], ctx->recv_len);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "COMMBUFF SET ERROR ");
        return ERR_ERR;
    }

    SYS_DBG("EXTSENDDATA  [%d][%.*s]", sysocbgs(ctx->cb, IDX_EXTSENDDATA), sysocbgs(ctx->cb, IDX_EXTSENDDATA),  EXTSENDDATA);


    /* 대외기관 G/W에 호출 하는 방식을 전달하기 위한 값을 set    */
    SYSGWINFO->time_val     = SYSGWINFO_SAF_DETL_TIMEOUT;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_SAF;      /* SAF방식 호출     */
    SYSGWINFO->rspn_flag    = SYSGWINFO_REPLY;              /* G/W로 부터 응답   */
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;           /* 전문 종류        */

    /* 대외기관 데이터 전송     */
    strcpy(SYSGWINFO->func_name, "NFEXTC");
    rc = sys_tpcall("SYEXTGW_NFC", ctx->cb, TPNOTRAN);

    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, 0265200, "GW SYEXTGW_NFC TPCALL ERROR");
        ex_syslog(LOG_ERROR, "[APPL_DM]%s nfn0012: f000_upd_jrn_proc() ERROR %d [해결방안] 대외기관 G/W담당자 CALL", __FILE__, tperrno);
        return ERR_ERR;
    }


    rc = z100_log_insert(ctx,  EXTSENDDATA, ctx->recv_len, '0', '1');
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%s nfn0012: NF_LOG_ERROR[UNIX->KFTC]", __FILE__);
    }

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    return ERR_ERR;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int z000_error_proc(nfn0012_ctx_t    *ctx)
{
    
    int                 rc = ERR_NONE;
    int                 len = 0;
    char                err_msg[20];

    SYS_TRSF;

    /* ------------------------------------------- */
    SYS_DBG("z000_error_proc  : err_code [%d]",  sys_err_code());
    SYS_DBG("z000_error_proc  : err_msg  [%s]",  sys_err_msg() );
    /* ------------------------------------------- */

    SET_1500ERROR_INFO(EXMSG1500);



    SYS_DBG("EXMSG1500->tx_code [%.10s]", EXMSG1500->tx_code);
    SYS_DBG("EXMSG1500->err_code [%.7s]", EXMSG1500->err_code);

    SYSGWINFO->gw_rspn_send = SYSGWINFO_GW_REPLY_SEND;      /*  호스트에 응답 전송    */
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;           /*  전문 타입 set       */

    rc = sysocbsi(ctx->cb,  IDX_SYSIOUTQ, EXMSG1500, sizeof(exmsg1500_t));

    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LC, SYS_GENERR, "EXT MSG COMMBUFF ERROR");
        return ERR_ERR;
    }



    SYS_TREF;

    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
static int z100_log_insert(nfn0012_ctx_t    *ctx, char *log_data, int size, char io_flag,   char sr_flag)
{

    int                 rc = ERR_NONE;
    nfi0003f_t          nfi0003f;
    commbuff_t          *dcb;
    char                *hp;

    SYS_TRSF;

    /* ------------------------------------------- */
    SYS_DBG("z100_log_insert  : len [%d]io_flag[%c]sr_flag[%c]",  size, io_flag, sr_flag);
    SYS_DBG("z100_log_insert  : msg [%d][%.*s]",  size, size, log_data );
    /* ------------------------------------------- */

    memset(&nfi0003f,   0x00, sizeof(nfi0003f_t));
    memcpy(nfi0003f.in.van_cd,  "26", 2);
    nfi0003f.in.io_flag     =  io_flag;
    nfi0003f.in.sr_flag     =  sr_flag;
    nfi0003f.in.log_len     =  size;
    memcpy(nfi0003f.in.msg_no,      ctx->msg_no, LEN_NFI0002F_MSG_NO);
    memcpy(nfi0003f.in.log_data,    log_data,    size);

    /* TCP/IP 통신 헤더 정보    */
    hp = sysocbgp(ctx->cb, IDX_TCPHEAD);
    if (hp != NULL){
        SYS_DBG("TCPHEAD LOG IS OK");
        memcpy(nfi0003f.in.tcp_head,    hp, LEN_TCP_HEAD);
        SYS_DBG("TCP HEAD [%s]", nfi0003f.in.tcp_head);
    }


    SYS_DBG("z100_log_insert: in.tcp_head[%d][%.*s]", strlen(nfi0003f.in.tcp_head), strlen(nfi0003f.in.tcp_head), nfi0003f.in.tcp_head);

    memset(&dcb, 0x00, sizeof(commbuff_t));
    rc = sysocbdb(ctx->cb, &dcb);       /* ctx->cb dcb로 복사    */
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s nfn0012: z100_log_insert() NFLOG ERROR sysocbdb log_type :%d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }


    rc = sysocbsi(&dcb, IDX_HOSTRECVDATA,   &nfi0003f, sizeof(nfi0003f_t));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s nfn0012: z100_log_insert() NFLOG ERROR sysocbsi log_type :%d", __FILE__, sr_flag);
        sys_err_init();
        sysocbfb(&dcb);
        return ERR_NONE;
    }

    rc = sys_tpcall("NFN3100F", &dcb, TPNOREPLY | TPNOTRAN);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s nfn0012: z100_log_insert() NFLOG ERROR sysocbsi log_type :%d", __FILE__, sr_flag);
        sys_err_init();
    }

    sysocbfb(&dcb);

    SYS_TREF;

    return ERR_NONE;

}

/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */
