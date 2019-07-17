/*******************************************************************************
 *
 * MODULE :Keypad Driver functions header file
 *
 * CREATED:2019/02/24 17:07:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:Microchip EEPROM driver
 *
 * CHANGE HISTORY:
 *
 * LAST MODIFIED BY:
 *
 *******************************************************************************
 * Copyright (c) 2019, Nakanohito
 * This software is released under the MIT License, see LICENSE.txt.
 * https://opensource.org/licenses/MIT
 ******************************************************************************/
#ifndef KEYPAD_H
#define KEYPAD_H

#if defined __cplusplus
extern "C" {
#endif

/******************************************************************************/
/***       Include files                                                    ***/
/******************************************************************************/
#include <xc.h>
#include "libcom.h"

/******************************************************************************/
/***       Macro Definitions                                                ***/
/******************************************************************************/
// KeyPad Setting
#ifndef KEYPAD_ROW_SIZE
#define KEYPAD_ROW_SIZE        (4)      // 行サイズ
#endif

#ifndef KEYPAD_COL_SIZE
#define KEYPAD_COL_SIZE        (4)      // 列サイズ
#endif

#ifndef KEYPAD_BUFF_SIZE
#define KEYPAD_BUFF_SIZE       (4)      // バッファサイズ
#endif

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
/**
 * KEYPAD用のステータス情報
 */
typedef struct {
    uint8 u8CheckCnt;                       // 1秒当たりのチェック回数
    uint16 u16PinCols[KEYPAD_COL_SIZE];     // 列ピンマップ配列
    uint16 u16PinRows[KEYPAD_ROW_SIZE];     // 行ピンマップ配列
    uint8 u8BeforeKeyNo;                    // 前回キー番号
    uint16 u16KeyChkCnt;                    // 同一キー連続検出回数
    uint8 u8KeyBuffer[KEYPAD_BUFF_SIZE];    // キーバッファ
    uint8 u8BuffSize;                       // キーバッファサイズ
    uint8 u8BuffBeginIdx;                   // キーバッファインデックス
    uint8 u8BuffEndIdx;                     // キーバッファインデックス
} tsKEYPAD_status;

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/
/** 初期化処理 */
extern void KEYPAD_vInit(tsKEYPAD_status *spStatus);
/** バッファ更新 */
extern bool KEYPAD_bUpdateBuffer();
/** バッファサイズ取得 */
extern uint8 KEYPAD_u8BufferSize();
/** バッファクリア */
extern void KEYPAD_vClearBuffer();
/** 現在キー値の読み込み */
extern uint8 KEYPAD_u8Read();
/** バッファからのキー読み込み */
extern uint8 KEYPAD_u8ReadBuffer();
/** 最終バッファキー読み込み */
extern uint8 KEYPAD_u8ReadFinal();

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* KEYPAD_H */

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
