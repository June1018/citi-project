
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

    /* 서버모드 session 정보 초기화 */
    SYS_TRY(init_listen(g_host_size));

    /* 회선정보 listen 정보 연결   */
    for (i = 0; i < g_listen_size; i++){
        /* 서버모든인 경우에만 처리   */
        if (g_bssess_stat[i].conn_type[0] == '2')
            g_lstninfo[i].cidx  = i;
        else 
            g_lstninfo[i].cidx  = -1;
    }

    /* client session 정보 초기화   */
    g_session_maxi = -1;
    SYS_TRY(init_session(MAX_CLI_SESSION_EXT));
    /******************************************************************************************
     BSSESS_STAT 테이블의 prod_name 필드를 활용하여 전문 헤더별료 활용               
    
     결제원 경우 TCP_TP : prod_name[4] == 'T'                                        
     기타기관은 TCP_2와 같이 셋팅 (2,3,4,5,67,8,9,0 사용 )                           
                                                                                     
     tr_flag 2 : size(4)+실전문 : size값은 실전문의 길이 (00101234567890)            
     tr_flag 3 : size(4)+실전문 : size값은 전체    길이 (0010123456)                 
                                                                                     
     tr_flag 4 : TR(9)+size+실전문 : size값은 실전문의 길이 (_________00101234567890)
     tr_flag 5 : TR(9)+size+실전문 : size값은 실전문의 길이 (_________0010123456)

     *******************************************************************************************/

    for (i = 0; i < g_host_size; i++){
        g_cli_session[i].tr_flag = g_bssess_stat[i].prod_name[4] - (int)('0');

        //결제원용은 tr_flag=1을 아래 루틴에서 세팅한다. 
    }


    /* 회선정보와 session 정보 연결  */
    for (i = 0; i < g_host_size; i++){
        /* client 모드인 경우에만 처리    */
        if (g_bssess_stat[i].conn_type[0] == '1'){
            g_cli_session[i].status     = READY;
            g_cli_session[i].cidx       = i; 

            /* TRANS_ID     FLAG SET   */
            if (g_bssess_stat[i].prod_name[4] == 'T') g_cli_session[i].tr_flag      = 1;
            if (g_bssess_stat[i].prod_name[5] == 'P') g_cli_session[i].poll_flag    = 0;
        }
        else{
            g_cli_session[i].cidx = -1;
        }
    }


    for (i = 0; i < g_host_size; i++){
        SYS_DBG("g_cli_session[%d]tr_flag[%s]", i, g_cli_session[i].tr_flag);
    }


    /* DB연결정보 clear    */
    set_session_init(g_prog_id, g_svc_name);

    /* SYSTEM GLOBAL 변수에 자신의 서비스명을 저장    */
    strcpy(g_arch_head.svc_name,    g_svc_name);
    memset(g_arch_head.err_file_name,   0x00, sizeof(g_arch_head.err_file_name));

    /* 대외기관 연결   */
    all_session_connect(OUTBOUND_SESSION, 1, 0, 3);


    /* 기타에서 사용할 MTI 정보를 가져온다.  */
    rc = get_mtietc_info();
    if (rc == ERR_ERR){
        ex_syslog(LOG_FATAL, "[APPL_DM] a000_initial() :get_mtietc_info [해결방안] 시스템 담당자 CALL");
        ex_syslog(LOG_ERROR, "[APPL_DM] %s a000_initial(): g_van_type[%d]g_mti_name[%s]", __FILE__, g_van_type, g_mti_name);
        return ERR_ERR;
    }

    /* 대외기관 서버모드 listen */
    session_listen();

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;
    return ERR_ERR;

}


/* ------------------------------------------------------------------------------------------------------------ */
static int a100_parse_custom_args(int argc,  char *argv[])
{
    int                 rc  = ERR_NONE;
    extern  char        *optarg;
    extern  int         optind;

    //

    g_tr_len            = 0;
    g_tcp_header_len    = 0;
    g_head_len          = 0;

    while((c = getopt(argc, argv, "w:s:r:c:m:h:p:t:d:v:n")) != EOF){
        /* ---------------------------------------------------- */
        SYS_DBG("GETOPT: -%c/n",   c);
        /* ---------------------------------------------------- */

        switch(c){
            case 'w':
                strcpy(g_prog_id, optarg);
                break;

            case 's':
                strcpy(g_svc_name, optarg);
                break;

            case 'r':
                g_rtry_time = atoi(optarg);
                break;

            case 'c':
                g_rtry_cnt  = atoi(optarg);
                break;

            case 'h':
                /* TR-code(9)size(4)와 같은 헤더라인 -h 13 -t 9 PRODUCT명은 TCP_4 또는 TCP_5 */
                g_head_len  = atoi(optarg);
                break;

            case 'p':   /* TCP/IP connect protocol   */
                g_conn_type = atoi(optarg);
                break;

            case 't':   /* TRAN CODE len             */
                g_tr_len    = atoi(optarg);
                break;

            case 'd':   /* TCP HEADER len            */
                g_tcp_header_len = atoi(optarg);
                break;

            case 'v':   /* van_type                  */
                g_van_type  = atoi(optarg);
                break;

            case 'n':   /* MTI NAME                  */
                strcpy(g_mti_name,  optarg);
                break;

            case '?':
                SYS_DBG("unrecognized option : %s", argv[optind]);
                return ERR_ERR;
                
        }
    }

    if (g_rtry_time         <= 0) g_rtry_time       = DFLT_SESSION_RTRY_INT;
    if (g_rtry_cnt          <= 0) g_rtry_cnt        = MAX_SESSION_RETRY_CNT;
    if (g_tr_len            <= 0) g_tr_len          = 0;
    if (g_tcp_header_len    <= 0) g_tcp_header_len  = 0; /* 결제원용을 사용하기 위해서는 tmax_config에 해당 값 셋팅   */
    if (g_head_len          <= 0) g_head_len        = g_tr_len + g_tcp_header_len;

    if (g_head_len <= 0){
        ex_syslog(LOG_FATAL,    "[APPL_DM] a100_parse_custom_arg(): HEADER Length error"
                                "[해결방안] 업무 담당자 call");
        ex_syslog(LOG_ERROR,    "[APPL_DM] %s a100_parse_custom_arg(): HEADER Length error"
                                "[해결방안] 업무담당자 call"
                                , __FILE__);
        return ERR_ERR;
    }

    /* ----------------------------------------------------------------------------- */
    SYS_DBG("prog_id                =[%s]", g_prog_id);
    SYS_DBG("rtry_time              =[%d]", g_rtry_time);
    SYS_DBG("rtry_cnt               =[%d]", g_rtry_cnt);
    SYS_DBG("svc_name               =[%s]", g_svc_name);
    SYS_DBG("conn_type              =[%d]", g_conn_type);
    SYS_DBG("tr_len                 =[%d]", g_tr_len);
    SYS_DBG("tcp_header_len         =[%d]", g_tcp_header_len);
    SYS_DBG("head_len               =[%d]", g_head_len);
    SYS_DBG("van_type               =[%d]", g_van_type);
    SYS_DBG("mti_name               =[%s]", g_mti_name);
    /* ----------------------------------------------------------------------------- */

    /* 서비스명 검증    */
    if ((strlen(g_svc_name) == 0)||
        (g_svc_name[0]      == 0x20))
        return ERR_ERR;

    return ERR_NONE;

}


/* ------------------------------------------------------------------------------------------------------------ */
static int c000_request_from_session(int idx)
{
    int                 i, rc, len, size, m;
    char                *dp, *dp2;
    char                jrn_no[LEN_JRN_NO + 10];
    char                rq_name[32], svc_name[32];
    char                tmp[256];
    tcp_session_t       *session;
    sysgwinfo_t         sysgwinfo;
    sysicomm_t          sysicomm;
    commbuff_t          cb;
    mtietc_t            mtietc;
    char                match_val[20];
    int                 match_len;



    SYS_TRSF;

    /* 에러 코드 초기화   */
    sys_err_init();

    /* ----------------------------------------------------------------------------- */
    SYS_DBG("c000_request_from_session: idx[%d]", idx);
    PRINT_SESSION_STAT(g_bssess_stat, g_cli_session[idx].cidx);
    /* ----------------------------------------------------------------------------- */

    /* 해당 session 검증   */
    session = (tcp_session_t *) & g_cli_session[idx];
    if (session->fd < 0){
        sys_tpissetfd(session->fd);
        return ERR_NONE;
    }

    rc = read_from_session_async(session);
    if ((rc == READ_ERROR) ||
        (rc == READ_CLOSE)){
        /* ---------------------------------------------------- */
        SYS_DBG("READ_ERROR:[%d] READ_CLOSE[%d]", READ_ERROR, READ_CLOSE);
        SYS_DBG("in case : rc[%d]", rc);
        /* ---------------------------------------------------- */
        c900_recv_error(session);
        return ERR_ERR;
    }

    if (rc == READ_INCOMPLETE){         /* msg not ready yet    */
        if (session->r_trycount > READY_TRYCNT_THRESHOLD) {
            /* ------------------------------------------------ */
            SYS_DBG("in case rc[%d]=READY INCOMPLETE and READY_TRYCNT THRESHOLD",rc);
            /* ------------------------------------------------ */
            c900_recv_error(session);
        }
        return ERR_NONE;
    }

    session->tdata     = session->rdata; 
    session->tlen      = session->rlen;   
    session->rdata     = NULL;
    session->rlen      = 0;

    /* polling 전문을 전송하지 않은 경우    */
    if (session->poll_flag == 0)
        time(&session->poll_tval);

    dp  = session->tdata;
    len = session->tlen; 

    /* --------------------------------------------------------------------- */
    SYS_DBG("c000_request_from_session :idx[%d], len[%d]",idx, len);
    /* --------------------------------------------------------------------- */
#ifdef  _DEBUG
    /* --------------------------------------------------------------------- */
    size = (len > 8192) ? 8192 : len;
    utohexdp((char *)dp, size);
    /* --------------------------------------------------------------------- */
#endif

    /* check poll message  */
    rc = c100_check_pollin(session);
    if (rc == READY_POLLING){
        return ERR_NONE;
    }
    else if (rc == ERR_ERR){
        c900_recv_error(session);
        return ERR_NONE;
    }

    /* -------------------------------------------------------------- */
    /* IF first byte is EBCDIC, first 9 bytes conversed to ASCII      */
    /* -------------------------------------------------------------- */
    if ((unsigned char)dp[0] > 0x5a){
        memcpy(tmp, &dp[0], 9);
        utoeb2as((unsigned char *)tmp, 9, (unsigned char)tmp);
        memcpy(&dp[0], tmp, 9);
    }

    /* 분기정보에서 저장할 RQ명 서비스명 search    */
    memset(rq_name , 0x00, sizeof(rq_name));
    memset(svc_name, 0x00, sizeof(svc_name));

    memset(match_val,   0x00, sizeof(match_val));
    for (m=0; m<g_mti_size; m++){
        if (g_mtietc[m].match_type[0] == '1'){
            SYS_DBG("INPUT data[%.*s] match_val[%s]", g_mtietc[m].pos1_len, &session->tdata[g_mtietc[m].pos1_idx], g_mtietc[m].match_val);
            if (memcmp(&session->tdata[g_mtietc[m].pos1_idx], g_mtietc[m].match_val, g_mtietc[m].pos1_len) == 0){
                strcpy(rq_name,     g_mtietc[m].rq_name);
                strcpy(svc_name,    g_mtietc[m].svc_name);
                break;
            }
        }
    }

    SYS_DBG("MTIETC rq_name[%s]svc_name[%s]", rq_name, svc_name);

    
    /* 전송할 버퍼 할당 */
    size = len + 100;
    dp2  = (char *) sys_tpalloc("CARRY", size);
    if (dp2 == NULL){
        ex_syslog(LOG_FATAL,    "[APPL_DM]메모리 할당 에러 "
                                "[해결방안] 시스템 담당자 CALL");
        ex_syslog(LOG_ERROR,    "[APPL_DM] %s c000_request_from_session(): 메모리 할당  에러 %d"
                                "[해결방안] 시스템 담당자 CALL",
                                __FILE__, size);
    
        /* message logging   */
        gw_err_logging(session, "RQ WRITE ERROR:MALLOC");
        goto SYS_CATCH;
    }

    /* 수신할 데이터 복사  */
    memcpy(dp2, svc_name, 32);
    switch(g_cli_session[g_session_idx].tr_flag){
        case 0:
            memcpy(dp2, svc_name, 32);
            memcpy(&dp2[32], session->tdata, len);
            len = session->tlen + 32;
            SYS_DBG("RQ data[%d][%.*s]", len, len, dp2);
            break;

        case 1:
            /* ============================= */
            /* 금결원 TCP/IP전송                */
            /* TR_CODE + 실전문 복사            */
            /* ============================= */
            memcpy(&dp2[32],            &session->tdata[0],          g_tr_len);
            memcpy(&dp2[32 + g_tr_len], &session->tdata[g_head_len], len - g_header_len);
            len = session->tlen + 32;
            break;
        case 2:
        case 3:
        case 4:
        case 5:
            memcpy(dp2      , svc_name, 32);
            memcpy(&dp2[32] , session->tdata, len);
            len = session->tlen + 32;
            SYS_DBG("RQ data[%d][%.*s]", len, len, dp2);
            break;
    }

    /* RQ write   */
    rc = sys_tpenq(rq_name, dp2, len, 0);
    SYS_DBG("##################  sys_tqenq rc[%d] ####################", rc);
    if (rc == ERR_ERR){
        ex_syslog(LOG_FATAL, "[APPL_DM] RQ WRITE ERROR"
                             "[해결방안] 시스템 담당자 CALL");
        ex_syslog(LOG_ERROR, "[APPL_DM] %s c000_request_from_session(): "
                             "[%s] RQ WRITE ERROR : TPERRNO[%d]"
                             "[해결방안] 시스템 담당자 CALL",
                                __FILE__, rq_name, tperrno);

        /* message logging   */
        sprintf(tmp, "%s RQ WRITE ERROR ", rq_name);
        gw_err_logging(session, tmp);
    }

    /* clear 수신 buffer */
    sys_tpfree(dp2);

SYS_CATCH:

    free(session->tdata);
    session->tdata = NULL;
    session->tlen  = 0;

    SYS_TREF;

    return ERR_NONE;
}

/* ------------------------------------------------------------------------------------------------------------ */
static int c100_polling_proc(tcp_session_t  *session)
{

    int                 rc    , len;
    char                *dp;

    dp  = session->tdata;
    len = session->tlen;

    /*  POLLING DATA 처리   */
    /*  POLLING 요청에 처리 유무 설정    */


    /* polling 요청에 대한 응답인 경우   */
    if (memcmp(&dp[7], "RESPOLL", 7) == 0){
        time(&session->poll_tval);
        session->poll_flag = 0;
        free(session->tdata);
        session->tdata  = NULL;
        session->tlen   = 0;
        return READ_POLLING;   
    }

    /* POLLING 요청 메세지인경우    */
    if (memcmp(&dp[7], "RESPOLL",   7) != 0)
        return ERR_NONE;

    /* POLLING 응답 전문생성       */
    memcpy(&dp[7], "RESPOLL"    ,   7);

    SYS_DBG("RESPONSE RESPOLL DATA data[%.*s]", len, dp);

    rc = network_write(session->fd, dp, len);
    if (rc != len)
        return ERR_ERR;

    free(session->tdata);
    session->tdata  = NULL;
    session->tlen   = 0;

    return READ_POLLING;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int c900_recv_error(tcp_session_t    *session)
{
    int                 i;
    

    i = session->cidx;
    if (i >= 0){
        ex_syslog(LOG_ERROR,   "[APPL_DM] %s c000_request_from_session():대외기관 수신 ERROR"
                               "연결을 끊음: symbname[%s] portno[%s]prog_id[%s]",
                               __FILE__, g_bssess_stat[i].symb_name,
                               g_bssess_stat[i].port_no, g_prog_id );
    }

    /* 대외기관에 전송할 데이터가 남아 있는 경우    */
    if (session->wlen > 0){
        /* 요청프로그램 에러인 경우   */
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR,  "DATA SEND ERR");
        sys_tprelay("EXTRELAY", &session->cb, 0, &session->tmax_ctx);

        /* 회선이 close 됨으로 linked list에 저장된 정보도 전송할 수 없음  */
        session->session_linked_delete(session);

    }

    cli_session_close(session);

    return ERR_ERR;
}


/* ------------------------------------------------------------------------------------------------------------ */
static int d000_reply_to_session(int idx)
{

    int                 rc;
    tcp_session_t       *session;

    /* ------------------------------------------------------------------ */
    SYS_DBG("d000_reply_to_session: idx[%d]", idx);
    /* ------------------------------------------------------------------ */

    /* 해당 session 검증   */
    session = (tcp_session_t *) &g_cli_session[idx];
    if ((session->fd     < 0) ||
        (session->wdata == NULL))
        return ERR_NONE;

    /* ------------------------------------------------------------------ */
    //SYS_DBG("d000_reply_to_session: idx[%d]", idx);
    /* ------------------------------------------------------------------ */

    rc = write_to_session_async(session);
    if (rc == WRITE_ERROR){
        /* 전송중 건수 감소   */
        if (session->wflag == SYS_TRUE){
            session->wflag = SYS_FALSE;
            g_send_able--;
            if (g_send_able < 0)
                g_send_able = 0;
        }

        /* 전송할 데이터 해제 */
        if (session->wdata != NULL){
            free(session->wdata);
            session->wdata = NULL;
            session->wlen  = 0;
        }

        /* 요청 프로그램 에러 전송 */
        SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "DATA SEND ERR");
        sys_tprelay("EXTRELAY", &session->cb, 0, &session->tmax_ctx);

        /* 회선이 close됨으로 linked list 에 저장된 정보를 전송할 수 없음.   */
        session_linked_delete(session);
        cli_session_close(session);

        return ERR_ERR;
    }

    if (rc == WRITE_INCOMPLETE) {       /* msg not fully written yet */
        /* 전송중 건수 증가   */
        if (session->wflag == SYS_FALSE){
            session->wflag =  SYS_TRUE;
            g_send_able++;
        }
        FD_SET(session->fd, &g_wnewset);
        g_wmaxfd = MAX(g_wmaxfd, session->fd);

        /* polling 전문을 전송하지 않은 경우 */
        if (session->poll_flag == 0)
            time(&session->poll_tval);

        return ERR_NONE;
    }

    /* 전송중 건수 감소  */
    if (session->wflag == SYS_TRUE){
        session->wflag =  SYS_FALSE;
        g_send_able--;
        if (g_send_able < 0)
            g_send_able = 0;
    }

    /* 전송할 데이터 해제 */
    FD_CLR(session->fd, &g_wnewset);
    free(session->wdata);
    session->wdata  = NULL;
    session->wlen   = 0;

    /* polling 전문을 전송하지 않은 경우 */
    if (session->poll_flag == 0)
        time(&session->poll_tval);

    /* 대외기관으로 부터 응답을 받지 않거나 
       또는 대외기관응답 데이터를 전송하지  않는 경우  
    */
    sys_tprelay("EXTRELAY", &session->cb,   0, &session->tmax_ctx);
    sysocbfb(&session->cb);
    memset(&session->cb,    0x00, sizeof(commbuff_t));

    /* --------------------------------------------------------------- */
    SYS_DBG("d000_reply_to_session save_cnt [%d]"   , session->tmax_ctx);
    /* --------------------------------------------------------------- */

    /* 전송할 다음 데이터가 있는지 검증하여 처리   */
    get_session_next_msg(session);
    session->wait_time = -1;

    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
static int  e000_not_wsession_ready(int idx, int cnt_check)
{

    time_t          tval;
    tcp_session_t   *session;

    /* --------------------------------------------------------------- */
    SYS_DBG("e000_not_wsession_ready :idx [%d]"   , idx  );
    /* --------------------------------------------------------------- */

    session = (tcp_session_t *) &g_cli_session[idx];
    if (session->fd < 0){
        if (session->tdata != NULL)
            free(session->tdata);
        session->tlen;
        if (session->rdata != NULL)
            free(session->rdata);
        if (session->wdata != NULL)
            free(session->wdata);
        init_session_elem(session);
        return ERR_ERR;
    }

    if (cnt_check){
        time(&tval);
        if (tval > (session->wrt_tval + SEND_INTERVAL_TIME)){
            /* 요청프로그램에 에러 전송   */
            SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "DATA SEND ERR");
            sys_tprelay("EXTRELAY" , &session->cb, 0, &session->tmax_ctx);

            /* 전송중  건수 감소  */
            if (session->wflag == SYS_TRUE){
                session->wflag  = SYS_FALSE;
                g_send_able--;
                if (g_send_able < 0)
                    g_send_able = 0;
            }

            /* 회선이 close됨 linked list 저장된 정보도 전송할 수 없음   */
            session_linked_delete(session);
            cli_session_close(session);
        }
        else{
            FD_SET(session->fd, &g_wnewset);
            g_wmaxfd = MAX(g_wmaxfd, session->fd);
        }
    }
    else{
        FD_SET(session->fd, &g_wnewset);
        g_wmaxfd = MAX(g_wmaxfd, session->fd);
    }

    return ERR_NONE;
}


/* ------------------------------------------------------------------------------------------------------------ */
static int x000_timeout_check(void)
{

    int                 i, rc;  
    time_t              tval;
    tcp_session_t       *session;
    bssess_stat_t       session_stat;

    time(&tval);
    for (i = 0; i <= g_session_maxi; i++){
        session = (tcp_session_t *) &g_cli_session[i];
        if ((session->fd        <  0) ||
            (session->wait_time <= 0))
            continue;

        if ((session->wrt_tval + session->wait_time) > tval)
            continue;

        /* --------------------------------------------------------------- */
        SYS_DBG("x000_timeout_check :idx [%d]wlen[%d]" , idx, session->wlen);
        /* --------------------------------------------------------------- */

        /* 전송중 건수 감소  */        
        if (session->wflag == SYS_TRUE){
            session->wflag  = SYS_FALSE;
            g_send_able--;
            if (g_send_able < 0)
                g_send_able = 0;
        }

        /* 전송할 데이터 해제 */
        if (session->wlen > 0){
            FD_CLR(session->fd, &g_wnewset);
            free(session->wdata);
            session->wdata  = NULL;
            session->wlen   = 0;
        }

        SYS_HSTERR(SYS_NN, ERR_SVC_TIMEOUT, "SERVICE TIMEOUT ERR");
        sys_tprelay("EXTRELAY", &session->cb, 0, &session->tmax_ctx);

        sysocbfb(&session->cb);
        memset(&session->cb, 0x00, sizeof(commbuff_t));

        /* 전송중에 타임아웃 발생하며 session close 처리 
           왜냐하면 데이터를 모두 전송하지 않으므로 다음 데이터 전송시 문제 가 발생함 */
        if (session->writing_len > 0){
            /* 회선이 close 됨으로 linked list 에 저장된 정보도 전송할 수 없음 */
            session_linked_delete(session);
            cli_session_close(session);
        }
    }

    /* polling 처리  */
    session_polling();

    /* 저장된 데이터도 타임 아웃 검증 */
    session_linked_list_timeout();

    /* 에러코드 초기화 */
    sys_err_init();

    return ERR_NONE;

}
/* ------------------------------------------------------------------------------------------------------------ */
static long  session_polling(void)
{


    int                 rc = ERR_NONE;
    int                 i;
    char                pmsg[1024];
    time_t              tval;
    tcp_session_t       *session;
    int                 tmp_polling_timne = 30 * 1; /* 60 sec * minute  */

    /* polling message */
    time(&tval);
    memset(pmsg,    0x00, sizeof(pmsg));
    make_req_poll_data(pmsg, tval);



    for ( i = 0; i <= g_session_maxi; i++){
        session = (tcp_session_t *) &g_cli_session[i];
        if (session->fd < 0)
            continue;

        //SYS_DBG("Polling information -> poll_flag _gs");
        /* polling check */
        //if((session->poll_flag == -1))
        if ((session->poll_flag == -1) || ((session->poll_tval + tmp_polling_timne) > tval)){
            //s

            continue;
        }


        /* ----------------------------------------------------------------------- */
        SYS_DBG("SEND REQPOLL DATA:idx[%ld][%d][%s]" , i, POLLING_DATA_LEN, pmsg);
        /* ----------------------------------------------------------------------- */

        rc = network_write(session->fd, pmsg, POLLING_DATA_LEN);
        if (rc != POLLING_DATA_LEN ){
            ex_syslog(LOG_ERROR, "[APPL_DM] %s polling 데이터 전송 오류 ", __FILE__);

            /* 전송할 데이터가 있는 경우 */
            if (session->wlen > 0){
                FD_CLR(session->fd, &g_wnewset);
                free(session->wdata);
                session->wdata  = NULL;
                session->wlen   = 0;

                SYS_HSTERR(SYS_NN, ERR_SVC_TIMEOUT, "POLLING SEND ERR - TIMEOUT ERR");
                sys_tprelay("EXTRELAY", &session->cb, 0, &session->tmax_ctx);

                sysocbfb(&session->cb);
                memset(&session->cb,    0x00, sizeof(commbuff_t));
            }

            /* 회선이 close 됨으로 linked list에 저장된 정보도 전송할 수 없음   */
            session_linked_delete(session);
            cli_session_close(session);
            continue;
        }

        session->poll_tval = tval;
        session->poll_flag = 1;
    }

    return ERR_NONE;
    
}

/* ------------------------------------------------------------------------------------------------------------ */
static long  polling_timeout_check(void)
{

    int                 rc = ERR_NONE;
    int                 i;
    time_t              tval;
    tcp_session_t       *session;

    time(&tval);
    for ( i = 0; i <= g_session_maxi; i++){
        session = (tcp_session_t *)&g_cli_session[i];
        if ((session->fd        <= 0)   ||
            (session->poll_flag != 1))
            continue;

        if ((session->poll_tval + DEF_POLLING_INT) > tval)
            continue;

        /* ------------------------------------------------------------- */
        SYS_DBG("polling timeout:idx[%ld]", i);
        /* ------------------------------------------------------------- */

        /* 전송할 데이터가 있는 경우   */
        if (session->wlen > 0){
            FD_CLR(session->fd, &g_wnewset);
            free(session->wdata);
            session->wdata  = NULL;
            session->wlen   = 0;

            SYS_HSTERR(SYS_NN,  ERR_SVC_TIMEOUT,    "POLLING RECV ERR - TIMEOUT ERR");
            sys_tprelay("EXTRELAY", &session->cb, 0, &session->tmax_ctx);

            sysocbfb(&session->cb);
            memset(&session->cb, 0x00, sizeof(commbuff_t));
        }

        /* polling에 대한 타임아웃이 발생하면 session close 처리 */
        session_linked_delete(session);
        cli_session_close(session);
    }

    return ERR_NONE;


}
/* ------------------------------------------------------------------------------------------------------------ */
int get_receive_length(char *dp)
{

    int                 len;
    char                buff[LEN_SIZE + 1];

#ifdef  _DEBUG
    /* ------------------------------------------------------ */    
    utohexdp((char *)dp, g_head_len);
    /* ------------------------------------------------------ */    
#endif

    memset(buff, 0x00, sizeof(buff));

    SYS_DBG("결제원용 g_tcp_header_len[%d]", g_tcp_header_len);
    SYS_DBG("g_cli_session[%d].tr_flag[%d] g_tr_len[%d] LEN_SIZE[%d]", g_session_idx, g_cli_session(g_session_idx).tr_flag, g_tr_len, LEN_SIZE);
    /******************************************************************************************
    
     결제원 경우 TCP_TP : prod_name[4] == 'T'                                        
     기타기관은 TCP_2와 같이 셋팅 (2,3,4,5,67,8,9,0 사용 )                           
                                                                                     
     tr_flag 2 : size(4)+실전문 : size값은 실전문의 길이 (00101234567890)            
     tr_flag 3 : size(4)+실전문 : size값은 전체    길이 (0010123456)                 
                                                                                     
     tr_flag 4 : TR(9)+size+실전문 : size값은 실전문의 길이 (_________00101234567890)
     tr_flag 5 : TR(9)+size+실전문 : size값은 실전문의 길이 (_________0010123456)
     결제원에서 들어오는 tr_flag = 0 이기 때문에 13자리임 

     *******************************************************************************************/

     switch(g_cli_session[g_session_idx].tr_flag){
        case 0:
            memcpy(buff, &dp[g_tr_len], LEN_SIZE);
            len = atoi(buff);
            break;
        
        case 1:
            if (memcmp(&dp[LEN_SIZE], "HDR", 3 ) == 0){
                memcpy(buff, dp, LEN_SIZE);
                /* 앞에 tr_len이 붙지 않는 경우인데 9바이트를 추가해서 읽었기 때문에 빼준다. */
                len = atoi(buff) - 12;
            }
            else{
                memcpy(buff, &dp[g_tr_len], LEN_SIZE);
                len = atoi(buff) - 3;
            }
            break;
        case 2:
            memcpy(buff, dp, g_header_len);
            len = atoi(buff);
            break;
        
        case 3:
            memcpy(buff, dp, g_header_len);
            len = atoi(buff) - g_header_len;
            break;

        case 4:
            memcpy(buff, &dp[g_tr_len], LEN_SIZE);
            len = atoi(buff);
            break;

        case 5:
            memcpy(buff, &dp[g_tr_len], LEN_SIZE);
            len = atoi(buff) - g_header_len;
            break;

        default:
            SYS_DBG("ERROR FOR HEADER TYPE");
            return ERR_ERR;

     }

     /* ------------------------------------------------------------------------------------------------- */
     SYS_DBG("get recevie length: LEN_SIZE[%d] len[%d]", LEN_SIZE, len, buff);
     /* ------------------------------------------------------------------------------------------------- */

     return len;
}


/* ------------------------------------------------------------------------------------------------------------ */
static int get_mtietc_info(void)
{

    int                 cnt, size;
    mtietc_t            mtietc;
    mtietc_t            *mti_ptr1;
    mtietc_t            *mti_ptr2;


    size                = sizeof(mtietc_t) * MTIETC_ARRAY_NUM;
    mti_ptr1            = (mtietc_t *) malloc(size);
    if (mti_ptr1 == NULL)
        return ERR_ERR;

    mti_ptr2   = (mtietc_t *) mti_ptr1;

    SYS_DBG("g_van_type[%d]g_mti_time[%s]", g_van_type, g_mti_name);

    EXEC SQL DECLARE MTIETC_CUR CURSOR FOR 
        SELECT  TO_CHAR(VAN_ID)   AS VAN_ID
              , MTI_NAME 
              , SEQ_NO 
              , MATCH_TYPE 
              , MATCH_VAL 
              , RQ_NAME 
              , SVC_NAME 
              , POS1_IDX 
              , POS1_LEN
              , POS2_IDX 
              , POS2_LEN 
              , POS3_IDX 
              , POS3_LEN 
         FROM MTIETC 
        WHERE VAN_ID    = :g_van_type
          AND MTI_NAME  = TRIM(:g_mti_name)
    EXEC SQL OPEN MTIETC_CUR;

    if (SYS_DB_CHK_FAIL){
        db_sql_error(SYS_DB_ERRORNUM, SYS_DB_ERRORSTR);
        free(mti_ptr1);
        g_mti_size = 0;
        return ERR_ERR;
    }

    cnt = 0;
    while(1){
        memset(&mtietc, 0x00, sizeof(mtietc_t));
        EXEC SQL FETCH MTIETC_CUR INTO :mtietc; 

        if (SYS_DB_CHK_FAIL)
            break;

        utortrim(mtietc.van_id);
        utortrim(mtietc.mti_name);
        utortrim(mtietc.match_type);
        utortrim(mtietc.match_val);
        utortrim(mtietc.rq_name);
        utortrim(mtietc.svc_name);

        PRINT_MTIETC(&mtietc);

       memcpy(mti_ptr2, &mtietc, sizeof(mtietc_t));
       mti_ptr2++;
       cnt++;

    }

    EXEC SQL CLOSE MTIETC_CUR;

    /*---------------------------------------------------*/
    SYS_DBG("mtietc:count[%d]", cnt);
    /*---------------------------------------------------*/

    /* 회선정보가 하나도 없는 경우 */
    if (cnt == 0){
        free(mti_ptr1);
        g_mti_size = 0;
        return NULL;
    }

    g_mti_size = cnt;

    g_mtietc = (mtietc_t *)mti_ptr1;

    return ERR_NONE;
}
/* ------------------------------------------------------------------------------------------------------------ */
int apsvrdone()
{

    int                 i;  

    /*---------------------------------------------------*/
    SYS_DBG("apsvrdone ********** server down *********");
    /*---------------------------------------------------*/

    for (i = 0; i < g_session_size; i++)
    {
        if (g_cli_session[i].fd < 0)
            continue;

        /* 대외기관에 전송할 데이터가 남아 있는경우 */
        if (g_cli_session[i].wlen > 0){
            /* 요청프로그램에 에러 전송  */
            SYS_HSTERR(SYS_NN, ERR_SVC_SNDERR, "SERVICE DOWN PROC");
            sys_tprelay("EXTRELAY", &g_cli_session[i].cb , 0 , &g_cli_session[i].tmax_ctx);
        }

        /* 전송중 건수 감소 */
        if (g_cli_session[i].wflag == SYS_TRUE){
            g_cli_session[i].wflag =  SYS_FALSE;
            g_send_able--;
            if (g_send_able < 0)
                g_send_able = 0;
        }

        /* 회선이 close됨으로 linked list에 저장된 정보도 전송할 수 없음  */
        session_linked_delete(&g_cli_session[i]);
        cli_session_close(&g_cli_session[i])
    }

    return ERR_NONE;
}

/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */