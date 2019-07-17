/*******************************************************************************
 *
 * MODULE :Keypad Driver functions source file
 *
 * CREATED:2016/09/18 07:15:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:Keypad driver
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
/******************************************************************************/
/***        Include files                                                   ***/
/******************************************************************************/
#include <xc.h>
#include <string.h>
#include "keypad.h"
#include "libcom.h"

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/
// KeyPad Check Cycle
// 秒間128回のチェックを想定
#ifndef KEYPAD_CHK_CNT_0
#define KEYPAD_CHK_CNT_0 (6)
#endif
#ifndef KEYPAD_CHK_CNT_1
#define KEYPAD_CHK_CNT_1 (128)
#endif
#ifndef KEYPAD_CHK_CNT_2
#define KEYPAD_CHK_CNT_2 (256)
#endif
#ifndef KEYPAD_CHK_CNT_3
#define KEYPAD_CHK_CNT_3 (268)
#endif

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
// ボタンが押下されている列の読み込み
static uint8 readColumn();

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
// デバイス情報
static tsKEYPAD_status *spKEYPAD_status;

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: KEYPAD_vInit
 *
 * DESCRIPTION:キーパッドの初期化処理
 *
 * PARAMETERS:      Name            RW  Usage
 * tsKEYPAD_status  *spStatus       R   デバイスステータス情報
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern void KEYPAD_vInit(tsKEYPAD_status *spStatus) {
    if (spStatus == NULL) {
        return;
    }
    // キー入力ステータス初期化
    spStatus->u8BeforeKeyNo  = 0;
    spStatus->u16KeyChkCnt    = 0;
     // バッファステータス初期化
    spStatus->u8BuffSize     = 0;
    spStatus->u8BuffBeginIdx = 0;
    spStatus->u8BuffEndIdx   = KEYPAD_BUFF_SIZE - 1;
    // キーバッファの初期化
    memset(spStatus->u8KeyBuffer, 0x00, KEYPAD_BUFF_SIZE);
    // デバイスステータス情報
    spKEYPAD_status = spStatus;
}

/*******************************************************************************
 *
 * NAME: KEYPAD_bUpdateBuffer
 *
 * DESCRIPTION:バッファ更新
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    bool_t        TRUE:更新成功
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern bool KEYPAD_bUpdateBuffer() {
    // バッファリング判定
    if (spKEYPAD_status->u8BuffSize >= KEYPAD_BUFF_SIZE) {
        // バッファが一杯の場合
        return false;
    }
    // キー番号読み込み
    uint8 u8KeyNo = KEYPAD_u8Read();
    if (u8KeyNo == 0xFF) {
        return false;
    }
    // クリティカルセクション開始
    criticalSec_vBegin();
    // バッファリング
    uint8 u8Idx = (spKEYPAD_status->u8BuffEndIdx + 1) % KEYPAD_BUFF_SIZE;
    spKEYPAD_status->u8KeyBuffer[u8Idx] = u8KeyNo;
    spKEYPAD_status->u8BuffEndIdx = u8Idx;
    spKEYPAD_status->u8BuffSize++;
    // クリティカルセクション終了
    criticalSec_vEnd();
    return true;
}

/*******************************************************************************
 *
 * NAME: KEYPAD_u8BufferSize
 *
 * DESCRIPTION:バッファサイズ取得
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8         バッファサイズ
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 KEYPAD_u8BufferSize() {
    return spKEYPAD_status->u8BuffSize;
}

/*******************************************************************************
 *
 * NAME: KEYPAD_vClearBuffer
 *
 * DESCRIPTION:バッファクリア
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t true:バッファクリア成功
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern void KEYPAD_vClearBuffer() {
    // クリティカルセクション開始
    criticalSec_vBegin();
    // バッファクリア
    spKEYPAD_status->u8BuffSize     = 0;
    spKEYPAD_status->u8BuffBeginIdx = 0;
    spKEYPAD_status->u8BuffEndIdx   = KEYPAD_BUFF_SIZE - 1;
    // クリティカルセクション終了
    criticalSec_vEnd();
}

/*******************************************************************************
 *
 * NAME:  KEYPAD_u8Read
 *
 * DESCRIPTION:現在キー値の読み込み
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8*      pu8Key          W   読み込みキー
 *
 * RETURNS:
 *   uint8 現在のキー番号、未入力時には0xFFを返却
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 KEYPAD_u8Read() {
    // ポートチェック
    uint8 u8PortA = PORTA;
#ifdef PORTB
    uint8 u8PortB = PORTB;
#endif
#ifdef PORTC
    uint8 u8PortC = PORTC;
#endif
    // キー入力チェック
    uint16 u16PinMap;
    uint8 u8Col;
    uint8 u8Row;
    for (u8Row = 0; u8Row < KEYPAD_ROW_SIZE; u8Row++) {
        // １行ごとにボタンを走査
        u16PinMap = spKEYPAD_status->u16PinRows[u8Row];
        switch (u16PinMap & 0xFF00) {
            case ID_PORTA:
                PORTA = u8PortA | (uint8)u16PinMap;
                u8Col = readColumn();
                PORTA = u8PortA;
                break;
#ifdef PORTB
            case ID_PORTB:
                PORTB = u8PortB | (uint8)u16PinMap;
                u8Col = readColumn();
                PORTB = u8PortB;
                break;
#endif
#ifdef PORTC
            case ID_PORTC:
                PORTC = u8PortC | (uint8)u16PinMap;
                u8Col = readColumn();
                PORTC = u8PortC;
                break;
#endif
        }
        // キー入力判定
        if (u8Col != 0xFF) {
            break;
        }
    }
    // キー入力判定
    if (u8Col == 0xFF) {
        spKEYPAD_status->u8BeforeKeyNo = 0xFF;
        spKEYPAD_status->u16KeyChkCnt   = 0;
        return 0xFF;
    }
    // キー番号取得
    uint8 u8KeyNo = u8Row * KEYPAD_ROW_SIZE + u8Col;
    // 同一キー入力判定
    if (u8KeyNo != spKEYPAD_status->u8BeforeKeyNo) {
        spKEYPAD_status->u8BeforeKeyNo = u8KeyNo;
        spKEYPAD_status->u16KeyChkCnt   = 1;
        return 0xFF;
    }
    // 同一キーチェック回数カウントアップ
    spKEYPAD_status->u16KeyChkCnt++;
    // チャタリングと押しっぱなしの判定（入力タイミング制御）
    if (spKEYPAD_status->u16KeyChkCnt == KEYPAD_CHK_CNT_0 ||
        spKEYPAD_status->u16KeyChkCnt == KEYPAD_CHK_CNT_1 ||
        spKEYPAD_status->u16KeyChkCnt == KEYPAD_CHK_CNT_2) {
        // ボタン番号を返却
        return u8KeyNo;
    } else if (spKEYPAD_status->u16KeyChkCnt == KEYPAD_CHK_CNT_3) {
        spKEYPAD_status->u16KeyChkCnt = KEYPAD_CHK_CNT_2;
        // ボタン番号を返却
        return u8KeyNo;
    }
    // 
    return 0xFF;
}

/*******************************************************************************
 *
 * NAME: KEYPAD_u8ReadBuffer
 *
 * DESCRIPTION:バッファ先頭文字の読み込み
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8*      pu8Key          W   読み込みキー
 *
 * RETURNS:
 *   uint8 バッファの先頭キー番号、バッファされていない場合には0xFFを返却
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 KEYPAD_u8ReadBuffer() {
    // バッファ文字の有無判定
    if (spKEYPAD_status->u8BuffSize == 0) {
        return 0xFF;
    }
    // クリティカルセクション開始
    criticalSec_vBegin();
    // バッファ読み込み
    uint8 u8Idx = spKEYPAD_status->u8BuffBeginIdx;
    uint8 u8KeyNo = spKEYPAD_status->u8KeyBuffer[u8Idx];
    spKEYPAD_status->u8BuffBeginIdx = (u8Idx + 1) % KEYPAD_BUFF_SIZE;
    spKEYPAD_status->u8BuffSize--;
    // クリティカルセクション終了
    criticalSec_vEnd();
    return u8KeyNo;
}

/*******************************************************************************
 *
 * NAME: KEYPAD_u8ReadFinal
 *
 * DESCRIPTION:バッファ終端文字の読み込み
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t         TRUE:読み込み成功
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 KEYPAD_u8ReadFinal() {
    // バッファ文字の有無判定
    if (spKEYPAD_status->u8BuffSize == 0) {
        return 0xFF;
    }
    // クリティカルセクション開始
    criticalSec_vBegin();
    // バッファ読み込み
    uint8 u8KeyNo = spKEYPAD_status->u8KeyBuffer[spKEYPAD_status->u8BuffEndIdx];
    // バッファクリア
    spKEYPAD_status->u8BuffBeginIdx = 0;
    spKEYPAD_status->u8BuffEndIdx   = KEYPAD_BUFF_SIZE - 1;
    spKEYPAD_status->u8BuffSize     = 0;
    // クリティカルセクション終了
    criticalSec_vEnd();
    return u8KeyNo;
}

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: readColumn
 *
 * DESCRIPTION:ボタン押下されている列の読み込み
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   uint8 入力された列番号、未入力時には0xFFを返却
 *
 * NOTES:
 * None.
 ******************************************************************************/
static uint8 readColumn() {
    // ポートチェック
    uint8 portA = PORTA;
#ifdef PORTB
    uint8 portB = PORTB;
#endif
#ifdef PORTC
    uint8 portC = PORTC;
#endif
    // ボタンをチェック
    uint16 u16PinMap;
    uint8 u8Port;
    uint8 u8Col;
    for (u8Col = 0; u8Col < KEYPAD_COL_SIZE; u8Col++) {
        u16PinMap = spKEYPAD_status->u16PinCols[u8Col];
        switch (u16PinMap & 0xFF00) {
            case ID_PORTA:
                u8Port = portA;
                break;
#ifdef PORTB
            case ID_PORTB:
                u8Port = portB;
                break;
#endif
#ifdef PORTC
            case ID_PORTC:
                u8Port = portC;
                break;
#endif
            default:
                // 押下されているキーが見つからなかった
                return 0xFF;
        }
        // 入力判定
        if ((u8Port & (uint8)u16PinMap) != 0x00) {
            return u8Col;
        }
    }
    // 押下されているキーが見つからなかった
    return 0xFF;
}

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
