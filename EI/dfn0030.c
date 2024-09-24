
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
        ex_syslog
    }
}
/* ----------------------------------------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------------------------------------- */