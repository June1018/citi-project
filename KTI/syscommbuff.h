#ifndef _SYSCOMMBUFF_H_
#define _SYSCOMMBUFF_H_

#include <exparm.h>

#define CD_MAGIC_NUM            120051010

/*
* INDEX  000 - 049 : 시스템업무 
* INDEX  050 - 059 : 업무공통  
* INDEX  060 - 069 : CD공동망 / 경찰망 
* INDEX  070 - 089 : 타행환망공동망 
* INDEX  090 - 109 : 전자금융공동망  
* INDEX  110 - 129 : ARS/PG공동망 
* INDEX  130 - 149 : 직불카드공동망 
* INDEX  150 - 169 : 일괄전송  
* INDEX  170 - 254 : 기타(그외업무및 업무별 필요시 사용)
*/

#define NUM_CB_ITEM             256         /* commbuff 항목 총 갯수   */

#define IDX_SYSICOMM              0         /* SYSICOMM              */
#define IDX_SYSGWINFO             1         /* G/W호출 정보            */
#define IDX_EXMSG1200             4         /* 호스트송수신 data        */
#define IDX_SYSIOUTQ              5         /* output                */
#define IDX_SYSIOUTH              6         /* output header data    */
#define IDX_EXTJRN                7         /* 저널1                  */
#define IDX_EXTJRN _1             9         /* 취소 저널               */
#define IDX_EXTJRN _2             8         /* 저널2                  */
#define IDX_SECINPUT              9         /* Secondary input       */

#define IDX_EXPARM                10        /* 거래파라미터             */
#define IDX_TCPHEAD               11        /* TCP/IP HEADER 정보     */
#define IDX_HOSTRECVDATA          4         /* 호스트 수신 데이터        */
#define IDX_HOSTSENDDATA          14        /* 호스트 송신 데이터        */
#define IDX_EXTSENDDATA           15        /* 대외기관 송신 데이터      */
#define IDX_EXTRECVDATA           16        /* 대외기관 수신 데이터      */

/* 각 팀에서는 다음 BASE 상수에 1씩 더하는 형태로 정의  */
/* cifcommbuff.h 참조 */
#define IDX_EXBASE                50         /*  ~ 59 업무공통         */
#define IDX_CDBASE                60         /*  ~ 69 CD공통          */
#define IDX_IXBASE                70         /*  ~ 89 타행환          */
#define IDX_ELBASE                90         /*  ~ 109 전자금융        */
#define IDX_ARBASE                110        /*  ~ 129 ARS/PG        */
#define IDX_EPBASE                130        /*  ~ 149 직불카드        */
#define IDX_FTBASE                150        /*  ~ 169 일괄전송        */
#define IDX_ETBASE                170        /*  ~ 기타               */

/* 업무별 COMMBUFF index정의 */
#define IDX_FTSKIPDATA            (IDX_FTBASE + 0)

/* combuff항목에 대한 포인터 매크로 정의    */
#define SYSICOMM                ((sysicomm_t *) (ctx->cb->item[IDX_SYSICOMM].buf_ptr))
#define IDX_SYSGWINFO           ((sysgwinfo_t *) (ctx->cb->item[IDX_SYSGWINFO].buf_ptr))
#define EXMSG1200               ((exmsg1200_t *) (ctx->cb->item[IDX_EXMSG1200].buf_ptr))
#define SYSIOUTQ                ((char *)(ctx->cb->item[IDX_SYSIOUTQ].buf_ptr))
#define SYSIOUTH                ((sysiouth_t *) (ctx->cb->item[IDX_SYSIOUTH].buf_ptr))
#define EXTJRN                  ((cojrn_t *) (ctx->cb->item[IDX_EXTJRN].buf_ptr))
#define EXTJRN_1                ((cojrn_t *) (ctx->cb->item[IDX_EXTJRN_1].buf_ptr))
#define EXTJRN_2                ((cojrn_t *) (ctx->cb->item[IDX_EXTJRN_2].buf_ptr))
#define SECINPUT                ((char *) (ctx->cb->item[IDX_SECINPUT].buf_ptr))

#define EXPARM                  ((exmqparm_t *) (ctx->cb->item[IDX_EXPARM].buf_ptr))
#define TCPHEAD                 ((char *) (ctx->cb->item[IDX_TCPHEAD].buf_ptr))
#define HOSTRECVDATA            ((char *) (ctx->cb->item[IDX_HOSTRECVDATA].buf_ptr))
#define HOSTSENDDATA            ((char *) (ctx->cb->item[IDX_HOSTSENDDATA].buf_ptr))
#define EXTSENDDATA             ((char *) (ctx->cb->item[IDX_EXTSENDDATA].buf_ptr))
#define EXTRECVDATA             ((char *) (ctx->cb->item[IDX_EXTRECVDATA].buf_ptr))
#define FTSKIPDATA              ((char *) (ctx->cb->item[IDX_FTSKIPDATA].buf_ptr))

#define SYSICOMM_SIZE           (ctx->cb->item[IDX_SYSICOMM].buf_size)