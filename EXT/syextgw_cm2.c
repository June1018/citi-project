
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
    }
}
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */