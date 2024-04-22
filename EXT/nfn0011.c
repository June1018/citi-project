
/*  @file               nfn0011.pc
*   @file_type          c source program
*   @brief              국고 한국은행 개설메인 
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
#include <nfi0001x.h>
#include <exi0212.h>        /* 거래 PARAMETER LOAD   */
#include <nfi3100f.h>
#include <sqlca.h>
#include <bssess_stat.h>
#include <nfjrn.h>
#include <nfi0002f.h>
#include <hcmihead.h>
#include <utoclck.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXMSG1500           (ctx->exmsg1500)


/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct nfn0011_ctx_s    nfn0011_ctx_t;
struct nfn0011_ctx_s {
    commbuff        *cb;  

    exmsg1500_t     _exmsg1500;
    exmsg1500_t     *exmsg1500;

    /*int             kti_recv_len;                        수신 길이 */

    char            msg_no   [LEN_NFI0002F_MSG_NO   + 1];
    char            corr_id  [LEN_NFI0002F_CORR_ID + 1];
    char            kti_flag;  /* KTI Flag 1:KTI 0:core    */
    
};


/* ------------------------------------- exported global variables definitions -------------------------------- */

/* ------------------------------------------ exported function  declarations --------------------------------- */
static int  a000_data_receive(nfn0011_ctx_t *ctx, commbuff_t    *commbuff);
static int  b000_init_proc(nfn0011_ctx_t *ctx, int log_type);
static int  c000_get_corr_id(nfn0011_ctx_t *ctx);
static int  d000_jrn_insert(nfn0011_ctx_t *ctx);
static int  e000_kti_msg_send(nfn0011_ctx_t *ctx);
static int  x000_nf_err_log(nfn0011_ctx_t *ctx); 
static int  z100_log_insert(nfn0011_ctx_t *ctx,  char *log_data, int size, char io_flag, char sr_flag);
/* ------------------------------------------------------------------------------------------------------------ */
/* host recevie 부터 host(sna)send 까지 업무처리                                                                    */
/* ------------------------------------------------------------------------------------------------------------ */
int nfn0011(commbuff_t   commbuff)
{
    
    int                 rc = ERR_NONE;
    nfn0011_ctx_t       _ctx;  
    nfn0011_ctx_t       *ctx = &_ctx;

    SYS_TRSF;

    /* CONTEXT 초기화 */
    SYS_TRY(a000_data_receive(ctx, commbuff));

    /* 초기화 처리  */
    SYS_TRY(b000_init_proc(ctx));

    /* CORR_ID 채번    */
    SYS_TRY(c000_get_corr_id(ctx));

    /* JRN Insert   */
    SYS_TRY(d000_jrn_insert(ctx));

    /* 대외기관 SEND  */
    SYS_TRY(e000_kti_msg_send(ctx));

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    if (rc == GOB_NRM) {
        SYS_DBG("RETURN ERR_NONE");
        return ERR_NONE;
    }

    x000_nf_err_log(ctx);

    SYS_TREF;

    return ERR_ERR;
}
/* ------------------------------------------------------------------------------------------------------------ */
static int a000_data_receive(nfn0011_ctx_t  *ctx, commbuff_t  commbuff)
{
    
    int                 rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff  */
    memset((char *)ctx, 0x00, sizeof(nfn0011_ctx_t));
    ctx->cb = commbuff;
    
    SYS_ASSERT(HOSTRECVDATA);

    /* 입력 채널 clear */
    SYSICOMM->intl_tx_flag = 0;
    memset(SYSICOMM->call_svc_name, 0,  sizeof(SYSICOMM->call_svc_name));
    memcpy(SYSICOMM->call_svc_name, "NFN0011", 7);

    ctx->exmsg1500 = &ctx->_exmsg1500;

    SYS_DBG(" HOSTRECVDATA LEN[%d]", sysocbgp(ctx->cb, IDX_HOSTRECVDATA), sysocbgs(ctx->cb, IDX_EXTRECVDATA), EXTRECDATA);
    //SYS_DBG(" HOSTRECVDATA [%s]"   , HOSTRECVDATA);
    /* --------------------------------------------------- */
    /* --------------------------------------------------- */




    SYS_TREF;

    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
static int b000_init_proc(nfn0011_ctx_t *ctx)
{

    int                 rc = ERR_NONE;
    nfi0001x_t          nfi0001x;
    char                data_len;
    char                tx_no;

    SYS_TRSF;

    /* 1500 send용 초기화    */
    memset(&nfi0001x,   0x00, sizeof(nfi0001x_t));
    nfi0001x.in.exmsg1500   = EXMSG1500;
    nf_proc_exmsg1500_init(&nfi0001x);

    /* 전문길이 setting  */
    char temp_str[LEN_EXMSG1500_DETL_REC_SIZE + 1];
    memset(temp_str,    0x00, LEN_EXMSG1500_DETL_REC_SIZE + 1);
    utol2an( (int) sysocbgs(ctx->cb, IDX_EXTRECVDATA), LEN_EXMSG1500_DETL_REC_SIZE, temp_str);
    memcpy(EXMSG1500->detl_rec_size,    temp_str, LEN_EXMSG1500_DETL_REC_SIZE);
    memcpy(&EXMSG1500->detl_area   ,    EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA));

    /* exmsg1500 comm field set 나머지는 nfo0001x에서 기본적으로 setting */
    memcpy(EXMSG1500->tx_code,      "7657700000",   LEN_EXMSG1500_TX_CODE);

    /* 저널용 데이터 셋팅   */
    memcpy(tx_no,   0x00,  9 + 1);
    memcpy(tx_no,   &EXTRECDATA[4], 9);

    memcpy(ctx->msg_no,         &EXTRECVDATA[15],   2);
    memcpy(&ctx->msg_no[2],     &EXTRECVDATA[32],   6);

    /* 전문관리번호 추가, 다른 전문일 경우에는 9900000로 셋팅    */
    if (memcmp(&EXTRECVDATA[4], "TEJG00001" , 9) == 0){
        memcpy(&ctx->msg_no[8], &EXTRECVDATA[38],  7);
    }else{
        memcpy(&ctx->msg_no[8], "9900000",  7);
    }


    SYS_DBG("#1 ctx->msg_no [%s]tx_no[%s]", ctx->msg_no, tx_no);


    /* 이체 거래 재처리 (05)건인 경우 저널에는 일반거래(01)로 저장, 응답에는 01로 바꿔어 오기 매핑   */
    if (strcmp(ctx->msg_no, "05") == 0 && strcmp(tx_no, "TEJG00001") == 0){
        memcpy(ctx->msg_no, "01" ,2);
        SYS_DBG("#2 ctx->msg_no[%s]", ctx->msg_no);
    }

    
    SYS_DBG("#3 ctx->msg_no[%s]", ctx->msg_no);

    ctx->kti_flag = '1'      /* 0:GCG   1:ICG    */

    rc = z100_log_insert(ctx,  (char *)EXTRECDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), 'I', '1');


#ifdef  _SIT_DBG
    PRINT_EXMSG1500(EXMSG1500);
#endif    

    SYS_TREF;

    return ERR_NONE;
}

/* ------------------------------------------------------------------------------------------------------------ */
static int c000_get_corr_id(nfn0011_ctx_t     *ctx)
{

    int                 rc = ERR_NONE;
    char                corr_id[LEN_HCMIHEAD_QUEUE_NAME + 1], buff[LEN_HCMIHEAD_DATA_LEN + 1];
    /*char              tcphead;  */
    hcmihead_t          hcmihead;

    SYS_TRSF;

    /* kti corrid 생성 */
    memset(corr_id,   0x00, sizeof(corr_id));

    utocick(corr_id);

    SYS_DBG("corr_id[%s]", corr_id);

    memcpy(ctx->corr_id, corr_id,   LEN_HCMIHEAD_QUEUE_NAME);

    /* init hcmihead */
    /* MQ에서 TCPHEAD = null을 방지하기 위해 공백 space 로 초기화 */
    memset(&hcmihead,   0x20, sizeof(hcmihead));

    hcmihead.comm_type = 'A';           /* COMMUNICATION 구분 : SYNC, ASYNC     */

    memcpy(hcmihead.queue_name,     corr_id,    LEN_HCMIHEAD_QUEUE_NAME);

    /* hcmihead.data.len에 1500전문 사이즈인 4400이 들어가야하는지 BOK전문길이 464 or 460 맞는지? */
    /* bfn0011.pc에서 4400으로 셋팅함, 다만 symqhead_tax에서 별도로 data_len에 memcpy하고 있음  */
    /* sprinf(buff, "%5d", EXTRECDATA_SIZE);        //sysocbgs(cd, IDX_HOSTSENDDATA);
       memcpy(hcmihead.data_len, buff, LEN_HCMIHEAD_DATA_LEN); */

    SYS_DBG("hcmihead.queue_name: [%.*s] ", LEN_HCMIHEAD_QUEUE_NAME, hcmihead.queue_name);
    SYS_DBG("hcmihead.tx_code   : [%.*s] ", LEN_HCMIHEAD_TX_CODE   , hcmihead.tx_code   );

#ifdef  _DEBUG
    PRINT_HCMIHEAD(&hcmihead);
#endif    

    /* set tcp_head   */
    rc = sysocbsi(ctx->cb, IDX_TCPHEAD, &hcmihead, sizeof(hcmihead_t));

    SYS_TREF;

    return ERR_NONE;
}



/* ------------------------------------------------------------------------------------------------------------ */
static int d000_jrn_insert(nfn0011_ctx_t    *ctx)
{

    int                 rc = ERR_NONE;


    nfi0002f_t          nfi0002f;


    SYS_TRSF;

    SYS_DBG("#1");
    memset(&nfi0002f,   0x00, sizeof(nfi0002f_t));

    SYS_DBG("#2");
    nfi0002f.in.exmsg1500   = EXMSG1500;

    SYS_DBG("#3");
    memcpy(nfi0002f.in.msg_no,     ctx->trace_no,   LEN_NFI0002F_MSG_NO);
    memcpy(nfi0002f.in.corr_id,    ctx->corr_id,    LEN_NFI0002F_CORR_ID);
    nfi0002f.in.kti_flag[0] = ctx->kti_flag;

    SYS_DBG("d000_jrn_insert ============ msg_no:[%s]corr_id[%s]", nfi0002f.in.msg_no, nfi0002f.in.corr_id);

    rc = nf_jrn_insupd(&nfi0002f);

    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s: d000_jrn_insert() : nf_jrn_insupd ERROR", __FILE__);
        return ERR_ERR;
    }

    SYS_TREF;
    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int e000_kti_msg_send(nfn0011_ctx_t    *ctx)
{
    int                 rc  = ERR_NONE;
    char                host_send_data[2500];
    char                msg_len[4 + 1];

    SYS_TRSF;

    /* 전송데이터 저장 */
    rc = sysocbsi(ctx->cb, IDX_HOSTSENDDATA, EXMSG1500,  sizeof(exmsg1500_t));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s NFN0020 :e000_kti_msg_send(): COMMBUFF ERROR [해결방안] 담당자 CALL", __FILE__);
        SYS_HSTERR(SYS_LN,  222600, "COMMBUFF HOSTSENDDATA ERROR");
        return ERR_ERR;
    }

    SYS_DBG("HOSTSENDDATA [%s][%.*s]", sysocbgs(ctx->cb, IDX_HOSTSENDDATA), sysocbgs(ctx->cb,   IDX_HOSTSENDDATA), HOSTSENDDATA );

    /* 대외기관 G/W에 호출 하는 방식을 전달하기 위한 값을 set    */
    SYSGWINFO->time_val     = SYSGWINFO_SAF_DETL_TIMEOUT;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_SAF;      /* SAF방식 호출     */
    SYSGWINFO->rspn_flag    = SYSGWINFO_REPLY;              /* G/W로 부터 응답   */
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;           /* 전문 종류        */

    /* HOST DATA전송
     rc = sys_tpcall("SYMQSEND_TAX", ctx->cb, TPNOTRAN);
     
     
    ig 와 동일하게 처리 2023
    #if define(SYS_DIRECT_SWAP)
    #include <exi6961x.h>
        exi6961x_t  exi6961x;
        memset(&exi6961x,   0x00,   sizeof(exi6961x_t));
        memcpy(exi6961x.in.appl_code,   "083",  3);
        exi6961x.in.in_out_flag = '1';
        sess_chg_proc(&exi6961x);

        if (exi6961x.out.ch_flag[0] == '0'){
            //rc = sys_tpcall("SYONHTSEND",  ctx->cb, TPNOTRAN);
            rc = sys_tpcall("SYMQSEND_TAX"), ctx->cb, TPNOTRAN);

        }else if (exi6961x.out.ch_flag[0] == '1'){
            rc = sys_tpcall("SYMQSEND_TAX"), ctx->cb, TPNOTRAN);

        }else if (exi6961x.out.ch_flag[0] == '2'){
            rc = sys_tpcall("SYMQSEND_TAX"), ctx->cb, TPNOTRAN);

        }else if (exi6961x.out.ch_flag[0] == '1'){
     

#else
    //
    //
#endif

     */
    if (rc =ERR_ERR ){
        ex_syslog(LOG_ERROR, "[APPL_DM]%s NFN0020 :e000_kti_msg_send(): "
                             " TPERRNO[%d]ERROR %d [해결방안] HOST G/W담당자 CALL", __FILE__, tperrno, sys_err_code());
        if (EXMSG1500->rspn_flag[0] == '1'){
            switch(sys_err_code()){
            case ERR_SVC_CONERR:
            case ERR_SVC_NOREADY:
            case ERR_SVC_SNDERR:
                SYS_HSTERR(SYS_NN,  222600, "HOST SEND ERR");
                break;

            case ERR_SVC_TIMEOUT:
            case ERR_SVC_RCVERR:
            case ERR_SVC_SVRDOWN:
            default:
                SYS_HSTERR(SYS_NN,  222700, "RECV TIME OVER");
                break;
            }
        }
        else{
            SYS_HSTERR(SYS_NN, 222800, "HOST SEND ERR");
        }
        return ERR_ERR;
    }

    /* format 오류 송신시 무응답 처리   */
    SYS_DBG("EXMSG1500->rspn_flag[%c]", EXMSG1500->rspn_flag[0]);
    if (EXMSG1500->rspn_flag[0] == '0'){
        return GOB_NRM;
    }

    SYS_TREF;

    return ERR_NONE;
}
/* ------------------------------------------------------------------------------------------------------------ */
static int x000_nf_err_log(nfn0011_ctx_t    *ctx)
{
    int                 rc  = ERR_NONE;
    commbuff_t          dcb;
    exmsg1500_t         errmsg;

    SYS_TRSF;


    /* DATA Copy  */
    memcpy(&err_msg, EXMSG1500, sizeof(exmsg1500_t));
    /* 에러 프로그램명    */
    SET_1500ERROR_INFO(&errmsg);

    memset(&dcb,    0x00, sizeof(commbuff_t));
    rc = sysocbdb(ctx->cb, &dcb);
    if (rc == ERR_ERR){
        ex_syslog("[APPL_DM]%s :x000_nf_err_log(): COMMBUFF BACKUP ERROR[%d] [해결방안] 출동 담당자 CALL", __FILE__, tperrno);
        return ERR_ERR;
    }


    rc = sysocbsi(ctx->cb, IDX_EXMSG1200, &errmsg, sizeof(exmsg1500_t));
    if (rc == ERR_ERR){
        ex_syslog("[APPL_DM]%s :x000_nf_err_log(): COMMBUFF BACKUP ERROR[%d] [해결방안] 출동 담당자 CALL", __FILE__, tperrno);
        sys_err_inti();
        sysocbfb(&dcb);
        return ERR_ERR;
    }

    sysocbfb(&dcb);
    SYS_TREF;

    return ERR_NONE;
}

/* ------------------------------------------------------------------------------------------------------------ */
static int z100_log_insert(nfn0011_ctx_t    *ctx, char *log_data, int size, char io_flag,   char sr_flag)
{

    int                 rc = ERR_NONE;
    nfi3100f_t          nfi3100f;
    commbuff_t          *dcb;

    SYS_TRSF;

    /* ------------------------------------------- */
    SYS_DBG("z100_log_insert  : len [%d]",  size);
    SYS_DBG("z100_log_insert  : msg [%s]",  log_data );
    /* ------------------------------------------- */

    memset(&nfi3100f,   0x00, sizeof(nfi3100f_t));
    memcpy(nfi3100f.in.van_cd,  "26", 2);
    nfi3100f.in.io_flag     =  io_flag;
    nfi3100f.in.sr_flag     =  sr_flag;
    nfi3100f.in.log_len     =  size;
    memcpy(nfi3100f.in.msg_no,      ctx->msg_no, LEN_NFI0002F_MSG_NO);
    memcpy(nfi3100f.in.log_data,    log_data,    size);

    memset(&dcb, 0x00, sizeof(commbuff_t));
    rc = sysocbdb(ctx->cb, &dcb);       /* ctx->cb dcb로 복사    */
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s NFN0011: z100_log_insert() NFLOG ERROR sysocbdb log_type :%d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }


    rc = sysocbsi(&dcb, IDX_HOSTRECVDATA,   &nfi3100f, sizeof(nfi3100f_t));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s NFN0011: z100_log_insert() NFLOG ERROR sysocbsi log_type :%d", __FILE__, sr_flag);
        sys_err_init();
        sysocbfb(&dcb);
        return ERR_NONE;
    }

    rc = sys_tpcall("NFN3100F", &dcb, TPNOREPLY | TPNOTRAN);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s NFN0011: z100_log_insert() NFLOG ERROR sysocbsi log_type :%d", __FILE__, sr_flag);
        sys_err_init();
    }

    sysocbfb(&dcb);

    SYS_TREF;

    return ERR_NONE;

}

/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */
