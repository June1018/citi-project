
/*  @file               nfo0001x.pc
*   @file_type          c source program
*   @brief              exmsg1500전문 초기화 
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
*
*
*
*   @generated at 2023/06/02 09:30
*   @history
*
*   성   명 :   일   자    근거자료         변경             내용   
*  ---------------------------------------------------------------------------------------------------------------
* 
*/



/*
 * main 
 * 1.   a000    DATA 초기화
 * 2.   b000    EXMSG1500전문 초기화 
 *
 *
 */


 /* ---------------------------------------------- include files ----------------------------------------------- */
#include <syscom.h>
#include <sysconst.h>
#include <exmsg1500.h>
#include <nfi0001f.h>
#include <sqlca.h>


/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define IN                  (nfi0001x->in)

/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct nfo0001_ctx_s    nfo0001_ctx_t;
struct nfo0001_ctx_s {
    long dummy;

};


/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int  a000_data_init(nfo0001_ctx_t *ctx, nfi0001x_t *nfi0001x);
static int  b000_exmsg1500_init(nfo0001_ctx_t *ctx, nfi0001x_t *nfi0001x);


/* ------------------------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------------------------ */
int nf_proc_exmsg1500_init(nfi0001x_t *nfi0001x);
{
    int                 rc = ERR_NONE;
    nfo0001_ctx_t       _ctx;  
    nfo0001_ctx_t       *ctx = &_ctx;

    SYS_TRSF;

    /* CONTEXT 초기화 */
    SYS_TRY(a000_data_init(nfi0001x_t *nfi0001x));

    /* 초기화 처리  */
    SYS_TRY(b000_exmsg1500_init(ctx, nfi0001x));

    SYS_TREF;
    return ERR_NONE;

SYS_CATCH:

    SYS_TREF;

    return ERR_ERR;
}
/* ------------------------------------------------------------------------------------------------------------ */
static int a000_data_init(nfo0001_ctx_t  *ctx, commbuff_t  commbuff)
{
    int                 rc = ERR_NONE;

    /* ---------------------------------------------------------- */
    /* 변수 초기화                                                  */
    /* ---------------------------------------------------------- */
    memset((char *)ctx, 0x00, sizeof(nfo0001_ctx_t));

    /* NULL 검증 */
    SYS_ASSERT(IN.exmsg1500);

    return ERR_NONE;

}


/* ------------------------------------------------------------------------------------------------------------ */
static int b000_exmsg1500_init(nfo0001_ctx_t *ctx)
{

    
    int                 rc = ERR_NONE;

    char                proc_date[LEN_EXMSG1500_TX_DATE + 1];
    char                proc_time[LEN_EXMSG1500_TX_TIME + 1];


    memset(proc_date,   0x00, sizeof(proc_date));
    memset(proc_time,   0x00, sizeof(proc_time));


    utodate1(proc_date);
    utotime1(proc_time);

    /* ---------------------------------------------------------- */
    /* 변수 초기화                                                  */
    /* ---------------------------------------------------------- */

    memcpy(IN.exmsg1500->tx_id,               "NFU2   ",      LEN_EXMSG1500_TX_ID );
    memset(IN.exmsg1500->tx_code,             '0'      ,      LEN_EXMSG1500_TX_CODE );
    memcpy(IN.exmsg1500->appl_code,           "026"    ,      LEN_EXMSG1500_APPL_CODE );
    memset(IN.exmsg1500->sub_appl_code,       '0'      ,      LEN_EXMSG1500_SUB_APPL_CODE );
    memset(IN.exmsg1500->header_filler,       ' '      ,      LEN_EXMSG1500_HEADER_FILLER );
    memset(IN.exmsg1500->rspn_flag,           '1'      ,      LEN_EXMSG1500_RSPN_FLAG );
    memset(IN.exmsg1500->err_code,            '0'      ,      LEN_EXMSG1500_ERR_CODE );
    memset(IN.exmsg1500->err_pgm_name         ' '      ,      LEN_EXMSG1500_ERR_PGM_NAME );
    memset(IN.exmsg1500->err_msg,             ' '      ,      LEN_EXMSG1500_ERR_MSG );
    memcpy(IN.exmsg1500->tx_date,             proc_date,      LEN_EXMSG1500_TX_DATE );
    memcpy(IN.exmsg1500->tx_time,             proc_time,      LEN_EXMSG1500_TX_TIME );
    memset(IN.exmsg1500->abs_time,            '0'      ,      LEN_EXMSG1500_ABS_TIME );
    memset(IN.exmsg1500->date_filler,         '0'      ,      LEN_EXMSG1500_DATE_FILLER);
    memset(IN.exmsg1500->van_type,            '0'      ,      LEN_EXMSG1500_VAN_TYPE );
    memset(IN.exmsg1500->telr_no,             '0'      ,      LEN_EXMSG1500_TELR_NO );
    memset(IN.exmsg1500->term_id,             ' '      ,      LEN_EXMSG1500_TERM_ID );
    memset(IN.exmsg1500->type_24,             '0'      ,      LEN_EXMSG1500_TYPE_24 );
    memset(IN.exmsg1500->canc_type,           '0'      ,      LEN_EXMSG1500_CANC_TYPE );
    memset(IN.exmsg1500->sub_text_no,         '0'      ,      LEN_EXMSG1500_SUB_TEXT_NO );
    memset(IN.exmsg1500->msg_no,              ' '      ,      LEN_EXMSG1500_MSG_NO );
    memset(IN.exmsg1500->detl_rec_cnt,        '0'      ,      LEN_EXMSG1500_DELT_REC_CNT );
    memset(IN.exmsg1500->detl_rec_size,       '0'      ,      LEN_EXMSG1500_DELT_REC_SIZE );
    memset(IN.exmsg1500->info_filler,         ' '      ,      LEN_EXMSG1500_INFO_FILLER );
    memset(IN.exmsg1500->req_id,              ' '      ,      LEN_EXMSG1500_REQ_ID );
    memset(IN.exmsg1500->othr_filler,         ' '      ,      LEN_EXMSG1500_OTHR_FILLER );
    memset(IN.exmsg1500->detl_area,           ' '      ,      LEN_EXMSG1500_DELT_AREA );

    return ERR_NONE;
}
/* ---------------------------------------- PROGRAM   END ----------------------------------------------------- */
