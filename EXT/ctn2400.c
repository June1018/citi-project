
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
        }else{
            db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        }

    }
}   
/* --------------------------------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------------------------------- */