
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
static int  a000_data_initial(int argc, char *argv[])
{

    int                 rc = ERR_NONE;

    SYS_TRSF;

    /* command argument 처리 */
    SYS_TRY(a100_parse_custom_args(argc, argv));

    /* command argument 처리 */
    SYS_TRY(a200_commbuff_init());

    /* RQ current index  */
    g_rq_idx = 0;

    /* tpcall cnt        */
    g_acall_cnt = 0;

    /* callback function register to tpacall response   */
    tpregcb(b000_tpacall_reply);

    /* SYSTEM GLOBAL변수에 자신의 서비스명 저장  */
    strcpy(g_arch_head.svc_name,    "SYRQWORK_DF");
    memset(g_arch_head.err_file_name, 0x00, sizeof(g_arch_head.err_file_name));


    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;
    return ERR_ERR;

}


/* ------------------------------------------------------------------------------------------------------------ */
static int a100_parse_custom_args(int argc, char *argv[])
{


    int                 c;  
    int                 i, num; 
    char                buff[512];
    char                *ptr; 
    extern char         *optarg;
    extern int          optind; 

    SYS_TRSF;

    //opterr = 0;
    g_acall_max = 0;
    memset(buff, 0x00, sizeof(buff));

    while(( c = getopt(argc, argv, "c:q:")) != EOF){
        /* -------------------------------------------------------------------- */
        SYS_DBG("GETOPT: -%c\n", c);
        /* -------------------------------------------------------------------- */

        switch(c){
        case 'c':
            g_acall_max = atoi(optarg);
            break;

        case 'q' :
            strcpy(buff, optarg);
            break;

        case '?':
            SYS_DBG("unreconized option : -%c %s", c argv[optind]);
            return ERR_ERR;   
        }
    }

    /* 응답을 받지 않고 최대 ACALL할 수 있는 갯수 */
    if (g_acall_max <= 0)
        g_acall_max = DFLT_MAX_AP_CALL_NUM;

    /* -------------------------------------------------------------------- */
    SYS_DBG("acall_num [%d]", g_acall_max);
    SYS_DGB("rq_name   [%s]", buff       );
    /* -------------------------------------------------------------------- */

    /* RQ 명과 RQ갯수를 구함 */
    memset(g_rq_info,   0x00, sizeof(g_rq_info));
    g_rq_num = 0;
    if (strlen(buff) <= 0){
        ex_syslog(LOG_ERROR, "[APPL_DM] %s a100_parse_custom_args(): RQ명 NOT FOUND",
                             __FILE__);
        return ERR_ERR;
    }

    /* RQ 하나인 경우  */
    strcat(buff, ":");
    ptr = strtok(buff, ":");
    if (ptr == NULL){
        strcpy(g_rq_info[g_rq_num].rq_name, buff);
        g_rq_info[g_rq_num++].acall_num = g_acall_max;
        return ERR_NONE;
    }

    /* RQ가 두개 이상인 경우 */
    while(1){
        strcpy(g_rq_info[g_rq_num++].rq_name, ptr);
        ptr += strlen(buff) + 1;
        strcpy(buff, ptr);
        if (strlen(buff) <= 0)
            break;

        ptr = strtok(buff, ":");
        if (ptr == NULL){
            if (strlen(buff) > 0)
                strcpy(g_rq_info[g_rq_num++].rq_name, buff);
            break;
        }
    }

    /* RQ가 두개이상인 경우 각 RQ및 처리건수를 동등하개 분배   */
    num = g_acall_max / g_rq_num;
    num = (num <= 0) ? 1 : num;
    for ( i = 0; i < g_rq_num; i++){
        g_rq_info[i].acall_num = num;
    }

    /* -------------------------------------------- */
    SYS_DBG("rq_num = [%d]", g_rq_num);
    /* -------------------------------------------- */

    SYS_TREF;

    return ERR_NONE;
}
/* ------------------------------------------------------------------------------------------------------------ */
static int  a200_commbuff_init(void)
{

    int                 rc = ERR_NONE;
    sysicomm_t          sysicomm;
    sysgwinfo_t         sysgwinfo;

    SYS_TRSF;

    /* set commbuff  */
    memset((char *)ctx, 0x00, sizeof(syrqwork_ctx_t));
    ctx->cb = &ctx->_cb; 

    /* SYSICOMM commbuff 항목 설정   */
    memset(&sysicomm, 0x00, sizeof(sysicomm_t));
    sysocbsi(ctx->cb, IDX_SYSICOMM, &sysicomm, sizeof(sysicomm_t));
    SYSICOMM->intl_tx_flag = 0;
    utoclck(SYSICOMM->ilog_no);
    strcpy(SYSICOMM->call_svc_name, "SYRQWORK_DF");

    SYSICOMM->pid_no    = getpid();
    strcpy(SYSICOMM->svc_name, "SYRQWORK_DF");

    /* GW Information COMMBUFF생성   */
    memset(&sysgwinfo, 0x00, sizeof(sysgwinfo_t));
    sysgwinfo.sys_type = SYSGWINFO_SYS_CORE_BANK;

    sysocbsi(ctx->cb, IDX_SYSGWINFO, &sysgwinfo, LEN_SYSGWINFO);

    SYS_TREF;

    return ERR_NONE;
    
}
/* ------------------------------------------------------------------------------------------------------------ */
int b000_tpacall_reply(UCSMSGINFO *reply)
{

    int                 rc = ERR_NONE;

    SYS_TRSF;

    g_acall_cnt -= 1;
    if (g_acall_cnt < 0)
        g_acall_cnt = 0;

    
    SYS_TREF;

    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
static int c000_rq_chk(void)
{


    int                 rc = ERR_NONE;
    int                 i, j;

    j = g_rq_idx + 1;
    for ( i = 0; i < g_rq_num; i++){
        if (j >= g_rq_num)
            j = 0;

        rc = tpqstat(g_rq_info[j].rq_name, TMAX_RPLY_QUEUE);
        if (rc <= 0) {
            j++;
            continue;
        }

        g_rq_idx = j;
        return rc; 
    }

    return ERR_ERR;
}


/* ------------------------------------------------------------------------------------------------------------ */
static int  d000_svc_call_proc(ind idx)
{

    int                 rc = ERR_NONE;
    char                *rqdata;
    char                svc_name[XATMI_SERVICE_NAME_LENGTH + 1];
    char                appl_code[LEN_APPL_CODE + 1];       //Decoupling 추가 
    char                rqdata_len;
    commbuff_t          dcb;  

    SYS_TRSF;


    rqdata = (char *) sys_tpalloc("CARRY", 2048);
    if (rqdata == NULL){
        ex_syslog(LOG_FATAL, "[APPL_DM]메모리 할당 애러 "
                             "[해결방안] 시스템 담당자 CALL");
        ex_syslog(LOG_ERROR, "[APPL_DM]%s c000_svc_call_proc():Memory alloc error[%d]"
                             "[해결방안] 시스템 담당자 CALL",
                             __FILE__, tperrno);
        sys_tpfree(rqdata);
        return ERR_ERR;
    }

    /* reply rq deque  */
    rqdata_len = 0;
    rc = tpdeq(g_rq_info[idx].rq_name, NULL, (char **)&rqdata, &rqdata_len, TPRQS);
    if (rc < 0){
        ex_syslog(LOG_FATAL, "[APPL_DM] c000_svc_call_proc(): TPDEQ error"
                             "[해결방안] TMAX 담당자 CALL");
        ex_syslog(LOG_ERROR, "[APPL_DM]%s c000_svc_call_proc():Memory alloc error[%d]"
                             "[해결방안] TMAX 담당자 CALL",
                             __FILE__, tperrno);
        sys_tpfree(rqdata);
        return ERR_ERR;
    }

    memset(svc_name, 0x00, sizeof(svc_name));
    memcpy(svc_name, rqdata, XATMI_SERVICE_NAME_LENGTH);
    utortrim(svc_name);

    /* -------------------------------------------- */
    SYS_DBG("CALL SVC NAME [%s]rqdata[%s]", svc_name, rqdata);
    /* -------------------------------------------- */

    /* commbuff 생성  */
    memset(&dcb, 0x00, sizeof(commbuff_t));

    rc = sysocbdb(ctx->cb, &dcb);

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%s c000_svc_call_proc():Commbuff Backup error"
                             "[해결방안] TMAX 담당자 CALL",
                             __FILE__);
        sys_tpfree(rqdata);
        sysocbfb(&dcb);
        return ERR_ERR;
    }

    /* EXTRECVDATA COMMBUFF set   */
    rqdata_len -= XATMI_SERVICE_NAME_LENGTH;

    rc = sysocbsi(&dcb, IDX_EXTRECVDATA, &rqdata[XATMI_SERVICE_NAME_LENGTH], rqdata_len);

    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%s c000_svc_call_proc():"
                             "COMMBUFF(EXTRECVDATA) SET ERROR "
                             "[해결방안] TMAX 담당자 CALL",
                             __FILE__ );
        sys_tpfree(rqdata);
        sysocbdb(&dcb);
        return ERR_ERR;
    } 
    /* ----------------------------------------------------------- */
    /* 개설서비스 분기위한 appl_code 설정 (rq_name으로 구분 )              */
    /* SVC_NAME 이 SYICGROUTE 경우 CD :chrq, chrq2 -> 086            */
    /* ----------------------------------------------------------- */
    if (memcmp(svc_name, "SYICGROUTE", 10) == 0){
        memset(appl_code,   0x00, sizeof(appl_code));

        if (memcmp(g_rq_info[idx].rq_name, "cdrq",  4) == 0 ||
            memcmp(g_rq_info[idx].rq_name, "cdrq2", 5) == 0 ){
            memcpy(appl_code, CD_CODE, LEN_APPL_CODE);
        }
        else if (memcmp(g_rq_info[idx].rq_name, "arsrq", 5) == 0){
            memcpy(appl_code, CD_CODE, LEN_APPL_CODE);
        }
        else if (memcmp(g_rq_info[idx].rq_name, "tlrq", 4) == 0){
            memcpy(appl_code, CD_CODE, LEN_APPL_CODE);
        }
        else if (memcmp(g_rq_info[idx].rq_name, "enrq", 4) == 0){
            memcpy(appl_code, CD_CODE, LEN_APPL_CODE);
        }

        rc = sysocbsi(&dcb, IDX_EXPARM, &appl_code, LEN_APPL_CODE);

        SYS_DBG("rq_name[%s]appl_code[%s]", g_rq_info[idx].rq_name, appl_code);
    }
    /* ------------------------------------------------------------------- */

    /* RQ 메모리 free  */
    sys_tpfree(rqdata);


    /* 업무서비스 call */
    rc = sys_tpacall(svc_name, &dcb, 0);
    if (rc == ERR_ERR){
        ex_syslog(LOG_ERROR, "[APPL_DM]%s c000_svc_call_proc():"
                             "%s 서비스 호출 ERROR: TPERRNO[%d]"
                             "[해결방안] TMAX 담당자 CALL",
                             __FILE__ , svc_name, tperrno);
        sysocbdb(&dcb);
        return ERR_ERR;    
    }

    sysocbdb(&dcb);

    g_acall_cnt += 1;

    SYS_TREF;

    return ERR_NONE;
}
/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */
