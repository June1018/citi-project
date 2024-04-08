
/*  @file               ixn0040.pc
*   @file_type          pc source program
*   @brief              타행환 개설 입금 main
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
*   @generated at 2023/06/02 09:30
*   @history
*
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*
*/


/*  @pcode
 *  main.
 *   commbuff interface layout이 input임 
 *        a000 : CONTEXT 초기화
 *        b000 : 수신로그처리 
 *        c000 : 입력데이터 검증 
 *        d000 : 대외기관 거래코드를 호스트 거래코드로 변환 
 *        e000 : 거래파라미터 조회 
 *        f000 : 1200전문 전환 
 *        g000 : 개설 요청 결증 검증 
 *        h000 : 중복 검증 
 *        i000 : 저널생성   
 *        j000 : 호스트 SAF처리 
 *        k000 : 대외기관 에러 응답 처리 
 *
 *        a000 CONTEXT 초기화 
 *                              
 *        b000 수신로그 처리 
 *             1. IX로그테이블 (IXKULOG)에 로그 처리 	
 *        
 *        c000 입력데이터 검증
 *             1. 각 수신전문별 전문 검증 처리 
 *             2. 전문검증 오류시 
 *                - 대외기관 포맷오류 전문 송신처리 
 *        
 *        d000 대외기관 거래코드를 호스트 거래코드 변환 
 *             1. 호스트 거래코드 GET
 *             2. 거래코드 변환 오류시 대외기관 무응답 
 *
 *        e000 거래파라미터 조회 
 *             1. 호스트 거래코드별 파라메터 조회 
 *             2. 파라메터 조회 오류시 대외기관 무응답 
 *     
 *        f000 1200전문 변환
 *             1. 1200전문 초기화 
 *             2. 대외가관 전문을 1200전문으로 변환
 *             3. 1200전문 변환 오류시 대외 기관 에러 응답 전문 조립처리 및 송신처리 
 *
 *        g000 개설 요청 결번 검증 
 *             1. 개설요청 결번 검증 거래가 아니면 리턴처리 
 *             2. 결번 검증 후 commit처리 
 *             3. 낵 요청 결번 검증오류시 대외기관 무응답 
 *      
 *        h000 중복검증 
 *             1. 중복검증 거래가 아니면 리턴 처리 
 *             2. 중복거래인 경우 이중거래 에러 (309)응답전문 조립 처리 및 송신처리 
 *             3. 중복거래가 아니면 정상처리 
 *             4. 기타 DB_ERROR인 경우 은행센터의 system장애 (413) 응답전문 조립처리 및 송신처리 
 *
 *        h100 EI_MSG_NO채번       
 *             1. 개시 업무에 대한 EI_MSG_NO 최대값 채번 
 *
 *        i000 저널 생성
 *             1. 저널 생성거래가 아니면 리턴처리 
 *             2. 중복거래시 대외기관 무응답 
 *             3. 저널 생성 에러시 대외기관 에러 응답 전문 조립처리 및 송신처리 
 *        
 *        j000 호스트 SAF 처리 
 *             1. 호스트 SAF 처리 거래가 아니면 리턴 처리 
 *             2. 호스트 SAF 처리 에러시 대외기관 에러 응답 전문 조립처리 및 송신처리 
 *
 *        k000 대외기관 에러 응답 처리 
 *             1. 대외기관 에러 응답 전문 조립처리 
 *             2. 대외기관 전송 데이터 길이 검증 
 *             3. 대외기관 전문 송신처리 
 *             4. 대외기관 송신 전문 로깅 
 *             5. 대외기관 에러 응답 처리 에러시 대외기관 무응답 
 *
 *
 *
 *
 *
 */

/* ---------------------------------------------- include files ----------------------------------------------- */
#include <syscom.h>
#include <exdefine.h>
#include <exmsg1200.h>
#include <exi0210x.h>
#include <exi0230x.h>
#include <exi0250x.h>
#include <exi4000x.h>
#include <exparm.h>
#include <ixdefine.h>
#include <ixmsgkftc.h>
#include <ixdetlarea.h>
#include <ixi0200x.h>
#include <ixi0220x.h>
#include <ixi1040x.h>
#include <ixi0110x.h>
#include <ixi0120f.h>
#include <ixi0140f.h>
#include <ixi1060f.h>
#include <ixi0230f.h>
#include <ixi0320f.h>
#include <exi0320f.h>  /* Decoupling */
#include <sqlca.h>

#define LEN_KTI_FLAG            1

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct ixn0040_ctx_s    ixn0040_ctx_t;
struct ixn0040_ctx_s {
    commbuff        *cb;  

    int             ext_recv_flag;                  /* 결제원 수신데이터 길이 */
    int             kftc_err_set;                   /* 결제원 에러 응답 전문조립 여부 */
    char            host_tx_code[LEN_TX_CODE + 1];  /* 내부 거래코드        */
    char            ext_recv_data[2000];            /* 대외기관 요청 전문    */

    ixi0220x_t      ixi0220x;

    /* Decoupling ************************************************************/
    /* kti flag     : exmsg1200->kti_flag를 임시보관하여 사용함                   */
    /* our_msg_no   : CORE, KTI에서 채번한 관리 일련번호로 exmsg1200->out_msg_no   */
    /* ei_msg_no    : IXMAX에서 채번한 EI관리일련번호 보관                         */
    /* acct_type    : exmsg1200->kti_flag의에 값에 따라 조립함                   */
    /*                b200_max_msg_no_proc에서 조립함                          */
    /************************************************************************/

    char            kti_flag[LEN_KTI_FLAG + 1];
    char            out_msg_no[LEN_MQMSG1200_OUT_MSG_NO + 1];
    char            ei_msg_no[LEN_MQMSG1200_EI_MSG_NO + 1];
    char            acct_type[LEN_MQMSG1200_CR_ACCT_TYPE +1];   

};



/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int          a000_data_receive(ixn0040_ctx_t *ctx, commbuff_t    *commbuff);
static int          b000_msg_logging(ixn0040_ctx_t *ctx, int log_type);
static int          c000_kftc_fild_chk(ixn0040_ctx_t *ctx);
static int          d000_tran_code_conv(ixn0040_ctx_t *ctx);
static int          e000_exparm_read(ixn0040_ctx_t *ctx);
static int          f000_msg_format(ixn0040_ctx_t *ctx);
/* Decoupling */
/* KIT FLAG 조회 및 exmsg1200에 조립 */
static int f100_gcg_icg_acct_chk_proc(ixn0040_ctx_t *ctx)
/******************************************************************************************/
static int g000_ix_skn_check(ixn0040_ctx_t *ctx)
static int h000_ix_dup_check(ixn0040_ctx_t *ctx)
/* Decoupling */
/* EI_MSG_NO 최대값 + 1조립  */
/******************************************************************************************/
static int h100_max_msg_no_proc(ixn0040_ctx_t *ctx)
/******************************************************************************************/
static int i000_ixjrn_insert(ixn0040_ctx_t *ctx)
static int j000_ix_host_saf_prod(ixn0040_ctx_t *ctx)
static int k000_kftc_err_send(ixn0040_ctx_t *ctx)


/* ------------------------------------------------------------------------------------------------------------ */
int ixn0040(commbuff_t  *commbuff)
{

    int                 rc = ERR_NONE;
    ixn0040_ctx_t       _ctx;  
    ixn0040_ctx_t       *ctx = &_ctx;   

    SYS_TRSF;

    /* CONTEXT 초기화  */
    SYS_TRY(a000_data_receive(ctx, commbuff));

    /* 입력전문 LOGGING  */
    SYS_TRY(b000_msg_logging(ctx, KFTC_RECV_LOG));

    /* 입력 데이터 검증  */
    SYS_TRY(c000_kftc_fild_chk(ctx));

    /* 대외기관 거래 코드를 호스트 거래코드 변환   */
    SYS_TRY(d000_tran_code_conv(ctx));

    /* 거래파라메타 조회   */
    SYS_TRY(e000_exparm_read(ctx));

    /* 1200전문 변환   */
    SYS_TRY(f000_msg_format(ctx));

    /* Decoupling     */
    /* 계좌종류 구분 코드 */
    SYS_TRY(f100_gcg_icg_acct_chk_proc(ctx));
    /******************************************************************************************/
    
    /* 개설요청 결번 검증   */
    SYS_TRY(g000_ix_skn_check(ctx));

    /* 중복 검증  */
    SYS_TRY(h000_ix_dup_check(ctx));

    /*  Decoupling   **************************************************************************/
    /* EI관리번호 채번     ***********************************************************************/
    /******************************************************************************************/
    SYS_TRY(h100_max_msg_no_proc(ctx));
    /******************************************************************************************/

    /* 저널 생성 번호 */
    SYS_TRY(i000_ixjrn_insert(ctx));

    /* 호스트 SAF처리  */
    SYS_TRY(j000_ix_host_saf_prod(ctx));


    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    switch(rc){
    case GOB_NRM;
        /* 대외기관 무응답   */
        break;

        default:
        /* 대외기관 에러 응답 전송   */
        k000_kftc_err_send(ctx);
        break;
    }


    SYS_TREF;
    return ERR_ERR;
}



/* ------------------------------------------------------------------------------------------------------------ */
static int          a000_data_receive(ixn0040_ctx_t *ctx, commbuff_t    *commbuff)
{
    int      rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(ixn0040_ctx_t));
    ctx->cb = commbuff;

    if (EXTRECVDATA == NULL) {
        SYS_HSTERR(SYS_NN, SYS_GENERR, "EXTRECVDATA DATA NOT FOUND");
        return GOB_NRM;
    }

    /* 입력 채널 clear */
    SYSICOMM->intl_tx_flag = 0;

    /* 결제원 응답 데이터 길이 set */
    ctx->ext_recv_len = sysocbgs(ctx->cb, IDX_EXTRECVDATA);
    memcpy(ctx->ext_recv_data, EXTRECVDATA, ctx->ext_recv_len);

    /* 결제원 에러 응답 조립여부 초기화 (0:조립하지 않음, 1:조립함.)   */
    ctx->kftc_err_set = 1;

#ifdef _SIT_DBG
    PRINT_IX_KFTC_DEBUG(ctx->ext_recv_data);
#endif

    SYS_TREF;

    return ERR_NONE;
}


/* ------------------------------------------------------------------------------------------------------------ */
static int b000_msg_logging(ixn0040_ctx_t   *ctx, int log_type)
{
    int                 rc  = ERR_NONE;
    ixi0230f_t          ixi0230f;
    commbuff_t          dcb;


    SYS_TRSF;

    /* --------------------------- */            
    /* logging                     */
    /* KFTC_RECV : KFTC -> UNIX    */
    /* KFTC_SEND : UNIX -> KFTC    */
    /* --------------------------- */
    memset(&ixi0230f,   0x00, sizeof(ixi0230f_t));

    /* 결제원 수신 전문 로깅 */
    if (log_type == KFTC_RECV_LOG){
        ixi0230f.in.flag            = 'R';
        ixi0230f.in.log_type        = 'K';
        ixi0230f.in.log_len         = ctx->ext_recv_len;
        memcpy(ixi0230f.in.log_data,  ctx->ext_recv_data, ixi230f.in.log_len);
    }

    /* ---------------------------------------------------------------------- */
    SYS_DBG("b000_msg_logging: len = [%d]", ixi0230f.in.log_len);
    SYS_DBG("b000_msg_logging: msg = [%s]", ixi0230f.in.log_data);
    /* ---------------------------------------------------------------------- */

    memset(&dcb,    0x00, sizeof(commbuff_t));
    rc = sysocbdb(ctx->cb,  &dcb);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s IXN0040: b000_msg_logging()"
                             "COMMBUFF BACKUP ERROR "
                             "[해결방안]시스템 담당자 call", 
                             __FILE__);
        sys_err_init();
        return ERR_NONE;
    }











    
}
/* ------------------------------------------------------------------------------------------------------------ */
static int   b100_mqparm_load(mqnsend01_ctx_t   *ctx)
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
static int   d100_init_mqcon(mqnsend01_ctx_t    *ctx)
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
static int   e100_get_sendmsg(mqnsend01_ctx_t   *ctx)
{

    int                 rc = ERR_NONE;
    mqimsg001_t         mqimsg001;

    /* DB 연결이 끊어진 경우 재 연결 */
    if (g_db_connect("") != ERR_NONE) {
        ex_syslog(LOG_ERROR, "e100_get_sendmsg db_connect re connecting ");
        tpschedule(60);
        return GOB_NRM;
    }

    EXEC SQL DECLARE SENDMSG_CURSOR CURSOR for 
        SELECT PROC_DATE,
               CHNL_CODE,
               APPL_CODE,
               IO_TYPE,
               MSG_ID,
               CORR_ID,
               PROC_TIME,
               RSPN_FLAG,
               ILOG_JRN_NO,
               SYS_MTIME,
               PROC_TYPE,
               ERR_CODE,
               ERR_MSG,
               MQ_DATE,
               MQ_TIME,
               MQLEN,
               MQMSG
          FROM EXMQMSG_001  
         WHERE CHNL_CODE = :g_chnl_code
           AND APPL_CODE = :g_appl_code
           AND PROC_DATE IN (TO_CHAR(SYSDATE, 'YYYYMMDD') , TO_CHAR(SYSDATE-1, 'YYYYMMDD')
           AND IO_TYPE   IN (3,4)
           AND PROC_TYPE = 0
         ORDER BY PROC_DATE, MSG_ID;

    EXEC SQL OPEN SENDMSG_CURSOR;

    if (SYS_DB_CHK_FAIL) {
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s e100_get_sendmsg(): CURSOR OPEN (EXMQMSG_001) ERROR [%d][해결방안] 업무담당자CALL: MSG[%s] ", __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        return ERR_ERR;

    } 

    while(1){
        /* error 코드 초기화 */
        memset(MQMSG001, 0x00, sizeof(exmqmsg_001_t));


        EXEC SQL FETCH SENDMSG_CURSOR
                INTO :MQMSG001->proc_date,
                     :MQMSG001->chnl_code,
                     :MQMSG001->appl_code,
                     :MQMSG001->io_type,
                     :MQMSG001->msg_id,
                     :MQMSG001->corr_id,
                     :MQMSG001->proc_time,
                     :MQMSG001->rspn_flag,
                     :MQMSG001->ilog_jrn_no,
                     :MQMSG001->sys_mtime,
                     :MQMSG001->proc_type,
                     :MQMSG001->err_code,
                     :MQMSG001->err_msg,
                     :MQMSG001->mq_date,
                     :MQMSG001->mq_time,
                     :MQMSG001->mqlen,
                     :MQMSG001->mqmsg;

        if (SYS_DB_CHK_FAIL){
            //SYS_DBG("EXMQMSG_001 DATA NOT FOUND");
            return GOB_NRM;
        }

        //
        //PRINT_EXMQMSG_001(MQMSG001);
        //
        //MQPUT
        if (f100_put_mqmsg(ctx) == ERR_ERR){
            SYS_DBG("CALL f100_put_mqmsg !!!!!!!!");
            return ERR_ERR;
        }




        memset(&mqimsg001,  0x00, sizeof(mqimsg001_t));
        mqimsg001.in.mqmsg_001 = MQMSG001;
        memcpy(mqimsg001.in.job_proc_type, "2", LEN_MQIMSG001_PROC_TYPE);
        rc = mqomsg001(&mqimsg001);
        if (rc == ERR_ERR){
            EXEC SQL ROLLBACK WORK;
            return ERR_ERR;
        }
        EXEX SQL COMMIT WORK;

    }

    return ERR_NONE;   

}


/* ------------------------------------------------------------------------------------------------------------ */
static int   f100_put_mqmsg(mqnsend01_ctx_t *ctx)
{
    int                 rc     = ERR_NONE;
    mq_info_t           mqinfo = {0};
    MQMD                md     = {MQMD_DEFAULT};



    SYS_TRSF;

    /* MQPUT을 위한 mq_info_t Set : MQIHEAD + INPDATA   */
    mqinfo.hobj             = &ctx->mqhobj;         /* PUT QUEUE OBJECT */
    mqinfo.hcon             = ctx->mqhconn;         /* QM Connection    */
    mqinfo.coded_charset_id = EXMQPARM->charset_id  /* CodedCharSetID   */

    //memcpy(md.MsgId,          MQMI_NONE,      sizeof(md.MsgId));
    //memcpy(MQMSG001->msg_id,  md.MsgId,       LEN_EXMQMSG001_MSG_ID);
    //memcpy(mqinfo.msg_id,     MQMSG001.msg_id,LEN_EXMQMSG001_MSG_ID);

    memcpy(mqinfo.corrid,       MQMSG001->corr_id,  strlen(MQMSG001->corr_id));
    memcpy(mqinfo.msgbuf,       MQMSG001->mqmsg,    MQMSG001->mqlen);
    mqinfo.msglen = MQMSG001->mq_len;
    

    rc = ex_mq_put_with_opts(&mqinfo, &md, 0);
    if (rc == ERR_NONE){
        rc = ex_mq_commit(&ctx->mqhconn);

        memcpy(MQMSG001->mq_date,   md.PutDate, LEN_EXMQMSG001_MQ_DATE);
        memcpy(MQMSG001->mq_time,   md.PutTime, LEN_EXMQMSG001_MQ_TIME);

    } else {

        /* reset flag for mq initialize */
        ctx->is_mq_connected = 0;

        SYS_DBG("CALL ex_mq_put FAIL !!!");
        ex_syslog(LOG_FATAL, "[APPL_DM] CALL ex_mq_put FAIL [해결방안]시스템 담당자  CALL");
        ex_mq_rollback(&ctx->mqhconn);
    }

    SYS_TREF;

    return ERR_NONE;

}


/* ------------------------------------------------------------------------------------------------------------ */
int    mqnsend01(commbuff_t *commbuff)
{
    int           i;


    return ERR_NONE;

}



/* ------------------------------------------------------------------------------------------------------------ */

int   apsvrdone()
{
    int           i;  


    return ERR_NONE;
}
/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */