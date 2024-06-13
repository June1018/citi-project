
/*  @file               mqo0001f.pc
*   @file_type          pc source program
*   @brief              ELJRN select
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
#include <syscom.h>
#include <excommon.h>
#include <exmqparam.h>
#include <mqi0001f.h>
#include <sqlca.h>
/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define IN                                  (mqi000f->in)

/* ------------------------------------------------------------------------------------------------------------ */
typedef struct mqo0001f_ctx_s   mqo0001f_ctx_t;
struct mqo0001f_ctx_s {
    long dummy;
}

/* -------------------------------------exported global variables declarations -------------------------------- */
/* ---------------------------------------- exported function declarations ------------------------------------ */
static int a000_data_init(mqo0001f_ctx_t *ctx, mqi0001f_t *mqi0001f);
static int b000_mqparm_select(mqo0001f_ctx_t *ctx, mqi0001f_t mqi0001f);

/* ------------------------------------------------------------------------------------------------------------ */
int mq_exmqparm_select(mqi0001f_t *mqi0001f)
{
    int                 rc = ERR_NONE;
    mqo0001f_ctx_t  _ctx; 
    mqo0001f_ctx_t  *ctx = &_ctx;

    SYS_TRSF;

    /* DATA 초기화 */
    SYS_TRY(a000_data_init(ctx, mqi0001f));

    /* 전자금융 저널  SELECT  */
    SYS_TRY(b000_mqparm_select(ctx, mqi0001f));

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:
    
    SYS_TREF;
    return ERR_ERR;

}

/* ------------------------------------------------------------------------------------------------------------ */
static int 
a000_data_init(mqo0001f_ctx_t   *ctx, mqi0001f_t *mqi0001f)
{
    int                 rc = ERR_NONE;

    /* -------------------------------------------------- */
    /* 변수 초기화                                          */
    /* -------------------------------------------------- */
    memset((char *)ctx, 0x00, sizeof(mqo0001f_ctx_t));

    return ERR_NONE;
}
/* ------------------------------------------------------------------------------------------------------------ */
static int 
b000_mqparm_select(mqo0001f_ctx_t *ctx, mqi0001f_t mqi0001f)
{
    int                 rc = ERR_NONE;
    exmqparm_t          *exmqparm;

    exmqparm  = (exmqparm_t *) IN.exmqparm;

    SYS_DBG("chnl_code = [%s]", exmqparm->chnl_code);
    SYS_DBG("appl_code = [%s]", exmqparm->appl_code);


    /* -------------------------------------------------- */
    /* ELJRN table select start                           */
    /* -------------------------------------------------- */
    EXEC SQL SELECT  
                    CHNL_CODE 
                   ,APPL_CODE 
                   ,STA_TYPE 
                   ,REG_DATE 
                   ,CHNL_NAME 
                   ,COMM_TYPE
                   ,MQ_MNGR
                   ,CHARSET_ID
                   ,SCHEDULE_MS
                   ,MQ_PROC_FLAG
                   ,MSG_LEN
                   ,GETQ_NAME
                   ,PUTQ_NAME
                   ,MQ_START_TIME
                   ,MQ_END_TIME
                   ,FIL1
                   ,FIL2
                   ,FIL3
                   ,MQ_GET_OPTS
                   ,MQ_PUT_OPTS
              INTO :exmqparm->chnl_code
                  ,:exmqparm->appl_code
                  ,:exmqparm->sta_type
                  ,:exmqparm->chnl_name
                  ,:exmqparm->comm_type 
                  ,:exmqparm->mq_mngr 
                  ,:exmqparm->charset_id 
                  ,:exmqparm->schedule_ms 
                  ,:exmqparm->mq_proc_flag 
                  ,:exmqparm->msg_len 
                  ,:exmqparm->getq_name 
                  ,:exmqparm->putq_name 
                  ,:exmqparm->mq_start_time
                  ,:exmqparm->mq_end_tiem 
                  ,:exmqparm->fil1 
                  ,:exmqparm->fil2 
                  ,:exmqparm->fil3 
                  ,:exmqparm->mq_get_opts 
                  ,:exmqparm->mq_put_opts 
              FROM  EXMQPARM 
             WHERE  CHNL_CODE = :exmqparm->chnl_code
               AND  APPL_CODE = :exmqparm->appl_code;

    if (SYS_DB_CHK_NOTFOUND) {
        ex_syslog(LOG_ERROR, "[APPL_DM] %s b000_mqparm_select(): NOT FOUND chnl_code[%s]appl_code[%s]",__FILE__, exmqparm->chnl_code, exmqparm->appl_code);
        return ERR_ERR;
    }

    /* --------------------------------------------------------- */
    PRINT_EXMQPARM(exmqparm);
    /* --------------------------------------------------------- */

    return ERR_NONE;
}

/* ------------------------------------------------------------------------------------------------------------ */
