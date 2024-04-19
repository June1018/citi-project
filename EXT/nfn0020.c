
/*  @file               nfn0020.pc
*   @file_type          c source program
*   @brief              국고 한국은행 취급응답  main
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
*   @generated at 2023/06/02 09:30
*   @history
*
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*/
/* @pcode
*  main
*  1.   a000    CONTEXT초기화 및 commbuff검증 대외기관 수신전문 logging 
*  2.   b000    1500전문 초기화  
*  3.   c000    ia1500 table 에서 원전문 select
*  4.   d000    거래 PARAMETER load
*  5.   e000    ia1500 table 에서 원전문 delete
*  6.   f000    대외기관 응답전문 검증 
*  7.   g000    Host전문 전송 
*
*/

 /* ---------------------------------------------- include files ----------------------------------------------- */
#include <syscom.h>
#include <sysconst.h>
#include <utodate.h>
#include <exdefine.h>
#include <common_define.h>
#include <exmsg1500.h>
#include <exi0212.h>
#include <nfi0001f.h>
#include <nfi0002f.h>
#include <nfi3100f.h>
#include <sqlca.h>
#include <uflog.h>
#include <hcmihead.h>

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXMSG1500           (ctx->exmsg1500)


/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct nfn0020_ctx_s    nfn0020_ctx_t;
struct nfn0020_ctx_s {
    commbuff        *cb;  

    exmsg1500_t     _exmsg1500;     /* host send 용 Structure */
    exmsg1500_t     *exmsg1500;

    char            msg_type [LEN_NFI0002F_MSG_TYPE + 1];
    char            msg_no   [LEN_NFI0002F_MSG_NO   + 1];
    char            trace_no [LEN_NFI0002F_TRACE_NO + 1];
    char            kti_flag;  /* KTI Flag 1:KTI 0:core    */


};


/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int  a000_data_receive(nfn0020_ctx_t *ctx, commbuff_t    *commbuff);
static int  b000_init_proc(nfn0020_ctx_t *ctx, int log_type);
static int  c000_sel_orig_msg_proc(nfn0020_ctx_t *ctx);
static int  d000_exparm_load(nfn0020_ctx_t *ctx);
static int  f000_host_msg_format_proc(nfn0020_ctx_t *ctx);
static int  e000_sel_jrn_proc(nfn0020_ctx_t *ctx);
static int  k000_host_msg_send(nfn0020_ctx_t *ctx);
static int  k100_kti_msg_send(nfn0020_ctx_t *ctx);
static int  m000_upd_jrn_proc(nfn0020_ctx_t *ctx);
static int  x000_nf_err_log(nfn0020_ctx_t *ctx); 
static int  z100_log_insert(nfn0020_ctx_t *ctx,  char *log_data, int size, char io_flag, char sr_flag);

/* ------------------------------------------------------------------------------------------------------------ */
/* host recevie 부터 host(sna)send 까지 업무처리                                                                    */
/* ------------------------------------------------------------------------------------------------------------ */
int nfn0020(commbuff_t   commbuff)
{
    int                 rc = ERR_NONE;
    nfn0020_ctx_t       _ctx;  
    nfn0020_ctx_t       *ctx = &_ctx;

    SYS_TRSF;

    /* CONTEXT 초기화 */
    SYS_TRY(a000_data_receive(ctx, commbuff));

    /* 초기화 처리  */
    SYS_TRY(b000_init_proc(ctx));

    /* orig msg selct */
    SYS_TRY(c000_sel_orig_msg_proc(ctx));

    /* kti msg select */
    SYS_TRY(e000_sel_jrn_proc(ctx));

    if (ctx->kti_flag == '0'){
        /* host 전문 전송  */
        SYS_TRY(k000_host_msg_send);
    }else{
        /* kti 전문 전송  */        
        SYS_TRY(k100_kti_msg_send);
    }

    /* nfjrn update   */
    SYS_TRY(m000_upd_jrn_proc(ctx));

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return ERR_ERR;

}
/* ------------------------------------------------------------------------------------------------------------ */
static int a000_data_receive(nfn0020_ctx_t  *ctx, commbuff_t  commbuff)
{

    int                 rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff  */
    memset((char *)ctx, 0x00, sizeof(nfn0020_ctx_t));
    ctx->cb = commbuff;
    
    SYS_ASSERT(HOSTRECVDATA);

    /* 입력 채널 clear */
    SYSICOMM->intl_tx_flag = 0;
    memset(SYSICOMM->call_svc_name, 0,  sizeof(SYSICOMM->call_svc_name));
    memcpy(SYSICOMM->call_svc_name, "NFN0020", 7);

    ctx->exmsg1500 = &ctx->_exmsg1500;

    SYS_DBG(" HOSTRECVDATA LEN[%d]", sysocbgp(ctx->cb, IDX_HOSTRECVDATA));

    SYS_TREF;

    return ERR_NONE;

}



/* ------------------------------------------------------------------------------------------------------------ */
static int b000_init_proc(nfn0020_ctx_t *ctx)
{

    int                 rc = ERR_NONE;
    nfi0001f_t          nfi0001f;



    SYS_TRSF;


    memse(nfi0001f, 0x00, sizeof(nfi0001f_t));

    /* HOST 1500 전문 commbuff set   */
    nfi0001f.in.exmsg1500 = EXMSG1500;
    nf_proc_exmsg1500_init(&nfi0001f);
    
    /* 전문길이 세팅          */
    char temp_str[LEN_EXMSG1500_DETL_REC_SIZE + 1];
    memset(temp_str,    0x00, LEN_EXMSG1500_DETL_REC_SIZE + 1);
    utol2an( (int) sysocbgs(ctx->cb, IDX_HOSTRECVDATA), LEN_EXMSG1500_DETL_REC_SIZE, temp_str);

    memcpy(EXMSG1500->detl_rec_size,    temp_str, LEN_EXMSG1500_DETL_REC_SIZE);
    memcpy(&EXMSG1500->detl_area   ,    EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA));

    memcpy(EXMSG1500->tx_id,        "UTRY   "   , LEN_EXMSG1500_TX_ID);


    memcpy(ctx->msg_no,         &EXTRECVDATA[15],   2);
    memcpy(&ctx->msg_no[2],     &EXTRECVDATA[32],   6);


    /* 전문관리번호 추가, 다른 전문일 경우에는 9900000로 셋팅    */
    if (memcmp(&EXTRECVDATA[4], "TEJG00001" , 9) == 0){
        memcpy(&ctx->msg_no[8], &EXTRECVDATA[38],  7);
    }else{
        memcpy(&ctx->msg_no[8], "9900000",  7);
    }

    memcpy(ctx->msg_type,   &EXTRECVDATA[13],  2);

    SYS_DBG("ctx->msg_no [%s]ctx->msg_type[%s]", ctx->msg_no, ctx->msg_type);


    /* 취급응답일 경우 0으로 set  */
    EXMSG1500->rspn_flag[0] = '0';

    rc = z100_log_insert(ctx,   EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), '0', 2);

#ifdef  _DEBUG
    /* --------------------------------------------------- */
    PRINT_EXMSG1500(EXMSG1500);
    /* --------------------------------------------------- */
#endif 

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int c000_sel_orig_msg_proc(nfn0020_ctx_t     *ctx)
{

    int                 rc = ERR_NONE;
    char                buff[LEN_TCP_HEAD + 1];
    nflog_t             nflog;

    SYS_TRSF;

    /* -------------------------------------------------------------------- */
    /* BBLOG정보 조회 처리                                                     */
    /* -------------------------------------------------------------------- */
    memset(&nflog,      0x00, sizeof(nflog_t));
    utodate1(nflog.proc_date);
    memcpy(nflog.msg_no,    ctx->msg_no,    LEN_NFI0002F_MSG_NO);

    /* -------------------------------------------------------------------- */
    SYS_DBG("c000_sel_orig_msg_proc: proc_date[%s] ", nflog.proc_date);
    SYS_DBG("c000_sel_orig_msg_proc: msg_no   [%s] ", nflog.msg_no   );
    /* -------------------------------------------------------------------- */

    /* rownum =1 을 준 이유는 호스트에서 전송한 후 에러 발생으로 응답을 못받고 
     * 동일 전문 번호로 재전송하는 경우 두건 이상이 나올 수 있으므로 
     * 가장 최근 TCP헤더 정보를 조회한다.  
    */
    EXEC SQL
        SELECT SUBSTR(TCP_HEAD, 1, 72)
          INTO :nflog.tcp_head 
          FROM (SELECT LOG_DATA,
                       SUBSTR(NVL(TCP_HEAD, ' '), 1, 72) TCP_HEAD 
                  FROM NFLOG 
                 WHERE PROC_DATE        = :nflog.proc_date
                   AND TRIM(MSG_NO)     = TRIM(:nflog.msg_no)
                   AND IO_FLAG          = '0'
                   AND SR_FLAG          = '1'
                 ORDER BY PROC_TIME   DESC 
               ) 
          WHERE ROWNUM = 1;
    if (SYS_DB_CHK_FAIL) {
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM]%s c000_sel_orig_msg_proc : 원거래 없음 [%s][%s][%d][%s]"
                , __FILE__ , nflog.proc_date, nflog.msg_no, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        SYS_TREF;
        return ERR_ERR;
    }

    /* TCP/IP헤더 정보   */
    utortrim(nflog.tcp_head);
    if (strlen(nflog.tcp_head) > 0){
        memset(buff, 0x20, sizeof(buff));
        memcpy(buff, nflog.tcp_head, strlen(nflog.tcp_head));
        rc = sysocbsi(ctx->cb, IDX_TCPHEAD, buff, LEN_TCP_HEAD);
        if (rc == ERR_ERR){
            ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_sel_orig_msg_proc() : COMMBUFF(TCPHEAD) SET ERROR [해결방안]시스템 담당자 CALL",
                      __FILE__);
            return ERR_ERR;
        }
    }


    SYS_TREF;
    return ERR_NONE;
    
}

/* ------------------------------------------------------------------------------------------------------------ */
static int e000_sel_jrn_proc(nfn0020_ctx_t    *ctx)
{

    int                 rc = ERR_NONE;
    nfi0002f_t          nfi0002f;

    SYS_TRSF;

    memset(&nfi0002f,   0x00, sizeof(nfi0002f_t));

    nfi0002f.in.exmsg1500 = (exmsg1500_t *) EXMSG1500;
    memcpy(&nfi0002f.in.msg_no,     ctx->msg_no,        LEN_NFI0002F_MSG_NO);
    memcpy(&nfi0002f.in.msg_type,   ctx->msg_type,      LEN_NFI0002F_MSG_TYPE);

    utodate1(ctx->proc_date);
    memcpy(&nfi0002f.in.proc_date,  ctx->proc_date,     LEN_NFI0002F_PROC_DATE);

    rc = nf_jrn_sel(&nfi0002f);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%s e000_sel_jrn_proc ERROR", __FILE__ );
        SYS_HSTERR(SYS_NN, SYS_GENERR, "NF_JRN_SEL_ERROR");
        return ERR_ERR;
    }

    /* -------------------------------------------------------------------- */
    SYS_DBG("nfjrn select ");
    PRINT_EXMSG1500(EXMSG1500);
    /* -------------------------------------------------------------------- */

    memset(ctx->corr_id, 0x00, LEN_NFJRN_CORR_ID);

    /* set kti_flag  */
    if (nfi0002f.out.nfjrn.kti_flag[0] == '0'){
        ctx->kti_flag = '0';
    }else{
        ctx->kti_flag = '1';
        memcpy(ctx->corr_id, nfi0002f.out.nfjrn.corr_id, LEN_NFJRN_CORR_ID);
    }

    memcpy(EXMSG1500->tx_code, nfi0002f.out.nfjrn.tx_code, LEN_EXMSG1500_TX_CODE);
    SYS_DBG("ctx->corr_id[%s]tx_code[%s]", ctx->corr_id,  EXMSG1500->tx_code);
    

    SYS_TREF;

    return ERR_NONE;

}
/*& working ......*/
/* ------------------------------------------------------------------------------------------------------------ */
static int m000_upd_jrn_proc(nfn0020_ctx_t      *ctx)
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

#if 1
    rc = nf_jrn_insupd(&nfi0002f);

    if (rc == ERR_ERR){
        ex_syslog("[APPL_DM]%s :m000_upd_jrn_proc(): nf_jrn_ins ERROR", __FILE__);
        return ERR_ERR;
    }

#endif 

    SYS_TREF;
    return ERR_NONE;
 
}

/* ------------------------------------------------------------------------------------------------------------ */
static int x000_nf_err_log(nfn0020_ctx_t    *ctx)
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
static int z100_log_insert(nfn0020_ctx_t    *ctx, char *log_data, int size, char io_flag,   char sr_flag)
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
        ex_syslog(LOG_ERROR, "[APPL_DM]%s nfn0020: z100_log_insert() NFLOG ERROR sysocbdb log_type :%d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }


    rc = sysocbsi(&dcb, IDX_HOSTRECVDATA,   &nfi3100f, sizeof(nfi3100f_t));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s nfn0020: z100_log_insert() NFLOG ERROR sysocbsi log_type :%d", __FILE__, sr_flag);
        sys_err_init();
        sysocbfb(&dcb);
        return ERR_NONE;
    }

    rc = sys_tpcall("NFN3100F", &dcb, TPNOREPLY | TPNOTRAN);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s nfn0020: z100_log_insert() NFLOG ERROR sysocbsi log_type :%d", __FILE__, sr_flag);
        sys_err_init();
    }

    sysocbfb(&dcb);

    SYS_TREF;

    return ERR_NONE;

}

/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */
