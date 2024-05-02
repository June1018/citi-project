
/*  @file               syextgw_cm2.pc
*   @file_type          pc source program
*   @brief              대외기관 송수신 프로그램(a000_initial 부분이 cm1과 다름 )
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
*   @generated at 2024/05/02 14:00
*   @history
*
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*
*
*/

/* ---------------------------------------------- include files ----------------------------------------------- */
#include <sytcpwhead.h>
#include <bssess_stat.h>
#include <bssess_ip.h>
#include <network.h>
#include <usrinc/atmi.h>
#include <usrinc/usc.h>
#include <sqlca.h>
#include <mtietc.h>


/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define DEF_POLLING_INT             300
#define POLLING_DATA_LEN            24
#define line_size                   4
/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct syextgw_cm2_ctx_s    syextgw_cm2_ctx_t;
struct syextgw_cm2_ctx_s {
    commbuff_t          *cb;
};

typedef struct dp_inf_s dp_inf_t;
struct dp_inf_s{
    char                *dp     ;   /* malloc pointer   */
    int                 dp_free ;   /* do free Y/N      */
};


/* ------------------------------------- exported global variables definitions -------------------------------- */
syextgw_cm2_ctx_t   _ctx; 
syextgw_cm2_ctx_t   *ctx = &_ctx;  

char                g_svc_name[32];         /* G/W서비스명            */
int                 g_mode;                 /* Online , batch구분    */
int                 g_conn_type;            /* TCP/IP connect구분    */
int                 g_send_able;            /* 전송데이터 유뮤          */

extern  int         g_temp_len;             /* 임시사용 buffer에 저장된 데이터 길이 */
extern  char        g_temp_buf[8192];       /* 임시사용 buffer        */

extern  int         g_rtry_time;            /* 회선 재접속 시간         */
extern  int         g_rtry_cnt;             /* 회선 재접속 횟수         */

extern  int         g_tcp_header_len;       /* 통신헤더 길이           */
extern  int         g_tr_len;               /* TRANS CODE 길이       */
extern  int         g_tcp_header_len;       /* TCP Header 길이       */
extern  char        g_prog_id[32];          /* 회선정보 구분명 n option */
char                g_mti_name[20];         /* 구분명    - n option  */
char                g_van_type;             /* VAN TYPE - v option  */

extern  int         g_wmaxfd;               /* socket maximun number */
extern  fd_set      g_wset;                 /* select : send         */
extern  fd_set      g_wnewset;              /* select : send         */

extern  int         g_host_size;            /* 회선정보 갯수            */
extern  int         g_host_idx;             /* 회선정보 idx            */
extern  bssess_stat_t *g_bssess_stat;       /* 회선정보                */

extern  int         g_listen_size;          /* listen info           */
extern  int         *g_lstninfo;

extern  int         g_session_maxi;
extern  int         g_session_idx;
extern  int         g_session_size;
extern  tcp_session_t *g_cli_session;

mtietc_t            *g_mtietc;              /* MTIETC                */
int                 g_mti_size;             /* MTI  SIZE             */

/* ------------------------------------------ exported function  declarations --------------------------------- */
int                 syextgw_cm2(commbuff_t  *commbuff);
static int          a000_initial(int argc,  char *argv[]);
static int          a100_parse_custom_args(int argc,  char *argv[]);
static int          c000_request_from_session(int idx);
static int          c100_polling_proc(tcp_session_t *session);
static int          c900_recv_error(tcp_session_t *session);
static int          d000_reply_to_session(int idx);
static int          e000_not_wsession_ready(int idx, int cnt_check);
static int          x000_timeout_check(void);
static long         session_polling(void);
static long         polling_timeout_check(void);
static int          get_mtietc_info(void);
int                 get_recevie_length(char *dp);
int                 apsvrdone();

extern int          make_external_send_data(commbuff_t  *cb, int tr_flag);
extern int          make_req_poll_char(char *buff, time_t req_time);

/* ------------------------------------------------------------------------------------------------------------ */
int usermain(int argc,  char *argv[])
{

    int                 i, rc, fd, fd2;
    int                 select_ret, trycnt_check;
    char                url_name[LEN_BSSESS_IP_URL_NAME + 1];
    fd_set              wallset;
    time_t              tval, current_time;
    time_t              wchk_time1, wchk_time2;
    struct              timeval  tv;

    /* initial   */
    rc = a000_data_initial(argc, argv);
    if (rc == ERR_ERR)
        exit(1);

    time(&current_time);
    wchk_time2 = wchk_time1 = current_time;

    while(1){
        /* select socket 정보   */
        g_wset = wallset = g_wnewset;
        FD_ZERO(&g_wnewset);
        errno = 0;

        /* 전송중 데이터 존재 유무에 따라서 processing 이 다름    */
        select_ret = -1;
        if (g_send_able > 0)
            /* ucs processing */
            rc = tpuschedule(100);

            /* check write socket   */
            tv.tv_sec   = 0;
            tv.tv_usec  = 10000;

            select_ret = select(g_wmaxfd + 1, NULL, &g_wset, NULL, &tv);
            if (select_ret < 0){
                ex_syslog(LOG_ERROR,  "[APPL_DM]%s user main() select errno[%d]",
                                      __FILE__, errno);
                g_wnewset = wallset;
                sleep(1);
                continue;
            }
        }
        else{
            /* ucs processing    */
            rc = tpuschedule(DELT_TIMECHK_INT);
        }

        /* check for write trycount */
        time(&wchk_time1);
        if (wchk_time1 > wchk_time2){
            trycnt_check = 1;
            wchk_time2   = wchk_time1;
        }
        else{
            trycnt_check = 0;
        }

        /* accept client    */
        if (rc == UCS_USER_MSG){


            SYS_DBG("g_listen_size == [%d]",   g_listen_size);
            for (i = 0; i < g_listen_size; i++){
                if ((fd = g_lstninfo[i].fd )< 0)
                    continue;

                if (sys_tpissetfd(fd)) {
                    if ((fd2 = network_accept(fd, url_name)) < 0 ){
                    ex_syslog(LOG_FATAL, "[APPL_DM] client socket accept error");
                    ex_syslog(LOG_ERROR, "[APPL_DM] %s client socket accept error: [%d]", 
                                            __FILE__, errno);
                    }
                    else{
                        if (chk_client_ip(fd2, url_name) < 0)
                            continue;

                            add_client_session(fd2, g_lstninfo[i].cidx, INBOUND_SESSION, 1,1);

                    }
                }
            }
        }

        /* request from client    */
        for (i = 0; i <= g_session_maxi; i++){
            if ((fd = g_cli_session[i].fd) < 0)
                continue;

            if ((rc == UCS_USER_MSG) || (select_ret > 0)) {
                if (sys_tpissetfd(fd)) {
                    c000_request_from_session(i);
                }

                if (FD_ISSET(fd, &wallset)){
                    if (FD_ISSET(fd, &g_wset)){
                        d000_reply_to_session(i);
                    }
                    else{
                        //
                        e000_not_wsession_ready(i, trycnt_check);
                    }
                }
            }
            else if (FD_ISSET(fd, &wallset)){
                e000_not_wsession_ready(i, trycnt_check);
            }
        }

        /* check timeout & connect   */
        time(&tval);
        tval -= current_time;
        if (tval >= DELT_TIMECHK_INT){
            current_time = tval;
            x000_timeout_check();
            polling_timeout_check();
            all_session_connect(OUTBOUND_SESSION, 1, 0, 3);
            session_listen();
        }
    }

    return ERR_NONE;
}

/* ------------------------------------------------------------------------------------------------------------ */
int syextgw_cm2(commbuff_t  *commbuff)
{

    
    int                 i, rc, len, idx, size;

    dp_inf_t            st_dpi;                 /* malloc 관리용1   */
    dp_inf_t            *dpi = &st_dpi;         /* malloc 관리용2   */


    /* 에러코드 초기화    */
    sys_err_init();

    /* set commbuff  */
    memset((char *)ctx, 0x00, sizeof(syextgw_cm2_ctx_t));
    ctx->cb = commbuff;
    dpi->dp_free = 0;

    /* SYSTEM GLOBAL 변수에 자신의 서비스명 저장     */
    strcpy(g_arch_head.svc_name, g_svc_name);

    /* 입력데이터 검증    */
    if (SYSGWINFO == NULL){
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR,  "INPUT DATA ERR");
        return ERR_ERR;
    }

    /* --------------------------------------------------- */
    PRINT_SYSGWINFO(SYSGWINFO);
    /* --------------------------------------------------- */

    SYS_DBG("+++++++++++++++++++++++++++++ SYSGWINFO->cont_type[%d]", SYSGWINFO->conn_type);

    /* 회선관련 명령어 검증    */
    if (SYSGWINFO->conn_type > 0){
        rc = session_command_proc(SYSGWINFO->conn_type, SYSGWINFO->func_name);
        return rc;
    }

    //SYS_DBG("+++++++++++++++++++++++++++++ EXTSENDDATA[%s]", EXTSENDDATA);

    /* 입력데이터 검증   */
    if (EXTSENDDATA == NULL){
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR,  "INPUT DATA ERR");
        return ERR_ERR;

    }

    /* 대외기관 연결정보 search     */
    rc = send_session_search(SYSGWINFO->func_name, &idx);
    if (rc == ERR_ERR){
        if (dpi->dp_free){      /* dpi->dp  */
            free(dpi->dp_free);
                 dpi->dp_free = 0;
        }
        SYS_HSTERR(SYS_NN, ERR_SVC_CONERR, "EXTERNAL NOT CONNECT");
        return ERR_ERR;
    }

    /* --------------------------------------------------- */
    PRINT_SESSION_STAT(&bssess_stat, g_cli_session[idx].cidx);
    /* --------------------------------------------------- */

    SYS_DBG("+++++++idx[%d] g_cli_session[idx].tr_flag[%d]", idx, g_cli_session[idx].tr_flag);

    /* 전송데이터 생성 */
    rc = make_external_send_data(ctx->cb, g_cli_session[idx].tr_flag);
    if (rc == ERR_ERR)
        return ERR_ERR;

    /* 전송할 버퍼 할당 */
    size = g_temp_len + 100;

    if (dpi->dp_free == 0){
        dpi->dp = (char *) malloc(size);

        if (dpi->dp == NULL){
            ex_syslog(LOG_FATAL, "[APPL_DM] 메모리할당  error [해결방안]시스템 담당자 call");
            ex_syslog(LOG_ERROR, "[APPL_DM] %s syextgw_cm2()메모리 할당 error: [%d] [해결방안]시스템 담당자call", __FILE__, size);
            SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "MEMORY ALLOC ERR");
            return ERR_ERR;
        }else{
            dpi->dp_free = 1;
        }
    }

    if (dpi->dp_free){
        /* 전송데이터 복사 */
        memcpy(dpi->dp, g_temp_buf, g_temp_len);
    }
    len = g_temp_len;

    /* --------------------------------------------------- */
    SYS_DBG("+external send data make g_temp_len[%d]g_temp_buff[%s]size[%d]",g_temp_len,g_temp_buf, size);
    /* --------------------------------------------------- */

    /* 호출방식에 따라서 처리   */
    if (IS_SYSGWINFO_CALL_TYPE_SAF){
        /* --------------------------------------------------- */
        SYS_DBG("g_cli_session[%d]s remider wlen[%d]", idx, g_cli_session[idx].wlen);
        /* --------------------------------------------------- */

        if (g_cli_session[idx].wlen > 0)
            rc = session_linked_list(ctx->cb, &g_cli_session[idx], dpi->dp, len, SYSGWINFO->time_val);
        else
            rc = session_immediately_send(ctx->cb, &g_cli_session[idx], dpi->dp, len, -1);

        if (rc == ERR_ERR){
            if (dpi->dp_free){
                free(dpi->dp_free);
                dpi->dp_free = 0;
            }

            return ERR_ERR;
        }

        /* 데이터전송을 위하여 select에 set  */
        FD_SET(g_cli_session[idx].fd,  &g_wnewset);
        g_wmaxfd = MAX(g_wmaxfd, g_cli_session[idx].fd);
    }
    else{
        /* 현재는 대외기관에서 직접 응답을 받는 서비스는 없고 요청과 응답이
           분리되어 있으므로 이 부분은 개발하지 않음..
        */
        if (dpi->dp_free){
            free(dpi->dp_free);
            dpi->dp_free = 0;
        }

        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "GWINFO CALL TYPE ERROR");
        return ERR_ERR;
    }

    return ERR_NONE;
}


/* ------------------------------------------------------------------------------------------------------------ */
static int  a000_data_initial(int argc, char *argv[])
{

    int                 rc = ERR_NONE;
    int                 i;

    SYS_TRSF;

    /* select 변수 초기화   */
    g_wmaxfd = 0;
    FD_ZERO(&g_wset);
    FD_ZERO(&g_wnewset);

    /* session 배열 index of OutBound 오류 제거를 위한 초기화    */
    g_session_idx = 0;

    /* command argument 처리     */
    SYS_TRY(a100_parse_custom_args(argc, argv));

    /* 대외기관 연결 정보를 구함   */
    SYS_TRY(get_connect_info());

}
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */