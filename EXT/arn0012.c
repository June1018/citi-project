
/*  @file               arn0012.c
*   @file_type          pc source program
*   @brief              ARS 개설 입금 main
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
 *        a000 : CONTEXT 초기화 및 Commbuff검증 
 *        b000 : 대외기관 수신로그 처리 
 *        c000 : 대외기관 수신전문 검증 
 *        d000 : 거래코드 변환 
 *        e000 : 거래파라미터 LOAD 
 *        f000 : 관리전문 검증  
 *        g000 : KTI송신전문 초기화 
 *        h000 : 거래고유번호 채번 
 *        i000 : 중복거래 검증 
 *        j000 : 결번조회 데이터 생성   
 *        k000 : KTI송신전문으로 변환
 *        l000 : KTI전문 송/수신 
 *        m000 : KTI응답전문 검증 
 *        n000 : 대외기관 송신전문으로 변환 응답코드 변환 
 *        o000 : 저널 생성
 *        p000 : 취소거래시 저널 반영
 *        q000 : 수표사고신고 결번 거래 saf생성
 *        r000 : SAF응답여부 반영 
 *        s000 : 집계반영 
 *        t000 : 관리전문 반영
 *        u000 : 대외기관으로 전문전송 
 *
 *        a000 CONTEXT 초기화 
 *             COMMBUFF 검증 
 * 
 *        b000 
 *             1. KTI송신용 EXMSG1200초기화 및 Commbuff set 
 *             2. 대외기관 수신전문 logging (aro2000x, ARKULOG INSERT)
 *             3. 사고 수표조회거래 (302500, 3035000)인 경우 ARKULOG INSERT        
 *
 *        c000 
 *             1. 대외기관 수신전문 field검증 (aro2130x) 
 *             2. 오류발생시 대외기관으로 오류 전문 전송후 거래 종료 
 *                - field검증시 오류에 대한 응답코드는 aro2130x에서 set 
 *        
 *        l000 
 *             1. KTI로 전문송신 
 *             2. KTI로부터 응답수신 
 *             3. Time out 수신오류인 경우 미완료 생성(aro2950x)
 *             4. KTI로부터 수신한 EXMSG1200 Commbuff set
 *                - 이후 function에서는 EXMSG1200 commbuff는 KTI응답전문 
 *             5. ARUHLOG, ARHULOG logging
 *
 *        m000 
 *             1. 1200응답전문  검증(aro2120x)
 *             2. 검증오류 발생시 미완료 생성(aro2950f) 
 *     
 *        n000 
 *             1. 대외기관 송신전문으로 변환 (exo4000x)
 *             2. 검증오류 발생시 미완료 생성(aro2950f)
 *             3. 응답코드 변환(exo0230x)
 *             4. 주택청약 거래이면서 278에러인 경우 에러코드 재처리 861
 *             5. 응답코드가 000이 아닌경우 대외기관으로 오류 전문 전송후 거래 종료 
 *
 *        o000 
 *             1. 저널 생성 
 *             2. 거래 parameter의 host_send_jrn_make_field값에 따라 
 *                (2: ARJRN2 - aro2520f, 3:ARJRN3 -aro2530f, 4:ARJRN4 - aro2540f)
 *             3. 오류발생시 미완료 생성(aro2950f)
 *      
 *        p000 
 *             1. 취소 거래시 저널 반영(aro2610f, aro2620f)
 *             2. 오류발생시 미완료 생성 
 *
 *        q000 
 *             1. 자기앞 사고 결번, 가계사고 결번 거래인 경우 saf생성(aro2930f, aro2940f)
 *             2. saf조회 실패시 KTI통신 후 결과 데이터 saf생성 
 *             3. 오류 발생시 미완료 생성(aro2950f)
 *        
 *        r000 
 *             1. SAF응답여부가 1인경우 (FIL2) SAF UPDATE (aro2900f) - 가계수표, 자기앞수표 사고 결번, 
 *             2. 오류 발생시 미완료 생성(aro2950f)
 *
 *        s000 
 *             1. 거래 파라미터 tot_pod_flag_1값에 따라 집계 처리 
 *                (2: aro2720f,  4: aro2740f, 5:aro2750f)
 *             2. 거래 파라미터 tot_pod_flag_2값에 따라 집계 처리 
 *                (4: aro2810f, 5:aro2820f)
 *
 *        t000 
 *             1. 관리전문 반영 (027, 099) 반영 (exo0240f)
 *
 *        u000 
 *             1. 대외기관으로 전문 전송 
 *             2. ARUKLOG logging (aro2000x)
 *
 *        z000
 *             1. 오류시 대외기관 전문 전송 
 *             
 *        z100
 *             1. KTI송신 이후 오류 발생시 미완성 생성   
 *
 *        z200
 *             1. ARUHLOG, ARHULOG, ARKULOG, ARUKLOG, INSERT 
 *
 *
 *
 *
 *
 */


/* ---------------------------------------------- include files ----------------------------------------------- */
#include <syscom.h>
#include <sysconst.h>
#include <utodate.h> 
#include <exdefine.h>
#include <exmsg1200.h>
#include <exmsg1200_ar34.h>
#include <exmsg1200_ar35.h>
#include <exi0230x.h>
#include <exi0240x.h>
#include <exi0280x.h>
#include <exi0350x.h>
#include <exi4000x.h>
#include <exmsg4000.h>
#include <ardefine.h>
#include <armsgkftc.h>
#include <armsg0201q.h>
#include <ari2120x.h>
#include <ari2130x.h>
#include <ari2520f.h>           /* jrn2 create */
#include <ari2530f.h>           /* jrn3 create */
#include <ari2540f.h>           /* jrn4 create */
#include <ari2550f.h>           /* jrnkti create */
#include <ari2610f.h>           /* jrn3 update */
#include <ari2620f.h>           /* jrn4 update */
#include <ari2720f.h>           /* artot2 update */
#include <ari2740f.h>           /* artot4 update */
#include <ari2750f.h>           /* artot5 update */
#include <ari2810f.h>
#include <ari2820f.h>
#include <ari2840f.h>
#include <ari2900f.h>           /* SAF update   */
#include <ari2930f.h>           /* SAF3 생성     */
#include <ari2940f.h>           /* SAF4 생성     */
#include <ari2950f.h>           /* SAF5 생성     */
#include <ari3100f.h>           /* ar_log ars insert serivce interface structure  */
#include <hcmihead.h>
#include <iconv.h>


/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define     SET_ERR_RSPN(X) do {   \
                            memcpy(((armsgcomm_t *)ctx->ext_send_date)->rspn_code, X, 3)
                            }
/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct arn0012_ctx_s    arn0012_ctx_t;
struct arn0012_ctx_s {
    commbuff        *cb;  
    int             err_ext_send_flag               /* GOB_NRM시 대외기관 오류 전송여부 1:send, 0:not send    */
    char            ext_send_data[2048];            /* 대외기관 송신용 bufffer     */
    exmsg1200_t     kti_recv_data;                  /* KTI 수신 data            */
    int             log_type_add;                   /* 1: ARKULOG   2: ARUKLOG   3: ARHULOG   4: ARUHLOG   --> 0 
                                                      11: ARKULOGC 12: ARUKLOGC 13: ARHULOGC 14: ARUHLOGC  --> 10 */
    int             saf_create_flag;                /* 1:saf_create  KTI통신후 flag겸          */
    int             append_index;                   /* 파라미터의 필러로 부터 가지고 오는 값으로      */
                                                    /* 대외기관 전문의 해당위치 이후 부터 호스트 전송시 1200이후 붙임          */
    int             append_size;                    /* 1200이후에 추가될 전문의 길이  */

};



/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int          a000_data_receive(arn0012_ctx_t *ctx, commbuff_t    *commbuff);
static int          b000_init_proc(arn0012_ctx_t *ctx);
static int          b100_kti_recv_logging(arn0012_ctx_t *ctx);
static int          c000_host_rspn_chk(arn0012_ctx_t *ctx);
static int          d000_ext_msg_unformat(arn0012_ctx_t *ctx);
static int          d200_err_conv(arn0012_ctx_t *ctx);
static int          e000_jrn_create(arn0012_ctx_t *ctx);
static int          f000_canc_jrn_update(arn0012_ctx_t *ctx);
static int          g000_saf_create(arn0012_ctx_t *ctx);
static int          h000_saf_update(arn0012_ctx_t *ctx);
static int          i000_tot_proc(arn0012_ctx_t *ctx);
static int          j000_net_mgr_proc(arn0012_ctx_t *ctx);
static int          k000_ext_send_msg(arn0012_ctx_t *ctx);
static int          z000_err_ext_send(arn0012_ctx_t *ctx);
static int          z100_log_insert(arn0012_ctx_t *ctx, char *log_data, int size, int log_type);
static int          utoiconv(char *fcode, char *tcode, char *in, size_t in_len, char *out, size_t out_lne);

/* ------------------------------------------------------------------------------------------------------------ */
int arn0012(commbuff_t  *commbuff)
{

    int                 rc = ERR_NONE;
    arn0012_ctx_t       _ctx;  
    arn0012_ctx_t       *ctx = &_ctx;   

    SYS_TRSF;

    /* CONTEXT 초기화  */
    SYS_TRY(a000_data_receive(ctx, commbuff));

    /* 대외기관 수신로그 처리  */
    SYS_TRY(b000_init_proc(ctx));

    /* KTI응답 전문 검증  */
    SYS_TRY(c000_host_rspn_chk(ctx));

    /* 대외기관 송신전문으로  변환   */
    SYS_TRY(d000_ext_msg_unformat(ctx));

    /* 저널생성   */
    SYS_TRY(e000_jrn_create(ctx));

    /* 취소거래시 저널 반영    */
    SYS_TRY(f000_canc_jrn_update(ctx));

    /* 가계수표, 자기앞 수표, 결번조회시 saf생성  */
    SYS_TRY(g000_saf_create(ctx));

    /* 가계수표, 자기앞 수표, 결번조회시 saf반영   */
    SYS_TRY(h000_saf_update(ctx));

    /* 거래집계 반영   */
    SYS_TRY(i000_tot_proc(ctx));

    /*  관리 전문 반영 */
    SYS_TRY(j000_net_mgr_proc(ctx));

    /* 대외기관 전문 송신  */
    rc = k000_ext_send_msg(ctx);

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    /* GOB_NRM : return ERR_NONE   -- > 정상 commit */
    /* ERR_ERR : return ERR_ERR    -- > rollback   */
    /*
     * 오류시 대외기관 전문 전송여부는 ctx->err_ext_send_flag로 판단 
     * 0: 미전송 1:전송  
    */

    SYS_DBG("ERR_EXT_SEND_FLAG[%d]", ctx->err_ext_send_flag);
    if (ctx->err_ext_send_flag == 1) {
        z000_err_ext_send(ctx);
    }

    if (rc == GOB_NRM){
        /* service shell commit  */
        return ERR_NONE;
    }
    
    SYS_TREF;
    /* service shell rollback  */
    return ERR_ERR;
}



/* ------------------------------------------------------------------------------------------------------------ */
static int          a000_data_receive(arn0012_ctx_t *ctx, commbuff_t    *commbuff)
{
    int      rc = ERR_NONE;

    SYS_TRSF;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(arn0012_ctx_t));
    ctx->cb = commbuff;

    ctx->append_index = 0;

    if (HOSTRECVDATA == NULL) {
        SYS_HSTERR(SYS_NN, SYS_GENERR, "HOSTRECVDATA IS NOT FOUND");
        return GOB_NRM;
    }

    /* 입력 채널 clear */
    SYSICOMM->intl_tx_flag = 0;


#ifdef _DBGUG

    SYS_DBG("RCV_DATA[%.*s]", sysocbgs(ctx->cb, IDX_HOSTRECVDATA), sysocbgs(ctx->cb, IDX_HOSTRECVDATA), sysocbgp(ctx->cb, IDX_HOSTRECVDATA));

#endif

    SYS_TREF;

    return ERR_NONE;
}


/* ------------------------------------------------------------------------------------------------------------ */
static int b000_init_proc(arn0012_ctx_t   *ctx)
{
    int                 rc  = ERR_NONE;
    int                 pay_cnt;
    char                detl_pay_cnt[LEN_EXMSG1200_AR35_DETL_PAY_CNT + 1];
    exmsg4000_t         *exmsg4000;
    ari2130x_t          ari2130x;
    exi0350f_t          exi0350f;

    SYS_TRSF;

    /* KTI 응답 전문 저장  */
    memcpy(&ctx->kti_recv_data, HOSTRECVDATA,   LEN_EXMSG1200);

    /* KTI에서 응답받은 1200msg set */
    rc = sysocbsi(ctx->cb, IDX_EXMSG1200, HOSTRECVDATA, sizeof(exmsg1200_t));
    if (rc == ERR_ERR) {
        SYS_HSTERR(SYS_LC, SYS_GENERR, "KTI MSG COMMBUFF ERROR ");
        return ERR_ERR;
    }

#ifdef _DBGUG
    SYS_DBG("호스트 수신 전문");
    PRINT_EXMSG1200((exmsg1200_t *) HOSTRECVDATA);
    PRINT_EXMSG1200_2((exmsg1200_t *) HOSTRECVDATA);
#endif

    /* --------------------------------------------------------------------------- */
    /* 대외기관 수신전문 조회                                                           */
    /* --------------------------------------------------------------------------- */
    memset(ctx->ext_send_data, 0x00, sizeof(ctx->ext_send_data));
    memset(&exi0350f,   0x00, sizeof(exi0350f_t));

    exi0350f.in.proc_type   = 1;
    memcpy(exi0350f.in.proc_date,   EXMSG1200->tx_date,     LEN_EXI0350F_PROC_DATE);
    memcpy(exi0350f.in.appl_code,   AR_CODE,                LEN_EXI0350F_APPL_CODE);
    memcpy(exi0350f.in.kftc_msg_no, &EXMSG1200->msg_no[1],  LEN_EXI0350F_KFTC_MSG_NO);

    rc = ex_kftc_r_data_proc(&exi0350f);

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s ars0011: d000_ext_msg_unformat();"
                             "ex_kftc_r_data_proc(select) ERROR,",__FILE__ );
        SET_ERR_RSPN("188");
        return ERR_ERR;
    }

    memcpy(ctx->ext_send_data,  exi0350f.out.kftc_recv_data, strlen(exi0350f.out.kftc_recv_data));
    //SYS_DBG("DB KFTC RECV DATA [%s]", ctx->ext_recv_data );

    /* 1200을 넘는 전문의 경우 파라미터의 여분8을 필드의 위치에 1200뒤의 데이터를 붙여서 전송 */
    //여분의 트림값이 숫자인 경우에만 
    if (utochknm(utotrim(EXPARM->fil8), strlen(utotrim(EXPARM->fil8)))  == SYS_TRUE){
        ctx->append_index = utoa2in(EXPARM->fil8, LEN_EXPARM_FIL8);

        if (ctx->append_index != 0){
            pay_cnt = 0;
            memset(detl_pay_cnt,    0x00, sizeof(detl_pay_cnt));

            //현재일괄납부 건수는 detl_area 86번째 위치함 추후 다른 경우가 있으면 if문 추가 해야함 
            /* memcpy(detl_pay_cnt, EXMSG1200->detl_area + 86, LEN_EXMSG1200_AR35_DETL_PAY_CNT); 

            pay_cnt = atoi(detl_pay_cnt);
            SYS_DBG("일괄납부 건수[%d]", detl_pay_cnt);

            ctx->append_size = 90 * pay_cnt;
            */
            //일괄납부인 경우 일괄데이터 크기 구함 
            ctx->append_size = strlne(&ctx->ext_send_data[ctx->append_index]);

            exmsg4000 = (exmsg4000_t *)HOSTRECVDATA;

            SYS_DBG("send to KFTC ctx->append index [%d]", ctx->append_index);
            SYS_DBG("send to KFTC ctx->append size  [%d]", ctx->append_size);
            SYS_DBG("exmsg4000->over1200  [%.*s]", ctx->append_size, exmsg4000->over1200);
        }
    }

    if ((memcmp(EXMSG1200->tx_code, "6728100092", 10) == 0) ||   /* 자기앞수표 사고 조회 */
        (memcmp(EXMSG1200->tx_code, "6728100192",  6) == 0)) {    /* 가계  수표 사고 조회 */

        ctx->log_type_add = 10;

    }else{

        ctx->log_type_add =  0;
    }

    /* --------------------------------------------------------------------------- */
    /* 대외기관 수신전문 삭제                                                           */
    /* --------------------------------------------------------------------------- */
    exi0350f.in.proc_type   = 3;
    rc = ex_kftc_r_data_proc(&exi0350f);

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s ars0011: d000_ext_msg_unformat();"
                             "ex_kftc_r_data_proc(select) ERROR,",__FILE__ );
        SET_ERR_RSPN("188");
        return ERR_ERR;
    }

    /* --------------------------------------------------------------------------- */
    /* KTI 수신전문 logging                                                          */
    /* --------------------------------------------------------------------------- */
    rc = b100_kti_recv_logging(ctx);
    if (rc == ERR_ERR){
        return ERR_ERR;
    }

    /* --------------------------------------------------------------------------- */
    /* 오류시 대외기관 전문전송 여부 설정                                                  */
    /* --------------------------------------------------------------------------- */
    memset(&ari2130x,   0x00, sizeof(ari2130x_t));
    ari2130x.in.armsg       = ctx->ext_send_data;
    ari2130x.in.data_len    = strlen(ctx->ext_send_data);

    rc = ar_kftc_fld_chk(&ari2130x);

    /* 오류시 대외기관 전문전송여부는 aro2130x에서 set    */
    ctx->err_ext_send_flag = ari2130x.out.err_ext_send_flag;
    if (rc == ERR_ERR){
        return ERR_ERR;
    }

    SYS_DBG("ctx->err_ext_send_flag[%d]", ari2130x.out.err_ext_send_flag)

    SYS_TREF;
    
    return ERR_NONE;
    
}

/* ------------------------------------------------------------------------------------------------------------ */
static int b100_kti_recv_logging(arn0012_ctx_t  *ctx)
{

    int                 rc = ERR_NONE;
    exi0280_t           exi0280;
    exmsg4000_t         exmsg4000;

    SYS_TRSF;

    /* logging KTI -> UNIX 
     * KTI와 거래 로그를 생략 추가 
     * 로그사이즈가 2048이 넘을 경우 메모리 dump발생됨, 그래서 skip시킴
     #define LEN_EXI0280_DATA       2048
    */
    if (EXPARM->fil5[0] == '1'){
        return ERR_NONE;
    }

    /* KTI송신 1200전문 null trim */
    memset(&exi0280,    0x00,   sizeof(exi0280_t));
    exi0280.in.type  = 2;       /* NULL TRIM     */
    memcpy(exi0280.in.data, HOSTRECVDATA,   sizeof(exmsg1200_t));

    rc = exmsg1200_null_proc(&exi0280);
    if (rc == ERR_ERR){
        SET_ERR_RSPN("188");
        return ERR_ERR;
    }

    /*
    1200넘는 부분까지 로깅을 위해서 append
    strlen(exi0280.out.data)에 추가분 데이터 사이즈가 포함됨에 유의.....
    기존KFTC에서 반은 1200초과되는 전문을 그대로 사용하는 것으로 변경 
    exmsg4000 = (&exmsg4000 *)HOSTRECVDATA;
    memcpy(&exi0280x.out.data)[sizeof(exmsg1200_comm_t)],  exmsg4000->over1200, ctx->append_size);
    */
    memcpy(&exi0280.out.data[sizeof(exmsg1200_comm_t)],  ctx->ext_send_data + ctx->append_index, ctx->append_size);

    SYS_DBG("strlen(exi0280.out.data)[%d]",strlen(exi0280.out.data));
    SYS_DBG("ctx->append_size[%d]", ctx->append_size);

    SYS_DBG("exi0280.out.data[%d][%.*s]", strlen(exi0280.out.data), strlen(exi0280.out.data)+ctx->append_size, exi0280.out.data);

    rc = z100_log_insert(ctx, exi0280.out.data, strlen(exi0280.out.data), 3 + ctx->log_type_add);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s ars0011: b100_kti_recv_logging();"
                             "AR_LOG_ERR log_type [%d],",__FILE__, 3 + ctx->log_type_add );
        
        sys_err_init();
        
    }

    SYS_TREF;

    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
static int c000_host_rspn_chk(arn0012_ctx_t *ctx)
{

    int                 rc = ERR_NONE;
    ari2120x_t          ari2120x;

    /* Host 응답전문 검증 */
    SYS_TRSF;

    if (EXPARM->host_comm_flag[0] != '1'){
        return ERR_NONE;
    }

    /*
    memset(&ari2120x,   0x00, sizeof(ari2120x_t));
    ari2120x.in.send1200    = (exmsg1200_t *)HOSTSENDDATA;
    ari2120x.in.recv1200    = &ctx->kti_recv_data;
    ari2120x.in.detl_type   = EXPARM->detl_msg_type;

    rc = ar_msg1200_rspn_chk(&ari2120x);
    if (rc == ERR_ERR){
        SET_ERR_RSPN("330");
        return ERR_ERR;
    }
    */



    SYS_TREF;

    return ERR_ERR;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int d000_ext_msg_unformat(arn0012_ctx_t    *ctx)
{
    int                 rc  = ERR_NONE;
    int                 proc_type;


    exi4000x_t          exi4000x;
    

    SYS_TRSF;

    if (EXPARM->host_comm_flag[0] != '1'){
        return ERR_NONE;
    }

    /* 대외기관 전문으로 unformat    */
    memset(&exi4000x, 0x00, sizeof(exi4000x_t));

    exi4000x.in.type            = EXI4000X_RES_MAPP;
    exi4000x.in.msg_len         = LEN_EXMSG1200;  
    exi4000x.in.base_len        = strlen(ctx->ext_send_data);
    exi4000x.in.inp_conv_flag   = EXI4000X_NULL_CONV;

    memcpy(exi4000x.in.msg      ,   EXMSG1200,          LEN_EXMSG1200);
    memcpy(exi4000x.in.base_msg ,   ctx->ext_send_data, strlen(ctx->ext_send_data));
    memcpy(exi4000x.in.appl_code,   AR_CODE           ,  LEN_APPL_CODE);
    memcpy(exi4000x.in.tx_code  ,   EXMSG1200->tx_code, LEN_TX_CODE);

    rc = ex_format(&exi4000x);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s n000_ext_msg_unformat: ex_format ERROR ",
                             __FILE__);
        SET_ERR_RSPN;
        return ERR_ERR;
    }

    /* 변환된 전문을 ext_send_data*/
    memcpy(ctx->ext_send_data, exi4000x.out.msg, LEN_TX_CODE);

    SYS_DSP(" exi4000x.out.msg [%.*s]", exi4000x.out.msg_len, exi4000x.out.msg);
    /*------------------------------------------------------------------------ */
    /* 1200전문을 넘는 경우                                                       */
    /* 파라미터의 여분 8필드의 위치에 1200뒤 데이터를 붙여서 전송                          */
    /*------------------------------------------------------------------------ */

    if (utochknm(utotrim(EXPARM->fil8), strlen(utotrim(EXPARM->fil8))) == SYS_TRUE){
        ctx->append_index = utoa2in(EXPARM->fil8, LEN_EXPARM_FIL8);
        if (ctx->append_index != 0){
            ctx->ext_send_data[ctx->append_index + ctx->append_size] = '\0';

            // 기존KFTC에서 반은 1200전문 초과되는 전문을 그대로 사용하는 것으로 변경 
            /* exmsg4000_t *exmsg4000;
               exmsg4000 = (exmsg4000_t *) HOSTRECVDATA;

               strncpy(&ctx->ext_send_data[ctx->append_index], exmsg4000->over1200, ctx->append_size);


            */


            SYS_DBG("SENDDATA to KFTC[%s]", ctx->ext_send_data);
        }
    }


    /*------------------------------------------------------------------------ */
    /* 주택청약순위 확인서 발급 / 조회 (tx_id = 304000)                               */
    /* 동일한 매핑으로 변환하되 MGR_TYPE가 02 or 03인 경우 inq_rec_1까지만 전송           */
    /*                    MGR_TYPE가 02 or 03인 아닌 경우 inq_rec_2까지만 전송       */
    /* 전문 전송길이는 strlen 함수를 사용하는 null까지만 읽어서 간다.                      */
    /*------------------------------------------------------------------------ */
    if (strncmp(EXMSG1200->tx_code, "6344100092", 10) == 0){
        if (strncmp( &ctx->ext_send_data[66], "02", 2) == 0 ||
            strncmp( &ctx->ext_send_data[66], "03", 2) == 0 ||
            strncmp( &ctx->ext_send_data[66], "05", 2) == 0 ){
            ctx->ext_send_data[295] = 0x00;
        }
    }
    /*------------------------------------------------------------------------ */

    rc = d200_err_conv(ctx);

    SYS_TREF;
    return rc; 

SYS_CATCH:
    SYS_TREF;

    return ERR_ERR;


}
/* ------------------------------------------------------------------------------------------------------------ */
static int d200_err_conv(arn0012_ctx_t      *ctx)
{
    int                 rc  = ERR_NONE;
    exi0230x_t          exi0230x;
    armsgcomm_t         *armsgcomm;

    SYS_TRSF;

    if (EXPARM->host_comm_flag[0] != '1'){
        return ERR_NONE;
    }

    if (memcmp(ctx->kti_recv_data.err_code, "0000000", LEN_EXMSG1200_ERR_CODE) == 0){
        return ERR_NONE;
    }

    memset(&exi0230x,   0x00,   sizeof(exi0230x_t));
    exi0230x.in.conv_flag[0] = EXI0230X_HOST_TO_EXT;
    strcpy(exi0230x.in.appl_code,       AR_CODE);
    strcpy(exi0230x.in.sub_appl_code,   "00");
    memcpy(exi0230x.in.err_code,        ctx->kti_recv_data.err_code,    LEN_EXI0230X_ERR_CODE);

    rc = ex_err_conv(&exi0230x);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%.7s: ARN0012 : d200_err_conv() "
                             " ex_err_conv ERROR", __FILE__);
        SET_ERR_RSPN("188");
        return ERR_ERR;
    }


    /* -------------------------------------------- */
    /* 주택청약 거래이면 278 에러인 경우 에러코드 재처리 861  */    
    /* 은행 사용 불가 응답코드(830)은 기타거래불가(861)로    */
    /* 응답처리함                                     */
    /* -------------------------------------------- */
    if ((memcmp(exi0230x.out.rspn_code, "278", 3) == 0) ||
        (memcmp(exi0230x.out.rspn_code, "830", 3) == 0)){
        if ((memcmp(EXPARM->tx_code, "6343100092", LEN_TX_CODE) == 0) ||
            (memcmp(EXPARM->tx_code, "6554100092", LEN_TX_CODE) == 0) ||
            (memcmp(EXPARM->tx_code, "6344100092", LEN_TX_CODE) == 0) ||
            (memcmp(EXPARM->tx_code, "6345100092", LEN_TX_CODE) == 0) ||
            (memcmp(EXPARM->tx_code, "6346100092", LEN_TX_CODE) == 0) ||
            (memcmp(EXPARM->tx_code, "6347100092", LEN_TX_CODE) == 0) ){
                memcpy(exi0230x.out.rspn_code, "861",   3);
        }
    }

    //rspn_code = 000으로 임시설정 (테스트)
    //memcpy(exi0230x.out.rspn_code,    "287",  3);

    /* ext_send_data DATA의 output rspn_code set */
    armsgcomm = (armsgcomm_t *)ctx->ext_send_data;
    memcpy(armsgcomm->rspn_code,    exi0230x.out.rspn_code, 3);

    /* -------------------------------------------- */
    /* 응답코드가 "000"이 아닐경우 처리                   */
    /* rollback,begin, return SUCCESS(JRN,TOT 미처리) */
    /* -------------------------------------------- */
    if ((memcmp(exi0230x.out.rspn_code, "000", 3) != 0) &&
        (EXPARM->rspn_cmp_code[0] == '1')){

        SYS_DBG("SAF CREATE SKIP RETURN ERR_ERR");
        /* KTI정상거래가 아닌경우 saf생성하지 않음  */
        ctx->saf_create_flag = 0;

        /* 응답코드가 000 아니고 JRN, TOT 미반영 거래인 경우 거래 종료  */
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;
}


/* ------------------------------------------------------------------------------------------------------------ */
static int e000_jrn_create(arn0012_ctx_t   *ctx)
{

    int                 rc  = ERR_NONE;
    ari2520f_t          ari2520f;
    ari2530f_t          ari2530f;
    ari2540f_t          ari2540f;
    ari2550f_t          ari2550f;
    armsgcomm_t         *armsgcomm;
    hcmihead_t          hcmihead;

    SYS_TRSF;
    /* EXPARM host send jrn make가 2,3,4인겨우 만 처리 
       2:사고수표저널처리 3:지로 저널, 4:공공요금 저널   
    */

    if ((EXPARM->host_send_jrn_make[0] != '2') &&
        (EXPARM->host_send_jrn_make[0] != '3') &&
        (EXPARM->host_send_jrn_make[0] != '4')){
        return ERR_NONE; 
    }

    armsgcomm = (armsgcomm_t *)ctx->ext_send_data;

    /* 사고수표 저널   */
    if (EXPARM->host_send_jrn_make[0] == '2'){
        memset(&ari2520f,   0x00,   sizeof(ari2520f_t));
        memcpy(ari2520f.in.kftc_send_data,  armsgcomm->msg_send_data, LEN_ARMSGCOMM_MSG_SEND_DATE);
        ari2520f.in.sys_type  = '1';
        ari2520f.in.exmsg1200 = EXMSG1200;
        rc = ar_jrn2_create(&ari2520f);
    }
    else if (EXPARM->host_send_jrn_make[0] == '3'){
        memset(&ari2530f,   0x00,   sizeof(ari2530f_t));
        memcpy(ari2530f.in.kftc_send_data,  armsgcomm->msg_send_data, LEN_ARMSGCOMM_MSG_SEND_DATE);
        ari2530f.in.sys_type  = '1';
        ari2530f.in.armsg     = ctx->ext_send_data;
        ari2530f.in.exmsg1200 = EXMSG1200;
        rc = ar_jrn3_create(&ari2530f);
    }
    else if (EXPARM->host_send_jrn_make[0] == '4'){
        memset(&ari2540f,   0x00,   sizeof(ari2540f_t));
        memcpy(ari2540f.in.kftc_send_data,  armsgcomm->msg_send_data, LEN_ARMSGCOMM_MSG_SEND_DATE);
        ari2540f.in.sys_type  = '1';
        ari2540f.in.armsg     = ctx->ext_send_data;
        ari2540f.in.exmsg1200 = EXMSG1200;
        rc = ar_jrn4_create(&ari2534f);
    } 

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%.7s: ARN0012 : e000_jrn_create() "
                             " ar_jrn%c create ERROR", __FILE__, EXPARM->host_send_jrn_make[0]);
    
        SET_ERR_RSPN("189");
        return ERR_ERR;
    }

    /* -------------------------------------------- */
    /* arjrnkti 저널 생성                             */
    /* -------------------------------------------- */
    memset(&ari2550f,   0x00, sizeof(ari2550f_t));

    //TCP_HEAD의 queue_name의 corr_id
    hcmihead = (hcmihead_t *)TCPHEAD;

    SYS_DBG("hcmihead->queue_name[%s]", hcmihead->queue_name);

    ar_data_convert(ari2550f.in.proc_date,  armsgcomm->msg_send_data);
    //
    memcpy(ari2550f.in.kftc_msg_no,     &EXMSG1200->msg_no[1],  LEN_ARI2550F_KFTC_MSG_NO);
    memcpy(ari2550f.in.corr_id,         hcmihead->queue_name ,  LEN_ARI2550F_CORR_ID);
    rc = ar_jrnkti_create(&ari2550f);

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%.7s: ARN0012 : e000_jrn_create() "
                             " ar_jrnkti create ERROR", __FILE__);
    
        SET_ERR_RSPN("189");
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;


}

/* ------------------------------------------------------------------------------------------------------------ */
static int f000_canc_jrn_update(arn0012_ctx_t    *ctx)
{
    int                 rc  = ERR_NONE;
    ari2610f_t          ari2610f;
    ari2620f_t          ari2620f;
    armsgcomm_t         *armsgcomm;

    /* 취소거래 시 jrn update    */
    SYS_TRSF;

    if ((EXPARM->canc_jrn_upd[0] != '3') &&         /* 지로 취소  */
        (EXPARM->canc_jrn_upd[0] != '4')){          /* 공공 취소  */
        return ERR_NONE; 
    }

    armsgcomm = (armsgcomm_t *)ctx->ext_send_data;

    if (EXPARM->canc_jrn_upd[0] == '3'){
        memset(&ari2610f,   0x00,   sizeof(ari2610f_t));
        memcpy(ari2610f.in.kftc_send_data,  armsgcomm->msg_send_data, LEN_ARMSGCOMM_MSG_SEND_DATE);
        ari2610f.in.exmsg1200 = EXMSG1200;

        rc = ar_jrn3_update(&ari2610f);
    }
    else if (EXPARM->canc_jrn_upd[0] == '4'){
        memset(&ari2620f,   0x00,   sizeof(ari2620f_t));
        memcpy(ari2620f.in.kftc_send_data,  armsgcomm->msg_send_data, LEN_ARMSGCOMM_MSG_SEND_DATE);
        ari2620f.in.exmsg1200 = EXMSG1200;

        rc = ar_jrn4_update(&ari2620f);
    } 

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%.7s: ARN0012 : f000_canc_jrn_update() "
                             " ar_jrn%c update ERROR", __FILE__, EXPARM->canc_jrn_upd[0] );
    
        SET_ERR_RSPN("189");
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int g000_saf_create(arn0012_ctx_t  *ctx)
{

    int                 rc  = ERR_NONE;
    ari2930f_t          ari2930f;
    ari2940f_t          ari2940f;

    /* 
      자기앞 수표, 가계 수표, 결번조회시 saf조회 실패한 경우 KTI통신함 
      KTI통신한 결과를 saf에 저장 
     */
    SYS_TRSF;

    if ((EXPARM->saf_tab_type[0] != '3') &&         /* 자기앞 수표 결번  */
        (EXPARM->saf_tab_type[0] != '4')){          /* 가계 수표 결번   */
        return ERR_NONE; 
    }

    if (EXPARM->saf_tab_type[0] == '3'){
        if (memcmp(EXMSG1200->err_code, "0000000", 7) != 0){
            EXPARM->saf_upd_flag[0] = '0';
            EXPARM->tot_pod_flag_1[0] = '0';   
        }
        else{
            memset(&ari2930f,   0x00,   sizeof(ari2930f_t));
            ari2930f.in.ar0200c,  (armsg0200c_t *)ctx->ext_send_data;
            ari2930f.in.exmsg1200 = &ctx->kti_recv_data;

            rc = ar_saf3_create(&ari2930f);

        }
    }
    else if (EXPARM->saf_tab_type[0] == '4'){
        if (memcmp(EXMSG1200->err_code, "0000000", 7) != 0){
            EXPARM->saf_upd_flag[0] = '0';
            EXPARM->tot_pod_flag_1[0] = '0';   
        }
        else{
            memset(&ari2940f,   0x00,   sizeof(ari2940f_t));
            ari2940f.in.ar0200c,  (armsg0200c_t *)ctx->ext_send_data;
            ari2940f.in.exmsg1200 = &ctx->kti_recv_data;
            rc = ar_saf4_create(&ari2940f);
        }
    }

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%.7s: ARN0012 : g000_saf_create() "
                             " ar_saf%c create ERROR", __FILE__, EXPARM->saf_tab_type[0] );
    
        SET_ERR_RSPN("189");
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;


}


/* ------------------------------------------------------------------------------------------------------------ */
static int h000_saf_update(arn0012_ctx_t  *ctx)
{
    int                 rc  = ERR_NONE;
    ari0290x_t          ari0290x;


    /*------------------------------------------------------------------------ */
    /* SAF응답여부  setting FLAG(FIL2)가 1이면 aro2900f call                       */
    /* 자기앞,가계수표  사고 결번조회시 saf update                                     */
    /*------------------------------------------------------------------------ */
    SYS_TRSF;

    if (EXPARM->saf_upd_flag[0] != '1') {
        return ERR_NONE; 
    }

    memset(&ari0290x, 0x00, sizeof(ari0290x_t));
    ari0290x.in.exmsg1200 = &ctx->kti_recv_data;
    ari0290x.in.armsgcomm = (armsgcomm_t *)ctx->ext_send_data; 

    rc = ar_saf_update(&ari0290x);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%.7s: ARN0012 : h000_saf_update() "
                             " ar_saf_update ERROR", __FILE__ );
    
        SET_ERR_RSPN("189");
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int i000_tot_proc(arn0012_ctx_t  *ctx)
{

    int                 rc  = ERR_NONE;
    ari2720f_t          ari2720f;
    ari2740f_t          ari2740f;
    ari2750f_t          ari2750f;
    ari2810f_t          ari2810f;
    ari2820f_t          ari2820f;
    ari2840f_t          ari2840f;

    /*------------------------------------------------------------------------ */
    /* 거래집계처리여부 FLAG1이 2/3/4/5/6이면 aro2720f/aro2730f/                     */
    /*------------------------------------------------------------------------ */
    SYS_TRSF;

    if (EXPARM->tot_pod_flag_1[0] == '2'){
        memset(&ari2720f,   0x00,   sizeof(ari2720f_t));
        if (EXPARM->host_comm_flag[0] == '0'){
            ari2720f.in.func_code   = ARI2720F_EXTMSG_INPUT;
            ari2720f.in.armsgcomm   = (armsgcomm_t *)ctx->ext_send_data; 
        }
        else{
            ari2720f.in.func_code   = ARI2720F_HOTMSG1200_INPUT;
            ari2720f.in.exmsg1200   = &ctx->kti_recv_data;
        }
        rc = ar_tot2_proc(&ari2720f);
    }
    else if (EXPARM->tot_pod_flag_1[0] == '4'){
        /* 지로출금 집계   */
        memset(&ari2740f,   0x00,   sizeof(ari2740f_t));
        memcpy(ari2740f.in.kti_flag,    "1",  ARI2740F_KTI_FLAG);
        ari2740f.in.exmsg1200 = &ctx->kti_recv_data;
        rc = ar_tot4_proc(&ari2740f);
    }
    else if (EXPARM->tot_pod_flag_1[0] == '5'){
        /* 공공요금 출금 집계   */
        memset(&ari2750f,   0x00,   sizeof(ari2750f_t));
        ari2750f.in.exmsg1200 = &ctx->kti_recv_data;
        rc = ar_tot5_proc(&ari2750f);
    }

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%.7s: ARN0012 : s000_tot_proc() "
                             " ar_tot_%c proc ERROR", __FILE__, EXPARM->tot_pod_flag_1[0]);
    
        SET_ERR_RSPN("189");
        return ERR_ERR;
    }

    /*------------------------------------------------------------------------ */
    /* 거래집계처리여부 FLAG2가 4/5/7이면 aro2810f/aro2820f/aro2840f/                */
    /*------------------------------------------------------------------------ */
    if (EXPARM->tot_pod_flag_2[0] == '4'){                     /* 지로출금 집계   */
        memset(&ari2810f,   0x00,   sizeof(ari2810f_t));    
        ari2810f.in.armsg = (armsg0700a_t *)ctx->ext_send_data;
        rc = ar_tot4_700(&ari2810f);
    }
    else if (EXPARM->tot_pod_flag_2[0] == '5'){                /* 공공요금 집계   */
        memset(&ari2820f,   0x00,   sizeof(ari2820f_t));    
        ari2820f.in.armsg = (armsg0700a_t *)ctx->ext_send_data;
        rc = ar_tot5_700(&ari2820f);
    }
    else if (EXPARM->tot_pod_flag_2[0] == '7'){                /* 전화요금 집계   */
        memset(&ari2840f,   0x00,   sizeof(ari2840f_t));    
        ari2840f.in.armsg = (armsg0700a_t *)ctx->ext_send_data;
        rc = ar_tot7_700(&ari2840f);
    }

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%.7s: ARN0012 : s000_tot_proc() "
                             " ar_tot_%c proc ERROR", __FILE__, EXPARM->tot_pod_flag_2[0]);

        SET_ERR_RSPN("189");
        return ERR_ERR;
    }

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int j000_net_mgr_proc(arn0012_ctx_t  *ctx)
{

    int                 rc  = ERR_NONE;
    armsg0800a_t        *armsg0800a;
    exi0240f_t          exi0240f;

    SYS_TRSF;

    if (EXPARM->kftc_mgr_flag[0] != '1'){
        return ERR_NONE;
    }

    armsg0800a = (armsg0800a_t *)ctx->ext_send_data;

    if (memcmp(armsg0800a->rspn_code,   "000", 3) != 0){
        return ERR_NONE;
    }

    memset(&exi0240f,   0x00,   sizeof(exi0240f_t));

    /* 053(kti) 기관코드에 대해서 027과 동일하게 상태  update처리    */
    exi0240f.in.upd_verf_flag = 'U';
    memcpy(exi0240f.in.inst_no      ,    "053"  ,   LEN_EXI0240F_INST_CODE); 
    memcpy(exi0240f.in.appl_code    ,   AR_CODE ,   LEN_EXI0240F_APPL_CODE);
    memcpy(exi0240f.in.sub_appl_code,     "00"  ,   LEN_EXI0240F_SUB_APPL_CODE);
    memcpy(exi0240f.in.tx_code      , EXMSG1200->tx_code, LEN_EXMSG1200_TX_CODE);
    ar_date_convert(exi0240f.in.tx_date,    armsg0800a->msg_send_date);

    rc = ex_net_upd_verf(&exi0240f);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%.7s: ARN0012 : t000_net_mgr_proc() "
                             " ex_net_upd_verf ERROR", __FILE__ );
        return GOB_NRM;
    }

    memcpy(exi0240f.in.inst_no,     "099",      LEN_EXI0240F_INST_CODE);
    
    rc = ex_net_upd_verf(&exi0240f);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%.7s: ARN0012 : t000_net_mgr_proc() "
                             " ex_net_upd_verf ERROR", __FILE__ );
        return GOB_NRM;
    }

    SYS_TREF;

    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
int static k000_ext_send_msg(arn0012_ctx_t *ctx)
{
    
    int                 rc  = ERR_NONE;
    armsg0201q_t        armsg0201q;
    char                cust_name[LEN_ARMSG0201Q_CUST_NAME + 1];

    SYS_TRSF;

    /* ----------------------------------------------------------------------- */
    /* 대외기관통신전문 commit여부 FLAG가 1이면 commit한다.                            */
    /* ----------------------------------------------------------------------- */
    
    if (EXPARM->kftc_send_comi[0] == '1'){
        rc = sys_tx_commit(TX_CHAINED);

        if (rc == ERR_ERR){
            ex_syslog(LOG_ERROR, "[APPL_DM]%.7s: u000_ext_send_msg()  sys_tx_commit ERROR", 
                                  __FILE__ );
            SET_ERR_RSPN("189");
            return ERR_ERR;
        }
    }

    /* 대외기관 전송데이터 길이 검증     
    len = ctx->ext_recv_len - 9;








    if (len <= 0){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s IXI0020: k000_ext_send_msg()"
                             "EXT GW LEN ERROR [len=%d]"
                             "[해결방안]ORACLE 담당자 CALL",
                             __FILE__, len);
        return GOB_ERR;
    }
    
    */

    /* 대외기관 G/W에 호출하는 방식을 전달하기 위한 값을 set */
    strcpy(SYSGWINFO->func_name, TCPSEND_KFTC);
    SYSGWINFO->time_val     = SYSGWINFO_SAF_DFLT_TIMEOUT;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_SAF;  /* SAF 방식호출  */
    SYSGWINFO->rspn_flag    = SYSGWINFO_REPLY;          /* G/W로 부터의 응답   */
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_ETC;        /* 전문종류 방식  */    

    /* transaction_id 9자리 빼야 함     */
    sysocbsi(ctx->cb, IDX_EXTSENDDATA, &ctx->ext_send_data[9], strlen(&ctx->ext_send_data[9]));
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s ARN0012: u000_ext_send_msg()"
                        "COMMBUFF(EXTSENDDATA) SET ERROR"
                        "[해결방안]시스템 담당자 CALL",
                        __FILE__);
        return ERR_ERR;
    }

#ifdef _DEBUG
    PRINT_ARS_KFTC_DEBUG(ctx->ext_send_data);
#endif

    SYS_DBG("tx_code [%s]fil10[%s]", EXPARM->tx_code, EXPARM->fil10);

    /* 대외기관 데이터 전송   */
    if (EXPARM->fil10[0] == '1'){
        strcpy(SYSGWINFO->func_name, "TCPKABC");
        rc = sys_tpcall("SYEXTGW_KAB", ctx->cb, TPNOTRAN);
    }else{
        rc = sys_tpcall("SYEXTGW_ARS", ctx->cb, TPNOTRAN);
    }

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s ARN0000: u000_ext_send_msg()"
                        "%s 서비스 호출 ERROR [%d][해결방안]대외기관 G/W 담당자 CALL",
                        __FILE__, tperrno);
        return ERR_ERR;
    }

    /* aruklog insert */
    rc = z100_log_insert(ctx, EXTSENDDATA, sysocbgs(ctx->cb, IDX_EXTSENDDATA), 2 + ctx->log_type_add);
     if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s ARN0012: u000_ext_send_msg()AR LOG ERROR :log_type[%d][해결방안]담당자 CALL",
                        __FILE__ ,  2);
        sys_err_init();
    }

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int z000_err_ext_send(arn0012_ctx_t    *ctx)
{
    
    int                 rc = ERR_NONE;
    armsgcomm_t         *armsgcomm;

    SYS_TRSF;

    rc = sys_tx_rollback(TX_CHAINED);

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %.7s z000_err_ext_send: sys_tx_rollback ERROR",
                            __FILE__);
        return ERR_ERR;
    }


    rc = sysocbsi(ctx->cb, IDX_EXTSENDDATA, &ctx->ext_send_data[9], strlen(&ctx->ext_send_data[9]));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s ARN0012: z000_err_ext_send()"
                             "COMMBUFF SET ERROR [해결방안]시스템 담당자 call",
                             __FILE__);
        return ERR_ERR;
    }

#ifdef  _DEBUG 
    PRINT_ARS_KFTC_DEBUG(ctx->ext_send_data);
#endif

    /* 대외기관 G/W에서 호출하는 방식을 전달하기 위한 값을 set  */
    /* TCP/IP ARS 전환                                */
    strcpy(SYSGWINFO->func_name,    TCPSEND_KFTC);
    SYSGWINFO->time_val = SYSGWINFO_SAF_DFLT_TIMEOUT;
    SYSGWINFO->call_type    = SYSGWINFO_CALL_TYPE_SAF;  /* SAF 방식호출  */
    SYSGWINFO->rspn_flag    = SYSGWINFO_REPLY;          /* G/W로 부터의 응답   */
    SYSGWINFO->msg_type     = SYSGWINFO_MSG_ETC;        /* 전문종류 방식  */    

    /* 대외기관 데이터 전송   */
    if (EXPARM->fil10[0] == '1'){
        strcpy(SYSGWINFO->func_name, "TCPKABC");
        rc = sys_tpcall("SYEXTGW_KAB", ctx->cb, TPNOTRAN);
    }else{
        rc = sys_tpcall("SYEXTGW_ARS", ctx->cb, TPNOTRAN);
    }

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s ARN0000: z000_err_ext_send()"
                        "%s 서비스 호출 ERROR [%d][해결방안]대외기관 G/W 담당자 CALL",
                        __FILE__, tperrno);
        return ERR_ERR;
    }

    /* aruklog insert */
    rc = z100_log_insert(ctx, EXTSENDDATA, sysocbgs(ctx->cb, IDX_EXTSENDDATA),
                             2 + ctx->log_type_add);
     if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s ARN0012: z000_err_ext_send()AR LOG ERROR :log_type[%d][해결방안]담당자 CALL",
                        __FILE__ ,  2 + ctx->log_type_add);
        sys_err_init();
    }

    SYS_TREF;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int z100_log_insert(arn0012_ctx_t    *ctx, char *log_data, int size, char io_flag,   char sr_flag)
{

    int                 rc = ERR_NONE;
    ari3100f_t          ari3100f;
    commbuff_t          *dcb;

    SYS_TRSF;

    /* ------------------------------------------- */
    SYS_DBG("z100_log_insert  : len [%d]",  size);
    SYS_DBG("z100_log_insert  : msg [%s]",  log_data );
    /* ------------------------------------------- */

    memset(&ari3100f,   0x00, sizeof(ari3100f_t));
    ari3100f.in.sys_type    =  1;
    ari3100f.in.log_type    =  log_type;
    memcpy(ari3100f.in.log_data,    log_data,    size);
    ari3100f.in.log_len     = size;

    if ((log_type == 1) ||
        (log_type == 11)){              /* 대왹기관 수신전문 tx_id 포함  */
        ari3100f.in.tx_id_flag[0] = '1';
    }
    else{
        ari3100f.in.tx_id_flag[0] = '0';
    }

    memset(&dcb, 0x00, sizeof(commbuff_t));

    rc = sysocbdb(ctx->cb, &dcb);       /* ctx->cb dcb로 복사    */
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s NFN0020: z100_log_insert() NFLOG ERROR sysocbdb log_type :%d", __FILE__, sr_flag);
        sys_err_init();
        return ERR_NONE;
    }


    rc = sysocbsi(&dcb, IDX_EXMSG1200,   &ari3100f, sizeof(ari3100f_t));
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s NFN0020: z100_log_insert() NFLOG ERROR sysocbsi log_type :%d", __FILE__, sr_flag);
        sys_err_init();
        sysocbfb(&dcb);
        return ERR_NONE;
    }

    rc = sys_tpcall("ARN3100F", &dcb, TPNOREPLY | TPNOTRAN);
    if (rc == ERR_ERR) {
        ex_syslog(LOG_ERROR, "[APPL_DM]%s NFN0020: z100_log_insert() NFLOG ERROR sysocbsi log_type :%d", __FILE__, sr_flag);
        sys_err_init();
    }

    sysocbfb(&dcb);

    SYS_TREF;

    return ERR_NONE;


}


/* ------------------------------------------------------------------------------------------------------------ */
/* function             utoiconv(char *fcode, char *tcode, char *in, size_t in_len, char *out, size_t   out_len)*/
/* brief        : 코드변환                                                                                        */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/*                                                                                                              */
/* ------------------------------------------------------------------------------------------------------------ */
static int utoiconv(char *fcode, char *tcode, char *in, size_t in_len, char *out, size_t  out_len)
{

    int                 rc  = 0;
    iconv_t             cd  = {0};
    char              *in2  = in;
    char              *out2 = out;

    if (in == 0x00){
        SYS_DBG("in is null");
        return -1;
    }

    if (out == 0x00){
        SYS_DBG("out is null");
        return -1;
    }

    if (in_len < 1){
        SYS_DBG("in_len[%d] is not vaild", in_len);
        return -1;
    }

    if (out_len < 1){
        SYS_DBG("out_len[%d] is not vaild", out_len);
        return -1;
    }

    cd = iconv_open(tcode, fcode);
    if ((int)cd < 0){
        SYS_DBG("icov_open failed errorno [%d][%s]", errno, strerror(errno));
        return -1;
    }

    cd = iconv(cd ,&in_len, &out, &out_len);
    if ((int)cd < 0){
        SYS_DBG("icov_open failed errorno [%d][%s]", errno, strerror(errno));
        rc = iconv_close(cd);
        return -1;
    }

    rc = iconv_close(cd);
    if ((int)cd < 0){
        SYS_DBG("icov_open failed errorno [%d][%s]", errno, strerror(errno));
        return -1;
    }

    /* UTF-8 변환 길이  */
    rc = out2 - out;
    return rc;

}

/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */