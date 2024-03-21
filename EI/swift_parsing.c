/* 

/* ----------------------------include files ------------------------------------------*/

#include <sqlca.h>
#include <syscom.h>
#include <syscommbuff.h>
#include <utodbio.h>
#include <ktitable.h>
#include <bpihead.h>
#include <table/pnctable.h>

/* ----------------------------constant macro define -----------------------------------*/

#define DATA_EXIST(DATA, LEN) ((utochknl(DATA, LEN)) == SYS_FALSE)
#define SEVEN_REQ_ACCT_NO   7
#define EIGHT_REQ_ACCT_NO   8
#define LEN_NCB_REFN        20
#define LEN_REMAIN_ACCT_NO  10
#define RCV_CUST_NO         5
#define REFERENCE           1
#define RCV_CUST_NO         5
#define REQ_CUST_NAME       6

#define TAG70_1             700 /* 70 tag add text - 1 */
#define TAG70_2             701 /* 70 tag add text - 2 */
#define TAG70_3             702 /* 70 tag add text - 3 */
#define TAG70_4             703 /* 70 tag add text - 4 */
#define TAG59_2             591 /* 59 tag add text - 1 */
#define TAG59_3             592 /* 59 tag add text - 2 */
#define TAG59_4             593 /* 59 tag add text - 3 */
#define MT100               1
#define MT202               2
#define MT103               3


#define LEN_MQ_TO_NCB                   sizeof(mq_to_ncb_t)
#define LEN_MQ_TO_NCB_PROC_DATE         8
#define LEN_MQ_TO_NCB_CURR_CODE         3
#define LEN_MQ_TO_NCB_AMOUNT            14
#define LEN_MQ_TO_NCB_REQ_CUST_NAME     20
#define LEN_MQ_TO_NCB_ADDRESS           40
#define LEN_MQ_TO_NCB_REQ_ACCT_NO       16
#define LEN_MQ_TO_NCB_OPP_BANK          3
#define LEN_MQ_TO_NCB_ENG_BANK_NAME     40
#define LEN_MQ_TO_NCB_RCV_ACCT_NO       16
#define LEN_MQ_TO_NCB_RCV_CUST_NAME     20
#define LEN_MQ_TO_NCB_REFER_NO          20
#define LEN_MQ_TO_NCB_INPUT_TYPE        2
#define LEN_MQ_TO_NCB_PHONE_NO          15
#define LEN_MQ_TO_NCB_FAX_NO            255
#define LEN_MQ_TO_NCB_EMAIL             255
#define LEN_MQ_TO_NCB_CITI_REF_NO       32
#define LEN_MQ_TO_NCB_DETAIL_1          35
#define LEN_MQ_TO_NCB_DETAIL_2          35
#define LEN_MQ_TO_NCB_DETAIL_3          35
#define LEN_MQ_TO_NCB_DETAIL_4          35
#define LEN_MQ_TO_NCB_RCV_DETAIL_1      35
#define LEN_MQ_TO_NCB_RCV_DETAIL_2      35
#define LEN_MQ_TO_NCB_RCV_DETAIL_3      35
#define LEN_MQ_TO_NCB_RCV_DETAIL_4      35
#define LEN_MQ_TO_NCB_DEPT_TYPE         2


#define LEN_TMID                        20
#define LEN_YEIL                        8
#define LEN_SAMT                        14
#define LEN_JNAM                        20 /* Length of Branch Name */
#define LEN_KFBR                        4 /* Length of KFBR (공통망, 지점코드, KFTC 공통) */
#define LEN_CHCD                        2 /* Length of Clearing House Code (어음코드) */
#define LEN_R_ID                        1 /* Length of R_ID (레코드 상태) of ELBNKNAM(은행명 table) */
#define LEN_BKCD                        2 /* Length of BKCD (은행코드 ) of ELBNKNAM(은행명 table) */
#define LEN_UPIL                        8 /* Length of UPIL (최종갱신일 ) of ELBNKNAM(은행명 table) */
#define LEN_M_ID                        24 /* Length of M_ID (등록자 ) of ELBNKNAM(은행명 table) */
#define LEN_M_IL                        14 /* Length of R_ID (등록일시 ) of ELBNKNAM(은행명 table) */
#define LEN_G_ID                        24 /* Length of G_ID (검증자 ) of ELBNKNAM(은행명 table) */
#define LEN_G_IL                        14 /* Length of G_ID (검증일시 ) of ELBNKNAM(은행명 table) */
#define LEN_BNAM                        50 /* Length of BNAM (은행명 ) of ELBNKNAM(은행명 table) */


#define LEN_AMNT                        14 /* Length of Kinds of Amount */
#define LEN_NAME                        20 /* Length of CODE SHORT NAME */
#define LEN_FNAM                        50 /* Lenght of CODE FULL NAME */
#define LEN_NKBN                        1 /* Lenght of NETTYPE */
#define LEN_INPT                        2 /* Length of INPUT TYPE */
#define LEN_VMAX                        50 /* Length of MAX */
#define LEN_HNAM                        50
#define LEN_PTP1                        1
#define LEN_PTP2                        1
#define LEN_PTP3                        1
#define LEN_PTYP                        1 /* 1.EFT 2.EFIN 3.CMS 4.KBFT 5.GIRO 6.SWP 7.GTA 8.CCM */

#define LEN_EFN                         10
#define LEN_BEFT                        10
#define LEN_KEFT                        10
#define LEN_KELB                        10
#define LEN_UTOXBKCK_RESULT             1
#define LEN_UTOXBNAM_RTCD               1
#define LEN_UTOXERCK_RESULT             3
#define LEN_UTOXNET_RTCD                1
#define LEN_UTOXCITI_RESULT             1

/* ----------------------------structure definitions -----------------------------------*/

typedef struct mq_to_ncb_s mq_to_ncb_t;
struct mq_to_ncb_s {

    char proc_date [LEN_MQ_TO_NCB_PROC_DATE + 1];           //처리일
    char curr_code [LEN_MQ_TO_NCB_CURR_CODE + 1];           //통화구분 
    char amount [LEN_MQ_TO_NCB_AMOUNT + 1];                 //금액
    char req_cust_name [LEN_MQ_TO_NCB_REQ_CUST_NAME + 1];   //의뢰인명(전각)
    char opp_bank [LEN_MQ_TO_NCB_OPP_BANK + 1];             //상대은행 
    char address [LEN_MQ_TO_NCB_ADDRESS + 1];               //주소 
    char req_acct_no [LEN_MQ_TO_NCB_REQ_ACCT_NO + 1];       //대체 계좌 
    char eng_bank_name [LEN_MQ_TO_NCB_ENG_BANK_NAME + 1];   //영문은행명 
    char rcv_acct_no [LEN_MQ_TO_NCB_RCV_ACCT_NO + 1];       //수취계좌은행
    char rcv_cust_name [LEN_MQ_TO_NCB_RCV_CUST_NAME + 1];   //수취인명 
    char refer_no [LEN_MQ_TO_NCB_REFER_NO + 1];             //Reference No
    char input_type [LEN_MQ_TO_NCB_INPUT_TYPE + 1];         //Input 구분 
    char phone_no [LEN_MQ_TO_NCB_PHONE_NO + 1];             //수신자 번호 
    char fax_no [LEN_MQ_TO_NCB_FAX_NO + 1];                 //Fax No 
    char email [LEN_MQ_TO_NCB_EMAIL + 1];                   //email
    char citi_ref_no [LEN_MQ_TO_NCB_CITI_REF_NO + 1];       //CITI-DIRECT REFERENCE NO
    char detail_1 [LEN_MQ_TO_NCB_DETAIL_1 + 1];             //Detail TEXT -1
    char detail_2 [LEN_MQ_TO_NCB_DETAIL_2 + 1];             //Detail TEXT -2
    char detail_3 [LEN_MQ_TO_NCB_DETAIL_3 + 1];             //Detail TEXT -3
    char detail_4 [LEN_MQ_TO_NCB_DETAIL_4 + 1];             //Detail TEXT -4
    char rev_detail_1 [LEN_MQ_TO_NCB_RCV_DETAIL_1 + 1];     //Recevier Detail - 1
    char rev_detail_2 [LEN_MQ_TO_NCB_RCV_DETAIL_2 + 1];     //Recevier Detail - 2
    char rev_detail_3 [LEN_MQ_TO_NCB_RCV_DETAIL_3 + 1];     //Recevier Detail - 3
    char rev_detail_4 [LEN_MQ_TO_NCB_RCV_DETAIL_4 + 1];     //Recevier Detail - 4
    char dept_type [LEN_MQ_TO_NCB_DEPT_TYPE + 1];           //업무부서구분 PCELNCMAS.sytp에 set 

};

typedef struct main_sample_ctx_s main_sample_ctx_t;
struct main_sample_ctx_s {
    commbuff_t *cb;
    commbuff_t _cb;
}

/* ---------------------- exported global variable declarations -----------------------*/

/* ------------------------ exported function declarations ---------------------------*/
static int a000_svc_init (main_sample_ctx_t *ctx, int argc, char **argv);
static int b000_test (main_sample_ctx_t *ctx);
int mt103_data_parsing(mq_to_ncb_t *rcv_mq, char *buff, int idx, int st);
void upset_str(char *destm, char *source, int len);

/* -------------------------------------------------------------------------------------*/
/* ----------------------------structure definitions -----------------------------------*/
int mt103_data_parsing(mq_to_ncb_t *rcv_mq, char *buff, int idx, int st)
{

    int rc = ERR_NONE;
    int len;
    int i = 0, j = 0;
    char temp[ 40+1];
    char rtcd[2];
    char *p_buff;
    char opp_bank[LEN_MQ_TO_NCB_OPP_BANK];

    SYS_TRSF;


    memset(rtcd, 0x00, sizeof(rtcd));

    //reference No check
    if (memcmp(buff, ":20:", 4) == 0){
        p_buff = buff + 4;
        len = strlen(utortrim(p_buff));
        if (len > LEN_NCB_REFN) {
            len = LEN_NCB_REFN;

        }

        memset(rcv->mq->refer_no, ' ', len);
        utoset_str(rcv->mq->refer_no, p_buff, len);

        SYS_TREF;
        return ERR_NONE;

        }/* end if (:20:) */

    /* 날짜 통화코드, 금액 Setting */
    if (memcmp(buff, ":32A:", 5) == 0) {
        p_buff = buff + 5;
        /* Y2K problem */
        if (memcmp(p_buff, "99", 2) < 0) {
            memcpy(rcv_mq->proc_date, "20", 2);
        }else{
            memcpy(rcv_mq->proc_date, "99", 2);
        }
        /* Data Set */
        memcpy(rcv_mq->proc_date + 2, p_buff , LEN_MQ_TO_NCB_PROC_DATE);
        /* Currency Code set */
        memcpy(rcv_mq->curr_code, p_buff + 6 , LEN_MQ_TO_NCB_CURR_CODE);
        /* Amount Set */
        memcpy(rcv_mq->amount , '0' , LEN_MQ_TO_NCB_AMOUNT);
        utoset_amt(rcv_mq->amount, p_buff + 9, LEN_MQ_TO_NCB_AMOUNT);
        SYS_TREF;
    }

    /* 의뢰인 성명 Setting(MT100 Only) */
    if((memcmp(buff, ":50:", 4) == 0) && (st == 1|| st == 3)) {
        if (st != 1 && st != 3){
            SYS_TREF;
            return ERR_NONE;
    }

    p_buff = buff + 4;
    len = strlen(utortrim(p_buff));
    if (len > LEN_MQ_TO_NCB_REQ_CUST_NAME){
        len = LEN_MQ_TO_NCB_REQ_CUST_NAME;
    }

    memset(rcv_mq->req_cust_name, ' ', len);
    /* 의뢰인 성명 setting(MT103 Only ) */
    if ((memcmp(buff, ":50K:", 5) == 0) &&
        (st == 1 || st == 3)) {
        if (st != 1 && st != 3){
            SYS_TREF;
            return ERR_NONE;
        }
    }

    p_buff = buff + 5;
    len = strlen(utortrim(p_buff));
    if (len > LEN_MQ_TO_NCB_REQ_CUST_NAME) {
        len = LEN_MQ_TO_NCB_REQ_CUST_NAME;
    }

    memset(rcv_mq->rcv_cust_name, ' ', len);
    /* 의뢰인 성명 설정 */
    utoset_str(rcv_mq->req_cust_name, p_buff, len);

    SYS_TREF;
    return ERR_NONE;

    }


    /* 의뢰인 성명 Setting(MT202 only) */
    if ((memcmp(buff, ":52:", 4) == 0) &&
        (st == 2)) {
        if (st !=2 ){
            SYS_TREF;
            return ERR_NONE;
        }

        p_buff = buff + 4;
        len = strlen(utortrim(p_buff));
        if( len > LEN_MQ_TO_NCB_REQ_CUST_NAME){
            len = LEN_MQ_TO_NCB_REQ_CUST_NAME;
        }

        memset(rcv_mq->req_cust_name, ' ', len);
        /* 의뢰인 성명 설정 */
        utoset_str(rcv_mq->req_cust_name, p_buff, len);

        SYS_TREF;
        return ERR_NONE;
    }

    /* 처리하지 않음 MT103 tag skip */
    if ((memcmp(buff, ":52A:", 5) == 0) && (st == 3))
    {
        SYS_DBG("52A tag 처리하지 않음 ");
        return ERR_NONE;
    }

    /* 의뢰인 계좌번호 Setting */
    if ((memcmp(buff, ":53A:", 5) == 0) ||
        (memcmp(buff, ":53B:", 5) == 0) ||
        (memcmp(buff, ":53D:", 5) == 0)) {
        if (buff[5] == '/'){
            /* '/' 문자 건너뜀 */
            p_buff = buff + 6;
        }else{
            p_buff = buff + 5;
        }

       len = strlen(p_buff);

        if (len > 40) {
            len = 40;
        }

        memset(temp, ' ', sizeof(temp));
        memset(rcv_mq->req_acct_no, '', LEN_MQ_TO_NCB_RCV_ACCT_NO);

        utoset_str(temp, p_buff, len);

        /* '-', '/', '' 문자 제거 */
        for(i = 0; i < len; i++){
            if ((temp[i] != '-') &&
                (temp[i] != '/') &&
                (temp[i] != ' ')){
                rcv_mq->req_acct_no[j++] = temp[i];
            } /* end if */

        } /* end for */

        SYS_TREF;
        return ERR_NONE;
        /* 대체계좌 7인 경우 */
        if ( j == 7){
            strcpy(temp, "0000");
            strncat(temp,rcv_mq->req_acct_no , SEVEN_REQ_ACCT_NO);
            memset(rcv->mq->rcv_acct_no , ' ' , LEN_MQ_TO_NCB_REQ_ACCT_NO);
            strncpy(rcv_mq->rcv_acct_no , temp, LEN_REMAIN_ACCT_NO);
        }


        SYS_TREF;
        return ERR_NONE;

    } /* end if */

    memset(opp_bank, 0x00, sizeof(opp_bank));
    /* 상태은행코드 check(MT100 only) */
    if(st == 1 || st == 3) {
        if((memcmp(buff, ":57A:", 5) == 0) ||
        (memcmp(buff, ":57B:", 5) == 0) ||
        (memcmp(buff, ":57D:", 5) == 0) ){
            if (st != 1 && st !=3) {
            SYS_TREF;
            return ERR_NONE;
        }
        p_buff = buff + 5;
        if (buff[5] == '/'){
            memcpy(opp_bank, p_buff + 1, LEN_MQ_TO_NCB_OPP_BANK);
        }else{
            memcpy(opp_bank, p_buff , LEN_MQ_TO_NCB_OPP_BANK);
        }
        SYS_INFO("strlen(opp_bank) [%d]", strlen(opp_bank));
        utohexdp(opp_bank, sizeof(opp_bank));

        /* 설정된 은행코드는 2자리 인데 3자리 read할 경우 line feed 포함되니까 제거한다. */
        for(i = 0; i < sizeof(opp_bank); i++){
            if (opp_bank[i] == '\n' || opp_bank[i] == 0x0d, || opp_bank[i] == 0x0a ){
                opp_bank[i] = 0x20;
            }

        }

        utotrim(opp_bank);
        SYS_INFO("strlen(opp_bank)[%d] opp_bank[%s]", strlen(opp_bank), opp_bank);
        utohexdp(opp_bank, sizeof(opp_bank));

        if( strlen(opp_bank) == LEN_MQ_TO_NCB_OPP_BANK) {
            memcpy(rcv_mq->opp_bank, opp_bank, LEN_MQ_TO_NCB_OPP_BANK);
        }else if{
            utosprintf_ibm(rcv_mq->opp_bank, "0%-2.2s", opp_bank);
        }else if{
            utosprintf_ibm(rcv_mq->opp_bank, "00%-1.1s",opp_bank);
        }else{
            memcpy(rcv_mq->opp_bank, "999", 3);
            SYS_INFO("은행코드 오류 rcv->mq->opp_bank[%-*.*s]", LEN_MQ_TO_NCB_OPP_BANK, rcv_mq->opp_bank);
        }


        SYS_INFO("rcv_mq->opp_bank[%-*.*s]", LEN_MQ_TO_NCB_OPP_BANK, rcv_mq->opp_bank);
        utohexdp(rcv_mq->opp_bank, LEN_MQ_TO_NCB_OPP_BANK);

        /* SWIFT 또는 GCN인 경우에는 은행명을 Set */
        if( utochknm(rcv_mq->opp_bank, LEN_MQ_TO_NCB_OPP_BANK) != SYS_TRUE) {
            len = strlen(p_buff);
        if (len > LEN_MQ_TO_NCB_OPP_BANK){
            len = LEN_MQ_TO_NCB_OPP_BANK;
        }/* end if */
        memset(rcv_mq->opp_bank, ' ' , LEN_MQ_TO_NCB_OPP_BANK) ;
        memset(rcv_mq->eng_bank_name, 0x20 , LEN_MQ_TO_NCB_ENG_BANK_NAME );

        if(buff[5] == '/'){
            utoset_str(rcv_mq->eng_bank_name, p_buff + 1, len);
        }else{
            utoset_str(rcv_mq->eng_bank_name, ' ' , len);
        }
        utohexdp(rcv_mq->opp_bank, , ' ' , LEN_MQ_TO_NCB_OPP_BANK);
        utohexdp(rcv_mq->eng_bank_name, 0x20, LEN_MQ_TO_NCB_ENG_BANK_NAME);

        * 영문 은행명으로 은행코드 얻어옴 */
        utoget_bkcd(utortrim(rcv_mq->eng_bank_name), rcv_mq->opp_bank, rtcd);

        if(rtcd[0] != '0') {
            memset(rcv_mq->opp_bank, ' ' , LEN_MQ_TO_NCB_OPP_BANK);
        }


        SYS_INFO("rtcd[%c] opp_bank[%s] rcv_mq->opp_bank[%-*.*s] rcv_mq->eng_bank_name[%-*.*s]", rtcd[0], opp_bank, LEN_MQ_TO_NCB_OPP_BANK
        , LEN_MQ_TO_NCB_OPP_BANK, rcv_mq->opp_bank, LEN_MQ_TO_NCB_ENG_BANK_NAME, LEN_MQ_TO_NCB_ENG_BANK_NAME, rcv_mq->eng_bank_name);

        SYS_TREF;
        return ERR_NONE;

    }
    
    memset(opp_bank, 0x00, sizeof(opp_bank));


    /* 상대은행코드 check(MT202 only) */
    if ((memcmp(buff, ":58A:", 5) == 0) ||
        (memcmp(buff, ":58B:", 5) == 0) ||
        (memcmp(buff, ":57A:", 5) == 0 && st == 2)){
        if (st != 2)
        {
            SYS_TREF;
            return ERR_NONE;
        }

        p_buff = buff + 5;
        if (buff[5] == '/')
        {
            memcpy(opp_bank, p_buff+1, LEN_MQ_TO_NCB_OPP_BANK);
        }else{
            memcpy(opp_bank, p_buff , LEN_MQ_TO_NCB_OPP_BANK);
        }
        SYS_INFO("strlen(opp_bank)[%d]", strlen(opp_bank));
        utohexdp(opp_bank, sizeof(opp_bank));

        for( i = 0; sizeof(opp_bank); i++){
            if(opp_bank[i] == '/' || opp_bank[i] == 0x0d || opp_bank[i] == 0x0a){
               opp_bank[i] = 0x20;

            }
        }

        utotrim(opp_bank);
        SYS_INFO("strlen(opp_bank)[%d], opp_bank[%s]", strlen(opp_bank), opp_bank);
        utohexdp(opp_bank, sizeof(opp_bank));

        /* 수신은행 코드가 3자리면 그래도 사용하고 2자리면 앞에 0을 붙인다. 
        2자리 앞에 0을 붙이고 1자리면 00을 붙인다. 
        은행코드가 없으면 초기화 한다. 
        */

        if (strlen(opp_bank) == LEN_MQ_TO_NCB_OPP_BANK){
            memcpy(rcv_mq->opp_bank, opp_bank, LEN_MQ_TO_NCB_OPP_BANK);
        }else if (strlen(opp_bank) == 2) {
            memcpy(rcv_mq->opp_bank, "0%-2.2s", opp_bank);
        }else if (strlen(opp_bank) == 1) {
            memcpy(rcv_mq->opp_bank, "0%-1.1s", opp_bank);
        }else{
            memcpy(rcv_mq->opp_bank, "999", 3);
        }

        SYS_INFO("rcv_mq->opp_bank)[%-*.*s]",LEN_MQ_TO_NCB_OPP_BANK, LEN_MQ_TO_NCB_OPP_BANK, rcv_mq->opp_bank);
        utohexdp(opp_bank, LEN_MQ_TO_NCB_OPP_BANK);

        /* SWIFT 또는 GCN인 경우에는 은행명을 set 한다. */
        if(utochknm(rcv_mq->opp_bank, LEN_MQ_TO_NCB_OPP_BANK) != SYS_TRUE) {
            len = strlen(p_buff);
            if (len > LEN_MQ_TO_NCB_OPP_BANK) {
                len = LEN_MQ_TO_NCB_OPP_BANK;
            } /* end if */

            /* 은행코드 변경에 따른 자리수 변경 */
            memset(rcv_mq->opp_bank, ' ' , LEN_MQ_TO_NCB_OPP_BANK);
            memset(rcv_mq->eng_bank_name, 0x20, LEN_MQ_TO_NCB_ENG_BANK_NAME);

            if (buff[5] == '/'){
                utoset_str(rcv_mq->eng_bank_name, p_buff + 1, len);
            }else{
                utoset_str(rcv_mq->eng_bank_name, p_buff , len);
            } /* end if */

            utohexdp(rcv_mq->opp_bank, LEN_MQ_TO_NCB_OPP_BANK);
            utohexdp(rcv_mq->eng_bank_name, LEN_MQ_TO_NCB_ENG_BANK_NAME);

            /* 영문명으로 은행코드 얻어옴 */
            utoget_bkcd(utortrim(rcv_mq->eng_bank_name), rcv_mq->opp_bank, rtcd);
            if (rtcd[0] != '0') {
                memset(rcv_mq->opp_bank, ' ', LEN_MQ_TO_NCB_OPP_BANK);
            }
        } 

        SYS_INFO("rtcd[%c] opp_bank[%s] rcv_mq->opp_bank)[%-*.*s] rcv_mq->eng_bank_name[%-*.*s]",rtcd[0], opp_bank
        , LEN_MQ_TO_NCB_OPP_BANK, LEN_MQ_TO_NCB_OPP_BANK, rcv_mq->opp_bank, LEN_MQ_TO_NCB_ENG_BANK_NAME, LEN_MQ_TO_NCB_ENG_BANK_NAME, rcv_mq->eng_bank_name);
        SYS_TREF;
        return ERR_NONE; 
    }    


    /* 수취인 계좌번호 check */
    if ((memcmp(buff, ":59:/", 5) == 0) || 
        (memcmp(buff, ":58D:/", 6) == 0)){
        if (memcmp(buff, ":59:/", 5) == 0){
            p_buff = buff + 5;
        }else{
            p_buff = buff + 6;
        }

        len = strlen(p_buff);
        if (len > 40) {
            len = 40;
        } /* end if */


        /* 수취인계좌번호 초기화 */
        memset(temp, 0x20, sizeof(temp));
        memset(rcv_mq->rcv_acct_no, 0x20, LEN_MQ_TO_NCB_RCV_ACCT_NO);
        utoset_str(temp, p_buff, len);

        j = 0;
        for ( i= 0; i < len; i++){
            if ((temp[i] != '-') &&
            (temp[i] != '/') &&
            (temp[i] != ' ')){
            rcv_mq->rcv_acct_no[j++] = temp[i];
            }

            if ( j >= LEN_MQ_TO_NCB_RCV_ACCT_NO ){
            SYS_DBG("copy buffer over [%s][%d][%d]", rcv_mq->rcv_acct_no, j, i);
            break;
            }

        }

        /* 수취은행 "27"이고 자리수가 7인 경우 */
        if ((memcmp(rcv_mq->opp_bank, "27", LEN_MQ_TO_NCB_OPP_BANK) == 0) && (j == 7)) {
            strcpy(temp, "000");
            strncat(temp, rcv_mq->rcv_acct_no, SEVEN_REQ_ACCT_NO);
            memset(rcv_mq->rcv_acct_no, 0x20, LEN_MQ_TO_NCB_RCV_ACCT_NO);
            strncpy(rcv_mq->rcv_acct_no, temp, LEN_REMAIN_ACCT_NO);
        } /* end if */

        SYS_TREF;
        return ERR_NONE;

    } /* end if */

    /* 의뢰인 정보 check */
    if (memcmp(buff, ":70:", 4) == 0) {
        p_buff = buff + 4;
        len = strlen(p_buff);
        if (len > LEN_MQ_TO_NCB_ADDRESS){
            len = LEN_MQ_TO_NCB_ADDRESS;
        } /* end if */
        memset(rcv_mq->address, 0x20, LEN_MQ_TO_NCB_ADDRESS );
        utoset_str(rcv_mq->address, p_buff, len);

        SYS_TREF;
        return ERR_NONE;

    }

    /* 수신 전화번호 */
    if (memcmp(buff, ":SI:", 4) == 0){
        p_buff = buff + 4;
        memset(temp, 0x20, sizeof(temp));
        memset(rcv_mq->phone_no, 0x20, LEN_MQ_TO_NCB_PHONE_NO);
        utoset_str(temp, p_buff, len);

        j = 0;
        for ( i= 0; i < len; i++){
            if (temp[i] >= '0') &&
            (temp[i] <= '9'){
            rcv_mq->phone_no[j++] = temp[i];
            }

            if ( j >= LEN_MQ_TO_NCB_PHONE_NO ){
                SYS_DBG("copy buffer over [%s][%d][%d]", rcv_mq->rcv_acct_no, j, i);
                break;
            }
        } /* end for */

        SYS_TREF;
       return ERR_NONE;
    } /* end if */

    /* 수취인 성명 check */
    if (idx == RCV_CUST_NO) {
        p_buff = buff;
        len = strlen(p_buff);

        if (len > LEN_MQ_TO_NCB_RCV_CUST_NAME){
            len = LEN_MQ_TO_NCB_RCV_CUST_NAME;
        } /* end if */
        memset(rcv_mq->rcv_cust_name, 0x20, LEN_MQ_TO_NCB_RCV_CUST_NAME );

        if (memcmp(buff, ":", 1) != 0){
            utoset_str(rcv_mq->rcv_cust_name, p_buff, len);
        }
        /* -------------------------------------------- */
        SYS_INFO("rcv_mq->rcv_cust_name[%s]", rcv_mq->rcv_cust_name);
        /* -------------------------------------------- */

        SYS_TREF;
        return ERR_NONE;
    }
    if (idx == REQ_CUST_NAME && st == 3 ){

        p_buff = buff;
        len = strlen(utortrim(p_buff));
        if ( len > LEN_MQ_TO_NCB_REQ_CUST_NAME) {
             len = LEN_MQ_TO_NCB_REQ_CUST_NAME;
        } /* end if */

        memset(rcv_mq->rcv_cust_name, ' ' , len);
        /* 의뢰인 성명 설정 */
        utoset_str(rcv_mq->rcv_cust_name, p_buff, len);
    
        SYS_TREF;
        return ERR_NONE;
    }

    SYS_TREF
    return ERR_NONE;

}
/* -------------------------------------------------------------------------------------*/
int main(int argc, char **arcv)
{

    int rc = ERR_NONE;
    int len;

    main_sample_ctx_t _ctx;
    main_sample_ctx_t *ctx = &_ctx;

    /* client 초기화 함수 */
    init_client(argc, argv);
    utosvcfini();

    SYS_TRSS;

    SYS_TRY(a000_svc_init(ctx, argc, argv));

    SYS_TRY(b000_test(ctx));

    SYS_TREF;

    /* client 종료시 함수 */
    fini_client();
    return ERR_NONE;

SYS_CATCH:

    SYS_DBG("EXCEPTION[%d]", rc);
    SYS_TRES;

    /* client 종료시 함수 */
    fini_client();
    return ERR_NONE;

}

/* -------------------------------------------------------------------------------------*/
static int a000_svc_init(main_sample_ctx_t *ctx, int argc, char **argv)
{

    int rc = ERR_NONE;
    SYS_TRSF;

    memset(ctx, 0x00, sizeof(main_sample_ctx_t));
    ctx->cb = &ctx->_cb;

    commhead_init(ctx->cb);

    SYS_TREF;
    return ERR_NONE;

}
/* -------------------------------------------------------------------------------------*/
static int b000_test(main_sample_ctx_t *ctx)
{

    int rc = ERR_NONE;
    char mt103_text[4000];
    char temp_buff[4000];
    mq_to_ncb_t mt103;
    int i = 0, j = 0;
    int chk = 0;
    char *inpt_stream;
    int NT;


    SYS_TRSF;


    memset(mt103_text, 0x00, sizeof(mt103_text);
    memset(temp_buff, 0x00, sizeof(temp_buff));
    strcpy(mt103_text, ":20:KRWSEP20101\n");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":32A:230516KRW12,");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":50:002282");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":LINE");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":LINE 2");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":LINE 3");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":53B:/5007410907");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":53B:004");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, "KOOK-MIN-BANK");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":59:/790210161226");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, "NAME");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, "ADD1");    
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, "ADD2");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, "ADD3");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, "ADD");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":71A:OUR");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":SI:03224220584");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":SG:03228373340");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":TP:test@citi.com");
    utosprintf_ibm(mt103_text, "%s%s\n", mt103_text, ":TS:e11a4241b95548008942d18cce963875");
    utosprintf_ibm(mt103_text, "%s%s"  , mt103_text, "-|");

    /* input_stream변수를 commbuff의 data영역설정 */
    inpt_stream = mt103_text;

    for (i = 0; i < strlen(mt103_text); i++){
        temp_buff[j++] = input_stream[i];

        /* new line check */
        if (input_stream[i] == '\n') {
            if (chk = REFERENCE_NO) {
                if (memcmp(temp_buff, ":21:", 4) == 0){
                    /* MT202전문 */
                    MT = MT202;
                }
                else if (memcmp(temp_buff, ":32A:", 5) == 0){
                    /* MT100전문 */
                    MT = MT100;
                }else if (memcmp(temp_buff, ":23B:", 5) == 0){
                    /* MT103(MT100 동일처리)전문 */
                    MT = MT103;
                }else{
                    /* --------------------------------------------------- */
                    SYS_HSTERR(SYS)LN, SYS_GENERR, "MQ FORMAT NOT FOUND ");
                    /* --------------------------------------------------- */
                    SYS_TREF;
                    return ERR_NONE;
                }
            } /* REFERENCE_NO end if */
            /* MQ 데이터 parsing : pcmqutil.pc 라이브러리 call */
            mt103_data_parsing(&mt103, temp_buff, chk, MT103);
            chk = 0;

            /* REFERENCE NO */
            if (memcmp(temp_buff, ":20:", 4) == 0 ) {
                chk = REFERENCE_NO;
            }
            /* 수취계좌 & 수치인명 */
            if (memcmp(temp_buff, ":59:/", 5) == 0) {
                chk = RCV_CUST_NO;
            }
            /* 의뢰인명, 전문에 대해서는 2번째 라인만 읽도록 함 */
            if (memcmp(temp_buff, ":50K:", 5) == 0) {
                chk = REQ_CUST_NAME;
            }
            memset(temp_buff, 0x00, sizeof(temp_buff));            
        } 
    } /* end FOR LOOP */
    SYS_TREF;
    return ERR_NONE;
 }
/* -------------------------------------------------------------------------------------------------- */
/*
function int utoget_ckcd(char *i_bank_name, char *o_bkcd, char, o_rtcd )
 brief      입력받은 은행명을 이용하여 은행코드를 구한다. 
 @param     i_bank_name     - 은행명 
            o_bkcd          - 은행코드 
            o_rtcd          - return code


*/
/* -------------------------------------------------------------------------------------------------- */
int utoget_bkcd(char *i_bank_name, char *o_bkcd, char, o_rtcd )
{
    int     rc = ERR_NONE;
    char    bank_code[LEN_BKCD + 1];
    char    bank_name[LEN_BNAM + 1];
    char    reg_type [1 + 1];
    char    rtcd[LEN_UTOXBNAM_RTCD];
    
    memset(bank_code, 0x00, sizeof(bank_code));
    memset(bank_name, 0x00, sizeof(bank_name));
    memset(reg_type , 0x00, sizeof(reg_type ));
    memset(rtcd     , 0x00, sizeof(rtcd));

    /* --------------- */
    SYS_INFO("i_bank_name [%s]",  i_bank_name);
    utohexdp(i_bank_name, 40);
    /* --------------- */
    /* 은행코드 체크시 Complete match case를 먼저 check */
    EXEC SQL 
        SELECT  BKCD 
             ,  BNAM 
             ,  REG_TYPE 
         INTO  :bank_code
             , :bank_name
             , :reg_type
         FROM  PCELBNKNAM
        WHERE  RTRIM(bnam) = :i_bank_name
        ;

    /* In case of Query Success */
    if (SYS_DB_CHK_SUCCESS) {
        memcpy(o_bkcd, bank_code, LEN_BKCD);
        /* 응답코드 설정 */
        memcpy(o_rtcd, "0", 1);
        /* --------------- */
        SYS_INFO("i_bank_name [%s]bank_code[%s] bank_name[%s]o_bkcd[%s]reg_type[%s]"
        ,  i_bank_name, bank_code, bank_name, o_bkcd, reg_type);
        /* --------------- */
        SYS_TREF;
        return ERR_NONE;
    }
    /* In case of NO DATA FOUND */
    else if(SYS_DB_CHK_NOTFOUND) 
    {
        /* --------------- */
        SYS_INFO("i_bank_name [%s] NOT FOUND",  i_bank_name);
        /* --------------- */
        memset(bank_code, 0x00, sizeof(bank_code));
        memset(bank_name, 0x00, sizeof(bank_name));
        memset(rtcd     , 0x00, sizeof(rtcd));
        /*******************************************/
        /* ELBNKNAM Table (은행명정보)에서            */
        /* 은행코드(BKCD) Select                     */
        /* 입력받은 Bank Name Select                 */
        /*******************************************/

        EXEC SQL DECLARE xbnamCur CURSOR FOR 
            SELECT BKCD , BNAM 
              FROM PCELBNKNAM
             WHERE :i_bank_name LIKE '%' ||RTRIM(bnam)||'%'
               AND REG_TYPE = '1'
             ORDER BY length(trim(bnam)) desc;
    
        EXEC SQL OPEN xbnamCur;

        /* In case of Cursor open Fail, Exception Process */
        if (SYS_DB_CHK_FETCHSUCCESS) 
        {
            SYS_INFO("PCEBNKAM Cursor Open Fail");
            
            SYS_TREF;
            return ERR_ERR;
        } 
        while(1){
            memset(bank_code, 0x00, sizeof(bank_code));
            memset(bank_name, 0x00, sizeof(bank_name));
            EXEC SQL FETCH xbnamCur
                INTO :bank_code, :bank_name;
        
            /* In Case of Fetch Success */
            if (SYS_DB_CHK_FETCHSUCCESS){
                memcpy(o_bkcd, bank_code, LEN_BKCD);
                memcpy(rtcd  , "0" , 1);
                EXEC SQL CLOSE xbnamCur;
                break;
            }
            /* In Case of Fetch End */
            else if (SYS_DB_CHK_FETCHFAIL){
                memcpy(o_bkcd, "  ", LEN_BKCD);
                memcpy(rtcd  , "9" , 1);
                EXEC SQL CLOSE xbnamCur;
                break;
            }else{
                EXEC SQL CLOSE xbnamCur;
                printf("Cursor Fetch Fail!\n");
                SYS_TREF;
                return ERR_ERR;
            }
        }
        memcpy(o_rtcd, rtcd, LEN_UTOXBNAM_RTCD);
        
        /* ----------------------------------------------------------------------------- */
        SYS_INFO("i_bank_name [%s]o_bkcd[%s]reg_type[%s]",  i_bank_name, o_bkcd, reg_type);
        /* ----------------------------------------------------------------------------- */

        SYS_TREF;
        return ERR_ERR;        
    }
    /* In case of Query Fail */
    else
    {
        /* bank code */
        memset(o_bkcd, ' ', 3);
        memcpy(o_bkcd, "9", 1);

        SYS_TREF;
        return ERR_ERR;
    }
    SYS_TREF;
    return rc;
}
void utoset_amt(char *dest, char *source, int len)
{
    int     i;
    int     char_cnt = 0;
    char    *ptr;

    ptr = source;
    while(( (*ptr++) > =0x30 && *(ptr) <= 0x39) &&
        char_cnt++ < len);
    ptr = source + char_cnt - 1;
    i = len;
    memset(dest, 0x30, len);
    while(i-- > 0 && (&ptr >= 0x30 && *ptr <= 0x39))
        *(dest+i) = *(ptr--);
}