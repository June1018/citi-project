
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

        ctx->
    }
