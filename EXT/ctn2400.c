
/*  @file               ctn2400.c                                             
*   @file_type          pc source program
*   @brief              대외계에서 host송신전문을 set하여 host로 송신함
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
*
*/

/*  @pcode
 *  처리 : 대외기관으로 부터 수신되어진 CMS(이용기관, 수납장표) 대하여 호스트로 송신
 *   user_main() main process  
 *        a000 : 데이터 초기화
 *        b000 : 전송할 데이터 조회
 *        c000 : 데이터 송신에 대한 주 업무 처리 및 마지막 송신종료에 대한 SEND
 *        c100 : 수신원장 FETCH
 *        c200 : cursor close
 *        c300 : 상태데이블 update
 *        v000 : 호스트 송신
 *
 *        a000 데이터 초기화     1) 데이터 초기화 
 *                            2) G/W 정보 셋팅
 *        b000 전송할 데이터 조회	
 *                            1) 전송할 데이터 조회
 *                                (1) sta 테이블 extn_flag '9'이면서 host_flag '0'인경우
 *                                ==> 대외기관으로 부터 수신후 host로 전송해야 할 시점의 file인 경우
 *                                (2) sta 테이블 extn_flag '9'이면서 host_flag '5'인경우
 *                                ==> 선택송신을 client화면에서 조작할 경우
 *                                (3) sta 테이블 extn_flag '9'이면서 host_flag '3'인경우
 *                                ==> 송신하다가 정상종료가 되지않을 file을 재전송하는 경우에 해당됨
 *                                (4) 송신할 데이터가 없는경우는 5분단위로 감시하며 상태 테이블을 검색
 *        c000 
 *                            1) 데이터 송신에 대한 주업무 처리 및 마지막 송신종료에 대한 SEND
 *                                - 전송할 데이터 존재시 업무 처리
 *                                (1) 전송할 전문 조립 setting 
 *                                (2) 선택송신일 경우 중복허용
 *                                (3) 에러코드가 0000000  아닌경우 오류처리
 *                                - 전송할 데이터가 미존재시 업무 처리
 *                                (1) 전송할 전문 조립 setting 
 *                                cthlay->rspn_flag          = '0' (응답 불필요)
 *                                cthlay->data_end_flag = '1'  (송수신 종료)
 *                                cthlay->session_flag     = '1'  (session종료)
 *                                setting 후 호스트로 전송
 *        c100 
 *                            1) 수신원장 fetch
 *        c200 
 *                            1) cursor close	
 *        c300 상태데이블 update
 *                            1) 블럭번호 스퀸스 번호가 0001 & 001 이면 상태 테이블에 host_flag = '1' 로 update
 *                            2) 각종오류발생시 상태테이블에 host_flag = '3'로 update
 *                            3) 블럭번호와 last_blk_seq_no와 일치하면 상태테이블에 host_flag를 9로 update
 *        v000 
 *                            1) 호스트로 송신
 */


/* -------------------------- include files  -------------------------------------- */
#include <syscom.h>
#include <exdefine.h>
#include <ctdefine.h>
#include <ctarg.h>
#include <cthlay.h>
#include <cti0026x.h>
#include <cti0030f.h>
#include <ctmstr.h>
#include <usrinc/atmi.h>
#include <usrinc/ucs.h>
#include <sqlca.h>


/* --------------------------constant, macro definitions -------------------------- */
/* --------------------------structure definitions -------------------------------- */
typedef struct ctn2400_ctx_s    ctn2400_ctx_t;
struct ctn2400_ctx_s {
    commbuff_t          _cb;
    commbuff_t          *cb;

    ctarg_t             ctarg;
    cthlay_t            cthlay;

    ctarg_t             ctarg_kti;      //Decoupling 추가
    cthlay_t            cthlay_kti;     //Decoupling 추가
    int                 sta_type;

    int                 host_send_len;  
    char                host_send_data[5000];
    int                 host_recv_len;
    char                host_recv_data[5000];
};


/* --------------exported global variables declarations ---------------------------*/
ctn2400_ctx_t   _ctx;  
ctn2400_ctx_t   *ctx = &_ctx; 

static int      g_ctinfo_cursor;
static int      g_ctmst_cursor;

static int      g_ctinfo_cursor_kti;      //Decoupling 추가
static int      g_ctmst_cursor_kti;       //Decoupling 추가

static int      g_kti_send_cnt;           //Decoupling 추가
static int      g_kti_temp_cnt;           //Decoupling 추가

/* ----------------------- exported function declarations ------------------------- */
static int      a000_init_proc(ctn2400_ctx_t    *ctx); 
static int      b000_csta_select(ctn2400_ctx_t  *ctx);
static int      c000_data_send_proc(ctn2400_ctx_t   *ctx, int end_flag);
static int      c100_ctmstr_select(ctn2400_ctx_t    *ctx);
static int      c200_ctmst_curosr_close(ctn2400_ctx_t   *ctx);
static int      c300_csta_update(ctn2400_ctx_t  *ctx, int host_sta_flag);
static int      v000_gw_call(ctn2400_ctx_t  *ctx, int rspn_flag);
/* --------------------------  */
/* Decoupling 추가              */
/* --------------------------  */
static int      e000_ctsta_select(ctn2400_ctx_t *ctx);
static int      f000_data_send(ctn2400_ctx_t    *ctx);
static int      f100_ctmstr_select(ctn2400_ctx_t    *ctx);
static int      f200_ctmstr_cursor_close(ctn2400_ctx_t  *ctx);
static int      f300_ctsta_update(ctn2400_ctx_t *ctx, int host_sta_flag);
static int      w000_make_kti_s_file(ctn2400_ctx_t  *ctx);
/* --------------------------------------------------------------------- */


/* --------------------------------------------------------------------------------------------------------- */
int usermain(int argc, char argv[])
{

    int                 rc = ERR_NONE;
    int                 time_val;
    int                 tot_cnt = 0;
    time_t              tval1, tval2;

    /* 초기화 처리 */
    a000_init_proc(ctx);

    /* waiting 시간 */
    time_val = 3;
    time(&tval1);

    while(1){
        /* TMAX message 처리 */
        rc =  tpschedule(time_val);

        /* 지정된 초가 경과한 경우 (3초이상 되면 continue )*/
        time(&tval2);
        if (time_val > (tval2 - tval1)){
            time_val -= (tval2 -tval1);
            if (time_val > 3){
                tval1 = tval2;
                continue;
            }
        }

        /* DB와 연결이 끊어진 경우 재 연결 */
        if (db_connect("") != ERR_NONE){
            time(&tval1);
            time_val = 300;
            continue;
        }

        /* --------------------------  */
        /* Decoupling 추가              */
        /* --------------------------  */
        /* GR1535 전송할 데이터 조회 (KTI) */
        rc = f000_data_send(ctx);
        if (rc == ERR_ERR || rc == ERR_NFD){
            memset(&ctx->ctarg_kti, 0x00, sizeof(ctarg_t));
        }
        else{
            /* GR1535 데이터  송신 주업무 처리(KTI) */
            rc = f000_data_send_proc(ctx);
            if (rc == ERR_ERR){

            }
        }
        /* ------------------------------------------------------ */

        /* 전송할 데이터 조회 CORE */
        rc = b000_csta_select(ctx);
        if (rc = ERR_ERR) {
            if (tot_cnt > 0){
                tot_cnt = 0;
                /* 마지막 송신종료에 대한 send */
                c000_data_send_proc(ctx, 1);
            }
            time(&tval1);
            time_val = 300;
            memset(&ctx->ctarg, 0x00, sizeof(ctarg_t));
            continue;
        }

        /* 데이터 송신 주업무 처리 (Core) */
        rc = c000_data_send_proc(ctx, 0;
        if (rc == ERR_TIME){
            time(&tval1);
            time_val = 100;
        }
        else{
            time(&tval1);
            time_val = 5;
            tot_cnt++;
        }
    }

    return ERR_NONE;
}


/* --------------------------------------------------------------------------------------------------------- */
static int a000_init_proc(ctn2400_ctx_t *ctx)
{

    int                 rc = ERR_NONE;
    sysicomm_t          sysicomm;
    sysgwinfo_t         sysgwinfo;

    SYS_TRST;

    /* set commbuff */
    memset((char *)ctx, 0x00, sizeof(ctn2400_ctx_t));
    ctx->cb = &ctx->commbuff_t _cb;

    /* SYSTEM GLOBAL 변수에 자신의 서비스명 저장 */
    strcpy(g_arch_head.svc_name, "CTN2400");
    memset(g_arch_head.err_file_name, 0x00, sizeof(g_arch_head.err_file_name));

    /* SYSICOMM commbuff 항목 설정 */
    memset(&sysicomm, 0x00, sizeof(sysicomm_t));
    sysocbsi(ctx->cb, IDX_SYSICOMM, &sysicomm, sizeof(sysicomm_t));
    SYSICOMM->intl_tx_flag = 0;
    utoclck(SYSICOMM->ilog_no);
    strcpy(SYSICOMM->svc_name,  "CTN2400");

    SYSICOMM->pid_no = getpid();
    strcpy(SYSICOMM->svc_name,  "CTN2400");

    /* G/W Information COMMBUFF 생성 */
    memset(&sysgwinfo, 0x00, sizeof(sysgwinfo_t));
    sysgwinfo.sys_type = SYSGWINFO_SYS_CORE_BANK;

    sysocbsi(ctx->cb, IDX_SYSGWINFO, &sysgwinfo, LEN_SYSGWINFO);

    SYS_TREF;

    return ERR_NONE; 

}


/* --------------------------------------------------------------------------------------------------------- */
static int b000_csta_select(ctn2400_ctx_t   *ctx)
{

    int                 rc = ERR_NONE;
    int                 temp_seq = 0;  
    ctarg_t             *ctarg;

    SYS_TRST;

    /*
        (1) sta 테이블 extn_flag '9'이면서 host_flag '0'인경우
        ==> 대외기관으로 부터 수신후 host로 전송해야 할 시점의 file인 경우
        (2) sta 테이블 extn_flag '9'이면서 host_flag '5'인경우
        ==> 선택송신을 client화면에서 조작할 경우
        (3) sta 테이블 extn_flag '9'이면서 host_flag '3'인경우
        ==> 송신하다가 정상종료가 되지않을 file을 재전송하는 경우에 해당됨
    */
    ctarg = (ctarg_t *) &ctx->ctarg;
    memset(ctarg, 0x00, sizeof(ctarg_t));

    if (g_ctinfo_cursor == 0){
        EXEC SQL DECLARE CUR_CTARG_01 CURSOR FOR 
                /* Decoupling 추가              */
                SELECT 1 seq ,
                       NVL(A.PROC_DATE, '           '),
                       NVL(A.DATA_DATE, '           '),
                       NVL(A.ITEM_NAME, '           '),
                       NVL(A.VAN_ID, '  '),
                       LTRIM(TO_CHAR(A.EXTN_LAST_NO, '0999999')),
                       A.HOST_FLAG  
                FROM   CTSTA A, CTINFO B 
               WHERE   A.SR_TYPE        = 'S'
                 AND   A.EXTN_FLAG      = '9'
                 AND   A.HOST_FLAG      = '8'   // KTI 전송대상이 완료 된 경우 
                 AND   A.VAN_ID        <> '997'
                 AND   ((B.AP_COMM_FLAG BETWEEN '0' AND '9') OR B.AP_COMM_FLAG IN ('I', 'K'))
                 AND   SUBSTR(A.ITEM_NAME,1,6) LIKE RTRIM(SUBSTR(B.ITEM_NAME,1,6)) || '%'
                 AND   A.VAN_ID         = B.VAN_ID


                UNION 
                /* ============================================ */

                SELECT 2 seq ,
                       NVL(A.PROC_DATE, '           '),
                       NVL(A.DATA_DATE, '           '),
                       NVL(A.ITEM_NAME, '           '),
                       NVL(A.VAN_ID, '  '),
                       LTRIM(TO_CHAR(A.EXTN_LAST_NO, '0999999')),
                       A.HOST_FLAG  
                FROM   CTSTA A, CTINFO B 
               WHERE   A.SR_TYPE        = 'S'
                 AND   A.EXTN_FLAG      = '9'
                 AND   A.HOST_FLAG      = '5' 
                 AND   A.VAN_ID        <> '997'
                 AND   ((B.AP_COMM_FLAG BETWEEN '0' AND '9') OR B.AP_COMM_FLAG IN ('I', 'K'))
                 AND   SUBSTR(A.ITEM_NAME,1,6) LIKE RTRIM(SUBSTR(B.ITEM_NAME,1,6)) || '%'
                 AND   A.VAN_ID         = B.VAN_ID

               UNION 

                SELECT 3 seq ,
                       NVL(A.PROC_DATE, '           '),
                       NVL(A.DATA_DATE, '           '),
                       NVL(A.ITEM_NAME, '           '),
                       NVL(A.VAN_ID, '  '),
                       LTRIM(TO_CHAR(A.EXTN_LAST_NO, '0999999')),
                       A.HOST_FLAG  
                FROM   CTSTA A, CTINFO B 
               WHERE   A.SR_TYPE        = 'S'
                 AND   A.EXTN_FLAG      = '9'
                 AND   A.HOST_FLAG      = '1' 
                 AND   A.VAN_ID        <> '997'
                 AND   ((B.AP_COMM_FLAG BETWEEN '0' AND '9') OR B.AP_COMM_FLAG IN ('I', 'K'))
                 AND   SUBSTR(A.ITEM_NAME,1,6) LIKE RTRIM(SUBSTR(B.ITEM_NAME,1,6)) || '%'
                 AND   A.VAN_ID         = B.VAN_ID

                SELECT 4 seq ,
                       NVL(A.PROC_DATE, '           '),
                       NVL(A.DATA_DATE, '           '),
                       NVL(A.ITEM_NAME, '           '),
                       NVL(A.VAN_ID, '  '),
                       LTRIM(TO_CHAR(A.EXTN_LAST_NO, '0999999')),
                       A.HOST_FLAG  
                FROM   CTSTA A, CTINFO B 
               WHERE   A.SR_TYPE        = 'S'
                 AND   A.EXTN_FLAG      = '9'
                 AND   A.HOST_FLAG      = '0'
                 AND   A.VAN_ID        <> '997'
                 AND   ((B.AP_COMM_FLAG BETWEEN '0' AND '9') OR B.AP_COMM_FLAG IN ('I', 'K'))
                 AND   SUBSTR(A.ITEM_NAME,1,6) LIKE RTRIM(SUBSTR(B.ITEM_NAME,1,6)) || '%'
                 AND   A.VAN_ID         = B.VAN_ID

               UNION 
               
                SELECT 5 seq ,
                       NVL(A.PROC_DATE, '           '),
                       NVL(A.DATA_DATE, '           '),
                       NVL(A.ITEM_NAME, '           '),
                       NVL(A.VAN_ID, '  '),
                       LTRIM(TO_CHAR(A.EXTN_LAST_NO, '0999999')),
                       A.HOST_FLAG  
                FROM   CTSTA A, CTINFO B 
               WHERE   A.SR_TYPE        = 'S'
                 AND   A.EXTN_FLAG      = '9'
                 AND   A.HOST_FLAG      = '3'
                 AND   A.VAN_ID        <> '997'
                 AND   ((B.AP_COMM_FLAG BETWEEN '0' AND '9') OR B.AP_COMM_FLAG IN ('I', 'K'))
                 AND   SUBSTR(A.ITEM_NAME,1,6) LIKE RTRIM(SUBSTR(B.ITEM_NAME,1,6)) || '%'
                 AND   A.VAN_ID         = B.VAN_ID
                 ORDER BY seq 

        EXEC SQL OPEN CUR_CTARG_01;

        if (SYS_DB_CHK_FAIL){
            db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
            ex_syslog(LOG_ERROR, "[APPL_DM] %s b000_csta_select():"
                             "CURSOR OEPN(CTSTA, CTINFO) ERROR[%d] MSG[%s] ",
                             __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
            return ERR_ERR;
        }
        
        g_ctinfo_cursor = 1;
    }
    
    EXEC SQL FETCH CUR_CTARG_01 
             INTO   :temp_seq,
                    :ctarg->proc_date,
                    :ctarg->data_date,
                    :ctarg->appl_name,
                    :ctarg->van_id,
                    :ctarg->host_last_no,
                    :ctarg->host_sta_flag;
    
    if (SYS_DB_CHK_FAIL){
        if (SYS_DB_CHK_NOTFOUND){
            EXEC SQL CLOSE CUR_CTARG_01;
            g_ctinfo_cursor = 0;
            return ERR_ERR;
        }
        else{
            db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
            ex_syslog(LOG_ERROR, "[APPL_DM] %s b000_csta_select():"
                        "CURSOR OEPN(CTSTA, CTINFO) ERROR[%d] MSG[%s] ",
                        __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
            EXEC SQL CLOSE CUR_CTARG_01;
            g_ctinfo_cursor = 0;
            return ERR_ERR;
        }
    }

    ctarg->sr_type[0]   = 'S';
    memset(ctarg->blkno , '0' , LEN_CTARG_BLK_NO);
    memset(ctarg->seq_no, '0' , LEN_CTARG_SEQ_NO);
    SYS_DBG(" ======================================================== ");
    SYS_DBG("                 FETCH DATA                               ");
    SYS_DBG(" ======================================================== ");
    SYS_DBG(" ctarg->proc_date          = [%.8s]", ctarg->proc_date     );
    SYS_DBG(" ctarg->data_date          = [%.8s]", ctarg->data_date     );
    SYS_DBG(" ctarg->appl_name          = [%.8s]", ctarg->appl_name     );
    SYS_DBG(" ctarg->van_id             = [%.3s]", ctarg->van_id        );
    SYS_DBG(" ctarg->host_last_no       = [%.7s]", ctarg->host_last_no  );
    SYS_DBG(" ctarg->host_sta_flag      = [%.1s]", ctarg->host_sta_flag );
    SYS_DBG(" ======================================================== ");

    SYS_TREF;

    return ERR_NONE;
}   

/* --------------------------------------------------------------------------------------------------------- */
static int c000_data_send_proc(ctn2400_ctx_t    *ctx, int end_flag)
{

    int                 rc = ERR_NONE;
    int                 i;
    cti0026x_t          cti0026x;
    cti0030f_t          cti0030f;  
    ctarg_t             *ctarg;
    cthlay_t            cthlay;
    cthlay_t            cthlay2;
 
    ctarg  = (ctarg_t  *) &ctx->ctarg;
    cthlay = (cthlay_t *) &ctx->cthlay;

    memcpy(ctarg->max_block_no, "0000", LEN_CTARG_MAX_BLOCK_NO);
    memcpy(ctarg->max_seq_no,   "000" , LEN_CTARG_MAX_SEQ_NO);

    /* 전송할 데이터가 존재시 */
    if (end_flag == 0){
        while(1){
            tpschedule(-1);

            /* ----------------------------------------- */
            /* 수신원장 dbio call                          */
            /* ----------------------------------------- */
            rc = c100_ctmstr_select(ctx);

            if (rc == ERR_NFD){
                break;
            }

            if (rc == ERR_ERR){
                ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_data_send_proc():"
                                     "대외계 -> 호스트 송신: 원장검색 오류 "
                                     "proc_date[%.8s] van_type[%.2s] tiem_appl_name[%.8s] "
                                     "[해결방안]업무 담당자 call"
                                     , __FILE__, cthlay->proc_date
                                     , cthlay->van_type, cthlay->item_appl_name);
                c300_csta_update(ctx, 3);
                return ERR_ERR;
            }

            /* ----------------------------------------- */
            /* host layout set call                      */
            /* ----------------------------------------- */
            memset(&cti0026x, 0x00, sizeof(cti0026x_t));
            cti0026x.in.cthlay   = &ctx->cthlay;
            cti0026x.in.svr_flag = 1;

            rc = cto0026x(&cti0026x);
            if (rc == ERR_ERR) {
                c200_ctmst_curosr_close(ctx);
                c300_csta_update(ctx, 3);
                ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_data_send_proc():"
                                     "대외계 -> 호스트 송신: 전문조립 오류 "
                                     "proc_date[%.8s] van_type[%.2s] tiem_appl_name[%.8s] "
                                     "[해결방안]업무 담당자 call"
                                     , __FILE__, cthlay->proc_date
                                     , cthlay->van_type, cthlay->item_appl_name);
                return ERR_ERR;
            }

            /* ----------------------------------------- */
            /* 마지막 블럭 및 일련번호인경우 종료               */
            /* ----------------------------------------- */
            if ((memcmp(&ctarg->host_last_no[0], cthlay->block_no, LEN_CTHLAY_BLOCK_NO) == 0) &&
                (memcmp(&ctarg->host_last_no[4], cthlay->seq_no  , LEN_CTHLAY_SEQ_NO  ) == 0)){
                    cthlay->data_end_flag[0] = '1';
            }

            /* ----------------------------------------- */
            /* 선택송신일 경우 중복 허용                       */
            /* ----------------------------------------- */
            if (ctarg->host_sta_flag[0] == '5'){
                    cthlay->dup_flag[0] = '1';
            }
            /* ----------------------------------------- */
            /* host로 송신 시작                            */
            /* ----------------------------------------- */
            if ((memcmp(cthlay->block_no, "0001", LEN_CTHLAY_BLOCK_NO) == 0 ) &&
                (memcmp(cthlay->seq_no,   "001" , LEN_CTHLAY_SEQ_NO  ) == 0 )){
                rc = c300_csta_update(ctx, 1);
                if (rc == ERR_ERR){
                    c200_ctmst_curosr_close(ctx);
                    ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_data_send_proc():"
                                            "상태 테이블(CTSTA) UPDATE ERROR 1", __FILE__);
                    return ERR_ERR;
                }
            }
            memcpy(cthlay->data_date, ctarg->data_date, LEN_CTHLAY_DATA_DATE);

            memcpy(ctx->host_send_data, cthlay,  sizeof(cthlay_t));
            ctx->host_send_len = strlen(ctx->host_send_data);

            /* ----------------------------------------- */
            /* 전문 송신 및 응답 처리                         */
            /* ----------------------------------------- */
            rc = v000_gw_call(ctx, 1);
            if (rc == ERR_ERR){
                c200_ctmst_curosr_close(ctx);
                c300_csta_update(ctx, 3);
                ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_data_send_proc():"
                                     "대외계 -> 호스트 송신: GW응답코드 오류 "
                                     "proc_date[%.8s] van_type[%.2s] tiem_appl_name[%.8s] "
                                     "[해결방안]업무 담당자 call"
                                     , __FILE__, cthlay->proc_date
                                     , cthlay->van_type, cthlay->item_appl_name);
                return ERR_TIME;
            }

            /* ----------------------------------------- */
            /* 에러  응답시 처리                             */
            /* ----------------------------------------- */
            cthlay2 = (cthlay *) ctx->host_recv_data;
            PRINT_CTHLAY((cthlay_t *) ctx->host_recv_data);
            if (memcmp(cthlay2->err_code, "0000000", LEN_CTHLAY_ERR_CODE) != 0 ){
                c200_ctmst_curosr_close(ctx);
                c300_csta_update(ctx, 3);
                /* ---------------------------- */
                /* 에러 공통모듈  call             */
                /* ---------------------------- */
                ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_data_send_proc():"
                                     "대외계 -> 호스트 송신: 응답코드 오류[%.7s] "
                                     "proc_date[%.8s] van_type[%.2s] tiem_appl_name[%.8s] "
                                     "[해결방안]업무 담당자 call"
                                     , __FILE__, cthlay2->err_code, cthlay->proc_date
                                     , cthlay->van_type, cthlay->item_appl_name);
                return ERR_ERR;
            }
            /* ----------------------------------------- */
            /* host로 송신 종료                             */
            /* ----------------------------------------- */
            if ((memcmp(&ctarg->host_last_no[0], cthlay2->block_no, LEN_CTHLAY_BLOCK_NO) == 0) &&
                (memcmp(&ctarg->host_last_no[4], cthlay2->seq_no  , LEN_CTHLAY_SEQ_NO  ) == 0)){
                    rc = c300_csta_update(ctx, 9);
                    if (rc == ERR_ERR){
                        ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_data_send_proc():"
                                                "상태 테이블(CTSTA) UPDATE ERROR 1", __FILE__);
                        return ERR_ERR;
                    }
                    
            }
        } /* end of while loop */
    }
    else{
        /* ----------------------------------------- */
        /* host로 layout set call                    */
        /* ----------------------------------------- */
        memset(&cti0026x, 0x00, sizeof(cti0026x_t));
        cti0026x.in.cthlay   = &ctx->cthlay;
        cti0026x.in.svr_flag = 1;

        rc = cto0026x(&cti0026x);
        if (rc == ERR_ERR){
            ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_data_send_proc():"
                                    "대외계 -> 호스트 송신: 전문조립 오류"
                                    "proc_date[%.8s] van_type[%.2s] tiem_appl_name[%.8s] "
                                    "[해결방안]업무 담당자 call"
                                    , __FILE__, cthlay->proc_date
                                    , cthlay->van_type, cthlay->item_appl_name);
            return ERR_ERR; 

        }

        cthlay->rspn_flag[0]        = '0'  /* 응답 불필요 */
        cthlay->data_end_flag[0]    = '1'  /* 송수신 종료 */
        cthlay->session_flag[0]     = '1'  /* session 종료 */

        /* 송수신 오류발생되면 3회 Retry */
        for (i = 0; i < 3; i++){
            memcpy(ctx->host_send_data, cthlay, sizeof(cthlay_t));
            ctx->host_send_len = strlen(ctx->host_send_data);
            SYS_DBG("======================== 전송시작 ========================")
            SYS_DBG("cthlay->rspn_flag        = [%0.1s]", cthlay->rspn_flag);
            SYS_DBG("cthlay->data_end_flag    = [%0.1s]", cthlay->data_end_flag);
            SYS_DBG("cthlay->session_flag     = [%0.1s]", cthlay->session_flag);
            /* ----------------------------------------- */
            /* host 송신                                  */
            /* ----------------------------------------- */
            rc = v000_gw_call(ctx, 0);
            if (rc == ERR_NONE){
                break;
            }
            else{
                ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_data_send_proc():"
                                        "[%.8s %.2s %.8s] HOST SEND ERR담당직원 확인 ! "
                                        , __FILE__, cthlay->proc_date
                                        , cthlay->van_type, cthlay->item_appl_name);
                tpschedule(10);
            }
        }
    }

    SYS_TREF;

    return ERR_NONE;
}


/* --------------------------------------------------------------------------------------------------------- */
static int c100_ctmstr_select(ctn2400_ctx_t *ctx)
{

    int                 rc = ERR_NONE;
    ctmstr_t            ctmstr;
    ctarg_t             *ctarg;
    cthlay_t            cthlay;

    SYS_TRST;

    ctarg  = (ctarg_t  *) &ctx->ctarg;  
    cthlay = (cthlay_t *) &ctx->cthlay;

    /* ------------------------------------------ */
    /* van_id, 처리일자, item_appl_name set         */
    /* ------------------------------------------ */
    memset(cthlay, 0x20, sizeof(cthlay_t));
    memcpy(cthlay->proc_date,      ctarg->data_date, LEN_CTHLAY_PROC_DATE);
    memcpy(cthlay->van_type ,      ctarg->van_id[1], LEN_CTHLAY_VAN_TYPE );
    memcpy(cthlay->item_appl_name, ctarg->appl_name, LEN_CTHLAY_ITEM_APPL_NAME);
    SYS_DBG(" ===================================================== ");
    SYS_DBG("             CTMSTR    DATA VALUE CHECK                ");
    SYS_DBG(" ===================================================== ");
    SYS_DBG(" ctarg->proc_date        = [%.8s]", ctarg->proc_date    );
    SYS_DBG(" ctarg->data_date        = [%.8s]", ctarg->data_date    );
    SYS_DBG(" ctarg->appl_name        = [%.8s]", ctarg->appl_name    );
    SYS_DBG(" ctarg->van_id           = [%.3s]", ctarg->van_id       );
    SYS_DBG(" ctarg->host_last_no     = [%.7s]", ctarg->host_last_no );
    SYS_DBG(" ctarg->blk_no           = [%.4s]", ctarg->blk_no       );
    SYS_DBG(" ctarg->seq_no           = [%.3s]", ctarg->seq_no       );
    SYS_DBG(" ===================================================== ");

    if ((memcmp(ctarg->blk_no, "0000", LEN_CTARG_BLK_NO) == 0) &&
        (memcmp(ctarg->seq_no, "000" , LEN_CTARG_SEQ_NO) == 0)) {
        
        /* --------------------------------------------------- */
        /* 테이블 변수 set                                       */
        /* --------------------------------------------------- */
        memset(&ctmstr, 0x00, sizeof(cthlay_t));
        memcpy(ctmstr.proc_date,   ctarg->proc_date, LEN_CTHLAY_PROC_DATE);
        memcpy(ctmstr.data_date,   ctarg->data_date, LEN_CTHLAY_DATA_DATE);
        memcpy(ctmstr.van_id,      ctarg->van_id,    LEN_CTHLAY_VAN_ID);
        memcpy(ctmstr.item_name,   ctarg->appl_name, LEN_CTHLAY_ITEM_NAME);
        utortrim(ctmstr.item_name);

        /* -------------------------------------------------------------- */
        SYS_DBG(" ctmstr.proc_date        = [%.8s]", ctmstr.proc_date    );
        SYS_DBG(" ctmstr.data_date        = [%.8s]", ctmstr.data_date    );
        SYS_DBG(" ctmstr.item_name        = [%.8s]", ctmstr.item_name    );
        SYS_DBG(" ctmstr.van_id           = [%.3s]", ctmstr.van_id       );        
        /* -------------------------------------------------------------- */

        /* 실data */
        EXEC SQL DECLARE CTMSTR_CUR_1 CURSOR FOR 
                SELECT NVL(BLK_NO, 0),
                       NVL(SEQ_NO, 0),
                       NVL(REC_CNT, 0),
                       NVL(FILE_SIZE, 0),
                       DETL_DATA
                FROM   CTMSTR  
                WHERE  PROC_DATE = :ctmstr.proc_date
                AND    DATA_DATE = :ctmstr.data_date
                AND    VAN_ID    = :ctmstr.van_id
                AND    ITEM_NAME = :ctmstr.item_name
                ORDER BY BLK_NO, SEQ_NO;

        EXEC SQL OPEN CTMSTR_CUR_1;

        if (SYS_DB_CHK_FAIL) {
            db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
            ex_syslog(LOG_ERROR, "[APPL_DM] %s c100_ctmstr_select(): "
                                 "data ctmstr select ERR[%d]item_name[%s]"
                                 "담당자 확인! MSG[%s]",
                                 __FILE__, SYS_DB_ERRORNUM,
                                 ctmstr.item_name, SYS_DB_ERRORSTR );
            return ERR_ERR;
        }

        g_ctmstr_cursor = 1;
    }
    
    /* -------------------------------------------------------------- */
    /* 수신정보  FETCH                                                  */
    /* -------------------------------------------------------------- */
    memset(&ctmstr, 0x00, sizeof(ctmstr_t));
    EXEC SQL FETCH CTMSTR_CUR_1
            INTO :ctmstr.blk_no,
                 :ctmstr.seq_no,
                 :ctmstr.rec_cnt,
                 :ctmstr.file_size,
                 :ctmstr.detl_data;
    
    if (SYS_DB_CHK_FAIL){
        if (SYS_DB_CHK_NOTFOUND){
            if ((memcmp(ctarg->blk_no, "0000", LEN_CTARG_BLK_NO) == 0) &&
                (memcmp(ctarg->seq_no,  "000", LEN_CTARG_SEQ_NO) == 0)){
                db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
                ex_syslog(LOG_ERROR, "[APPL_DM] %s c100_ctmstr_select(): "
                                    "FETCH(CTMSTR) 원장정보가 존재하지 않습니다.[%d]"
                                    "담당자 확인! MSG[%s]",
                                    __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR );
                c200_ctmst_curosr_close(ctx);
                return ERR_ERR;
            }
            else{
                SYS_DBG("더이상 정보가 존재하지 않습니다. ");
                c200_ctmst_curosr_close(ctx);
                return ERR_ERR;
            }
        }
        else{
            db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
            ex_syslog(LOG_ERROR, "[APPL_DM] %s c100_ctmstr_select(): "
                                "FETCH(CTMSTR) ERR[%d]"
                                "담당자 확인! MSG[%s]",
                                __FILE__, SYS_DB_ERRORNUM, SYS_DB_ERRORSTR );
            c200_ctmst_curosr_close(ctx);
            return ERR_ERR;
        }

    }

    /* -------------------------------------------------------------- */
    /* 조회정보 DATA SET                                                */
    /* -------------------------------------------------------------- */
    utol2an(ctmstr.blk_no,     LEN_CTHLAY_BLOCK_NO,    cthlay->block_no);
    utol2an(ctmstr.seq_no,     LEN_CTHLAY_SEQ_NO,      cthlay->seq_no  );
    utol2an(ctmstr.rec_cnt,    LEN_CTHLAY_REC_CNT,     cthlay->rec_cnt );
    /* 20080202 wizkim - cthlay_t에서 file_size 필드가 존재하지 않음       */
    /* rec_size에 file_size를 넣을 것 같음....                           */
    utol2an(ctmstr.file_size,  LEN_CTHLAY_REC_SIZE,    cthlay->rec_size);

    memset(cthlay->detl_data,  0x20,  LEN_CTHLAY_DETL_DATA);
    memcpy(cthlay->detl_data,  ctmstr.detl_data, strlen(ctmstr.detl_data));
    
    /* -------------------------------------------------------------- */
    /* 블럭번호 및 일련번호 CTARG SET                                      */
    /* -------------------------------------------------------------- */
    memcpy(ctarg->blk_no, cthlay->block_no,   LEN_CTARG_BLK_NO);
    memcpy(ctarg->seq_no, cthlay->seq_no,     LEN_CTARG_SEQ_NO);

    SYS_DBG(" ===================================================== ");
    SYS_DBG("             CTMSTR FETCH   DATA                       ");
    SYS_DBG(" ===================================================== ");
    SYS_DBG(" ctmstr.proc_date        = [%.4s]", ctmstr.proc_date    );
    SYS_DBG(" ctmstr.data_date        = [%.3s]", ctmstr.data_date    );
    SYS_DBG(" ctmstr.rec_cnt          = [%.6s]", ctmstr.rec_cnt      );
    SYS_DBG(" ctmstr.file_size        = [%.6s]", ctmstr.file_size    );
    SYS_DBG(" ctmstr.detl_area        = [%.100s]", ctmstr.detl_data  );
    SYS_DBG(" ===================================================== ");
    
    SYS_TREF;

    return ERR_ERR;

}
/* --------------------------------------------------------------------------------------------------------- */
static int c200_ctmst_curosr_close(ctn2400_ctx_t    *ctx)
{

    int                 rc = ERR_NONE;

    ctarg_t             *ctarg;

    ctarg  = (ctarg_t  *) &ctx->ctarg;

    if (g_ctmst_cursor == 1){
        EXEC SQL CLOSE CTMSTR_CUR_1;
        g_ctmst_cursor = 0;
    }

    /* -------------------------------------------------------------- */
    /* 블럭번호 및 일련번호 RESET                                         */
    /* -------------------------------------------------------------- */
    memset(ctarg->blk_no,   '0',    LEN_CTARG_BLK_NO);
    memset(ctarg->seq_no,   '0',    LEN_CTARG_SEQ_NO);

    return ERR_NONE;

}

/* --------------------------------------------------------------------------------------------------------- */
static int c300_csta_update(ctn2400_ctx_t   *ctx)
{

    int                 rc = ERR_NONE;
    cti0030f_t          cti0030f;
    ctarg_t             *ctarg;

    ctarg   = (ctarg_t  *) &ctx->ctarg;
    memset(&cti0030f, 0x00, sizeof(cti0030f_t));
    cti0030f.in.ctarg   = ctarg; 
    cti0030f.in.i_flag  = host_sta_flag;

    rc = cto0030f(&cti0030f);
    if (rc == ERR_ERR){
        c200_ctmst_curosr_close(ctx);
        return ERR_ERR;
    }

    return ERR_NONE;
}


/* --------------------------------------------------------------------------------------------------------- */
static int v000_gw_call(ctn2400_ctx_t   *ctx, int rspn_flag)
{

    int                 rc = ERR_NONE;
    char                *ptr;
    commbuff_t          dcb;
    sysgwinfo_t         *sysgwinfo;

    int                 retry_cnt 10;

    SYS_TRST;

    /* -------------------------------------------------------------- */
    SYS_DBG(" ====================  전송시작 ======================== ");
    SYS_DBG(" v000_gw_call: rspn_flag = [%d]"    , rspn_flag          );
    SYS_DBG(" v000_gw_call: send_len  = [%d]"    , ctx->host_send_len );
    SYS_DBG(" v000_gw_call: send_data = [%.100s]", ctx->host_send_data);
    /* -------------------------------------------------------------- */

    memset(&dcb, 0x00, sizeof(commbuff_t));
    rc = sysocbdb(ctx->cb, &dcb);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s v000_gw_call(): "
                             "COMMBUFF BACKUP ERROR"
                             "[해결방안]일괄전송 담당자 CALL",
                            __FILE__ );
        sysocbfb(&dcb);
        return ERR_ERR;
    }
    
}
/* --------------------------------------------------------------------------------------------------------- */