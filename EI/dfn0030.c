
/*  @file               dfn0030.pc
*   @file_type          pc source program
*   @brief              일괄전송 프로그램 
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
#include <dfi0001x.h>
#include <dfi0002f.h>
#include <dfi0003f.h>
#include <dfi3100f.h>
#include <exi0251x.h>
#include <exparm.h>
#include <sqlca.h>
#include <bssess_stat.h>
#include <exmqmsg.h>
#include <mqmsg.h>
#include <exmqmsg_df.h>
#include <hcmihead.h>
#include <eierrhead.h>
#include <utoclck.h>
#include <exmsg11000.h>
/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXMSG11000              (ctx->exmsg11000)
#define FCY_CODE                "073"
#define LEN_SIZE                5
/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct dfn0030_ctx_s dfn0030_ctx_t;
struct dfn0030_ctx_s {
    commbuff_t   *cb;

    exmsg11000_t   _exmsg11000;     /* host send 용 structure  */
    exmsg11000_t   *exmsg11000;

    char msg_type           [LEN_DFI0030_MSG_TYPE       + 1];   /* 전문종별코드         */
    char proc_code          [LEN_DFI0030_PROC_CODE      + 1];   /* 거래구분코드         */
    char status_code        [LEN_DFI0030_STATUS_CODE    + 1];   /* 상태구분코드         */
    char rspn_code          [LEN_DFI0030_RSPN_CODE      + 1];   /* 응답   코드         */
    char rspn_bank_code     [LEN_DFI0030_RSPN_BANK_CODE + 1];   /* 응답코드 부여대표기관 코드 */
    char proc_date          [LEN_DFI0030_PROC_DATE      + 1];   /* 전문전송일자         */
    char proc_time          [LEN_DFI0030_PROC_TIME      + 1];   /* 전문전송시간         */
    char trace_no           [LEN_DFI0030_TRACE_NO       + 1];   /* 전문추적번호         */
    char recv_filler        [LEN_DFI0030_RECV_FILLER    + 1];   /* 송신자예비정보 필드    */
    char msg_no             [LEN_DFI0030_MSG_NO         + 1];   /* 거래고유번호           */
    char trd_data_nm        [LEN_DFI0030_TRD_DATA_NM    + 1];   /* 거래및 결제대사자료 이동  */
    char div_tot_cnt        [LEN_DFI0030_DIV_TOT_CNT    + 1];   /* 분할된 전문의 총개수     */
    char div_sr_num         [LEN_DFI0030_DIV_SR_NUM     + 1];   /* 분할된 전문일련 번호     */
    char filler             [LEN_DFI0030_FILLER         + 1];   /* FILLER              */
    char trd_comp_len       [LEN_DFI0030_TRD_COMP_LEN   + 1];   /* 거래및 결제 대사자료길이  */
    char trd_comp_data      [LEN_DFI0030_TRD_COMP_DATA  + 1];   /* 거래및 결제 대사자료DATA */
    char corr_id            [LEN_DFI0030_CORR_ID        + 1];   /* corr_id             */
    char host_tx_code       [LEN_DFI0030_TX_CODE        + 1];   /* KTI TX_CODE         */ 
    char acct_tran_code     [LEN_DFI0030_ACCT_TRAN_CODE + 1];   /* 계좌송금/수취인송금 코드 */
    char rcv_acct_no        [LEN_DFI0030_ACCT_NO        + 1];   /* 수취계좌번호           */
    char io_flag; 
    char kti_flag;          /* 시스템구분 - 0:GCG 1:ICG       */
    int  ext_recv_len;
    char ext_recv_data      [11000 + 1];
    char tmp_tx_code        [LEN_DFI0030_TX_CODE        + 1];   /* temp TX_CODE         */ 
    char kftc_tx_code       [LEN_DFI0030_TX_CODE        + 1];
    int  host_err_send;
    int  csf_flag;
    int  len;
};

/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int a000_data_receive(dfn0030_ctx_t *ctx, commbuff_t  *commbuff);
static int b100_init_proc(dfn0030_ctx_t *ctx);
static int b200_routing_proc(dfn0030_ctx_t *ctx);
static int c000_tran_code_conv(dfn0030_ctx_t *ctx);
static int d000_get_corr_id(dfn0030_ctx_t *ctx);
static int e000_msgdf_insert(dfn0030_ctx_t *ctx);
static int f000_host_msg_send(dfn0030_ctx_t *ctx);
static int y000_csf_data_insert(dfn0030_ctx_t *ctx);
static int z100_log_insert(dfn0030_ctx_t *ctx, char *log_data, int size, char io_flag, char sr_flag);
/* ----------------------------------------------------------------------------------------------------------- */
/* host receive 부터 host(sna)send 전 까지 업무 처리                                                                */
/* ----------------------------------------------------------------------------------------------------------- */
int dfn0030(commbuff_t  *commbuff)
{
    int                 rc = ERR_NONE;
    dfn0030_ctx_t   _ctx;
    dfn0030_ctx_t   *ctx = &_ctx;

    SYS_TRSF;
    /* 데이터 수신              */
    SYS_TRY(a000_data_receive(ctx, commbuff));
    /* 초기화 처리              */
    SYS_TRY(b100_init_proc(ctx));
    /* 대외기관수신 거래코드 -> host 거래코드 변환       */
    SYS_TRY(c000_tran_code_conv(ctx));
    /* corr_id 채번            */
    SYS_TRY(d000_get_corr_id(ctx));
    /* EXMQMSG_DF INSERT      */
    SYS_TRY(e000_msgdf_insert(ctx));
    /* HOST 전문 전송           */
    SYS_TRY(f000_host_msg_send(ctx));

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    SYS_DBG("SYS_CATCH");

    SYS_TREF;
    return ERR_ERR;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int a000_data_receive(dfn0030_ctx_t *ctx, commbuff_t  *commbuff)
{
    int                 rc  = ERR_NONE;

    SYS_TRSF;
    /* set commbuff      */
    memset((char *)ctx, 0x00, sizeof(dfn0030_ctx_t));
    ctx->cb = commbuff;

    SYS_ASSERT(EXTRECVDATA);

    /* input channel clear */
    SYSICOMM->intl_tx_flag = 0;
    memset(SYSICOMM->call_svc_name, 0, sizeof(SYSICOMM->call_svc_name));
    memcpy(SYSICOMM->call_svc_name, "DFN0030", 7);

    /* 11000메세지          */
    ctx->exmqmsg11000 = &ctx->_exmsg11000;

    SYS_DBG("EXTRECVDATA[%d][%.*s]", sysocbgs(ctx->cb, IDX_EXTRECVDATA), sysocbgs(ctx->cb, IDX_EXTRECVDATA), EXTRECVDATA);

    SYS_TREF;

    return ERR_NONE;    
}
/* ----------------------------------------------------------------------------------------------------------- */
static int b100_init_proc(dfn0030_ctx_t *ctx)
{
    int                 rc  = ERR_NONE;
    char                temp_str[LEN_EXMSG11000_DETL_REC_SIZE + 1];
    char                cati_target[1+1];
    char                msg_type[2];

    exmqmsg11000_t      exmqmsg11000;

    SYS_TRSF;
    /* 전문길이 set       */
    memset(temp_str, 0x00, LEN_EXMSG11000_DETL_REC_SIZE + 1);
    utol2an( (int)sysocbgs(ctx->cb, IDX_EXTRECVDATA), LEN_EXMSG11000_DETL_REC_SIZE, temp_str);

    memcpy(EXMSG11000->detl_rec_size    , temp_str    , LEN_EXMSG11000_DETL_REC_SIZE);
    memcpy(&EXMSG11000->detl_area       , EXTRECVDATA , sysocbgs(ctx->cb, IDX_EXTRECVDATA));
    ctx->ext_recv_len = sysocbgs(ctx->cb, IDX_EXTRECVDATA);

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int c000_tran_code_conv(dfn0030_ctx_t *ctx)
{
    int                 rc  = ERR_NONE;
    dfi0003f_t          dfi0003f;
    exi0251x_t          exi0251x;

    SYS_TRSF;
    memset(&exi0251x,   0x00, sizeof(exi0251x_t));

    exi0251x.in.tx_flag[0] = '7';
    exi0251x.in_conv_flag  = 'K';
    memcpy(exi0251x.in.appl_code   , FCY_CODE      , LEN_APPL_CODE);
    memcpy(exi0251x.in.msg_type    , ctx->msg_type , LEN_MSG_TYPE );
    memcpy(exi0251x.in.kftc_tx_code, ctx->proc_code, LEN_KFTC_TX_CODE);
    exi0251x.in.msg_type_len      = 4;
    exi0251x.in.kftc_tx_code_len  = 6;
    exi0251x.in.ext_recv_data     = EXTRECVDATA;    /* 대외수신데이터  set   */

    rc = ex_tran_code_convrt(&exi0251x);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_tran_code_conv(): "
                             "거래코드 변환 ERROR : tx_code[%s] kftc_tx_code[%s]"
                             "code/msgp[%d][%s]",
                             __FILE__, ctx->msg_type, ctx->kftc_tx_code,
                             sys_error_code(), sys_error_msg());
        return ERR_ERR;
    }
    /* 거래파라미터 LOAD이후 오류 발생시 HOST로 오류 문자 전송      */
    ctx->host_err_send = 1;
    memcpy(ctx->host_tx_code,   exi0251x.out.tx_code, LEN_TX_CODE);
    SYS_DBG("c000_tran_code_conv :tx_code[%s]tx_name[%s]", exi0251x.out.tx_code, exi0251x.out.tx_name);

    /* ------------------------------------------------------ */
    SYS_DBG("[c000]host_tx_code[%s]", ctx->host_tx_code);
    /* ------------------------------------------------------ */
    SYS_TREF;

    return ERR_NONE;
}
/* ----------------------------------------------------------------------------------------------------------- */
static int d000_get_corr_id(dfn0030_ctx_t *ctx)
{
    int                 rc  = ERR_NONE;
    char                corr_id[LEN_HCMIHEAD_QUEUE_NAME + 1], buff[LEN_HCMIHEAD_DATA_LEN + 1];
    hcmihead_t          hcmihead;

    SYS_TRSF;

    /* kti 전송용 corr_id 생성      */
    memset(corr_id, 0x00, sizeof(corr_id));

    utoick(corr_id);

    SYS_DBG("corr_id[%s]", corr_id);
    memcpy(ctx->corr_id, corr_id, LEN_HCMIHEAD_QUEUE_NAME);

    /* init hcmihead    */
    /* MQ에서 TCPHEAD = NULL 방지하기 위해 공백을 초기화  */
    memset(&hcmihead, 0x20, sizeof(hcmihead));

    hcmihead.comm_type = 'A';       /* COMMUNICATION 구분 : Sync, Async     */
    memcpy(hcmihead.queue_name,           corr_id , LEN_HCMIHEAD_QUEUE_NAME);
    memcpy(hcmihead.tx_code   , ctx->host_tx_code , LEN_EXMSG11000_TX_CODE );

#ifdef _DEBUGS
    PRINT_HCMIHEAD(&hcmihead);
#endif

    /* set tcp_head     */
    rc = sysocbsi(ctx->cb, IDX_TCPHEAD, &hcmihead, sizeof(hcmihead_t));

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int e000_msgdf_insert(dfn0030_ctx_t *ctx)
{
    int                 rc  = ERR_NONE;
    char                buff[LEN_SIZE+1];
    exmqmsg_df_t        exmqmsg_df;
    dfi0003f_t          dfi0003f;

    SYS_TRSF;

    /* init  */
    memset(&exmqmsg_df, 0x00, sizeof(exmqmsg_df_t));

    utodate1(exmqmsg_df.proc_date);
    utotime1(exmqmsg_df.proc_time);
    utotime2(exmqmsg_df.sys_mtime);
    utoclck(exmqmsg_df.ilog_jrn_no);

    memcpy(exmqmsg_df.chnl_code , "007", LEN_EXMQMSG_DF_CHNL_CODE);
    memcpy(exmqmsg_df.appl_code , "06" , LEN_EXMQMSG_DF_APPL_CODE); //EXMQPARM의 APPL_CODE 05[KTI->EI]06[EI->KTI]

    exmqmsg_df.io_type = 3;     /* default :3 */
    memcpy(exmqmsg_df.msg_id  , ctx->corr_id, LEN_EXMQMSG_DF_CORR_ID);
    memcpy(exmqmsg_df.corr_id , ctx->corr_id, LEN_EXMQMSG_DF_CORR_ID);

    utodate1(exmqmsg_df.mq_date);
    utotime1(exmqmsg_df.mq_time);

    exmqmsg_df.proc_type = 0; // insert초기값 0 mq put후에 9로 업데이트 
    memcpy(buff, &EXTRECVDATA[4], 5);
    ctx->len = atoi(buff);

    exmqmsg_df.mqlen = ctx->len + LEN_DFI0030_HDR_DATA; /* swift헤더 9자리 THDR12345        */
    memcpy(exmqmsg_df.mqmsg, EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA));

    /* ------------------------------------------------------ */
    PRINT_EXMQMSG_DF(&exmqmsg_df);
    /* ------------------------------------------------------ */

    /* ------------------------------------------------------ */
    /* EXMQMSG_DF INSERT                                      */
    /* ------------------------------------------------------ */
    EXEC SQL INSERT INTO EXMQMSG_DF (
                        PROC_DATE                 /* [8] 처리일자    */
                      , CHNL_CODE                 /* [3] 채널코드    */
                      , APPL_CODE                 /* [2] APPL_CODE */
                      , IO_TYPE                   /* [1] IO_TYPE   */
                      , MSG_ID                    /* [24]메세지ID    */
                      , CORR_ID                   /* [20]CORR ID   */
                      , PROC_TIME                 /*  [6]처리시간    */
                      , RSPN_FLAG                 /*  [1]처리결과    */
                      , ILOG_JRN_NO               /* [27]ILOGJRNNO */
                      , SYS_MTIME                 /*  [6]SYS시간    */
                      , PROC_TYPE                 /*  [1]처리구분    */
                      , ERR_CODE                  /*  [7]에러코드    */
                      , ERR_MSG                   /*[100]에러메세지   */
                      , MQ_DATE                   /* [8] MQ일자     */
                      , MQ_TIME                   /* [8] MQ시간     */
                      , MQLEN                     /* [5] MQ길이     */
                      , MQMSG                     /* CLOB:MQ메세지   */
                    )VALUES (
                        :exmqmsg_df.proc_date
                       ,:exmqmsg_df.chnl_code
                       ,:exmqmsg_df.appl_code
                       ,:exmqmsg_df.io_type 
                       ,:exmqmsg_df.msg_id
                       ,:exmqmsg_df.corr_id
                       ,:exmqmsg_df.proc_time
                       ,:exmqmsg_df.rspn_flag 
                       ,:exmqmsg_df.ilog_jrn_no
                       ,:exmqmsg_df.sys_mtime
                       ,:exmqmsg_df.proc_type
                       ,:exmqmsg_df.err_code
                       ,:exmqmsg_df.err_msg
                       ,:exmqmsg_df.mq_date
                       ,:exmqmsg_df.mq_time
                       ,:exmqmsg_df.mqlen
                       ,:exmqmsg_df.mqmsg
                    );
    if (SYS_DB_CHK_SUCCESS){
        return ERR_NONE;
    }
    else if (SYS_DB_CHK_DUPKEY){
        return ERR_NONE;
    }else{
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s EXMQMSG_DF(): INSERT [%d]"
                             "해결방안 : 외화자금이체 담당자   call:msg[%s]",
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "EXMQMSG_DF INSERT ERROR");
        return ERR_ERR;
    }

    SYS_TREF;

SYS_CATCH:

    return rc;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int f000_host_msg_send(dfn0030_ctx_t *ctx)
{
    int                 rc  = ERR_NONE;
    char                log_data[5000+1];
    dfi0030x_t          dfi0030x;

    SYS_TRSF;

    /* kftc 보내는 응답 메세지       */
    memset(&dfi0030x, 0x20, sizeof(dfi0030x_t));

    memcpy(dfi0030x.hdr_data        ,&EXTRECVDATA[0]   , LEN_DFI0030_HDR_DATA   );
    memcpy(dfi0030x.system_id       ,&EXTRECVDATA[9]   , LEN_DFI0030_SYSTEM_ID  );
    memcpy(dfi0030x.msg_type        , "0210"           , LEN_DFI0030_MSG_TYPE   );
    memcpy(dfi0030x.proc_code       , ctx->proc_code   , LEN_DFI0030_PROC_CODE  );
    memcpy(dfi0030x.io_flag         , "2"              , LEN_DFI0030_IO_FLAG    );
    memcpy(dfi0030x.status_code     , ctx->status_code , LEN_DFI0030_STATUS_CODE);
    memcpy(dfi0030x.rspn_code       , "000"            , LEN_DFI0030_RSPN_CODE  );
    memcpy(dfi0030x.rspn_bank_code  , "027"            , LEN_DFI0030_RSPN_BANK_CODE);
    utodate1(dfi0030x.proc_date);
    utotime1(dfi0030x.proc_time);
    memcpy(dfi0030x.trace_no        , ctx->trace_no    , LEN_DFI0030_TRACE_NO   );
    memcpy(dfi0030x.recv_filler     , "             "  , LEN_DFI0030_RECV_FILLER);  /* 예비정보 필드 10자리     */
    memcpy(dfi0030x.msg_no          , ctx->msg_no      , LEN_DFI0030_MSG_NO     );
    memcpy(dfi0030x.trd_data_nm     , ctx->trd_data_nm , LEN_DFI0030_TRD_DATA_NM);
    memcpy(dfi0030x.div_tot_cnt     , ctx->div_tot_cnt , LEN_DFI0030_DIV_TOT_CNT);
    memcpy(dfi0030x.div_sr_num      , ctx->div_sr_num  , LEN_DFI0030_DIV_SR_NUM );
    ctx->ext_recv_len = ctx->len + LEN_DFI0030_HDR_DATA;

    memcpy(&EXMSG11000->detl_area, &dfi0030x , ctx->ext_recv_len);

    SYS_DBG("EXTSENDDATA[%d][%.*s]", ctx->ext_recv_len, ctx->ext_recv_len, &EXMSG11000->detl_area);
    rc = sysocbsi(ctx->cb, IDX_EXTSENDDATA, &EXMSG11000->detl_area, ctx->ext_recv_len);
    if (rc == ERR_ERR){
        SYS_HSTERR(SYS_LN, SYS_GENERR, "COMMBUFF SET ERROR");
        return ERR_ERR;
    }

    /* 대외기관 G/W에 호출하는 방식을 전달하기 위한 값 set */
    strcpy(SYSGWINFO->func_name , "TCPDFC");                /* 대외기관 G/W[server]TCPDFS  [client]TCPDFC   */
    SYSGWINFO->time_val     = SYSGWINFO_SAF_DFLT_TIMEOUT;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_SAF;      /* SAF 방식 호출        */
    SYSGWINFO->rspn_flag    = SYSGWINFO_REPLY;              /* G/W 로 부터의 응답    */
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_1500;           /* 1200전문외에는 etc전문구분 0=1200 전문 1:etc 기타 전문  */

    /* 대외기관 데이터 전송     */
    rc = sys_tpcall("SYSEXTGW_DF", ctx->cb, TPNOTRAN);
    if (rc == ERR_ERR) {
        SYS_HSTERR(SYS_LN, 0265200. "GW SYEXTGW_DF TPCALL ERROR");
        ex_syslog(LOG_ERROR, "[APPL_DF] %s dfn0030: m000_ext_msg_send() ERROR %d"
                             "해결방안 대외기관 G/W담당자 call ", __FILE__, tperrno );
        return ERR_ERR;
    }

    rc = z100_log_insert(ctx, (char *)EXTRECVDATA, sysocbgs(ctx->cb, IDX_EXTRECVDATA), 'I', '4');       /* 개설응답 io_flag: I[개설거래]sr_flag:4[EI->KFTC]  */
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s dfn0030: DFLOG ERROR() [UNIX->HOST]", __FILE__);
    }

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    return ERR_ERR;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int z100_log_insert(dfn0030_ctx_t *ctx, char *log_data, int size, char io_flag, char sr_flag)
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
        SYS_DBG("TCP HEAD LOG I")
    }

}

/* ----------------------------------------------------------------------------------------------------------- */
/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */