/*  @file               dfo0001x.pc
*   @file_type          pc source program
*   @brief              exmsg11000 전문 초기화 
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
*   @generated at 2024/05/29 09:30
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
#include <dfi0001x.h>
#include <sqlca.h>

#include <exmsg11000.h>
/* ---------------------------------------- constant, macro definitions --------------------------------------- */
#define IN                      (dfi0001x->in) 

/* ---------------------------------------- structure definitions --------------------------------------------- */
typedef struct dfo0001x_ctx_s  dfo0001x_ctx_t;
struct dfo0001x_ctx_s {
    long dummy;
};

/* ------------------------------------- exported global variables definitions -------------------------------- */
/* ------------------------------------------ exported function  declarations --------------------------------- */
static int a000_data_init(dfo0001x_ctx_t  *ctx, dfi0001x_t  *dfi0001x);
static int b000_exmsg11000_init(dfo0001x_ctx_t  *ctx, dfi0001x_t  *dfi0001x);

/* ----------------------------------------------------------------------------------------------------------- */
int df_proc_exmsg11000_init(dfi0001x_t  *dfi0001x)
{
    int                 rc = ERR_NONE;
    dfo0001x_ctx_t      _ctx;  
    dfo0001x_ctx_t      *ctx = &_ctx; 

    SYS_TRSF;

    /* 데이터 초기화        */
    SYS_TRY(a000_data_init(ctx, dfi0001x));
    /* EXMSG11000 초기화  */
    SYS_TRY(b000_exmsg11000_init(ctx, dfi0001x));

    SYS_TREF;

SYS_CATCH:    

    SYS_TRSF;

    return ERR_ERR;
}

/* ----------------------------------------------------------------------------------------------------------- */
static int a000_data_init(dfo0001x_ctx_t  *ctx, dfi0001x_t  *dfi0001x)
{

    int                 rc = ERR_NONE;

    SYS_TRSF;
    /*-----------------------------------------------------------*/
    /* 변수화 초기화                                                */
    /*-----------------------------------------------------------*/
    memset((char *)ctx, 0x00, sizeof(dfo0001x_ctx_t));

    /* NULL 검증    */
    SYS_ASSERT(IN.exmsg11000);

    SYS_TREF;

    return ERR_NONE;

}
/* ----------------------------------------------------------------------------------------------------------- */
static int b000_exmsg11000_init(dfo0001x_ctx_t  *ctx, dfi0001x_t  *dfi0001x)
{

    int                 rc = ERR_NONE;

    char                proc_date[LEN_EXMSG11000_TX_DATE + 1];
    char                proc_tiem[LEN_EXMSG11000_TX_TIME + 1];

    memset(proc_date,  0x00, sizeof(proc_date));
    memset(proc_tiem,  0x00, sizeof(proc_tiem));

    utodate1(proc_date);
    utotime1(proc_time);
    /*-----------------------------------------------------------*/
    /* 변수화 초기화                                                */
    /*-----------------------------------------------------------*/
    memcpy(IN.exmsg11000->tx_id,         "DFCY      ",  LEN_EXMSG11000_TX_ID );
    memcpy(IN.exmsg11000->appl_code,     "094"       ,  LEN_EXMSG11000_APPL_CODE);
    memcpy(IN.exmsg11000->sub_appl_code, "0"         ,  LEN_EXMSG11000_SUB_APPL_CODE);
    memcpy(IN.exmsg11000->header_filler, ' '         ,  LEN_EXMSG11000_HEADER_FILLER);
    memcpy(IN.exmsg11000->rspn_flag,     '1'         ,  LEN_EXMSG11000_RSPN_FLAG);
    memcpy(IN.exmsg11000->err_code,      ' '         ,  LEN_EXMSG11000_ERR_CODE);
    memcpy(IN.exmsg11000->err_msg,       ' '         ,  LEN_EXMSG11000_ERR_MSG);
    memcpy(IN.exmsg11000->tx_date,       proc_date   ,  LEN_EXMSG11000_TX_DATE);
    memcpy(IN.exmsg11000->tx_time,       proc_time   ,  LEN_EXMSG11000_TX_TIME);
    memcpy(IN.exmsg11000->abs_time,      '0'         ,  LEN_EXMSG11000_ABS_TIME);
    memcpy(IN.exmsg11000->date_filler,   '0'         ,  LEN_EXMSG11000_DATE_FILLER);
    memcpy(IN.exmsg11000->van_type,      '0'         ,  LEN_EXMSG11000_VAN_TYPE);
    memcpy(IN.exmsg11000->telr_no,       '0'         ,  LEN_EXMSG11000_TELR_NO);
    memcpy(IN.exmsg11000->term_id,       ' '         ,  LEN_EXMSG11000_TERM_ID);
    memcpy(IN.exmsg11000->type_24,       '0'         ,  LEN_EXMSG11000_TYPE_24);
    memcpy(IN.exmsg11000->canc_type,     '0'         ,  LEN_EXMSG11000_CANC_TYPE);
    memcpy(IN.exmsg11000->sub_text_no,   '0'         ,  LEN_EXMSG11000_SUB_TEXT_NO);
    memcpy(IN.exmsg11000->msg_no,        ' '         ,  LEN_EXMSG11000_MSG_NO);
    memcpy(IN.exmsg11000->detl_rec_cnt,  '0'         ,  LEN_EXMSG11000_DETL_REC_CNT);
    memcpy(IN.exmsg11000->info_filler,   ' '         ,  LEN_EXMSG11000_INFO_FILLER);
    memcpy(IN.exmsg11000->req_id,        ' '         ,  LEN_EXMSG11000_REQ_ID;
    memcpy(IN.exmsg11000->othr_filler,   ' '         ,  LEN_EXMSG11000_OTHR_FILLER);

    return ERR_NONE;

}
/* ---------------------------------------- PROGRAM   END ---------------------------------------------------- */