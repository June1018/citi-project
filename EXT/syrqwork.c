
/*  @file               syrqwork.c
*   @file_type          c source program
*   @brief              RQ Worker 프로그램 (RQ POLLING)
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
*   @generated at 2024/05/08 14:00
*   @history
*
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*
*
*/
/* ---------------------------------------------- include files ----------------------------------------------- */
#include <syscom.h>
#include <exdefine.h>
#include <usrinc/atmi.h>
#include <usrinc/usc.h>


/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define DFLT_MAX_AP_CALL_NUM        100
#define RQ_POLLING_INTERVAL         150000      /* 0.15 초 */


/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct syrqwork_ctx_s    syrqwork_ctx_t;
struct syrqwork_ctx_s {
    commbuff_t          _cb;
    commbuff_t          *cb;
};

typedef struct rq_info_s rq_info_t;
struct rq_info_s{
    char                rq_name[XATMI_SERVICE_NAME_LENGTH + 1];
    int                 acall_num;
};


/* ------------------------------------- exported global variables definitions -------------------------------- */
syrqwork_ctx_t   _ctx; 
syrqwork_ctx_t   *ctx = &_ctx;  

int                 g_acall_cnt;            /* 현재 호출한 서비스 갯수    */
int                 g_acall_max;            /* 최대 서비스 호출 갯수      */

int                 g_rq_num;               /* RQ 정보 갯수            */
int                 g_rq_idx;               /* RQ 정보 index          */
rq_info_t           g_rq_info[20];          /* RQ 정보                */


/* ------------------------------------------ exported function  declarations --------------------------------- */
static int          a000_initial(int argc,  char *argv[]);
static int          a100_parse_custom_args(int argc,  char *argv[]);
static int          a200_commbuff_init(void);
int                 b000_tpacall_reply(UCSMSGINFO *reply);
static int          c000_rq_chk(void);
static int          d000_svc_call_proc(int idx);


/* ------------------------------------------------------------------------------------------------------------ */
int usermain(int argc,  char *argv[])
{

    int                 rc  = ERR_NONE;
    int                 i, num;

    /* initial   */
    rc = a000_data_init(argc, argv);
    if (rc == ERR_ERR)
        exit(1);

    while(1){
        rc = tpschedule(RQ_POLLING_INTERVAL);
        if (g_acall_cnt >= g_acall_max)
            continue;

        /* RQ 데이타가 존재 하는지 검증  */
        num = c000_rq_chk();
        if (num == ERR_ERR)
            continue;

        num = MIN(num, g_rq_info[g_rq_idx].acall_num);
        for ( i = 0; i < num ; i++){
            if (g_acall_cnt >= g_acall_max)
                break;

            /* 업무서비스 호출  */
            d000_svc_call_proc(g_rq_idx);

            /* tpacall reply 처리 */
            tpschedule(-1);
        }
    }

    return ERR_NONE;
}

/* ------------------------------------------------------------------------------------------------------------ */
