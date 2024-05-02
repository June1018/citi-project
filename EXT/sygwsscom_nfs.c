
/*  @file               sygwsscom_nfs.pc
*   @file_type          pc source program
*   @brief              Gateway session 관련 common module
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
*   @generated at 2024/04/29 09:30
*   @history
*
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*
*/

/* ---------------------------------------------- include files ----------------------------------------------- */
#include <sytcpwhead.h>
#include <network.h>
#include <bssess_stat.h>
#include <sqlca.h>
#include "INL_external.h"
#include <nfmsg.h>


/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define LOG_FATAL_CHK_CNT       1000


/* ---------------------------------------- structure definitions --------------------------------------------- */
/* ------------------------------------- exported global variables definitions -------------------------------- */
int                 g_rtry_time;            /* 회선 재접속 시간        */
int                 g_rtry_cnt;             /* 회선 재접속 횟수        */

int                 g_head_len;             /* 통신 헤더 길이          */
int                 g_tr_len;               /* TRANS CODE길이        */
int                 g_tcp_header_len;       /* TCP헤더 길이           */
char                g_prog_id[32];          /* 회선 정보 구분명         */

int                 g_host_size;            /* 회선 정보 갯수          */
int                 g_host_idx;             /* 회선 정보 idx          */
bssess_stat_t       *g_bssess_stat;         /* 회선 정보              */

int                 g_listen_size;          /* listend info         */
listen_t            *g_lstninfo;

int                 g_session_maxi;         /* client session info  */
int                 g_session_idx;          
int                 g_session_size;
tcp_session_t       *g_cli_session;

int                 g_connection_maxi;      /* client connect info  */
int                 g_connection_size;
connect_t           *g_cli_connect;

linked_t            *g_linked_head = NULL;
linked_t            *g_linked_tail = NULL;

int                 g_wmaxfd;               /* socket maxium number */
fd_set              g_wset;                 /* select : send        */
fd_set              g_wnewset;              /* select : send        */

net_ctx*            is_ctx = NULL;          /* INISAFE 암호화 구조체   */

extern              char  g_svc_name[32];   /* G/W서비스 명           */
extern              int   g_send_able;      /* 전송데이터 유무          */
extern              int   g_mode;           /* 1:온라인 2:배치         */

extern              int   gs_step;          //세션키 교환상태 0:초기화 1:HandShake_Init전송후 2:HandShake_Update전송후 9:HandShake_Final전송후 
int                 g_usleep_time;
int                 g_nf_pf_flag;           //국고 평문통신 여부 0:평문통신, 9:암호화 사용  

/* ------------------------------------------ exported function  declarations --------------------------------- */
extern int  get_receive_length(char  *dp);
static void session_io_error(tcp_session_t  *session, int io);

/* ------------------------------------------------------------------------------------------------------------ */
int cli_listen_close(listen_t   *lstn)
{

    int                 i; 


    /* ------------------------------------------- */
    SYS_DBG("cli_listen_close: fd = %d",  lstn->fd);
    /* ------------------------------------------- */

    if (lstn->fd < 0)
        return ERR_NONE;

    /* 회선정보가 존재하면 회선정보 변경   */
    i = lstn->cidx;
    if (i > 0){
        set_session_info(g_prog_id, g_bssess_stat[i].symb_name, '0');
    }

    /* socket close */
    sys_tpclrfd(lstn->fd);
    close(lstn->fd);

    /* session 정보 초기화  */
    init_listen_elem(lstn);

    return ERR_NONE;
}


/* ------------------------------------------------------------------------------------------------------------ */
int init_listen(int max)
{

    int                 i; 
    size_t           size; 

    size = sizeof(listen_t)* max;
    g_lstninfo= (listen_t) malloc(size);
    if (g_lstninfo == NULL ) {
        ex_sylog(LOG_FATAL, "[APPL_DM]메모리 할당 에러 "
                            "[해결방안] 시스템 담당자 CALL");
        ex_sylog(LOG_ERROR, "[APPL_DM]%s init_listen(): 메모리 할당 에러 : %d"
                            "시스템 담당자 CALL",
                            __FILE__, size);
        return ERR_ERR;
    }

    g_listen_size = max;
    for (i = 0; i < g_listen_size; i++){
        init_listen_elem(&g_lstninfo[i]);

        g_lstninfo[i].status = READY;
        g_lstninfo[i].cidx   = i;
        g_lstninfo[i].tval   = 0;
    }

    return ERR_NONE;
}


/* ------------------------------------------------------------------------------------------------------------ */
int init_listen_elem(listen_t   *elem)
{
    elem->fd        = -1;
    elem->status    = NOT_READY;
    elem->portno    = -1;
    elem->cidx      = -1;
    elem->try_cnt   =  0;
    time(&elem->tval);

}


/* ------------------------------------------------------------------------------------------------------------ */
int cli_session_close(tcp_session_t  *session)
{

    int                 i; 

    /* ---------------------------------------------------------------- */
    SYS_DBG("cli_session_close: fd          = %d",  session->fd);
    SYS_DBG("cli_session_close: cidx        = %d",  session->cidx);
    SYS_DBG("cli_session_close: direction   = %d",  session->direction);
    SYS_DBG("gs_step [%d]", gs_step);
    SYS_DBG("session->cb           [%p]",  session->cb);
    SYS_DBG("session->wlen         [%d]",  session->wlen);
    SYS_DBG("session->wdata        [%p]",  session->wdata);
    SYS_DBG("session->wdata        [%s]",  session->wdata);
    /* ---------------------------------------------------------------- */

    if (session->fd < 0)
        return  ERR_NONE;
    
    /* 회선정보가 존재하면서 client mode인경우 회선정보 변경     */
    i = session->cidx;
    if (i >= 0){
        if (session->direction == OUTBOUND_SESSION) {
            set_session_info(g_prog_id, g_bssess_stat[i].symb_name, '0');
        }
    }

    /* clear recevie buffer    */
    if (session->rdata != NULL){
        free(session->rdata);
        session->rdata = NULL;
        session->rlen  = 0;
    }



    //free core 발생 방지를 위해서 
    //gs_step 1,9 이고 wlen > 0 이지만 wdata[] or wlen과 다른 길이의 데이터가 있음.
    //서버가 down될때 호출되면, 서비스명이 null인경우 
    if (gs_step > 0 && session->wlen > 0){
        SYS_DBG("wlen strlen [%d]", strlen(session->wdata));
        if (strlen(session->wdata) < session->wlen){
            SYS_DBG("session->wdata NULL, wlen, ZERO set ");
            session->wdata = NULL;
            session->wlen  = 0;
        }
    }

    /* clear recevie buffer    */
    if (session->rdata != NULL){
        /* free(session->rdata); free에서 invaild pointer core에러 발생 */
        session->rdata = NULL;
        session->rlen  = 0;
    }


    /* clear total recevie buffer   */
    if (session->tdata != NULL)
        /* free(session->rdata); free에서 invaild pointer core에러 발생 */
        session->tdata = NULL;
        session->tlen  = 0;

    /* clear commbuff   */
    sysocbfb(&session->cb);

    /* socket close   */
    sys_tpclrfd(session->fd);
    FD_CLR(session->fd, &g_wnewset);
    close(session->fd);

    SYS_DBG("cli_session_close ===========> after close ");

    /* 전송 중 건수 감소 */
    if (session->wflag == SYS_TRUE){
        session->wflag = SYS_FALSE;
        g_send_able--;
        if (g_send_able < 0)
            g_send_able = 0;
    }

    /* session 정보 초기화 */
    init_listen_elem(session);

    SYS_DBG("cli_session_close ===========> after init_session_elem ");

    /* poll 전문을 보냈을 경우    */
    if (g_cli_session[i].poll_flag == 1)
        g_cli_session[i].poll_flag = 0;


    /* session 연결갯수 감소    */
    if (session->idx >= g_session_maxi)
        g_session_maxi = session->idx - 1;

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
int init_session(int max)
{
    int                 i; 
    size_t           size;


    size = sizeof(tcp_session_t)* max;
    g_cli_session= (tcp_session_t) malloc(size);
    if (g_cli_session == NULL ) {
        ex_sylog(LOG_FATAL, "[APPL_DM]메모리 할당 에러 "
                            "[해결방안] 시스템 담당자 CALL");
        ex_sylog(LOG_ERROR, "[APPL_DM]% init_session(): 메모리 할당 에러 : %d"
                            "시스템 담당자 CALL",
                            __FILE__, size);
        return ERR_ERR;
    }

    g_listen_size = max;
    for (i = 0; i < g_listen_size; i++){
        g_cli_session[i].cidx   = -1; 
        init_listen_elem(&g_cli_session[i]);
        g_cli_session[i].idx    = i;
        g_cli_session[i].tval   = 0;
        g_cli_session[i].poll_flag = -1;

    }

    return ERR_NONE;
}
/* ------------------------------------------------------------------------------------------------------------ */
void init_session_elem(tcp_session_t    *session)
{
    /* Accept로 받은 session 회선정보 clear   */
    if (session->direction == INBOUND_SESSION)
        session->cidx      = 1;

    /* 회선정보가 없는 경우 clear    */
    if (session->cidx < 0)
        session->status = NOT_READY;

    session->fd         = -1; 
    session->cd         = -1;
    session->wait_time  = -1;
    session->uid[0]     = 0x00;
    session->r_trycount = -1;
    session->aszie      = 0;
    session->reading_len = 0;
    session->writing_len = 0;
    session->rlen       = 0;
    session->wlen       = 0;
    session->tlen       = 0;
    session->direction  = -1;
    session->msg_type   = -1;
    session->save_cnt   = 0;
    session->wflag = SYS_FALSE;
    session->tr_flag    = 0;
    time(&session->tval);
    time(&session->wrt_tval);
    time(&session->poll_tval);
    session->rdata      = NULL;
    session->wdata      = NULL;
    session->tdata      = NULL;
    session->head       = NULL;
    session->tail       = NULL;

    memset(session->uid,    0x00, LEN_UNIQUE_KEY);
    memset(&session->cd,    0x00, sizeof(commbuff_t));


}



/* ------------------------------------------------------------------------------------------------------------ */
int add_client_session(inf fd, int cidx, int direction, int keep_alive, int linger )
{
    int                 rc = ERR_NONE;
    int                 i;  

    /* ------------------------------------------- */
    SYS_DBG("add_client_session: fd = [%d] cidx=[%d]",  fd, cidx);
    /* ------------------------------------------- */

    if (keep_alive){
        rc = network_keepalive(fd, 1);
        if (rc == ERR_ERR){
            ex_sylog(LOG_FATAL, "[APPL_DM] KEEPALIVE 설정 ERROR"
                                "[해결방안] 업무 담당자 CALL");
            ex_sylog(LOG_FATAL, "[APPL_DM] %s add_client_session():"
                                "KEEP ALIVE설정 ERROR :[해결방안]업무 담당자 CALL",
                                __FILE__);
            close(fd);
            return ERR_ERR;
        }
    }

    if (linger){
        rc = network_linger(fd, 1);
        if (rc == ERR_ERR){
            ex_sylog(LOG_FATAL, "[APPL_DM] KEEPALIVE 설정 ERROR"
                                "[해결방안] 업무 담당자 CALL");
            ex_sylog(LOG_FATAL, "[APPL_DM] %s add_client_session():"
                                "KEEP ALIVE설정 ERROR :[해결방안]업무 담당자 CALL",
                                __FILE__);
            close(fd);
            return ERR_ERR;
        }
    }

    for (i = 0; i < g_session_size; i++){

        if (g_cli_session[i].cidx == cidx){
            /* session close   */
            cli_session_close(&cli_session[i]);
            /* ------------------------------------------- */
            SYS_DBG("reconnect client session: idx=[%d]", i);
            /* ------------------------------------------- */
        }

        if (g_cli_session[i].cidx < 0){
            g_cli_session[i].fd          = fd;
            g_cli_session[i].cidx        = cidx;
            g_cli_session[i].status      = READY;
            g_cli_session[i].direction   = direction;
            g_cli_session[i].writing_len = 0;
            g_cli_session[i].wait_time   = -1;

            /* polling 처리 유무 추가    */
            time(&g_cli_session[i].poll_tval);
            if(g_bssess_stat[cidx].prod_name[5] == 'P')
                g_cli_session[i].poll_flag      = 0;
            else 
                g_cli_session[i].poll_flag      = -1;

            time(&g_cli_session[i].tval);
            if (g_cli_session[cidx].prod_name[4] == 'T')
                g_cli_session[i].tr_flag        = 1;

            network_nonblock(fd);
            sys_tpsetfd(fd);
            g_session_maxi = MAX(g_session_maxi, i);

            /* ------------------------------------------- */
            SYS_DBG("add client session: idx=[%d]", i);
            /* ------------------------------------------- */

            /* 새로운 connection 이 맺어졌을때는 gs_step를 초기화해서 session_init로 부터 처리되게끔 유도    */
            if (g_nf_pt_flag == 9){
                gs_step = 0;
            }else{
                gs_step = 9;
            }

            return i;

        }
    }

    /* 사용가능한 slot이 없는 경우   */
    ex_sylog(LOG_FATAL, "[APPL_DM] session slot not available"
                        "[해결방안] 업무 담당자 CALL");
    ex_sylog(LOG_FATAL, "[APPL_DM] %s add_client_session():"
                        " session slot not available : [%d]"
                        "[해결방안]시스템 담당자 CALL",
                        __FILE__, g_session_size);
    close(fd);

    return ERR_ERR;

}

/* ------------------------------------------------------------------------------------------------------------ */
int cli_connect_close(connect_t *cont)
{

    /* -------------------------------------------------------------------- */
    SYS_DBG("int cli_connect_close =fd [%d]", cont->fd);
    /* -------------------------------------------------------------------- */

    if (cont->fd < 0)
        return ERR_NONE;

    /* clear send buffer */
    if (cont->wdata != NULL ){
        free(cont->wdata);
        cont->wdata = NULL;
        cont->wlen;
    }

    /* clear commbuff */
    sysocbfb(&cont->cb);

    /* socket close   */
    FL_CLR(cont->fd, &g_wnewset);
    close(cont->fd);

    /* 전송중 건수 감소   */
    if (cont->wflag == SYS_TRUE){
        cont->wflag = SYS_FALSE;
        g_send_able--;
        if (g_send_able < 0)
            g_send_able = 0;
    }

    init_connect_elem(cont);

    /* session 연결갯수 감소  */
    if (cont->idx >= g_connection_maxi)
        g_connection_maxi = cont->idx -1;

    return ERR_NONE;

}


/* ------------------------------------------------------------------------------------------------------------ */
int init_connect(int max)
{


    int                 i; 
    size_t           size; 

    size = sizeof(connect_t)* max;
    g_cli_connect= (connect_t) malloc(size);
    if (g_cli_connect == NULL ) {
        ex_sylog(LOG_FATAL, "[APPL_DM]메모리 할당 에러 "
                            "[해결방안] 시스템 담당자 CALL");
        ex_sylog(LOG_ERROR, "[APPL_DM]%s init_connect(): 메모리 할당 에러 : %d"
                            "시스템 담당자 CALL",
                            __FILE__, size);
        return ERR_ERR;
    }

    g_connect_size = max;
    for (i = 0; i < g_connect_size; i++){
        init_connect_elem(&g_cli_connect[i]);
        g_cli_connect[i].idx   = i;
    }

    return ERR_NONE;

}

/* ------------------------------------------------------------------------------------------------------------ */
void    init_connect_elem(connect_t *cont)
{
    cont->status        = NOT_READY;
    cont->fd            = -1;
    cont->portno        = -1;
    cont->tval          = 0;
    cont->sidx          = -1;
    cont->wait_time     = 0;
    cont->wlen          = 0;
    cont->wdata         = NULL;
    cont->ipaddr[0]     = 0x00;
    cont->wflag         = SYS_FALSE;

    memset(&cont->cb,   0x00, sizeof(commbuff_t));
}

/* ------------------------------------------------------------------------------------------------------------ */
int   add_connect_session(int fd, int portno, char  *ipaddr)
{

    int                 i; 

    /* -------------------------------------------------------------------- */
    SYS_DBG("add_connect_session =  fd [%d] port_no = [%d] ", fd, portno);
    /* -------------------------------------------------------------------- */

    for (i = 0; i < g_connect_size; i++){
        if (g_cli_connect[i].fd < 0){
            g_cli_connect[i].fd           = fd;
            g_cli_connect[i].portno       = portno;
            g_cli_connect[i].status       = READY;
            strcpy(g_cli_connect[i].ipaddr, ipaddr);
            time(&g_cli_connect[i].tval);

            FD_SET(g_cli_connect[i].fd, &g_wnewset);
            g_wmaxfd    = MAX(g_wmaxfd, fd);
            network_nonblock(fd);
            g_connect_maxi   = MAX(g_connect_maxi, i);

            g_cli_connect[i].wflag  = SYS_TRUE;
            g_send_able++;

            /* -------------------------------------------------------------------- */
            SYS_DBG("add_connect_session =  idx [%d] ", i);
            /* -------------------------------------------------------------------- */

            return i;
        }
    }

    /* 사용가능한 slot이 없는 경우   */
    ex_sylog(LOG_FATAL, "[APPL_DM] session slot not available"
                        "[해결방안] 업무 담당자 CALL");
    ex_sylog(LOG_FATAL, "[APPL_DM] %s add_connect_session():"
                        " connect slot not available : [%d]"
                        "[해결방안]시스템 담당자 CALL",
                        __FILE__, g_session_size);

    close(fd);

    return -1;

}


/* ------------------------------------------------------------------------------------------------------------ */
int session_listen(void)
{
    int                 i, j, fd, portno;
    int                 rtime;
    time_t              tval;

    time(&tval);

    for (i = 0; i < g_listen_size; i++){
        /* 이미 연결이 되어 있으면 다시 연결하지 않음.    */
        if ((g_lstninfo[i].fd       >= 0) ||
            (g_lstninfo[i].status   == UNREGISTERED))
            continue;

        /* 회선 정보 index */
        j = g_lstninfo[i].cidx;
        
    }
}
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */
