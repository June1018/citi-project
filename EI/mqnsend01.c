
/*  @file               mqnsend01.pc
*   @file_type          pc source program
*   @brief              CHANNEL ==> MQ ==> EI로 들어오는 PROCESS
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
/* ---------------------------------------------- include files ----------------------------------------------- */
#include <sytcpgwhead.h>
#include <syscom.h>
#include <fxarg.h>
#include <mqi0001f.h>
#include <cmqc.h>
#include <exmq.h>
#include <exmqparam.h>
#include <exmqmsg_001.h>
#include <exmqsvc.h>
#include <mqimsg001.h>
#include <usrinc/atmi.h>
#include <usrinc/ucs.h>
#include <sqlca.h>
/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define EXCEPTION_SLEEP_INTV                3000                        /* EXCEPTION SLEEP Interval 5 min       */

#define EXMQPARM                            (&ctx->exmqparm)
#define MQMSG001                            (&ctx->exmqmsg_001)

/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct mqnsend01_ctx_s  mqnsend01_ctx_t;
struct mqnsend01_ctx_s {
    exmqparm_t      exmqparm;
    long            is_mq_connected;
    MQHCONN         mqhconn;
    MQHOBJ          mqhobj;
    exmqmsg_001_t   exmqmsg_001;

};

/* ------------------------------------- exported global variables definitions -------------------------------- */
char                g_svc_name[32];                     /* G/W 서비스명         */
char                g_chnl_code[3 + 1];                 /* CHANNEL CODE       */
char                g_appl_code[2 + 1];                 /* APPL_CODE          */
int                 g_sleep_sec;  


/* ------------------------------------------ exported function  declarations --------------------------------- */
int                 mqnsend01(commbuff_t    *commbuff);
static int          a000_initial(int argc,  char *argv[]);
static int          a100_parse_custom_args(int argc,  char *argv[]);
static int          b100_mqparm_load(mqnsend01_ctx_t    *ctx);
static int          d100_init_mqcon(mqnsend01_ctx_t     *ctx);
static int          e100_get_sendmsg(mqnsend01_ctx_t    *ctx);
static int          f100_put_mqmsg(mqnsend01_ctx_t      *ctx);
static int          g100_update_proc_code(mqnsend01_ctx_t   *ctx);
static int          z200_mqmsg_update(mqnsend01_ctx_t   *ctx);

int                 apsvrdone();



/* ------------------------------------------------------------------------------------------------------------ */
static int          a000_init_proc(int argc,  char *argv[])
{

    int                 rc = ERR_NONE;
    int                 i;

    SYS_TRSF;

    /* command argument 처리 */
    SYS_TRY(a100_parse_custom_args(argc, argv));

    strcpy(g_arch_head.svc_name,    g_svc_name);


    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;
    return ERR_ERR;
}

/* ------------------------------------------------------------------------------------------------------------ */
static int          a100_parse_custom_args((int argc,  char *argv[])
{
    int      c;

    g_sleep_sec = 0.1;

    while((c = getopt(argc, argv, "s:c:a")) != EOF )
}
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */