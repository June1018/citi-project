
/*  @file               ixn0020.pc
*   @file_type          pc source program
*   @brief              취급응답 main
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
*
*
*
*
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
*
*/














/*  @pcode
 *  main.
 *   commbuff interface layout이 input임 
 *        a000 : CONTEXT 초기화
 *        b000 : 결제원 수신전문 로깅  
 *        c000 : 전문유형별 field 체크 
 *        d000 : 거래구분코드 변환 
 *        e000 : 거래파라미터 조회 
 *        f000 : 대외전문을 1200전문으로  전환 
 *        g000 : 거래구분코드 검증및 결번 응답 처리 
 *        h000 : 대외기관 결번 검증 
 *        j000 : 중복거래 검증 
 *        k000 : 최소 거래시 원저널 검증 
 *        l000 : 요구에 대한 응답전문 검증 
 *        m000 : 응답코드 검증
 *        n000 : 원전문을 조립 
 *        o000 : 취소거래시 원저널 update 
 *        p000 : 저널 update
 *        q000 : 저널 생성 
 *        r000 : 집계 반영 
 *        r500 : HOST 전송전 commit
 *        s000 : HOST전송 
 *        t000 : 호스트 SAF처리 
 *        x000 : 순채무한도 에러 서비스 호출 
 *        z000 : 대외기관 에러응답 송신 
 *
 *        a000 CONTEXT 초기화 
 *             1.context 초기화 
 *             2.대외기관 수신전문 NULL검증 
 *             3.대외기관 수신전문 ext_recv_data에 copy
 *             4.대외기관 에러 응답 전송및 에러 응답 조립여부 FLAG초기화 
 *                 
 *        b000 결제원 수신전문 로깅 
 *             1. IX로그테이블 (IXKULOG)에 로그 처리 	
 *        
 *        c000 전문유형별 field 체크 
 *             1. 대외기관 수신전문 필드 검증 
 *             2. 필드 검증 오류시 
 *                - 대외기관 포맷오류 전문 송신처리 
 *        
 *        d000 거래구분 코드 변환 
 *             1. 대외기관 종별코드, 거래구분코드를 호스트 거래코드 10자리로 변환 
 *             2. 자기앞 수표 지급 거래는 전문내의 proc_code값에 따라 재요청과 취소응답 구분 
 *             3. 거래구분코드 변환 에러시 대외기관 에러 응답 송신 
 *
 *        e000 거래파라미터 조회 
 *             1. 거래 파라미터 조회 
 *             2. 거래 파라미터 조회 에러시 대외기관 에러 응답 송신 
 *     
 *        f000 대외전문을 1200전문으로  변환
 *             1. 타점권 재연장 거래나 관리전문 응답 수신시 무시 (GOB_NRM 리턴) 
 *             2. 호스트 원전문 조립 거래가 아니거나 결번 응답시 리턴 처리 
 *             3. 대외가관 전문을 1200전문으로 변환
 *             4. 응답코드를 대외기관 응답코드를 내부 에러코드로 변환 및 1200전문에 set
 *             5. 대외전문을 1200전문으로 변환 에러시 대외기관 에러 응답 송신 
 *
 *        g000 거래구분코드 검증 및 결번 응답 처리 
 *             1. 거래코드별 내부 거래 구분 flag 값 set 
 *             2. 호스트 원전문 조립 거래가 아니거나 결번 응답시 리턴처리 
 *             3. 수표조회 거래 대외 기관 에러코드 변환 
 *                - 에러시 대외기관 에러 응답송신 
 *             4. 결번 응답 거래시 IXKSKN에 해당 결번 DELETE
 *                - 에러시 대외기관 무응답 
 *      
 *        h000 대외기관 결번 검증 
 *             1. 대외기관 전문 관리번호로 결번 검증 
 *             2. 결번 처리후 commit 중복검증 flag set 
 *             3. 에러시 대외기관 무응답 
 *             
 *        j000 중복거래 검증        
 *             1. 중복거래 검증 거래가 아니면 리턴처리 
 *             2. 중복거래인 경우 대외기관 무응답 
 *             3. 기타 DB ERROR인 경우, 대외기관 에러응답 송신 
 *
 *        k000 최소거래시 원저널 검증 
 *             1. 취소거래가 아니면 리턴 처리 
 *             2. 원저널의 취소 상태를 전역변수에 저장 
 *             3. 원저널 검증 에러시 대외기관 무응답 
 *
 *        l000 요구에 대한 응답전문 검증 
 *             1. 원거래 검증 거래가 아니면 리턴 처리 
 *             2. 원저널 검증 에러시 대외기관 포맷에러 응답송신 
 *             3. 요청시 TCP/IP헤더 정보를 COMMBUFF에 set
 *        
 *        m000 응답코드 검증  
 *             1. rspn_cmp_code 파라미터 값에 따라 다르게 처리함 
 *             2. '1': m100_proc_rspn_chk()에서 처리 
 *             3. 수표조회, 처리결과 조회, 입금 취소 거래인 경우  
 *              -  IXJRN에서의 응답코드가 불능(TIME OVER)인 경우 무시 
 *             4. 입금 불능전문을 수신한 경우 불능 저널 생성 
 *             5. 입금 불능이나 기입금 '309'거래는 불능 저널을 생성.
 *             6. 취소거래인 경우 
 *               - 원저널이 취소되지 않았고, 기취소 에러이면 정상취소거래로 처리 
 *               - 취소거래의 응답코드가 정상이 아니면, 원저널 미반영, 집계 미반영 
 *        
 *        m100 입금거래 응답코드 검증
 *             1. 대외기관 응답코드가 정상인 경우 
 *               - 저널의 응답코드가 정상이면 무시 
 *               - 저널의 응답코드가 이중거래이거나 아직 응답전문을 수신하지 않으면 정상처리 
 *               - 이미 불능응답으로 전문을 수신한 경우 무시 
 *             2. 대외기관 응답코드가 이중거래 에러인 경우 
 *               - 저널의 응답코드가 정상이거나 불능이면 무시 
 *               - 저널의 미완료로 되어 있는경우 , 저널에 반영하고 집계 미반영 
 *             3. 입금불능전문을 수신한 경우   
 *               - 순채무한도 에러인 경우 관리전문 수동처리 서비스 호출하여 취급거래 발생하지 않게 함.
 *               - 저널의 응답코드가 미완료이면 집계 미반영 
 *               - 저널의 응답코드가 이중거래이이면 저널에 반영, 집계 미반영 
 *               - 이미 입급불능전문을 수신한 상태이면 무시 
 *
 *        n000 원전문 조립 
 *             1. 원전문 조립거래가 아니면 리턴 처리 
 *             2. 저널 데이터를 1200전문에 set
 *             3. 거래코드별 1200전문 조립 
 *
 *        o000 취소거래시 원저널 반영 
 *             1. 취소시 원저널 반영거래가 아니면 리턴 처리 
 *             2. 취소거래시 거래고유번호로 저널 상태를 취소 반영
 *             3. 취소거래시 원저널 반영 에러인 경우 호스트 에러 응답 송신 
 *
 *        p000 저널 반영 
 *             1. 원저널 반영 거래가 아니면 리턴 처리 
 *             2. 취급응답시 저널 반영 
 *             3. 대외기관 에러 코드 및 에러 메세지를 저널에 반영 
 *             4. 변환된 에러 메세지는 호스트로 송신하지 않음 
 *
 *        q000 저널 생성 
 *             1. 호스트 송신전  저널 생성 거래가 아니면 리턴 처리 
 *             2. 대외기관 에러코드 및 에러 메시지를 저널에 반영 
 *             3. 변환된 에러 메세지는 호스트로 송신하지 않음 
 *
 *        r000 집계 반영 
 *             1. 집계반영 거래가 아니면 리턴 처리 
 *             2. 집계반영 
 *
 *        r100 집계 반영  (IXTOT1)
 *             1. 집계반영 거래가 아니면 리턴 처리 
 *             2. 집계반영 
 *
 *        r500 HOST전송전 COMMIT
 *             1. 호스트 송신전 commit 거래만 COMMIT
 *
 *        s000 HOST전송
 *             1. 호스트 통신여부 검증 거래가 아니면 리턴 처리 
 *             2. 호스트 송신 전문 SET 
 *             3. 거래 파라미터에서 host로부터 응답을 받을지 여부 SET
 *             4. 호스트송신 
 *             5. 호스트송신 에러인 경우 SAF방식 거래는 HOST SAF처리함 
 *             6. 호스트로 부터 응답을 받는 거래인 경우, 응답 데이터 저장 
 *               - 호스트 응답 코드가 에러인 경우, HOST SAF처리함 
 *
 *        t000 호스트 SAF처리 
 *             1. HOST SAF거래가 아니면 리턴 처리 
 *             2. IXHSAF에 생성 
 *
 *        x000 순채무한도 에러 서비스 호출 
 *             1. 순채무한도 에러 서비스 호출 
 *             2. 에러 발생시에도 정상 리턴 처리  
 *
 *        z000 대외기관 에러 응답 송신 
 *             1. 대외기관 에러 응답 송신 거래가 아니면 리턴 처리 
 *             2. 대외기관 에러 응답 전문 조립 
 *             3. 대외기관 응답 송신 
 *
 *
 */
/* ---------------------------------------------- include files ----------------------------------------------- */
#include <syscom.h>
#include <exdefine.h>
#include <exmsg1200.h>
#include <exi0250x.h>
#include <exi0210x.h>
#include <exi0280x.h>
#include <exi4000x.h>
#include <exi0230x.h>
#include <ixdefine.h>
#include <ixdetlarea.h>
#include <ixmsgkftc.h>
#include <ixi0230f.h>
#include <ixi0140f.h>
#include <ixi0110x.h>
#include <ixi1100x.h>
#include <ixi1040x.h>
#include <ixi1020x.h>
#include <ixi0180x.h>
#include <ixi0130f.h>
#include <ixi0131f.h>  /* Decoupling */
#include <ixi0320f.h>  /* Decoupling */
#include <ixi0200x.h>
#include <ixi0220x.h>
#include <ixi0120f.h>
#include <ixi1060f.h>
#include <ixi0150f.h>
#include <ixi0151f.h>
#include <vbcomm.h>
#include <ixjrn.h>
#include <sqlca.h>

#define LEN_KTI_FLAG            1

/* ---------------------------------------- constant, macro definitions --------------------------------------- */
/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct ixn0020_ctx_s    ixn0020_ctx_t;
struct ixn0020_ctx_s {
    commbuff        *cb;  

    int             ext_recv_len;                       /* 결제원 응답데이터 길이 */
    int             kftc_reply;                         /* 결제원 에러 응답여부   */
    int             kftc_err_set;                       /* 결제원 에러 응답 전문조립 여부 */
                                                        /* KFTC_SEND_LOG 결제원송신전문 로깅 */
    long            tx_num;                             /* 거래구분코드         */
    char            err_msg[LEN_EXMSG1200_ERR_MSG +1];  /* 호스트 에러 메세지    */
    char            lmt_svc_name[8];                    /* 순채무 한도 에러 처리 서비스명 */
    char            orig_canc_type[1];                  /* 원 저널의 취소 상태   */
    char            host_tx_code[LEN_TX_CODE + 1];      /* 내부 거래코드        */
    char            ext_recv_data[2048];                /* 대외기관 요청 전문    */

    ixjrn_t         ixjrn;
    ixi0220x_t      ixi0220x;                           /* 결제원 전문 에러 응답 SET 코드 */
    exmsg1200_t     exmsg1200;

    char            kti_flag[LEN_KTI_FLAG + 1];
    char            out_msg_no[LEN_MQMSG1200_OUT_MSG_NO + 1];
    char            ei_msg_no[LEN_MQMSG1200_EI_MSG_NO + 1];
    char            acct_type[LEN_MQMSG1200_CR_ACCT_TYPE +1];   


};

/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int  a000_data_receive(ixn0020_ctx_t *ctx, commbuff_t    *commbuff);
static int  b000_msg_logging(ixn0020_ctx_t *ctx, int log_type);
static int  c000_ix_kftc_fild_val(ixn0020_ctx_t *ctx);
static int  d000_tran_code_conv(ixn0020_ctx_t *ctx);
static int  e000_exparm_read(ixn0020_ctx_t *ctx);
static int  f000_exmsg1200_make(ixn0020_ctx_t *ctx);
static int  g000_tx_code_check(ixn0020_ctx_t *ctx);
static int  h000_ix_skn_check(ixn0020_ctx_t *ctx);
static int  j000_ix_dup_check(ixn0020_ctx_t *ctx);
static int  k000_canc_orig_chk(ixn0020_ctx_t *ctx);
static int  l000_proc_rspn_chk(ixn0020_ctx_t *ctx);
static int  m000_rspn_code_check(ixn0020_ctx_t *ctx);
static int  m100_proc_rspn_chk(ixn0020_ctx_t *ctx);
static int  n000_ix_host_orig_msg_make(ixn0020_ctx_t *ctx);
static int  o000_ix_canc_orig_update(ixn0020_ctx_t *ctx);
static int  p000_ixjrn_update(ixn0020_ctx_t *ctx);
static int  q000_ixjrn_insert(ixn0020_ctx_t *ctx);
static int  r000_ix_tot_proc(ixn0020_ctx_t *ctx);
static int  r100_ix_tot_proc(ixn0020_ctx_t *ctx);
static int  r500_host_send_commit(ixn0020_ctx_t *ctx);
static int  s000_host_send(ixn0020_ctx_t *ctx);
static int  t000_ix_host_saf_prod(ixn0020_ctx_t *ctx);
static int  x000_msg_svc_call(ixn0020_ctx_t *ctx);
static int  z000_error_proc(ixn0020_ctx_t *ctx);


/* ------------------------------------------------------------------------------------------------------------ */
int ixn0020(commbuff_t  *commbuff)
{

    int                 rc = ERR_NONE;
    ixn0020_ctx_t       _ctx;  
    ixn0020_ctx_t       *ctx = &_ctx;   

    SYS_TRSF;

    /* CONTEXT 초기화  */
    SYS_TRY(a000_data_receive(ctx, commbuff));

    /* 결제원 수신 전문 LOGGING  */
    SYS_TRY(b000_msg_logging(ctx, KFTC_RECV_LOG));

    /* 전문유형별 field 체크   */
    SYS_TRY(c000_kftc_fild_chk(ctx));

    /* 거래구분코드 변환                    */  
    SYS_TRY(d000_tran_code_conv(ctx));

    /* 거래파라미터 조회                    */ 
    SYS_TRY(e000_exparm_read(ctx));

    /* 대외전문을 1200전문으로  전환          */ 
    SYS_TRY(f000_exmsg1200_make(ctx));

    /* 거래구분코드 검증및 결번 응답 처리        */
    SYS_TRY(g000_tx_code_check(ctx));

    /* 대외기관 결번 검증 */                       
    SYS_TRY(h000_ix_skn_check(ctx));

    /* 중복거래 검증                    */ 
    SYS_TRY(j000_ix_dup_check(ctx));

    /* 최소 거래시 원저널 검증 */                      
    SYS_TRY(k000_canc_orig_chk(ctx));

    /* 요구에 대한 응답전문 검증                    */ 
    SYS_TRY(l000_proc_rspn_chk(ctx));

    /* 응답코드 검증            */  
    SYS_TRY(m000_rspn_code_check(ctx));

    /* 원전문을 조립                    */ 
    SYS_TRY(n000_ix_host_orig_msg_make(ctx));

    /* 취소거래시 원저널 update */  
    SYS_TRY(o000_ix_canc_orig_update(ctx));

    /* 저널 update       */
    SYS_TRY(p000_ixjrn_update(ctx));

    /* 저널 생성                    */ 
    SYS_TRY(q000_ixjrn_insert(ctx));

    /* 집계 반영                    */ 
    SYS_TRY(r000_ix_tot_proc(ctx));

    /* 집계 반영  (IXTOT)           */ 
    SYS_TRY(r100_ix_tot_proc(ctx));

    /* HOST 전송전 commit           */ 
    SYS_TRY(r500_host_send_commit(ctx));

    /* HOST전송         */  
    SYS_TRY(s000_host_send(ctx));

    /* 호스트 SAF처리 */                       
    SYS_TRY(t000_ix_host_saf_prod(ctx));


    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    if (rc == GOB_NRM)
        return ERR_NONE;

    /* 결제원 에러 응답 처리   */
    z000_error_proc(ctx);
    if (rc == ERR_ERR){
        return ERR_ERR;
    }

    /* 결제원 송신전문 로깅   */
    b000_msg_logging(ctx, KFTC_SEND_LOG);

    SYS_TREF;
    return ERR_ERR;
}


/* ------------------------------------------------------------------------------------------------------------ */
static int  a000_data_receive(ixn0020_ctx_t *ctx, commbuff_t    *commbuff)
{
    int      rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(ixn0020_ctx_t));
    ctx->cb = commbuff;

    if (EXTRECVDATA == NULL) {
        SYS_HSTERR(SYS_NN, SYS_GENERR, "EXTRECVDATA DATA NOT FOUND");
        return GOB_NRM;
    }

    /* 결제원 응답 데이터 길이 */
    ctx->ext_recv_len = sysocbgs(ctx->cb, IDX_EXTRECVDATA);
    memcpy(ctx->ext_recv_data,  EXTRECVDATA, ctx->ext_recv_len);

    /* 결제원 에러 응답 전송 여부 초기화  */
    ctx->kftc_reply = 0;

    /* 결제원 에러 응답 조립여부 초기화 (0:조립하지 않음, 1:조립함.)   */
    ctx->kftc_err_set = 1;

    /* 입력 채널 clear */
    SYSICOMM->intl_tx_flag = 0;

    memset(ctx->err_msg, 0x20, LEN_EXMSG1200_ERR_MSG);

#ifdef _SIT_DBG
    PRINT_IX_KFTC_DEBUG(ctx->ext_recv_data);
#endif

    SYS_TREF;

    return ERR_NONE;
}


/* ------------------------------------------------------------------------------------------------------------ */
static int b000_msg_logging(ixn0020_ctx_t   *ctx, int log_type)
{
    int                 rc  = ERR_NONE;
    ixi0230f_t          ixi0230f;
    commbuff_t          dcb;


    SYS_TRSF;


    /* ---------------------------------------------------------------------- */
    SYS_DBG("b000_msg_logging: len = [%d]", ixi0230f.in.log_len);
    SYS_DBG("b000_msg_logging: msg = [%s]", ixi0230f.in.log_data);
    /* ---------------------------------------------------------------------- */

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
    /* 결제원 송신전문 로깅 */
    else{
        ixi0230f.in.flag            = 'S';
        ixi0230f.in.log_type        = 'K';
        ixi0230f.in.log_len         = ctx->ext_recv_len - 9;
        memcpy(ixi0230f.in.log_data,  EXTSENDDATA, ixi0230f.in.log_len );        
    }

    memset(&dcb,    0x00, sizeof(commbuff_t));
    rc = sysocbdb(ctx->cb,  &dcb);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s ixn0020: b000_msg_logging()"
                             "COMMBUFF BACKUP ERROR "
                             "[해결방안]시스템 담당자 call", 
                             __FILE__);
        sys_err_init();
        return ERR_NONE;
    }

     rc = sysocbsi(&dcb, IDX_EXMSG1200, ixi0230f, sizeof(ixi0230f));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s ixn0020: b000_msg_logging()"
                             "COMMBUFF BACKUP ERROR "
                             "[해결방안]시스템 담당자 call", 
                             __FILE__);
        sys_err_init();
        sysocbfb(&dcb);
        return ERR_NONE;
    }

     rc = sys_tpcall("IXN0230F", &dcb, TPNOREPLY | TPNOTRAN);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s ixn0020: b000_msg_logging()"
                             "IXI0230F 서비스호출 ERROR [%d:%d] "
                             "[해결방안]TMAX 담당자 call", 
                             __FILE__, tperrno, sys_err_code());
        sys_err_init();

    }

    sysocbfb(&dcb);

    SYS_TREF;
    return ERR_NONE;
    
}
/* ------------------------------------------------------------------------------------------------------------ */
static int c000_ix_kftc_fild_val(ixn0020_ctx_t *ctx)
{

    int                 rc = ERR_NONE;
    ixi0220x_t          ixi0220x;

    SYS_TRSF;

    memset(&ixi0200x, 0x00, sizeof(ixi0200x_t));
    ixi0200x.in.ext_recv_data = ctx->ext_recv_data;
    /* ------------------------------------------------------------------------- */
    /* 필드 validation  check                                                     */
    /* ------------------------------------------------------------------------- */
    SYS_TRY(ix_kftc_fild_val(&ixi0200x));

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    ctx->kftc_reply;
    ctx->kftc_err_set = 0;

    SYS_TREF;
    return ERR_ERR;
}
/* ------------------------------------------------------------------------------------------------------------ */
static int d000_tran_code_conv(ixn0020_ctx_t    *ctx)
{
    int                 rc  = ERR_NONE;
    exi0250x_t          exi0250x;

    SYS_TRSF;

    ix0200a = (ix0200a_t *) ctx->ext_recv_data;

    /*------------------------------------------------------------------------ */
    /* 거래구분 코드 변환                                                          */
    /*------------------------------------------------------------------------ */
    memset(&exi0250x, 0x00, sizeof(exi0250x_t));
    exi0250x.in.conv_flag       = k;  

    memcpy(exi0250x.in.appl_code,   IX_CODE,            LEN_APPL_CODE);
    memcpy(exi0250x.in.tx_flag  ,   ix0200a->send_flag, LEN_EXI0250X_TX_FLAG);
    memcpy(exi0250x.in.msg_type ,   ix0200a->msg_code,  LEN_MSG_TYPE);
    memcpy(exi0250x.in.kftc_tx_code,ix0200a->appl_code, 3);
    exi0250x.in.ext_recv_data   = ctx->ext_recv_data;

    SYS_TRY(ex_tran_code_conv(&exi0250x));

    if (utochksp(exi0250x.out.tx_code, LEN_TX_CODE) == SYS_TRUE)
        goto SYS_CATCH;

    memcpy(ctx->host_tx_code, exi0250x.out.tx_code, LEN_TX_CODE);

    /*------------------------------------------------------------------------ */
    /* 타행자기 앞수표 지급                                                         */
    /* proc_code = 02 : 타행자기앞 수표 지급재요청 응답 (취급)                          */
    /* proc_code = 03 : 타행자기앞 수표 지급취소응답 (창구, 취급)                       */
    /*------------------------------------------------------------------------ */
    /* proc_code 재정리                                                         */
    /* proc_code = 01 : 타행자기앞 수표 지급재요청응답 (취급)                          */
    /* proc_code = 02 : 타행자기앞 지급확인 조회 응답  (취급)                          */
    /* proc_code = 03 : 타행자기앞 지급취소 응답 (청구, 취급)                          */
    /*------------------------------------------------------------------------ */

    if (memcmp(ctx->host_tx_code, "3089", 4) == 0){
        if (memcmp(&(ctx->ext_recv_data[96]), "01", 2) == 0){
            memcpy(ctx->host_tx_code,  "3089000100", LEN_TX_CODE);

        }else if (memcmp(&(ctx->ext_recv_data[96]), "02", 2) == 0){
            memcpy(ctx->host_tx_code,  "3189000100", LEN_TX_CODE);

        }else if (memcmp(&(ctx->ext_recv_data[96]), "03", 2) == 0){
            memcpy(ctx->host_tx_code,  "3189000000", LEN_TX_CODE);
        }
    }

    /*------------------------------------------------------------------------ */
    SYS_DBG("d000_tran_code_conv: host_tx_code[%s]" , ctx->host_tx_code );
    /*------------------------------------------------------------------------ */

    SYS_TREF;

    return ERR_NONE;


SYS_CATCH:

    ex_syslog(LOG_ERROR, "[APPL_DM] % 거래코드 변환ERROR"
                         "msg_type/kftc_tx_code [%s%s]"
                         "[해결방안]업무담당자 call"),
                         __FILE__, exi0250x.in.msg_type, exi0250x.in.kftc_tx_code);

    ctx->ixi0220x.in.msg_code   = '1';
    ctx->ixi0220x.in.send_flag  = '2';
    memcpy(ctx->ixi0220x.in.rspn_code, "413", LEN_RSPN_CODE);
    ctx->kftc_reply = '1';

    SYS_TREF;
    return ERR_ERR;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int e000_exparm_read(ixn0020_ctx_t   *ctx)
{

    int                 rc  = ERR_NONE;
    exi0210x_t          exi0210x;

    SYS_TRSF;

    /*------------------------------------------------------------------------ */
    SYS_DSP(" e000_exparm_read: tx_code [%s]", ctx->host_tx_code);
    /*------------------------------------------------------------------------ */

    /*------------------------------------------------------------------------ */
    /* EXPARM TABLE LOAD                                                       */
    /*------------------------------------------------------------------------ */
    memset(&exi0210x,   0x00, sizeof(exi0210x_t));
    memcpy(exi0210x.in.appl_code, IX_CODE           , LEN_APPL_CODE);
    memcpy(exi0210x.in.tx_code  , ctx->host_tx_code , LEN_TX_CODE);

    rc = ex_parm_load(&exi0210x);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s e000_exparm_read(): 거래코드 Loading Error"
                             "host_tx_code/code/msg [%s:%d:%s]"
                             "[해결방안] 업무담당자 CALL",
                             __FILE__, ctx->host_tx_code, sys_err_code(), sys_err_msg());
        ctx->ixi0220x.in.msg_code   = '1';
        ctx->ixi0220x.in.send_flag  = '2';
        memcpy(ctx->ixi0220x.in.rspn_code, "413", LEN_RSPN_CODE);
        ctx->kftc_reply = 1;
        return ERR_ERR;
    }

    /* exparm을 commbuff에 저장 */
    rc = sysocbsi(ctx->cb, IDX_EXPARM, &exi0210x.out.exparm, sizeof(exparm_t));
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s e000_exparm_read():"
                             "COMMBUFF(EXPARM) SET ERROR host_tx_code[%s]"
                             "[해결방안] 업무담당자 CALL",
                             __FILE__, ctx->host_tx_code);
        ctx->ixi0220x.in.msg_code   = '1';
        ctx->ixi0220x.in.send_flag  = '2';
        memcpy(ctx->ixi0220x.in.rspn_code, "413", LEN_RSPN_CODE);
        ctx->kftc_reply = 1;
        return ERR_ERR;

    }

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int f000_exmsg1200_make(ixn0020_ctx_t    *ctx)
{
    int                 rc  = ERR_NONE;
    int                 err_code;
    ixi1040x_t          ixi1040x;
    exi4000x_t          exi4000x;
    exi0230x_t          exi0230x;
    exmsg1200_t         exmsg1200;
    exmsg1200_ix02_t    *ix02;

    SYS_TRSF;

    /*------------------------------------------------------------------------ */
    SYS_DBG(" f000_exmsg1200_make: host_tx_code  [%.6s]", ctx->tx_code);
    SYS_DBG(" f000_exmsg1200_make: host_orgi_msg_make [%s]", ctx->host_orgi_msg_make);
    /*------------------------------------------------------------------------ */

    /*------------------------------------------------------------------------ */
    /* 취급         타점권 재연장                                                  */
    /* 취급         관리전문 응답                                                  */
    /*------------------------------------------------------------------------ */
    /* 이 거래에  대해서는 HOST를 전송하지 않고 무시한다.                                */
    /* timer는 당행 전문관리번호를 관리하나 이 거래는 적용할 수 없으므로 이전문에             */
    /* 대해 응답전문을 수신할 경우 무시한다.                                           */
    /*------------------------------------------------------------------------ */
    if ((utoa2in(ctx->host_tx_code, 6) == 357000) ||
        (utoa2in(ctx->host_tx_code, 6) == 676100))
        return GOB_NRM;

    /*------------------------------------------------------------------------ */
    /* 취급 결번 응답 (357100)                                                    */
    /*------------------------------------------------------------------------ */
    if ((EXPARM->host_orgi_msg_make[0] != '1') ||
        (utoa2in(ctx->host_tx_code, 6) == 357000)
        return ERR_NONE;

    /*------------------------------------------------------------------------ */
    /* 1200전문 초기화                                                           */
    /*------------------------------------------------------------------------ */
    memset(&ixi1040x, 0x00, sizeof(ixi1040x_t));
    ixi1040x.in.exmsg1200   = &exmsg1200;
    ix_ex1200_init(&ixi1040x);

    memcpy(exmsg1200.tx_code,   ctx->host_tx_code,  LEN_TX_CODE);

    /*------------------------------------------------------------------------ */
    /* 대외전문을 1200byte 전문으로 FORMATING                                      */
    /*------------------------------------------------------------------------ */
    memset(&exi4000x,   0x00,   sizeof(exi4000x_t));
    exi4000x.in.type            = EXI4000X_REQ_MAPP;
    exi4000x.in.inp_conv_flag   = EXI4000X_NULL_CONV;
    exi4000x.in.out_conv_flag   = EXI4000X_NULL_CONV;
    exi4000x.in.base_len        = LEN_EXMSG1200;
    exi4000x.in.msg_len         = ctx->ext_recv_len;

    memcpy(exi4000x.in.base_msg,    &exmsg1200,         LEN_EXMSG1200);
    memcpy(exi4000x.in.msg,         ctx->ext_recv_data, exi4000x.in.msg_len);
    memcpy(exi4000x.in.appl_code,   IX_CODE,            LEN_APPL_CODE);
    memcpy(exi4000x.in.tx_code,     EXPARM->tx_code,    LEN_TX_CODE);    

    rc = ex_format(&exi4000x);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s f000_exmsg1200_make()"
                             "FORMAT ERROR: tx_code [%s]"
                             "[해결방안]IX 담당자 CALL",
                             __FILE__, exi4000x.tx_code);

        ctx->ixi0220x.in.msg_code   = '1';
        ctx->ixi0220x.in.send_flag  = '2';
        memcpy(ctx->ixi0220x.in.rspn_code, "413", LEN_RSPN_CODE);
        ctx->kftc_reply = 1;
        return ERR_ERR;

    }

    /* EXMSG1200을 commbuff에 저장  */
    rc = sysocbsi(ctx->cb, IDX_EXMSG1200,   &exmsg1200, LEN_EXMSG1200);
    if (exi4000x.out.msg_len <= 0){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s f000_exmsg1200_make()"
                             "FORMAT ERROR: tx_code [%s]"
                             "[해결방안]시스템 담당자 CALL",
                             __FILE__,  exi4000x.tx_code);

        ctx->ixi0220x.in.msg_code   = '1';
        ctx->ixi0220x.in.send_flag  = '2';
        memcpy(ctx->ixi0220x.in.rspn_code, "413", LEN_RSPN_CODE);
        ctx->kftc_reply = 1;
        return ERR_ERR;
    }

    /*------------------------------------------------------------------------ */
    SYS_DBG(" f000_exmsg1200_make: EXMSG1200->tx_code    [%.8s]", EXMSG1200->tx_code);
    SYS_DBG(" f000_exmsg1200_make: EXMSG1200->msg_no    [%.12s]", EXMSG1200->msg_no);
    SYS_DBG(" f000_exmsg1200_make: EXMSG1200->out_msg_no[%.10s]", EXMSG1200->out_msg_no);
    /*------------------------------------------------------------------------ */

    if (memcmp(&EXMSG1200->tx_code[2], "971020", 6) > 0){
        memcpy(EXMSG1200->tx_date,  "19", 2);
        memcpy(EXMSG1200->val_date, "19", 2);
    }
    else{
        memcpy(EXMSG1200->tx_date,  "20", 2);
        memcpy(EXMSG1200->val_date, "20", 2);
    }


    memcpy(EXMSG1200->tx_code,  EXPARM->tx_code,    LEN_EXMSG1200_TX_CODE);

    /* ------------------------------------------------------------------- */
    /* 수표조회거래인 경우 상세부                                                */
    /* ------------------------------------------------------------------- */
    if (memcmp(EXMSG1200->tx_code, "3561000000", LEN_EXMSG1200_TX_CODE) == 0) {
        ix02 = (exmsg1200_ix02_t *) EXMSG1200->detl_area;

        if (memcmp(ix02->chq_issu_date, "971020", 6) >= 0){
            memcpy(ix02->chq_issu_date, "19",  2);
        }
        else{
            memcpy(ix02->chq_issu_date, "20",  2);
        }
    }

    /* ------------------------------------------------------------------- */
    /* 응답코드를 대외기관 응답코드를 내부에러코드로 반환                              */
    /* ------------------------------------------------------------------- */
    memset(&exi0230x,   0x00, sizeof(exi0230x_t));
    exi0230x.in.conv_flag[0] = 'K';
    memcpy(exi0230x.in.appl_code,       IX_CODE,        LEN_APPL_CODE);
    memcpy(exi0230x.in.sub_appl_code,   SUB_IX_CODE,    LEN_EXI0230X_SUB_APPL_CODE);
    /* bank code length  */
    /* memcpy(exi0230x.in.rspn_code)*/
    memcpy(exi0230x.in.rspn_code,   &ctx->ext_recv_data[22],    LEN_EXI0230X_RSPN_CODE);

    rc = ex_err_conv(&exi0230x);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s f000_exmsg1200_make()"
                             "FORMAT ERROR: tx_code [%s] rspn_code[%s]"
                             "[해결방안]IX 담당자 CALL",
                             __FILE__, exi4000x.tx_code, exi0230x.in.rspn_code);

        ctx->ext_recv_data[IX_IDX_MSG_TYPE] = '9';
        ctx->ixi0220x.in.msg_code   = '1';
        ctx->ixi0220x.in.send_flag  = '2';
        memcpy(ctx->ixi0220x.in.rspn_code, "306", LEN_RSPN_CODE);
        ctx->kftc_reply = 1;
        return ERR_ERR;
    }

    /* error code */
    memcpy(EXMSG1200->err_code, exi0230x.our.err_code, LEN_EXI0230X_ERR_CODE);
    /* error msg */
    memcpy(EXMSG1200->err_msg,  exi0230x.our.err_name, LEN_EXI0230X_ERR_MSG );

    err_code = utoa2in(exi0230x.our.err_code, LEN_EXI0230X_ERR_CODE);
    SYS_HSTERR(SYS_NN, err_code, "");

    if (strlen(exi0230x.our.rspn_code) > 0)
        
        /* bankcode lenght */
        memcpy(&ctx->ext_recv_data[22], exi0230x.out.rspn_code, LEN_EXI0230X_RSPN_CODE);
    
    memcpy(ctx->err_msg,    exi0230x.out.err_name, LEN_EXI0230X_ERR_MSG);


    /* -------------------------------------------------------------------------- */
    /* IXJRN 읽어 ctx->kti_flag set 한다.                                           */
    /* -------------------------------------------------------------------------- */
    char                tmp_kti_flag [LEN_KTI_FLAG              + 1];
    char                tmp_tx_date  [LEN_EXMSG1200_TX_DATE     + 1];
    char                tmp_ei_msg_no[LEN_EXMSG1200_EI_MSG_NO   + 1];
    memset(tmp_kti_flag,    0x00, sizeof(tmp_kti_flag   ));
    memset(tmp_tx_date,     0x00, sizeof(tmp_tx_date    ));
    memset(tmp_ei_msg_no,   0x00, sizeof(tmp_ei_msg_no  ));

    memcpy(tmp_tx_date,     EXMSG1200->tx_date   , LEN_EXMSG1200_TX_DATE);
    memcpy(tmp_ei_msg_no,   EXMSG1200->old_msg_no, LEN_EXMSG1200_EI_MSG_NO  );

    EXEC SQL SELECT KTI_FLAG
               INTO :tmp_kti_flag
               FROM IXJRN
              WHERE TX_DATE    = :tmp_tx_date
                AND EI_MSG_NO  = :tmp_ei_msg_no
                AND SEND_FLAG  != '5';

    if (SYS_DB_CHK_FETCHFAIL){
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        ex_syslog(LOG_ERROR, "[APPL_DM] %s f000_exmsg1200_make() SELECT(IXJRN) [%d]"
                             "[해결방안] ORACLE 담당자 CALL MSG[%s], ei_msg_no[%s]",
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR, tmp_ei_msg_no);
        SYS_HSTERR(SYS_NN, SYS_GENERR, "IXJRN SELECT ERROR");
    }


    memcpy(ctx->kti_flag,   tmp_kti_flag, LEN_KTI_FLAG);
    /* ------------------------------------------------------------------------- */

#ifdef _SIT_DBG
    /* ------------------------------------------------------------------------- */
    PRINT_EXMSG1200(EXMSG1200);
    PRINT_EXMSG1200_2(EXMSG1200);
    /* ------------------------------------------------------------------------- */
#endif

    SYS_TREF;
    return ERR_NONE;


}

/* ------------------------------------------------------------------------------------------------------------ */
static int  g000_tx_code_check(ixn0020_ctx_t    *ctx)
{

    int                 rc  = ERR_NONE;
    int                 err_code;
    char                kftc_skip_no[12 + 1];
    exi0230x_t          exi0230x;
    ixmsg0200b_t        *ix0200b;
    ixmsg0800d_t        *ix0800d;
    exmsg1200_ix02_t    *ix02; 

    SYS_TRSF;
    /* ------------------------------------------------------------------------- */
    SYS_DBG("host_tx_code [%.6s]", ctx->host_tx_code);
    /* ------------------------------------------------------------------------- */

    /* ------------------------------------------------------------------------- */
    /* 거래코드로 거래구분                                                            */
    /* ------------------------------------------------------------------------- */
    switch(utoa2in(ctx->host_tx_code, 6)){
    /* ------------------------------------------------------------------------- */
    /* 취급입금거래인 경우                                                            */
    /* ------------------------------------------------------------------------- */
    case 308101:
    case 308102:
    case 308201:
    case 308202:
         ctx->tx_num = 1;
         break;

    /* ------------------------------------------------------------------------- */
    /* 취급수표조회인 경우                                                            */
    /* ------------------------------------------------------------------------- */
    case 356100:
         ctx->tx_num = 2;

         ix0200b = (ixmsg0200b_t *)ctx->ext_recv_data;
         ix02    = (exmsg1200_ix02_t *) EXMSG1200->detl_area;

        if ((memcmp(EXMSG1200->err_code, "0000000", 7) == 0) &&
             (memcmp(ix02->chq_sta_code1, "   "    , 3) != 0)){
            memset(ix02->chq_sta_code1, '0', 3);
        }
        
        /* --------------------------------------------------- */
        /* 수표상태코드가 스페이스가 아니면 에러 변환 함                 */
        /* --------------------------------------------------- */
        if ((memcmp(EXMSG1200->err_code, "0000000", 7) == 0) &&
            (memcmp(ix02->chq_sta_code1, "000"    , 3) !=)) {
        
            /* --------------------------------------------------- */
            /* 응답코드를 대외기관 응답코드를 내부 에러코드로 변환             */
            /* --------------------------------------------------- */
            memset(&exi0230x,   0x00,   sizeof(exi0230x_t));
            exi0230x.in.conv_flag[0] = 'K';
            memcpy(exi0230x.in.appl_code,       IX_CODE,            LEN_APPL_CODE);
            memcpy(exi0230x.in.sub_appl_code,  SUB_IX_CODE,         LEN_EXI0230X_SUB_APPL_CODE);
            memcpy(exi0230x.in.rspn_code    ,  ix02->chq_sta_code1, LEN_EXI0230X_RSPN_CODE);

            rc = ex_err_conv(&exi0230x);
            if (rc == ERR_ERR){
                ex_syslog(LOG_ERROR, "[APPL_DM] %s g000_tx_code_check()"
                                     "reason code conv error : chq_sta_code1[%.3s]"
                                    "[해결방안]IX 담당자 CALL",
                                    __FILE__, ix02->chq_sta_code1);

                ctx->ext_recv_data[IX_IDX_MSG_TYPE] = '9';
                ctx->ixi0220x.in.msg_code   = '1';
                ctx->ixi0220x.in.send_flag  = '2';
                memcpy(ctx->ixi0220x.in.rspn_code, "306", LEN_RSPN_CODE);
                ctx->kftc_reply = 1;
                return ERR_ERR;
            }

            memcpy(EXMSG1200->err_code, exi0230x.out.err_code,  LEN_EXI0230X_ERR_CODE);
            memcpy(ctx->err_msg,      , exi0230x.out.err_name,  LEN_EXI0230X_ERR_MSG );
            err_code = utoa2in(exi0230x.out.err_code,   LEN_EXI0230X_ERR_CODE);
            SYS_HSTERR(SYS_NN, "");
        }
        break;
    /* --------------------------------------------------- */
    /* 처리결과 조회인 경우                                     */
    /* --------------------------------------------------- */
    case 356200:
        ctx->tx_num = 3;
        break;
    /* --------------------------------------------------- */
    /* 취급현금입금취소 / 취급제휴입금취소/ 취급추심대전입금 취소       */
    /* --------------------------------------------------- */
    case 318100:
    case 318200:
        ctx->tx_num = 4;
        break;


    /* --------------------------------------------------- */
    /* 미결제 어음 통보                                       */
    /* ssu 부도 출금 tx_num 7,8 번은 검증없음                   */
    /* ixo0150f에도 in_flag = 0으로 세팅해서 jrn을 update함     */
    /* --------------------------------------------------- */
    case 318300:
    case 330700:
        ctx->tx_num = 5;
        break;

    /* --------------------------------------------------- */
    /* 타행 자기앞 수표 지급                                    */
    /* --------------------------------------------------- */
    case 308900:
    case 318900:
        ctx->tx_num = 6;
        break;

    /* --------------------------------------------------- */
    /* 부도어음 통보                                          */
    /* --------------------------------------------------- */
    case 330100:
    case 330101:
        ctx->tx_num = 7;
        break;

    /* --------------------------------------------------- */
    /* 수취조회인  경우                                        */
    /* --------------------------------------------------- */
    case 308700:
        ctx->tx_num = 8;
        break;

    /* --------------------------------------------------- */
    /* 기업구매자금 어음 통보추가                                */
    /* 기업구매자금어음 거래 역정 조회                            */
    /* --------------------------------------------------- */
    case 331800:
    case 331900:
        ctx->tx_num = 9;
        break;

    /* --------------------------------------------------- */
    /* 취급결번 응답                                          */
    /* --------------------------------------------------- */
    case 357100:
        ix0800d = (ixmsg0800d_t *) ctx->ext_recv_data;

        /* ----------------------------------------------- */
        /* 결번요구 응답전문의 응답코드가                         */
        /* "701" (결번처리완료), "702"(결번아님 )인 경우          */
        /* IXSKN에서 해당 RECORD를 DELTE 한다.                 */
        /* ----------------------------------------------- */
        memset(kftc_skip_no,    0x00, sizeof(kftc_skip_no));
        /* 
        
        */

        memcpy(&kftc_skip_no[0], "027",  3);
        memcpy(&kftc_skip_no[3], ix0800d->skip_msg_no,  6);

        if ((memcmp(ix0800d->rspn_code, "701", 3) == 0) ||
            (memcmp(ix0800d->rspn_code, "702", 3) == 0)) {
            EXEC SQL DELETE IXKSKN 
                      WHERE KFTC_SKIP_NO  = RPAD(:kftc_skip_no, 12 , ' ');
            switch(SYS_DB_ERRORNUM) {
            case SYS_DB_SUCCESS:
            case SYS_DB_NOTFOUND:
                 sys_tx_commit(TX_CHAINED);
                 break;

            default:
                db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
                ex_syslog(LOG_ERROR, "[APPL_DM] %s g000_tx_code_check()"
                                     "DELETE(IXKSKN) ERROR  :[%d]"
                                    "[해결방안]ORACLE 담당자 CALL MSG[%s]",
                                    __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);

                return ERR_ERR;
            }
        }
        return GOB_NRM;

        default:
            SYS_HSTERR(SYS_NN, SYS_GENERR, "INVAILD TX_CODE ERROR");
            return ERR_ERR;
    }

    /* ------------------------------------------------------------ */
    SYS_DBG("g000_tx_code_check tx_num[%d]", ctx->tx_num);
    /* ------------------------------------------------------------ */

    SYS_TREF;

    return ERR_NONE;

}


/* ------------------------------------------------------------------------------------------------------------ */
static int h000_ix_skn_check(ixn0020_ctx_t  *ctx)
{

    int                 rc  = ERR_NONE;
    ixi0120f_t          ixi0120f;

    SYS_TRSF;

    /* ------------------------------------------------------------ */
    SYS_DBG("h000_ix_skn_check kftc_skn_chk[%s]", EXPARM->kftc_skn_chk);
    /* ------------------------------------------------------------ */

    if (EXPARM->kftc_skn_chk[0] != '1')
        return ERR_NONE;

    /* ------------------------------------------------------------ */
    /* 결번 검증 처리                                                  */
    /* ------------------------------------------------------------ */
    memset(&ixi0120f,   0x00, sizeof(ixi0120f_t));

    /* Decoupling ***************************************************/
    ixi0120f.in.in_flag         = 1;        /* 0.취급업무 , 1.개설업무  */
    ixi0120f.in.in_max_flag     = 0;        /* 0.중복검증 , 1.MAX채번  */
    memcpy(ixi0120f.in.in_kti_flag, ctx->kti_flag,  LEN_IXI0120F_KTI_FLAG);

    SYS_TRY(g000_ix_skn_check(&ixi0120f));

    sys_tx_commit(TX_CHAINED);
    EXPARM->kftc_dup_chk[0] = ixi0120f.out.dup_chk_flag[0];

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;
    return ERR_ERR;

}


/* ------------------------------------------------------------------------------------------------------------ */
static int j000_ix_dup_check(ixn0020_ctx_t  *ctx)
{

    int                 rc  = ERR_NONE;

    ixi0110x_t          ixi0110x;


    exmsg1200_ix02_t    *ix02_area;
    exmsg1200_ix21_t    *ix21_area;
    exmsg1200_ix24_t    *ix24_area;
    exmsg1200_ix25_t    *ix25_area;


    SYS_TRSF;

    /* ------------------------------------------------------------ */
    SYS_DBG("j000_ix_dup_check kftc_dup_chk[%s]", EXPARM->kftc_dup_chk);
    /* ------------------------------------------------------------ */

    if (EXPARM->kftc_dup_chk[0] != '1')
        return ERR_NONE;

    /* ------------------------------------------------------------ */
    /* 중복거래 검증                                                   */
    /* ------------------------------------------------------------ */
    memset(&ixi0110x,   0x00,   sizeof(ixi0110x_t));
    ixi0110x.in.in_flag = 1;
    memcpy(ixi0110x.in.tx_date, EXMSG1200->tx_date, LEN_IXI0110X_TX_DATE);
    memcpy(ixi0110x.in.msg_no , EXMSG1200->msg_no , LEN_IXI0110X_MSG_NO );
    /* bank_code length  */
    memcpy(ixi0110x.in.our_msg_no,  EXMSG1200->our_msg_no, LEN_IXI0110X_OUT_MSG_NO);
    /* EXMSG1200->old_msg_no에 EI_MSG_NO가 조립되어 있다.   */
    memcpy(ixi0110x.in.ei_msg_no,   EXMSG1200->old_msg_no, LEN_IXI0110X_OUT_MSG_NO);
    memcpy(ixi0110x.in.kti_flag,    ctx->kti_flag        , LEN_IXI0110X_KTI_FLAG  );

    /* ------------------------------------------------------------ */
    SYS_DBG("j000_ix_dup_check ei_msg_no[%s]", ixi0110x.in.msg_no );
    /* ------------------------------------------------------------ */

    rc = ix_dup_chk(&ixi0110x);

    SYS_DBG("ixi0110x.out.rspn_code[%.3s]", ixi0110x.our.rspn_code);

    switch(rc){
    /* ------------------------------------------------------------ */
    /* 중복거래인 경우                                                 */
    /* 중계센터로 "이중거래임 "으로 응답코드를 set 전송                      */
    /* ------------------------------------------------------------ */
    case ERR_NONE:
        return ERR_ERR;





    /* ------------------------------------------------------------ */
    /* 중복거래가 아닌 경우                                              */
    /* ------------------------------------------------------------ */
    case SYS_DB_NOTFOUND:
        break;

    /* ------------------------------------------------------------ */
    /* 기타 DB_ERROR인경우                                             */
    /* 중계센터로 "은행센터의 SYSTEM장애로 "로 SET 하여 전송                 */
    /* ------------------------------------------------------------ */
    case ERR_ERR:
         ctx->ixi0220x.in.msg_code      = '1';
         ctx->ixi0220x.in.send_flag     = '2';
         memcpy(ctx->ixi0220x.in.rspn_code, "413",  LEN_RSPN_CODE);
         ctx->kftc_reply    = 1;
         return ERR_ERR;


    default:
        break;
    }
    
    SYS_TREF;

    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
/* 취소거래시 원거래 저널 검증을 한다.                                                                                  */
/* 원 저널 정보를 ctx->ixjrn에 보관을 하는데 .. EXPARM->canc_org_chk[0] = '1' 이 아니면                                  */
/* n000_ix_host_orig_make에서 exmsg1200전문을 원복하지 못한다.                                                        */
/* ------------------------------------------------------------------------------------------------------------ */
static int k000_canc_orig_chk(ixn0020_ctx_t     *ctx)
{

    int                 rc = ERR_NONE;
    ixi1020x_t          ixi1020x;
    exmsg1200_ix21_t    *ix21;


    SYS_TRSF;

    /* ------------------------------------------------------------ */
    SYS_DBG("k000_canc_orig_chk : canc_orig_chk[%s]", EXPARM->canc_orig_chk );
    /* ------------------------------------------------------------ */

    if (EXPARM->canc_orig_chk[0] != '1')
        return ERR_NONE;

    ix21 = (exmsg1200_ix21_t *) EXMSG1200->detl_area;

    /* ------------------------------------------------------------ */
    /* 취소거래시 원저널 검증                                            */
    /* ------------------------------------------------------------ */
    memset(&ixi1020x,   0x00, sizeof(ixi1020x_t));
    ixi1020x.in.in_flag     = 0;
    ixi1020x.in.exmsg1200   = EXMSG1200;
    memcpy(ixi1020x.ion.tx_date,    EXMSG1200->tx_date,     LEN_IXI1020X_TX_DATE);
    /* 수표요건 수정 */


    memcpy(ixi1020x.in.prc_flag         , "IXN002001"           , 9);
    memcpy(ixi1020x.in.msg_no           ,  ix21->orig_ei_msg_no , 10);
    memcpy(ixi1020x.in.orig_ei_msg_no   ,  ix21->orig_ei_msg_no , 10);
    memcpy(ixi1020x.in.kti_flag         , ctx->kti_flag         , LEN_KTI_FLAG);  /* 시스템 구분 : 0:GCG 1:ICG   */


    SYS_DBG("k000_canc_orig_chk 001");
    SYS_DBG("k000_canc_orig_chk :ixi1020x.in.prc_flag       [%.8s]" , &ixi1020x.in.prc_flag );
    SYS_DBG("k000_canc_orig_chk :ix21->orig_ei_msg_no       [%.10s]", &ix21->orig_ei_msg_no);
    SYS_DBG("k000_canc_orig_chk :ixi1020x.in.msg_no         [%.10s]", &ixi1020x.in.msg_no );
    SYS_DBG("k000_canc_orig_chk :ixi1020x.in.orig_ei_msg_no [%.10s]", &ixi1020x.in.orig_ei_msg_no );

    /**
    * 타행 자기앞 거래시 거래고유번호 12자리 read& hold
    *
    *
    *
    *
    */


    
    
    
    SYS_TRY(ix_jrn_hold(&ixi1020x));

    /* ------------------------------------------------------------ */
    /* 취소거래시 IXJRN이라는 structure를 "원저널 검증 "과                  */
    /* "송신전문과 수신전문 비교"에서 두번 사용하므로 원저널의                   */
    /*  취소여부를 뒤에서 알수가 없으므로 원저널 (입금거래 )의                  */
    /*  취소상태를 orig_canc_type이라는 변수에 보관하였다가 뒤에서             */
    /* 원저널이 취소가 안되었으나 수신한 취소거래의 응답코드가                   */
    /* 기취소로 왔을 경우 정상처리하기 위함                                 */
    /* ------------------------------------------------------------ */
    ctx->orig_canc_type[0] = ixi1020x.out.ixjrn.canc_type[0];
    memcpy(&ctx->ixjrn, &ixi1020x.out.ixjrn,    sizeof(ixjrn_t));

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return ERR_ERR;
    
}

/* ------------------------------------------------------------------------------------------------------------ */
/* 취급시 전송된 전문과 취급응대시 수신한 전문의 일치 여부를 검증한다.                                                         */
/* ------------------------------------------------------------------------------------------------------------ */
static int l000_proc_rspn_chk(ixn0020_ctx_t     *ctx)
{

    int                 rc  = ERR_NONE;
    char                buff[LEN_TCP_HEAD + 1];
    ixi0180x_t          ixi0180x;


    exmsg1200_ix02_t    *ix02_area;
    exmsg1200_ix21_t    *ix21_area;
    exmsg1200_ix24_t    *ix24_area;
    exmsg1200_ix25_t    *ix25_area;

    SYS_TRSF;

    /* ------------------------------------------------------------ */
    SYS_DBG("l000_proc_rspn_chk kftc_orig_flag[%s]", EXPARM->kftc_orig_flag );
    /* ------------------------------------------------------------ */

    if (EXPARM->kftc_orig_flag[0] != '1')
        return ERR_NONE;

    /* ------------------------------------------------------------ */
    /* 송수신한 전문과 수신한 전문을 일치하는지 확인한다.                       */
    /* ------------------------------------------------------------ */
    memset(&ixi0180x,   0x00, sizeof(ixi0180x_t));
    ixi0180x.in.in_flag     = '0';
    ixi0180x.in.tx_num      = ctx->tx_num;
    ixi0180x.in.exmsg1200   = EXMSG1200;
    ixi0180x.in.ixjrn       = &ctx->ixjrn;
    /* EXMSG1200->old_msg_no에 EI_MSG_NO가 채워져 온다.     */
    memcpy(&ixi0180x.ei_msg_no, &EXMSG1200->old_msg_no, LEN_EXMSG1200_OUR_MSG_NO);

    /* ------------------------------------------------------------ */
    SYS_DBG("ixi0180x.in.ei_msg_no [%.10s]", ixi0180x.in.ei_msg_no);
    /* ------------------------------------------------------------ */

    SYS_TRY(ix_orig_verf(&ixi0180x));

    memcpy(ctx->kti_flag,   ixi0180x.out.kti_flag,  LEN_KTI_FLAG);

    /* ------------------------------------------------------------ */
    SYS_DBG("l000_proc_rspn_chk:ixi0180x.out.rspn_code[%s]  ", ixi0180x.out.rspn_code);
    SYS_DBG("l000_proc_rspn_chk:ixi0180x.out.kti_flag [%s]  ", ixi0180x.out.kti_flag );
    SYS_DBG("l000_proc_rspn_chk:ctx->kti_flag         [%s]  ", ctx->kti_flag         );
    /* ------------------------------------------------------------ */

    /* ------------------------------------------------------------ */
    /* 만약 에러 발생시에 KFTC로 에러내역을 SET하여 전송한다.                 */
    /* ------------------------------------------------------------ */
    if (memcmp(ixi0180x.our.rspn_code, "000", LEN_IXI0180X_RSPN_CODE) != 0 ){
        ctx->ext_recv_data[IX_IDX_MSG_TYPE] = '9';
        /* bank code length */

        memcpy(&ctx->ext_recv_data[22], ixi0180x.out.rspn_code, LEN_IXI0180X_RSPN_CODE);
        ctx->kftc_reply     = 1;
        ctx->kftc_err_set   = 0;
        return ERR_ERR;
    }

    /* TCP/IP 헤더터정보    */
    utortirm(ixi0180x.out.tcp_head);
    if (strlen(ixi0180x.out.tcp_head) > 0 ){
        memset(buff,    0x20, sizeof(buff));
        memcpy(buff, ixi0180x.out.tcp_head, strlen(ixi0180x.out.tcp_head));
        rc = sysocbsi(ctx->cb, IDX_TCPHEAD, buff, LEN_TCP_HEAD);
        if (rc == ERR_ERR){
            ex_syslog(LOG_ERROR, "[APPL_DM] %s l000_proc_rspn_chk()"
                                 " COMMBUFF(TCPHEAD) SET ERROR "
                                 "[해결방안] 업무 담당자 CALL ",
                                __FILE__ );

            return ERR_ERR;

        }
    }

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return ERR_ERR;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int m000_rspn_code_check(ixn0020_ctx_t       *ctx)
{

    int                 rc  = ERR_NONE;
    ixmsg0200a_t        *ixmsg0200a;
    ixmsg0400_t         *ixmsg0400;
    exmsg1200_ix21_t    *ix21;


    SYS_TRSF;

    /* ------------------------------------------------------------ */
    SYS_DBG("EXPARM->rspn_cmp_code[%c]", EXPARM->rspn_cmp_code[0] );
    /* ------------------------------------------------------------ */    

    /* ------------------------------------------------------------ */
    /* IXJRN에서 SELECT한 전문의 응답코드와 수신한 전문의                    */
    /* 응답코드를 비교하여 확인 한다.                                      */
    /* ------------------------------------------------------------ */
    switch(EXPARM->rspn_cmp_code[0]){
    case '1':
        ix0200a = (ixmsg0200a_t *) ctx->ext_recv_data;
        SYS_TRY(m100_proc_rspn_chk(ctx));
        break;
    /* ------------------------------------------------------------ */
    /* 수표조회(2), 처리결과조회(3), 입금취소거래(4)인 경우                   */
    /* ------------------------------------------------------------ */
    case '2':
    case '3':
    case '4':
        /* -------------------------------------------------------- */
        /* 만약 IXJRN에서의 응답코드가 불능(TIME OVER)인 경우 무시           */
        /* -------------------------------------------------------- */
        if ((memcpmp(ctx->ixjrn.rspn_code, "000",  LEN_RSPN_CODE) != 0) &&
            (memcpmp(ctx->ixjrn.rspn_code, "   ",  LEN_RSPN_CODE) != 0)){
            return ERR_ERR;
        }
        break;
    }
    /* ------------------------------------------------------------ */
    /* 입금불능전문 (송수신 flag == 5)을 수신한 경우 불능 저널을              */
    /* 생성한다.                                                      */
    /* (기입금 '309'거래는 불능 저널을 생성하지 않는다. )                    */
    /* ------------------------------------------------------------ */
    if (ctx->tx_num == 1){
        if (ix0200a->send_flag[0] == '5') {
            if (memcmp(ix0200a->rspn_code, "309",  LEN_RSPN_CODE) ==0 ){
                EXPARM->host_send_jrn_make[0] = '0';
                EXPARM->host_send_tot_flag[0] = '0';
            }
            else if (memcmp(ix0200a->rspn_code, "000", LEN_RSPN_CODE) != 0){
                EXPARM->host_send_jrn_make[0] = '2';
            }
        }
    }

    /* ------------------------------------------------------------ */
    /* 응답코드가 정상이 아닌지 확인한다.                                   */
    /* ------------------------------------------------------------ */
    if (ctx->tx_num == 4){
        ix21 = (exmsg1200_ix21_t *) EXMSG1200->detl_area;

        if ((ctx->orig_canc_type[0] == '0') &&
            (memcmp(ix21->rspn_code, "607",  LEN_RSPN_CODE) == 0)) {
            /* -------------------------------------------------------- */
            /* 원저널이 취소되지 않았고 취소거래의 응답코드가                      */
            /* 기취소이면 정상취소거래로 처리한다.                              */
            /* -------------------------------------------------------- */
            ix0400b = (ixmsg0400_t *) ctx->ext_recv_data;
            memset(ix0400b->rspn_code,  '0',    LEN_RSPN_CODE);
            memset(ix21->rspn_code,     '0',    LEN_RSPN_CODE);
            memset(EXMSG1200->err_code, '0',    LEN_EXMSG1200_ERR_CODE);
            SYS_HSTERR(SYS_NN, 0, "");
        }else{
            if (memcmp(EXMSG1200->err_code, "0000000", LEN_EXMSG1200_ERR_CODE) != 0 ){
                /* -------------------------------------------------------- */
                /* 상기경우외에 취소거래의 응답코드가 정상이 아니면                    */
                /* 취소시 원저널 UPDATE FLAG를 '0'으로 SET하고                   */
                /* 집계반영 FLAG를 '0'으로 SET한다.                             */
                /* -------------------------------------------------------- */
                EXPARM->canc_org_upd[0]         = '0';
                EXPARM->host_send_tot_flag[0]   = '0';
            }
        }
    }

    SYS_TREF;

SYS_CATCH:
    
    SYS_TREF;

    return ERR_ERR;

}


/* ------------------------------------------------------------------------------------------------------------ */
static int m100_proc_rspn_chk(ixn0020_ctx_t     *ctx)
{

    int                 rc = ERR_NONE;
    char                func_name[LEN_EXPARM_SUB_PGM_NAME + 1];
    ixmsg0200a          *ix0200a;

    SYS_TRSF;

    ix0200a = (ixmsg0200a_t *) ctx->ext_recv_data;

    /* ------------------------------------------------------------ */
    SYS_DBG("EXT   : rspn_code[%.3s]", ix0200a->rspn_code  );
    SYS_DBG("IXJRN : rspn_code[%.3s]", ctx->ixjrn.rspn_code);
    /* ------------------------------------------------------------ */

    /* ------------------------------------------------------------ */
    /* 만약 수신한 전문이 정상응답이면                                     */
    /* ------------------------------------------------------------ */
    if (memcmp(ix0200a->rspn_code, "000", LEN_RSPN_CODE) == 0) {
        /* --------------------------------------------------- */
        /* 만약 IXJRN에서의 응답코드가 정상이면 무시                   */
        /* --------------------------------------------------- */
        if (memcmp(ctx->ixjrn.rspn_code, "000", LEN_RSPN_CODE) == 0){
            return ERR_ERR;
        }
        /* --------------------------------------------------- */
        /* 만약 IXJRN에서의 응답코드가 이중거래이거나                   */
        /* 아직응답전문을 수신하지 않고                               */
        /* 정상응답전문을 수신하면 정상처리한다.                        */
        /* --------------------------------------------------- */
        else if (memcmp(ctx->ixjrn.rspn_code, "309", LEN_RSPN_CODE) == 0){
            return ERR_NONE;
        }
        /* --------------------------------------------------- */
        /* 이전에 이미 불능응답으로 전문을 수신하고                     */
        /* 다시 정상응답으로 전문을 수신한 경우 무시                    */
        /* --------------------------------------------------- */
        else if (memcmp(ctx->ixjrn.rspn_code, "  ", LEN_RSPN_CODE) != 0){
            return ERR_ERR;
        }
        /* --------------------------------------------------- */
        /* 아직 응답 전문을 수신하지 않고                             */
        /* 정상응답을 수신하면 정상처리한다.                           */
        /* --------------------------------------------------- */
        else {
            return ERR_NONE;
        }
    }
    /* ------------------------------------------------------------ */
    /* 이중거래 에러로 전문을 수신한 경우                                   */
    /* ------------------------------------------------------------ */
    else if (memcmp(ix0200a->rspn_code, "309", LEN_RSPN_CODE) == 0){
        /* --------------------------------------------------- */
        /* 만약 IXJRN에서의 응답코드가 정상이면 무시                   */
        /* --------------------------------------------------- */
        if (memcmp(ctx->ixjrn.rspn_code, "000", LEN_RSPN_CODE) == 0){
            return ERR_ERR;
        }
        /* --------------------------------------------------- */
        /* 만약 IXJRN에서의 응답코드가 불능이면 무시                   */
        /* --------------------------------------------------- */
        else if (memcmp(ctx->ixjrn.rspn_code, "  ", LEN_RSPN_CODE) != 0){
            return ERR_ERR;
        }
        /* --------------------------------------------------- */
        /* 만약 IXJRN는 미완료로 되어있는 경우                        */
        /* IXJRN은 UPDATE하고 IXTOT는 반영하지 않는다.               */
        /* --------------------------------------------------- */
        EXPARM->host_send_jrn_upd[0]  = '2';
        EXPARM->host_send_tot_flag[0] = '0';
    }
    /* ------------------------------------------------------------ */
    /* 입금불능 전문을 수신한 경우                                        */
    /* ------------------------------------------------------------ */
    else {
        /* --------------------------------------------------- */
        /* 순채무한도 에러로 전문을 수신한 경우                        */
        /* 에러처리 서비스 호출                                    */
        /* --------------------------------------------------- */
        if (memcmp(ix0200a->rspn_code, "999", LEN_RSPN_CODE) == 0){
            memset(func_name,   0x00, sizeof(func_name));
            if (EXPARM->sub_pgm_flag[0] == '1'){
                memcpy(func_name,   EXPARM->sub_pgm_name, LEN_EXPARM_SUB_PGM_NAME);
            }
            else{
                strcpy(func_name, "EXO6800");
            }
            utortrim(func_name);

            /* 관리전문 수동처리 서비스 호출    */
            x000_msg_svc_call(ctx, func_name);
    
        }

        /* --------------------------------------------------- */
        /* 만약 IXJRN에서 응답코드가 미완료이면                       */
        /* 집계반영 FLAG를 '0'으로 RESET                          */
        /* --------------------------------------------------- */
        if (memcmp(ctx->ixjrn.rspn_code, "   ", LEN_RSPN_CODE) == 0){
            EXPARM->host_send_tot_flag[0] = '0';
        }
        /* --------------------------------------------------- */
        /* 이미 이중거래전문을 수신한 상태이면                         */
        /* IXJRN에 반영하고 IXTOT에는 반영하지 않는다.                */
        /* --------------------------------------------------- */
        else if (memcmp(ctx->ixjrn.rspn_code, "309", LEN_RSPN_CODE) == 0){
            EXPARM->host_send_jrn_upd[0]    = '2';
            EXPARM->host_send_tot_flag[0]   = '0';
        }
        /* --------------------------------------------------- */
        /* 이미 입금불능전문을 수신한 상태이면 무시                     */
        /* --------------------------------------------------- */
        else if (memcmp(ctx->ixjrn.rspn_code, "000", LEN_RSPN_CODE) != 0){
            return ERR_ERR;
        }
        /* --------------------------------------------------- */
        /* 이미 입금정상전문을 수신했으면 정상                         */
        /* 처리한다.                                             */
        /* --------------------------------------------------- */
    }

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
/* IXJRN을 읽어 원거래의 중요항목을 조립한다.                                                                           */
/* ------------------------------------------------------------------------------------------------------------ */
static int n000_ix_host_orig_msg_make(ixn0020_ctx_t     *ctx)
{

    int                 rc = ERR_NONE;
    int                 len;
    ixi1040x_t          ixi1040x;
    ixi0110x_t          ixi1100x;
    exmsg1200_ix21_t    *ix21a;
    exmsg1200_ix21_t    *ix21b;
    exmsg1200_ix25_t    *ix25a;
    exmsg1200_ix25_t    *ix25b;


    SYS_TRSF;

    /* ------------------------------------------------------------ */
    SYS_DBG("host_orig_msg_make (n000_ix_host_orig_msg_make) -001 [%s]", EXPARM->host_orgi_msg_make  );
    
    /* ------------------------------------------------------------ */
    /* 원전문을 조립한다.                                               */
    /* ------------------------------------------------------------ */
    if (EXPARM->host_orgi_msg_make[0] != '1')
        return ERR_NONE;

    /* 1200전문 초기화    */
    memset(&ixi1040x,   0x00,   sizeof(ixi1040x_t));
    ixi1040x.in.exmsg1200 = &ctx->exmsg1200;
    ix_ex1200_init(&ixi1040x);

    /* 저널 데이터를 이용해 1200전문을 조립힌다. */
    memset(&ixi1100x,   0x00,   sizeof(ixi1100x_t));
    ixi1100x.in.ixjrn       = &ctx->ixjrn;
    ixi1100x.in.exmsg1200   = &ctx->exmsg1200;

    /* ------------------------------------------------------------ */
    /* IXJRN DATA를 1200 바이트 전문으로 변환                            */
    /* ------------------------------------------------------------ */
    ix_jrn_ex1200_make(&ixi1100x);

    SYS_DBG("&ctx->ixjrn.kti_flag[0]=[%s]", &ctx->ixjrn.kti_flag);

    /* KTI 전송시 passwd  */
    if (ctx->kti_flag[0] == '1'){
        if (memcmp(EXMSG1200->pswd_1, "@@@@@@@@@@@@@@@@", LEN_EXMSG1200_PSWD_1) == 0){
            memset(EXMSG1200->pswd_1, 0x20, LEN_EXMSG1200_PSWD_1);
        }
    }

    /* 거래공통   1200전문 조립 */
    memcpy(ctx->exmsg1200.tx_id, EXMSG1200->tx_id,   LEN_EXMSG1200_TX_ID);
    ctx->exmsg1200.tx_code[5] =  EXMSG1200->tx_code[5];

    memcpy(ctx->exmsg1200.err_code,  EXMSG1200->err_code, LEN_EXMSG1200_ERR_CODE);
    if (sys_err_code() > 0) {
        sprintf(ctx->exmsg1200.err_code, "%07d", sys_err_code());
    }

    len = strlen(sys_err_msg());
    if (len > 0){
        len = (len > LEN_EXMSG1200_ERR_MSG) ? LEN_EXMSG1200_ERR_MSG : len; 
        memcpy(ctx->exmsg1200.err_msg, sys_err_msg(), len);
    }

    memcpy(ctx->exmsg1200.cr_brn_no,    EXMSG1200->cr_brn_no,   LEN_EXMSG1200_CR_BRN_NO  );
    memcpy(ctx->exmsg1200.msg_no   ,    EXMSG1200->msg_no   ,   LEN_EXMSG1200_MSG_NO     );
    memcpy(ctx->exmsg1200.teir_msg_no,  EXMSG1200->teir_msg_no, LEN_EXMSG1200_TEIR_MSG_NO);

    /* 거래코드별 1200 전문 조립     */
    switch(ctx->tx_num){
    case 1:
    case 3:
    case 4:
    case 6:
    case 8:
        ix21a = (exmsg1200_ix21_t *) EXMSG1200->detl_area;
        ix21b = (exmsg1200_ix21_t *) ctx->exmsg1200.detl_area;

        SYS_DBG("ix21a->chq_brn_no   =[%.*s]", LEN_EXMSG1200_IX21_CHQ_BRN_NO  , ix21a->chq_brn_no);
        SYS_DBG("ix21a->chq_cash_amti=[%.*s]", LEN_EXMSG1200_IX21_CHQ_CASH_AMT, ix21a->chq_cash_amt);

        memcpy(ix21b->rspn_code,        ix21a->rspn_code,       LEN_EXMSG1200_IX21_RSPN_CODE);
        memcpy(ix21b->kftc_time,        ix21a->kftc_time,       LEN_EXMSG1200_IX21_KFTC_TIME);
        memcpy(ix21b->teir_time,        ix21a->teir_time,       LEN_EXMSG1200_IX21_TEIR_TIEM);
        memcpy(ix21b->chq_no   ,        ix21a->chq_no   ,       LEN_EXMSG1200_IX21_CHQ_NO   );
        memcpy(ix21b->chq_brn_no,       ix21a->chq_brn_no,      LEN_EXMSG1200_IX21_CHQ_BRN_NO);
        memcpy(ix21b->orig_msg_no,      ix21a->orig_msg_no,     LEN_EXMSG1200_IX21_ORIG_MSG_NO);

        SYS_DBG("ix21a->chq_brn_no   =[%.*s]", LEN_EXMSG1200_IX21_CHQ_BRN_NO  , ix21a->chq_brn_no);
        SYS_DBG("ix21a->chq_cash_amti=[%.*s]", LEN_EXMSG1200_IX21_CHQ_CASH_AMT, ix21a->chq_cash_amt);

        memcpy(ctx->exmsg1200.rcv_cust_name,    EXMSG1200->rcv_cust_name, LEN_EXMSG1200_RCV_CUST_NAME);

        if (ctx->tx_num == 1){
            ctx->exmsg1200.rspn_flag[0] = '1';
            /* --------------------------------------------------------- */
            /* 정상 입금 응답인   경우                                        */
            /* --------------------------------------------------------- */
            if (memcmp(ix21a->rspn_code, "000", LEN_EXMSG1200_IX21_RSPN_CODE) == 0){
                ctx->exmsg1200.tx_code[5] = '1';
            }
            /* --------------------------------------------------------- */
            /* 불능 입금 응답인   경우                                        */
            /* --------------------------------------------------------- */
            else {
                ctx->exmsg1200.tx_code[5]   = '2';
                ctx->exmsg1200.canc_type[0] = '1';
            }
        }
        break;

    case 2:
    /* ----------------------------------------------------------------- */
    /* 어음 미결제 통보인 경우                                                */
    /* ----------------------------------------------------------------- */
    case 5:
    /* ----------------------------------------------------------------- */
    /* 기업구매자금 어음 통보인 추가                                            */
    /* ----------------------------------------------------------------- */
    case 9:
        ctx->exmsg1200.rspn_flag[0] = '1';
        memcpy(ctx->exmsg1200.detl_area,    EXMSG1200->detl_area,   LEN_EXMSG1200_DETL_AREA);

        /* 기업구매자금 어음 거래약정 조회 추가  */
        memcpy(ctx->exmsg1200.reqs_cust_name,   EXMSG1200_reqs_cust_name,   LEN_EXMSG1200_REQS_CUST_NAME);
        break;

    /* ----------------------------------------------------------------- */
    /* 부도  어음 통보인 경우                                                 */
    /* ----------------------------------------------------------------- */
    case 7:
        ctx->exmsg1200.rspn_flag[0] = '1';
        ix25a   = (exmsg1200_ix25_t *) EXMSG1200->detl_area;
        ix25b   = (exmsg1200_ix25_t *) ctx->exmsg1200.detl_area;

        memcpy(ctx->exmsg1200.comm_filler,  EXMSG1200->comm_filler, LEN_EXMSG1200_COMM_FILLER);
        memcpy(ctx->exmsg1200.cust_id    ,  EXMSG1200->cust_id    , LEN_EXMSG1200_CUST_ID    );
        memcpy(ctx->exmsg1200.detl_area  ,  EXMSG1200->detl_area  , LEN_EXMSG1200_DETL_AREA  );
        break;

    default:
        break; 
    }



    /* 타행환 취소 및 수취조회 전문 변경 */
    int tmp_rc = 0;
    tmp_rc = utostcmp(ctx->host_tx_code,    "3181000000", "3181000300", "3181000500", "3181000900", "3181001000", "3181100000", "");
    if (tmp_rc > 0) {
        memcpy(&ctx->exmsg1200.detl_area[290],  &EXMSG1200->detl_area[290], 9);
        SYS_DBG("&ctx->exmsg1200.detl_area[290]==>[%.9s] &EXMSG1200->detl_area[290]==>[%.9s]", &ctx->exmsg1200.detl_area[290], &EXMSG1200->detl_area[290]);
    }
    tmp_rc = 0;
    tmp_rc = utostcmp(ctx->host_tx_code, "3087000000", "3087000300", "3577100000");
    if (tmp_rc > 0) {
        memcpy(&ctx->exmsg1200.detl_area[290],  &EXMSG1200->detl_area[290], 7);
        SYS_DBG("&ctx->exmsg1200.detl_area[290]==>[%.7s] &EXMSG1200->detl_area[290]==>[%.7s]", &ctx->exmsg1200.detl_area[290], &EXMSG1200->detl_area[290]);

    }


    SYS_TREF;

    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
static int o000_ix_canc_orig_update(ixn0020_ctx_t       *ctx)
{
    int                 rc = ERR_NONE;
    char                kftc_msg_no[LEN_EXMSG1200_IX]

}
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */