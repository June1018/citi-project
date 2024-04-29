
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

net_ctx*            is_ctx = NULL;

/* ------------------------------------------ exported function  declarations --------------------------------- */
static int  a000_data_receive(nfn0020_ctx_t *ctx, commbuff_t    *commbuff);
static int  b000_init_proc(nfn0020_ctx_t *ctx, int log_type);
static int  c000_sel_orig_msg_proc(nfn0020_ctx_t *ctx);
static int  d000_exparm_load(nfn0020_ctx_t *ctx);
static int  f000_host_msg_format_proc(nfn0020_ctx_t *ctx);
static int  e000_sel_jrn_proc(nfn0020_ctx_t *ctx);
static int  k000_host_msg_send(nfn0020_ctx_t *ctx);
static int  k100_kti_msg_send(nfn0020_ctx_t *ctx);
static int  m000_upd_jrn_proc(nfn0020_ctx_t *ctx);
static int  x000_nf_err_log(nfn0020_ctx_t *ctx); 
static int  z100_log_insert(nfn0020_ctx_t *ctx,  char *log_data, int size, char io_flag, char sr_flag);

/* ------------------------------------------------------------------------------------------------------------ */
/* host recevie 부터 host(sna)send 까지 업무처리                                                                    */
/* ------------------------------------------------------------------------------------------------------------ */