
/*  @file               ign0000.c                                             
*   @file_type          pc source program
*   @brief              인터넷 뱅킹지로 IG취급 요청메인 
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
*   @generated at 2023/03/06 09:30
*   @history
*
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*
*
*/

/*  @pcode
 *   user_main() main process  
 *        a000 : CONTEXT 초기화 및 Commbuff검증
 *        b000 : HOST  수신전문 logging
 *        c000 : IGJRN TABLE Insert
 *        d000 : Host 수신전문 field 검증 
 *        e000 : 통신망 검증 
 *        f000 : 거래고유번호 채번 
 *        g000 : 대외기관전문 전송, et1500, table insert 
 *
 */


/* -------------------------- include files  -------------------------------------- */
#include <syscom.h>
#include <sysconst.h>
#include <utodate.h>
#include <exdefine.h>
#include <exmsg1500.h>
#include <exparam.h>
#include <hcmihead.h>
#include <igmsgcomm.h>
#include <igi3100f.h>
#include <iglog.h>
#include <sqlca.h>
#include <igjrn.h>
#include <igi0002f.h>

/* --------------------------constant, macro definitions -------------------------- */
#define EXMSG1500       (ctx->exmsg1500)
/* --------------------------structure definitions -------------------------------- */
typedef struct ign0000_ctx_s    ign0000_ctx_t;
struct ign0000_ctx_s {
    commbuff_t  *cb;  

    exmsg1500_t _exmsg1500;
    exmsg1500_t *exmsg1500;

    int                 recv_len;               /* 수신길이         */
    int                 send_len;               /* 대외기관 전송길이  */
    char                io_flag[1 + 1]; 
    char                sr_flag[1 | 1]; 
    char                msgcd           [LEN_IGMSGCOMM_MSGCD            + 1];
    char                trace_no        [LEN_IGMSGCOMM_BANK_TRACE_NO    + 1];
    char                proceed         [LEN_IGMSGCOMM_PROCCD           + 1];
    char                msglength       [LEN_IGMSGCOMM_MSGLENGTH        + 1];
    char                kti_flag;       /* KTI 거래여부 (1:KTI 거래 0:CORE 거래 ) */
    char                corr_id         [LEN_IGJRN_CORR_ID              + 1];
    char                respecd         [LEN_IGMSGCOMM_RESPCD           + 1];
    char                proc_time       [LEN_IGJRN_PROC_TIME            + 1];
};



/* --------------exported global variables declarations ---------------------------*/
/* ----------------------- exported function declarations ------------------------- */
static int      a000_data_receive(ign0000_ctx_t    *ctx, commbuff_t *commbuff); 
static int      b000_init_proc(ign0000_ctx_t  *ctx);
static int      c000_jrn_insert(ign0000_ctx_t   *ctx);
static int      d000_ext_msg_send(ign0000_ctx_t    *ctx);
static int      z000_error_proc(ign0000_ctx_t   *ctx);
static int      z100_log_insert(ign0000_ctx_t  *ctx, int host_sta_flag);


/* -------------------------------------------------------------------------------- */
/* host recevie 부터 host(sna) send 전 까지 업무처리                                     */
/* -------------------------------------------------------------------------------- */
int ign0000(commbuff_t  *commbuff)
{
    int                 rc = ERR_NONE;
    ign0000_ctx_t   _ctx; 
    ign0000_ctx_t   *ctx = &_ctx;  

    SYS_TRSF;

    /* CONTEXT 초기화  */
    SYS_TRY(a000_data_receive(ctx, commbuff));

    /* 초기화 처리 */
    SYS_TRY(b000_init_proc(ctx));

    /* IGJRN INSERT */
    SYS_TRY(c000_jrn_insert(ctx));

    /* 대외기관 전문 전송 */
    SYS_TRY(d000_ext_msg_send(ctx));

    SYSGWINFO->gw_rspn_send = SYSGWINFO_GW_REPLY_SEND_NO;       /* host에서 응답 전송안함  */

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    switch(rc){
        case GOB_ERR;
        SYS_TREF;

        /* host에 무응답 */
        SYSGWINFO->gw_rspn_send = SYSGWINFO_GW_REPLY_SEND_NO;
        return ERR_ERR;
    default:

        z000_error_proc(Ctx);
        break;

    }

    SYS_TREF;
    return ERR_ERR;


}

/* -------------------------------------------------------------------------------- */
static int a000_data_receive(ign0000_ctx_t  *ctx, commbuff_t    *commbuff)
{
    int                         rc = ERR_NONE;
    igmsgcomm_minustran_t       *igmsgcomm;
    hcmihead_t          hcmihead;
    char                *hp;

    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(ign0000_ctx_t));
    ctx->cb  =  commbuff;

    /* 입력 channel clear */
    SYSICOMM->intl_tx_flag = 0;

    ctx->exmsg1500  =  &ctx->_exmsg1500;

    igmsgcomm = (igmsgcomm_minustran_t *)&HOSTRECVDATA[300];
    SYS_DBG("Receive data [%s]", HOSTRECVDATA);

    EXMSG1500 = (exmsg1500_t *)HOSTRECVDATA;

    PRINT_EXMSG1500(EXMSG1500);

#ifndef _SIT_DBG
    PRINT_IGMSGCOMM_MINUSTRAN(igmsgcomm);
#endif

    memcpy(ctx->msgcd,      igmsgcomm->msgcd,           LEN_IGMSGCOMM_MSGCD);
    memcpy(ctx->trace_no,   igmsgcomm->bank_trace_no,   LEN_IGMSGCOMM_BANK_TRACE_NO);
    memcpy(ctx->proccd,     igmsgcomm->proccd,          LEN_IGMSGCOMM_PROCCD);
    memcpy(ctx->msglength,  igmsgcomm->msglength,       LEN_IGMSGCOMM_MSGLENGTH);
    memcpy(ctx->respecd,    igmsgcomm->respecd,         LEN_IGMSGCOMM_RESPCD);

    utodate1(EXMSG1500->tx_date);
    utotime2(ctx->proc_time);

    /* KTI 여부(0:코어, 2:KTI)  */
    if (SYSGWINFO->sys_type == 2) {
        ctx->kti_flag   = '1';

        /* TCP/IP 통신 헤더 정보 */
        hp = sysocbgp(ctx->cb, IDX_TCPHEAD);
        if (hp != NULL){
            memset(&hcmihead,   0x00, sizeof(hcmihead_t));
            memcpy(&hcmihead,   hp,  sysocbgs(ctx->cb, IDX_TCPHEAD));

            memcpy(ctx->corr_id,    hcmihead.queue_name,    LEN_HCMIHEAD_QUEUE_NAME);
            SYS_DBG("corr_id    : [%s]",    ctx->corr_id);
        }else{
            SYS_DBG("a000_data_receive  TCPHEAD is null");
            return GOB_ERR;
        }
    } else{
        ctx->kti_flag = 0;
    }

    SYS_DBG("ctx->msglength=[%s]ctx->msgcd[%s]ctx->trace_no[%s]ctx->proccd[%s]", ctx->msglength, ctx->msgcd, ctx->trace_no, ctx->proccd);

    SYS_TREF;
    return ERR_NONE;
}

/* -------------------------------------------------------------------------------- */
static int b000_init_proc(ign0000_ctx_t *ctx)
{
    int                 rc = ERR_NONE;
    SYS_TRSF;

    SYS_TREF;
    return ERR_NONE;

}
/* -------------------------------------------------------------------------------- */
static int c000_jrn_insert(ign0000_ctx_t    *ctx)
{

    int                 rc = ERR_NONE;
    ign0002f_t          igi0002f;
    
    SYS_TRSF;

    SYS_DBG("#1");
    memset(&ign0002f,   0x00, sizeof(igi0002f_t));

    SYS_DBG("#2");
    igi0002f.in.exmsg1500   = EXMSG1500;

    SYS_DBG("#3");
    memcpy(igi0002f.in.msg_no,     ctx->trace_no,   LEN_IGI0002F_MSG_NO);
    memcpy(igi0002f.in.respcd,     ctx->respcd,     LEN_IGI0002F_RESPCD);
    memcpy(igi0002f.in.corr_id,    ctx->corr_id,    LEN_IGI0002F_CORR_ID);
    memcpy(igi0002f.in.proc_time,  ctx->proc_time,  LEN_IGI0002F_PROC_TIME);
    igi0002f.in.kti_flag[0] = ctx->kti_flag;

    SYS_DBG("jrn_no ================== msg_no:[%s][%s]", igi0002f.in.msg_no, igi0002f.in.corr_id);

    rc = ig_jrn_insupd(&igi0002f);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s: c000_jrn_insert() : ig_jrn_ins ERROR", __FILE__);
        return ERR_ERR;
    }

    SYS_TREF;
    return ERR_NONE;

}

/* -------------------------------------------------------------------------------- */
static int d000_ext_msg_send(ign0000_ctx_t  *ctx)
/* -------------------------------------------------------------------------------- */
