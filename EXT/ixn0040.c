
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
    /* EI관리번호 채번  ***********************************************************************/
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
    /* 결제원 송신전문 로깅 */
    else{
        ixi0230f.in.flag            = 'S';
        ixi0230f.in.log_type        = 'K';
        ixi0230f.in.log_len         = sysocbgs(ctx->cb, IDX_EXTSENDDATA);
        memcpy(ixi0230f.in.log_data,  EXTSENDDATA, ixi0230f.in.log_len );        
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

     rc = sysocbsi(&dcb, IDX_EXMSG1200, ixi0230f, sizeof(ixi0230f));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s IXN0040: b000_msg_logging()"
                             "COMMBUFF BACKUP ERROR "
                             "[해결방안]시스템 담당자 call", 
                             __FILE__);
        sys_err_init();
        sysocbfb(&dcb);
        return ERR_NONE;
    }

     rc = sys_tpcall("IXN0230F", &dcb, TPNOREPLY | TPNOTRAN);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s IXN0040: b000_msg_logging()"
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
static int c000_kftc_fild_chk(ixn0040_ctx_t *ctx)
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

    /* 결제원 에러 응답 조립하지 않음. */
    ctx->kftc_err_set = 0;

    SYS_TREF;
    return ERR_ERR;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int d000_tran_code_conv(ixn0040_ctx_t    *ctx)
{
    int                 rc  = ERR_NONE;
    exi0250x_t          exi0250x;
    ixmsg0200a_t        ix0200a;

    SYS_TRSF;

    ix0200a = (ix0200a_t *) ctx->ext_recv_data;

    /*------------------------------------------------------------------------ */
    /* 거래구분 코드 변환                                                          */
    /*------------------------------------------------------------------------ */
    memset(&exi0250x, 0x00, sizeof(exi0250x_t));
    exi0250x.in.conv_flag       = k;  
    exi0250x.in.ext_recv_data   = ctx->ext_recv_data;

    memcpy(exi0250x.in.appl_code,   IX_CODE,            LEN_APPL_CODE);
    memcpy(exi0250x.in.tx_flag  ,   ix0200a->send_flag, LEN_EXI0250X_TX_FLAG);
    memcpy(exi0250x.in.msg_type ,   ix0200a->msg_code,  LEN_MSG_TYPE);
    memcpy(exi0250x.in.kftc_tx_code,ix0200a->appl_code, 3);

    rc = ex_tran_code_conv(&exi0250x);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s IXN0040: d00_tran_code_conv():"
                             "거래코드 변환 ERROR : code/msg[%d:%s]"
                             "[해결방안] IX담당자 CALL" ,
                             __FILE__, sys_err_code, sys_err_msg());
        return GOB_NRM;
    }

    

    memcpy(ctx->host_tx_code, exi0250x.out.tx_code, LEN_TX_CODE);

    /*------------------------------------------------------------------------ */
    SYS_DSP(" d000_tran_code_conv:host_tx_code [%s]", ctx->host_tx_code);
    /*------------------------------------------------------------------------ */

    SYS_TREF;

    return ERR_ERR;


}

/* ------------------------------------------------------------------------------------------------------------ */
static int e000_exparm_read(ixn0040_ctx_t   *ctx)
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
        return GOB_NRM;
    }

    /* exparm을 commbuff에 저장 */
    rc = sysocbsi(ctx->cb, IDX_EXPARM, &exi0210x.out.exparm, sizeof(exparm_t));
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s e000_exparm_read():"
                             "COMMBUFF(EXPARM) SET ERROR host_tx_code[%s]"
                             "[해결방안] 업무담당자 CALL",
                             __FILE__, ctx->host_tx_code);
        return GOB_NRM;
    }

    SYS_TREF;

    return ERR_NONE;


}

/* ------------------------------------------------------------------------------------------------------------ */
static int f000_msg_format(ixn0040_ctx_t    *ctx)
{
    int                 rc  = ERR_NONE;
    ixi1040x_t          ixi1040x;
    exi4000x_t          exi4000x;
    exmsg1200_t         exmsg1200;

    SYS_TRSF;

    /*------------------------------------------------------------------------ */
    SYS_DSP(" f000_msg_format: host_msg_make [%s]", EXPARM->host_msg_make);
    /*------------------------------------------------------------------------ */

    if (EXPARM->host_msg_make[0] != '1')
        return ERR_NONE;

    /*------------------------------------------------------------------------ */
    /* 1200전문 초기화                                                           */
    /*------------------------------------------------------------------------ */
    memset(&ixi1040x, 0x00, sizeof(ixi1040x_t));
    ixi1040x.in.exmsg1200   = &exmsg1200;
    ix_ex1200_init(&ixi1040x);

    /*------------------------------------------------------------------------ */
    /* 호스트전문으로 FORMAT                                                      */
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
        ex_syslog(LOG_ERROR, "[APPL_DM] %s f000_msg_format()"
                             "FORMAT ERROR: host_tx_code [%s]"
                             "[해결방안]업무담당자 CALL",
                             __FILE__, ctx->host_tx_code);
        goto SYS_CATCH;
    }

    if (exi4000x.out.msg_len <= 0){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s f000_msg_format()"
                             "FORMAT ERROR: host_tx_code [%s]"
                             "[해결방안]업무담당자 CALL",
                             __FILE__, ctx->host_tx_code);
        goto SYS_CATCH;
    }

    /* 전문변환 데이터 1200 bytes */
    memcpy(&exmsg1200, exi4000x.out.msg, LEN_EXMSG1200);

    /*------------------------------------------------------------------------ */
    /* 변환된전문을 COMMBUFF에 set                                                 */
    /*------------------------------------------------------------------------ */
    rc = sysocbsi(ctx->cb, IDX_EXMSG1200,   &exmsg1200, LEN_EXMSG1200);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s f000_msg_format()"
                             "COMMBUFF(EXMSG1200) SET ERROR"
                             "[해결방안]업무담당자 CALL",
                             __FILE__);
        goto SYS_CATCH;
    }

    memcpy(EXMSG1200->tx_code, ctx->host_tx_code, LEN_TX_CODE);
    utodate1(EXMSG1200->tx_date);
    utotime1(EXMSG1200->tx_time);

#ifdef _SIT_DBG
    /* ------------------------------------------------------------------------- */
    PRINT_EXMSG1200(EXMSG1200);
    PRINT_EXMSG1200_2(EXMSG1200);
    /* ------------------------------------------------------------------------- */
#endif

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    ctx->ixi0220x.in.msg_code = '1';
    ctx->ixi0220x.in.msg_code = '4';
    memcpy(ctx->ixi0220x.in.rspn_code, "413", LEN_RSPN_CODE);

    SYS_TREF;
    return ERR_ERR;

}

/* ------------------------------------------------------------------------------------------------------------ */
/* g100 계좌종류구분 체크 신규 추가                                                                                   */
/* ------------------------------------------------------------------------------------------------------------ */
static int f100_gcg_icg_acct_chk_proc(ixn0040_ctx_t *ctx)
{
    int                 rc = ERR_NONE;
    char                in_acct_no [16 + 1];
    char                tmp_acctype[ 1 + 1];

    exi0320f_t          exi0320f;

    SYS_TRSF;

    /****
     * 계좌종류 구분 체크 FLAG(EXPARM->fil3)
     * 0:대상아님 (기존처럼 core전송 )
     * 1:입금계좌 (cr_acct_no)로 계좌구분
     * 2:출금계좌 (dr_acct_no)로 계좌구분
     * 3:입금계좌 + 출금계좌(출금계좌기준 전송)
    */
    SYS_DBG("EXPARM->fil3=>[%c]", EXPARM->fil3[0]);

    /* host통신여부 검증 */
    if (EXPARM->fil3[0] == '0'){
        return ERR_NONE;
    }

    memset(in_acct_no,  0x00, sizeof(in_acct_no));
    memset(tmp_acctype, 0x00, sizeof(tmp_acctype));
    memset(&exi0320f,   0x00, sizeof(exi0320f_t ));


    /****
     * 개설프로그램 기본 CORE
     * EXPARM_FIL3 0:처리안함 
     * EXPARM_FIL3 1: cr_acct_no (입금계좌)로 계좌구분
     * EXPARM_FIL3 2: dr_acct_no (출금계좌)로 계좌구분
     * EXPARM_FIL3 3:입금계좌 + 출금계좌(출금계좌기준 전송)
    */
    ctx->kti_flag[0] = '0' ;  //초기값 0:GCG

    if (EXPARM->fil3[0] == '1') {

        // EXPARM->fil3 = 1 => CR_ACCT_NO 로 계좌구분
        memcpy(in_acct_no, EXMSG1200->cr_acct_no, LEN_EXMSG1200_CR_ACCT_NO);

        /*------------------------------------------------------------------------ */
        /* ACCT_TYPE(계좌종류 구분필드 )판단모듈 호출 -START                              */
        /*------------------------------------------------------------------------ */
        memcpy(exi0320f.in.acct_no,     in_acct_no,     LEN_EXI0320F_ACCT_NO);
        memcpy(exi032of.in.curr_code,   "KRW",          LEN_EXI0320F_CURR_CODE);

        ex_icg_gcg_acct(&exi0320f);     /* ACCT_TYPE(계좌종류 구분필드) 판단모듈 호출     */

        SYS_DBG("exi0320f.out.acct_type[%d]", exi0320f.out.acct_type);

        /* 계좌종류구분 4:ICG가상계좌, 5:ICG계좌만 KTI로 전송     */
        if (exi0320f.out.acct_type == 4 || exi0320f.out.acct_type = 5 ) {
            ctx->kti_flag[0] = '1'; //1:ICG
        }

        /* integer Type Acct_type을 string 타입으로 변환 */
        sprintf(tmp_acctype,    "%.1d", exi0320f.out.acct_type);

        if (EXPARM->fil3[0] == '1') {
            //cr_acct_type SET
            memcpy(EXMSG1200->cr_acct_type, tmp_acctype,    strlen(tmp_acctype));
        }else if (EXPARM->fil3[0] == '2' || EXPARM->fil3[0] == '3'){
            //dr_acct_type SET
            memcpy(EXMSG1200->dr_acct_type, tmp_acctype, strlen(tmp_acctype));
        }

        /*--------------------------------------------------------------------
          ACCT_TYPE(계좌종류 구분필드)판단 모듈 호출 - END
        --------------------------------------------------------------------*/
    }else {
        ctx->kti_flag[0] = '0';          //초기값 0:GCG
        EXMSG1200->cr_acct_no[0] = '1';  //초기값 1:수신계좌 GCG
        EXMSG1200->dr_acct_no[0] = '1';  //초기값 1:수신계좌 GCG

    }


#ifdef _DEBUG
    /*------------------------------------------------------------------------ */
    SYS_DBG("================================================================");
    SYS_DBG("                         GCG ICG 계좌체크                         ");
    SYS_DBG("================================================================");
    SYS_DBG("ctx->kit_flag          : [%s]",   ctx->kti_flag                  );
    SYS_DBG("EXMSG1200->cr_acct_type: [%s]",   EXMSG1200->dr_acct_type        );
    SYS_DBG("EXMSG1200->cr_acct_type: [%s]",   EXMSG1200->cr_acct_type        );
    SYS_DBG("================================================================");
    /*------------------------------------------------------------------------ */
#endif

    SYS_TREF;

    return ERR_NONE;


}

/* ------------------------------------------------------------------------------------------------------------ */
static int g000_ix_skn_check(ixn0040_ctx_t  *ctx)
{
    int                 rc  = ERR_NONE;
    ixi0120f_t          ixi0120f;

    SYS_TRSF;

    /*------------------------------------------------------------------------ */
    SYS_DSP("g000_ix_skn_check : kftc_skn_chk[%s]",  EXPARM->kftc_skn_chk);
    /*------------------------------------------------------------------------ */

    if (EXPARM->kftc_skn_chk[0] != '1')
        return ERR_NONE;

    /*------------------------------------------------------------------------ */
    /* 결번검증 처리                                                              */
    /*------------------------------------------------------------------------ */
    memset(&ixi0120f, 0x00, sizeof(ixi0120f_t));

    /* Decoupling ------------------------------------------------------------ */
    ixi0120f.in.in_flag         = 1;        /* 0:취급업무   1:개설업무             */
    ixi0120f.in.max_flag        = 0;        /* 0:결번검증   1:MAX채번             */
    memcpy(ixi0120f.in.in_host_skn_chk, EXPARM->in_host_skn_chk, 1); /* 결번검증 여부 */
    memcpy(ixi0120f.in.in_kti_flag    , ctx->kti_flag          , 1); /* 시스템구분 0:GCG , 1:ICG */
    /* Decoupling ------------------------------------------------------------ */

    ixi0120f.in.exmsg1200   = EXMSG1200;

    SYS_TRY(ix_skn_chk(&ixi0120f));

    sys_tx_commit(TX_CHAINED);
    EXPARM->kftc_dup_chk[0] = ixi0120f.out.dup_chk_flag[0];

    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return GOB_ERR;

}


/* ------------------------------------------------------------------------------------------------------------ */
static int h000_ix_dup_check(ixn0040_ctx_t  *ctx)
{
    int                 rc  = ERR_NONE;
    char                err_code[LEN_ERR_CODE + 1];
    ixi0110x_t          ixi0110x;
    exi0230x_t          exi0230x;

    SYS_TRSF;

    /*------------------------------------------------------------------------ */
    SYS_DSP("h000_ix_dup_check : kftc_dup_chk[%s]",  EXPARM->kftc_dup_chk);
    /*------------------------------------------------------------------------ */

    if (EXPARM->kftc_dup_chk[0] != '1')
        return ERR_NONE;


    /*------------------------------------------------------------------------ */
    /* 중복거래 검증                                                              */
    /*------------------------------------------------------------------------ */
    memset(&ixi0110x, 0x00, sizeof(ixi0110x_t));
    ixi0110x.in.in_flag = 1;
    memcpy(ixi0110x.in.tx_date, EXMSG1200->tx_date, LEN_IXI0110X_TX_DATE);
    memcpy(ixi0110x.in.msg_no , EXMSG1200->msg_no , LEN_IXI0110X_MSG_NO );

    rc = ix_dup_chk(&ixi0110x);

    /*------------------------------------------------------------------------ */
    SYS_DBG("h000_ix_dup_check : rc[%d]",  rc);
    /*------------------------------------------------------------------------ */

    switch(rc){
    /*------------------------------------------------------------------------ */
    /* 중복거래인 경우                                                            */
    /* 중계센터로 "이중거래임"으로 답코드를 set하여 전송                                 */
    /*------------------------------------------------------------------------ */
    case ERR_NONE :
        /* ------------------------------------------------------- */
        /* 중계센터로 "이중거래임"으로                                   */
        /* 응답코드를 set하여 전송한다 .                                 */
        /* ------------------------------------------------------- */
        ctx->ixi0220x.in.msg_code   = '1';
        ctx->ixi0220x.in.send_flag  = '4';
        memcpy(ctx->ixi0220x.in.rspn_code, "309", LEN_RSPN_CODE);
        return ERR_ERR;

    /*------------------------------------------------------------------------ */
    /* 중복거래가 아닌 경우                                                         */
    /*------------------------------------------------------------------------ */
    case SYS_DB_NOTFOUND:
        break;

    /*------------------------------------------------------------------------ */
    /* 기타 DB ERROR인 경우                                                       */
    /* 중계 센터로 "은행센터의 system장애 "로 set하여 전송                              */
    /*------------------------------------------------------------------------ */
    default:
        /* ------------------------------------------------------- */
        /* 중계 센터로 "은행센터의 system장애 "                          */
        /* 로 set하여 전송                                            */
        /* ------------------------------------------------------- */
        ex_syslog(LOG_ERROR, "[APPL_DM] %s h000_ix_dup_check()"
                             "중복검증 ERROR"
                             "[해결방안]업무담당자 CALL",
                             __FILE__);
        ctx->ixi0220x.in.msg_code   = '1';
        ctx->ixi0220x.in.send_flag  = '4';
        memcpy(ctx->ixi0200x.in.rspn_code,  "413", LEN_RSPN_CODE);
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;


}
/* Decoupling ------------------------------------------------------------ */
/* EI대표전문 채번 : 대외기관에 전송하는 전문일련번호 채번                             */
/* TABLE        : IXMAX                                                    */
/* 은행코드       : 053                                                      */
/* 취급거래       : MAX(MAX_OUT_MSG_NO  + 1)                                 */
/* 개설거래       : MAX(MAX_KFTC_MSG_NO + 1)                                 */
/* Decoupling ------------------------------------------------------------ */
static int h100_max_msg_no_proc(ixn0040_ctx_t   *ctx)
{
    int                 rc = ERR_NONE;
    ixi0120f_t          ixi0120f;

    SYS_TRSF;
    
    
    /*------------------------------------------------------------------------ */
    SYS_DBG("h100_max_msg_no_proc START ");
    /*------------------------------------------------------------------------ */

    memset(&ixi0120f,   0x00, sizeof(ixi0120f_t));

    ixi0120f.in.in_flag         = 1;    /* 0:취급업무,  1:개설업무  */
    ixi0120f.in.max_flag        = 1;    /* 0:결번검증,  1:MAX채번  */
    memcpy(ixi0120f.in.in_host_skn_chk, EXPARM->host_skn_chk,   1); /* 결번 검증 여부 */
    memcpy(ixi0120f.in.in_kti_flag    , ctx->kti_flag       ,   1); /* 시스템구분 - 0:GCG, 1:ICG  */
    ixi0120f.in.exmsg1200   = EXMSG1200;

    rc = ix_skn_chk(&ixi0120f);

    if (rc == ERR_ERR){
        return ERR_ERR;
    }

    //
    //sys_tx_commit(TX_CHAINED);
    /*********************************************************************************/
    /* IXI0120F.pc를 호출후 채번된 일련번호                                               */
    /* ctx_ei_msg_no에 조립하여 관련 테이블 생성 및 갱신시 사용된다.                           */
    /*********************************************************************************/
    memcpy(ctx->ei_msg_no,  ixi0120f.out.out_max_ei_msg_no, LEN_IXI0120F_MAX_EI_MSG_NO);

    SYS_DBG("ixi0120f CALL AFTER ");
    SYS_DBG("ctx->ei_msg_no                 :[%s]", ctx->ei_msg_no);
    SYS_DBG("ixi0120f.out.out_max_ei_msg_no :[%s]", ixi0120f.out.out_max_ei_msg_no);


    /*  ---------------------------------------------------------------------- */
    /* exmsg1200.our_msg_no 를 ctx->out_msg_no에 조립하여 임시보관                  */
    /* exmsg1200.our_msg_no  금융결제원에 제공되는 번호이어서                          */
    /* exmsg1200.our_msg_no에는 IXI0120F.pc에서 채번된 일련번호를                    */
    /* 조립하고, ctx->ei_msg_no는 저널 생성시 our_msg_no에 조립하여                    */
    /* IXJRN을 생성할떄 사용하려고 함.                                               */
    /*  ---------------------------------------------------------------------- */




    SYS_DBG("ixi0120f CALL Result ");
    SYS_DBG("ctx->ei_msg_no                 :[%s]", ctx->ei_msg_no);
    SYS_DBG("ixi0120f.out.out_max_ei_msg_no :[%s]", ixi0120f.out.out_max_ei_msg_no);

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int i000_ixjrn_insert(ixn0040_ctx_t  *ctx)
{
    int                 rc = ERR_NONE;
    ixi0140x_t          ixi0140x;

    SYS_TRSF;

    /*------------------------------------------------------------------------ */
    SYS_DSP("i000_ixjrn_insert : host_send_jrn_make[%s]",  EXPARM->host_send_jrn_make);
    /*------------------------------------------------------------------------ */

    if (EXPARM->host_send_jrn_make[0] != '2')
        return ERR_NONE;


    /* ----------------------------------------------------------------------- */
    /* JRN 생성                                                                 */
    /* ----------------------------------------------------------------------- */
    memset(&ixi0140f,   0x00, sizeof(ixi0140f));
    ixi0140f.in.exmsg1200       = EXMSG1200;
    ixi0140f.in.ext_send_data   = ctx->ext_recv_data;

    /*
     *
    */
    memcpy(ixi0140f.in.in_new_flag  ,       "NEW",       , 3); /* NEW JRN 생성, INQ중복검증   */ 
    memcpy(ixi0140f.in.in_comm_flag ,       "R",         , 1); /* S취급거래    , R개설거래     */
    memcpy(ixi0140f.in.ei_msg_no    ,    ctx->ei_msg_no  ,10); /* 채번된 EI관리 일련번호        */
    memcpy(ixi0140f.in.in_orig_ei_msg_no    ,"0000000000",10); /* 원거래 EI관리 일련번호        */
    memcpy(ixi0140f.in.in_orig_proc_msg_no  ,"0000000000",10); /* 원거래 당행관리 일련번호       */

    /* 결번검증하지 않는 거래는 EI_MSG_NO는 0을 10개 채운다.  */
    if (EXPARM->host_skn_chk[0] != '1'){
        memcpy(ixi0140f.in.in_ei_msg_no,    "0000000000" ,10); /* EI대표전문번호              */
    }


    memcpy(ixi0140f.in.in_kti_flag, ctx->kti_flag,  LEN_IXI0140F_KTI_FLAG); /* 시스템구분 0:GCG, 1:ICG  */

    rc = ix_jrn_ins(&ixi0140f);

    switch(rc){
    case ERR_NONE:
        break;
    case TX_DUP:
        return GOB_ERR;
    default:

        ex_syslog(LOG_ERROR, "[APPL_DM] %s i000_ixjrn_insert()"
                             "IXJRN INSERT ERROR"
                             "[해결방안]ORACLE 담당자 CALL",
                             __FILE__);
        /* ------------------------------------------------------- */
        /* JRN생성의 결과가 error 이면                                 */
        /* 금융결제원으로 에러전문을 SEND한다.                            */
        /* ------------------------------------------------------- */
        ctx->ixi0220x.in.msg_code   = '1';
        ctx->ixi0220x.in.send_flag  = '4';
        memcpy(ctx->ixi0220x.in.rspn_code, "413", LEN_RSPN_CODE);

        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int j000_ix_host_saf_prod(ixn0040_ctx_t  *ctx)
{
    int                 rc  = ERR_NONE;
    ixi1060f_t          ixi1060f;

    SYS_TRSF;

    /*------------------------------------------------------------------------ */
    SYS_DSP("j000_ix_host_saf_prod : host_send_jrn_make[%s]",  EXPARM->host_send_jrn_make);
    /*------------------------------------------------------------------------ */    

    if (EXPARM->host_send_jrn_make[0] != '2'){

        /* ----------------------------------------------------------------------- */
        SYS_DSP("IXHSAF insert skip" );
        /* ----------------------------------------------------------------------- */
        
        return ERR_NONE;

    }


    /* ----------------------------------------------------------------------- */
    SYS_DSP("j000_ix_host_saf_prod : host_send_jrn_make[%s]",  EXPARM->host_send_jrn_make);
    /* ----------------------------------------------------------------------- */

    /* ----------------------------------------------------------------------- */
    /* HOST미전송  생성처리                                                        */
    /* ----------------------------------------------------------------------- */
    memset(&ixi1060f,   0x00, sizeof(ixi1060f_t));
    ixi1060f.in.exmsg1200       = EXMSG1200;

    /*
     *
    */
    memcpy(ixi1060f.in.ei_msg_no    ,    ctx->ei_msg_no  ,10); /* 채번된 EI관리 일련번호        */
    memcpy(ixi1060f.in.in_kti_flag  ,    ctx->kti_flag   , LEN_IXI1060F_KTI_FLAG ); /* 시스템구분 0:GCG, 1:ICG  */

    SYS_TRY(ix_host_saf_prod_prod(&ixi1060f));

#ifdef _DEBUG
    /*------------------------------------------------------------------------ */
    SYS_DBG("================================================================");
    SYS_DBG("      j000_ix_host_saf_prod : EXMSG1200                         ");
    SYS_DBG("================================================================");
    PRINT_EXMSG1200(EXMSG1200);
    PRINT_EXMSG1200_2(EXMSG1200);
    /*------------------------------------------------------------------------ */
#endif

    SYS_TREF;

    return ERR_NONE;

SYS_CATCH:

    ex_syslog(LOG_ERROR, "[APPL_DM] %s IXI0040: j000_ix_host_saf_prod()"
                         "SAF INSERT ERROR"
                         "[해결방안]ORACLE 담당자 CALL",
                          __FILE__);
    /* ------------------------------------------------------- */
    /* 금융결제원으로 에러전문을 SEND한다.                            */
    /* ------------------------------------------------------- */
    ctx->ixi0220x.in.msg_code       = '1';
    ctx->ixi0220x.in.send_flag      = '4';
    memcpy(ctx->ixi0220x.in.rspn_code, "413", LEN_RSPN_CODE);

    SYS_TREF;
    return ERR_ERR;

}
/* ------------------------------------------------------------------------------------------------------------ */
int static k000_kftc_err_send(ixn0040_ctx_t *ctx)
{
    int                 rc  = ERR_NONE;
    int                 len;
    char                ext_gw_svc_name[15];

    SYS_TRSF;

    /* ----------------------------------------------------------------------- */
    /* 결제원 에러응답 전문 조립                                                     */
    /* ----------------------------------------------------------------------- */
    if (ctx->kftc_err_set == 1){
        ctx->ixi0220x.in.ext_recv_data = ctx->ext_recv_data;
        ix_kftc_err_set(&ctx->ixi0220x);
    }

    len = ctx->ext_recv_len - 9;

    /* 대외기관 전송데이터 길이 검증     */
    if (len <= 0){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s IXI0020: k000_kftc_err_send()"
                             "EXT GW LEN ERROR [len=%d]"
                             "[해결방안]ORACLE 담당자 CALL",
                             __FILE__, len);
        return GOB_ERR;
    }

    /* 대외기관 G/W에 호출하는 방식을 전달하기 위한 값을 set */
    strcpy(SYSGWINFO->func_name, "X25IXIO");
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_ETC;        /* 전문종류 방식  */
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_SAF;  /* SAF 방식호출  */
    SYSGWINFO->rspn_flag    = SYSGWINFO_REPLY;          /* G/W로 부터의 응답   */

    SYSGWINFO->time_val     = utoa2ln(EXPARM->time_val, LEN_EXPARM_TIME_VAL);
    if (SYSGWINFO->time_val <= 0)                       /* SAF방식 타임 아웃   */
        SYSGWINFO->time_val = SYSGWINFO_SAF_DFLT_TIMEOUT;

#ifdef _SIT_DBG
    PRINT_IX_KFTC_DEBUG(ctx->ext_recv_data);
#endif

    /* output queue 에 데이터 저장  */
    rc = sysocbsi(ctx->cb, IDX_EXTSENDDATA, &ctx->ext_recv_data[9], len);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s IXI0040: k000_kftc_err_send()"
                        "COMMBUFF(EXTSENDDATA) SET ERROR"
                        "[해결방안]시스템 담당자 CALL",
                        __FILE__);
        return GOB_ERR;
    }


    /* 대외기관 데이터 전송   */
    rc = sys_tpcall("SYEXTGW_IX", ctx->cb, TPNOTRAN);
    
    strcpy(ext_gw_svc_name, "SYEXTGW_IX");
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s IXI0040: k000_kftc_err_send()"
                        "%s 서비스 호출 ERROR [%d:%d]: func_name[%s]"
                        "[해결방안]대외기관 G/W 담당자 CALL",
                        __FILE__, ext_gw_svc_name, tperrno, sys_err_code(),
                        SYSGWINFO->func_name);
        return GOB_ERR;
    }

    /* 결제원 송신 전문 로깅 */
    b000_msg_logging(ctx, KFTC_SEND_LOG);

    SYS_TREF;

    return ERR_NONE;

}
/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */