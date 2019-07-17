/*******************************************************************************
 *
 * MODULE :ST7032 Driver functions source file
 *
 * CREATED:2019/03/11 23:25:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:ST7032I LCD I2C draiver
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
#include "setting.h"            // 設定値
#include "libcom.h"             // 共通定義
#include "st7032.h"             // LCDライブラリ
#include "i2cUtil.h"            // I2C関数ライブラリ用

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/
// Address Read/Write
#define ST7032_I2C_ADDR             (0x3E)

// コントロールバイト（コマンド）
#define ST7032_CNTR_CMD             (0x00)
// コントロールバイト（データ）
#define ST7032_CNTR_DATA            (0x40)

// ST7032インストラクション
#define ST7032_CMD_CLEAR_DISP       (0b00000001)    // クリアディスプレイ
#define ST7032_CMD_ENTRY_MODE_SET   (0b00000100)    // 入力モード設定
#define ST7032_CMD_DISP_CNTR_DEF    (0b00001000)    // ディスプレイ設定
#define ST7032_CMD_OSC_FREQ         (0b00010100)    // 内部オシレータ設定
#define ST7032_CMD_FUNC_SET_DEF     (0b00111000)    // ファンクション設定　IS(instruction table select)＝0
#define ST7032_CMD_FUNC_SET_EX      (0b00111001)    // ファンクション設定　IS(instruction table select)＝1
#define ST7032_CMD_DISP_CNTR_EX     (0b01011000)    // 拡張ディスプレイ設定、アイコン表示ON、ブースター回路OFF、コントラスト設定（上2桁）
#define ST7032_CMD_FOLLOWER_CNTR    (0b01101100)    // フォロア回路ON、Rab0?Rab2(0b100)
#define ST7032_CMD_CONTRAST_LOW     (0b01110000)    // コントラスト設定（下4桁）
#define ST7032_CMD_SET_CGRAM        (0b01000000)    // CGRAMアドレス設定
#define ST7032_CMD_SET_ICON_ADDR    (0b01000000)    // アイコンアドレス設定
#define ST7032_CMD_SET_DD_ADDR      (0b10000000)    // カーソルアドレス設定

// 待ち時間
#ifndef ST7032_DEF_WAIT
#define ST7032_DEF_WAIT             (26)
#endif
#ifndef ST7032_EX_WAIT
#define ST7032_EX_WAIT              (1080)
#endif

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
/**
 * 構造体：LCDの状態情報
 */
typedef struct {
	// カーソル位置
	uint8 u8CursorPos;
    // ICON display(1bit). booster circuit(1bit). Contrast(6bit).
	uint8 u8Settings;
} ST7032_state;

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
// Execute Command
static void vExecCmdSSP1(uint8 u8Cmd);
#ifdef SSP2STAT
static void vExecCmdSSP2(uint8 u8Cmd);
#endif

// Execute Command and Stop End
static void vExecCmdEndSSP1(uint8 u8Cmd);
#ifdef SSP2STAT
static void vExecCmdEndSSP2(uint8 u8Cmd);
#endif

// Move cursor
static bool bSetCursorSSP1(uint8 u8Pos);
#ifdef SSP2STAT
static bool bSetCursorSSP2(uint8 u8Pos);
#endif

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
/** カーソル位置(SSP1) */
static ST7032_state stStateSSP1;
/** カーソル位置(SSP2) */
#ifdef SSP2STAT
static ST7032_state stStateSSP2;
#endif


/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/
/*******************************************************************************
 *
 * NAME: ST7032_vInitSSP1
 *
 * DESCRIPTION:Initialize LCD
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vInitSSP1() {
    // カーソル位置
    stStateSSP1.u8CursorPos = 0x00;
    // ICON,Booster,Contrast
    stStateSSP1.u8Settings = 0xE8;
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // ファンクション設定　IS(instruction table select)＝1
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // 内部オシレータ設定
    vExecCmdSSP1(ST7032_CMD_OSC_FREQ);
    __delay_us(ST7032_DEF_WAIT);
    // コントラスト設定（下4桁）
    vExecCmdSSP1(ST7032_CMD_CONTRAST_LOW | (stStateSSP1.u8Settings & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // 拡張ディスプレイ設定、アイコン表示ON、ブースター回路ON、コントラスト設定（上2桁）
    vExecCmdSSP1(ST7032_CMD_DISP_CNTR_EX | (stStateSSP1.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // フォロア回路ON、Rab0?Rab2(0b100)
    vExecCmdSSP1(ST7032_CMD_FOLLOWER_CNTR);
    __delay_us(ST7032_DEF_WAIT);
    // ファンクション設定　IS(instruction table select)＝0
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // ディスプレイ設定、表示ON、カーソル表示OFF、カーソル点滅OFF
    vExecCmdSSP1(ST7032_CMD_DISP_CNTR_DEF | 0x04);
    __delay_us(ST7032_DEF_WAIT);
    // カーソル設定
    vExecCmdSSP1(ST7032_CMD_SET_DD_ADDR);
    __delay_us(ST7032_DEF_WAIT);
    // クリアディスプレイ
    vExecCmdEndSSP1(ST7032_CMD_CLEAR_DISP);
    __delay_us(ST7032_EX_WAIT);
}

/*******************************************************************************
 *
 * NAME: ST7032_vInitSSP2
 *
 * DESCRIPTION:Initialize LCD
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vInitSSP2() {
    // カーソル位置
    stStateSSP2.u8CursorPos = 0x00;
    // ICON,Booster,Contrast
    stStateSSP2.u8Settings = 0xE8;
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // ファンクション設定　IS(instruction table select)＝1
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // 内部オシレータ設定
    vExecCmdSSP2(ST7032_CMD_OSC_FREQ);
    __delay_us(ST7032_DEF_WAIT);
    // コントラスト設定（下4桁）
    vExecCmdSSP2(ST7032_CMD_CONTRAST_LOW | (stStateSSP2.u8Settings & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // 拡張ディスプレイ設定、アイコン表示ON、ブースター回路ON、コントラスト設定（上2桁）
    vExecCmdSSP2(ST7032_CMD_DISP_CNTR_EX | (stStateSSP2.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // フォロア回路ON、Rab0?Rab2(0b100)
    vExecCmdSSP2(ST7032_CMD_FOLLOWER_CNTR);
    __delay_us(ST7032_DEF_WAIT);
    // ファンクション設定　IS(instruction table select)＝0
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // ディスプレイ設定、表示ON、カーソル表示OFF、カーソル点滅OFF
    vExecCmdSSP2(ST7032_CMD_DISP_CNTR_DEF | 0x04);
    __delay_us(ST7032_DEF_WAIT);
    vExecCmdSSP2(ST7032_CMD_ENTRY_MODE_SET | 0x02);
    __delay_us(ST7032_DEF_WAIT);
    // カーソル設定
    vExecCmdSSP2(ST7032_CMD_SET_DD_ADDR);
    __delay_us(ST7032_DEF_WAIT);
    // クリアディスプレイ
    vExecCmdEndSSP2(ST7032_CMD_CLEAR_DISP);
    __delay_us(ST7032_EX_WAIT);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vSetContrastSSP1
 *
 * DESCRIPTION:LCD Contrast Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        uint8     u8Contrast      R   Display Contrast
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vSetContrastSSP1(uint8 u8Contrast) {
    // 入力チェック
    uint8 u8Val = u8Contrast & 0x3F;
    if ((stStateSSP1.u8Settings & 0x3F) == u8Val) {
        return;
    }
    // 設定更新
    stStateSSP1.u8Settings = (stStateSSP1.u8Settings & 0xC0) | u8Val;
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // ファンクション設定　IS(instruction table select)＝1
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // コントラスト設定（下4桁）
    vExecCmdSSP1(ST7032_CMD_CONTRAST_LOW | (stStateSSP1.u8Settings & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // 拡張ディスプレイ設定、アイコン表示ON、ブースター回路OFF、コントラスト設定（上2桁）
    vExecCmdSSP1(ST7032_CMD_DISP_CNTR_EX | (stStateSSP1.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // ファンクション設定　IS(instruction table select)＝0
    vExecCmdEndSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
}

/*******************************************************************************
 *
 * NAME: ST7032_vSetContrastSSP2
 *
 * DESCRIPTION:LCD Contrast Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        uint8     u8Contrast      R   Display Contrast
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vSetContrastSSP2(uint8 u8Contrast) {
    // 入力チェック
    uint8 u8Val = u8Contrast & 0x3F;
    if ((stStateSSP2.u8Settings & 0x3F) == u8Val) {
        return;
    }
    // 設定更新
    stStateSSP2.u8Settings = (stStateSSP2.u8Settings & 0xC0) | u8Val;
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // ファンクション設定　IS(instruction table select)＝1
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // コントラスト設定（下4桁）
    vExecCmdSSP2(ST7032_CMD_CONTRAST_LOW | (stStateSSP2.u8Settings & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // 拡張ディスプレイ設定、アイコン表示ON、ブースター回路OFF、コントラスト設定（上2桁）
    vExecCmdSSP2(ST7032_CMD_DISP_CNTR_EX | (stStateSSP2.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // ファンクション設定　IS(instruction table select)＝0
    vExecCmdEndSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vClearDispSSP1
 *
 * DESCRIPTION:Clear Display
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vClearDispSSP1() {
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // LCDクリア
    vExecCmdEndSSP1(ST7032_CMD_CLEAR_DISP);
    // カーソル位置の初期化
    stStateSSP1.u8CursorPos = 0;
    __delay_us(ST7032_EX_WAIT);
}

/*******************************************************************************
 *
 * NAME: ST7032_vClearDispSSP2
 *
 * DESCRIPTION:Clear Display
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vClearDispSSP2() {
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // LCDクリア
    vExecCmdEndSSP2(ST7032_CMD_CLEAR_DISP);
    // カーソル位置の初期化
    stStateSSP2.u8CursorPos = 0;
    __delay_us(ST7032_EX_WAIT);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vClearIconSSP1
 *
 * DESCRIPTION:Clear Icon
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vClearIconSSP1() {
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
	// インストラクションテーブルチェンジ（IS=1）
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
	// コマンドの送信（アイコンアドレス）
    vExecCmdSSP1(ST7032_CMD_SET_ICON_ADDR);
    __delay_us(ST7032_DEF_WAIT);
    // インストラクションテーブルチェンジ（IS=0）
    vExecCmdEndSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // データの送信
    uint8 u8Addr;
    for (u8Addr = 0x00; u8Addr < 0x10; u8Addr++) {
        I2C_u8MstTxSSP1(0x00);
        __delay_us(ST7032_DEF_WAIT);
    }
    // ストップビット;
    I2C_vMstStopSSP1();
    // カーソルアドレスへ変換
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // カーソル位置を再設定
    bSetCursorSSP1(stStateSSP1.u8CursorPos);
}

/*******************************************************************************
 *
 * NAME: ST7032_vClearIconSSP2
 *
 * DESCRIPTION:Clear Icon
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vClearIconSSP2() {
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
	// インストラクションテーブルチェンジ（IS=1）
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
	// コマンドの送信（アイコンアドレス）
    vExecCmdSSP2(ST7032_CMD_SET_ICON_ADDR);
    __delay_us(ST7032_DEF_WAIT);
    // インストラクションテーブルチェンジ（IS=0）
    vExecCmdEndSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // データの送信
    uint8 u8Addr;
    for (u8Addr = 0x00; u8Addr < 0x10; u8Addr++) {
        I2C_u8MstTxSSP2(0x00);
        __delay_us(ST7032_DEF_WAIT);
    }
    // ストップビット;
    I2C_vMstStopSSP2();
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // カーソル位置を再設定
    bSetCursorSSP2(stStateSSP2.u8CursorPos);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vDispIconSSP1
 *
 * DESCRIPTION:Icon Display Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        bool      bDisp           R   表示設定
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vDispIconSSP1(bool bDisp) {
    // 更新判定
    if ((stStateSSP1.u8Settings >> 7) == bDisp) {
        return;
    }
    // アイコン表示設定の更新
    stStateSSP1.u8Settings = (stStateSSP1.u8Settings & 0x7F) | (bDisp << 7);
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // ファンクション設定　IS(instruction table select)＝1
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // 拡張ディスプレイ設定、アイコン表示ON、ブースター回路ON、コントラスト設定（上2桁）
    vExecCmdSSP1(ST7032_CMD_DISP_CNTR_EX | (stStateSSP1.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // ファンクション設定　IS(instruction table select)＝0
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);    
}

/*******************************************************************************
 *
 * NAME: ST7032_vDispIconSSP2
 *
 * DESCRIPTION:Icon Display Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        bool      bDisp           R   表示設定
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vDispIconSSP2(bool bDisp) {
    // 更新判定
    if ((stStateSSP2.u8Settings >> 7) == bDisp) {
        return;
    }
    // アイコン表示設定の更新
    stStateSSP2.u8Settings = (stStateSSP2.u8Settings & 0x7F) | (bDisp << 7);
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // ファンクション設定　IS(instruction table select)＝1
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // 拡張ディスプレイ設定、アイコン表示ON、ブースター回路ON、コントラスト設定（上2桁）
    vExecCmdSSP2(ST7032_CMD_DISP_CNTR_EX | (stStateSSP2.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // ファンクション設定　IS(instruction table select)＝0
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vDispSettingSSP1
 *
 * DESCRIPTION:LCD Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        bool      bDisp           R   Display LCD
 *        bool      bCursor         R   Cursor Display
 *        bool      bBlink          R   Blink Cursor
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vDispSettingSSP1(bool bDisp, bool bCursor, bool bBlink) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // ディスプレイ設定、表示ON、カーソル表示OFF、カーソル点滅OFF
    vExecCmdEndSSP1(ST7032_CMD_DISP_CNTR_DEF | (bDisp << 2) | (bCursor << 1) | bBlink);
    __delay_us(ST7032_DEF_WAIT);
}

/*******************************************************************************
 *
 * NAME: ST7032_vDispSettingSSP2
 *
 * DESCRIPTION:LCD Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        bool      bDisp           R   Display LCD
 *        bool      bCursor         R   Cursor Display
 *        bool      bBlink          R   Blink Cursor
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vDispSettingSSP2(bool bDisp, bool bCursor, bool bBlink) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // ディスプレイ設定、表示ON、カーソル表示OFF、カーソル点滅OFF
    vExecCmdEndSSP2(ST7032_CMD_DISP_CNTR_DEF | (bDisp << 2) | (bCursor << 1) | bBlink);
    __delay_us(ST7032_DEF_WAIT);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_u8GetCursorRowNoSSP1
 *
 * DESCRIPTION:get cursor row no
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8 カーソル行
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern uint8 ST7032_u8GetCursorRowNoSSP1() {
    return stStateSSP1.u8CursorPos / 40;
}

/*******************************************************************************
 *
 * NAME: ST7032_u8GetCursorRowNoSSP2
 *
 * DESCRIPTION:get cursor row no
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8 カーソル行
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern uint8 ST7032_u8GetCursorRowNoSSP2() {
    return stStateSSP2.u8CursorPos / 40;
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_u8GetCursorColNoSSP1
 *
 * DESCRIPTION:get cursor column no
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8 カーソル列アドレス
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern uint8 ST7032_u8GetCursorColNoSSP1() {
    return stStateSSP1.u8CursorPos % 40;
}

/*******************************************************************************
 *
 * NAME: ST7032_u8GetCursorColNoSSP2
 *
 * DESCRIPTION:get cursor column no
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8 カーソル列番号
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern uint8 ST7032_u8GetCursorColNoSSP2() {
    return stStateSSP2.u8CursorPos % 40;
}
#endif


/*******************************************************************************
 *
 * NAME: ST7032_bSetCursorSSP1
 *
 * DESCRIPTION:Move cursor
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8RowNo         R   Row No
 *       uint8      u8ColNo         R   Column No
 *
 * RETURNS:
 *     bool true:設定成功、false:設定失敗
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern bool ST7032_bSetCursorSSP1(uint8 u8RowNo, uint8 u8ColNo) {
    // 位置判定
    if (u8RowNo > ST7032_ROW_MAX || u8ColNo > ST7032_COL_MAX) {
        return false;
    }
    // カーソル移動判定
    uint8 u8Addr = (u8RowNo * 40) + u8ColNo;
    if (u8Addr == stStateSSP1.u8CursorPos) {
        return true;
    }
    // カーソル移動
    return bSetCursorSSP1(u8Addr);
}

/*******************************************************************************
 *
 * NAME: ST7032_bSetCursorSSP2
 *
 * DESCRIPTION:Move cursor
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8RowNo         R   Row No
 *       uint8      u8ColNo         R   Column No
 *
 * RETURNS:
 *     bool true:設定成功、false:設定失敗
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern bool ST7032_bSetCursorSSP2(uint8 u8RowNo, uint8 u8ColNo) {
    // 位置判定
    if (u8RowNo > ST7032_ROW_MAX || u8ColNo > ST7032_COL_MAX) {
        return false;
    }
    // カーソル移動判定
    uint8 u8Addr = (u8RowNo * 40) + u8ColNo;
    if (u8Addr == stStateSSP2.u8CursorPos) {
        return true;
    }
    // カーソル移動
    return bSetCursorSSP2(u8Addr);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_bCursorLeftSSP1
 *
 * DESCRIPTION:Move cursor to left
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     bool true:設定成功、false:設定失敗
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern bool ST7032_bCursorLeftSSP1() {
    // カーソル位置判定
    if (stStateSSP1.u8CursorPos == 0) {
        return false;
    }
    return bSetCursorSSP1(stStateSSP1.u8CursorPos - 1);
}

/*******************************************************************************
 *
 * NAME: ST7032_bCursorLeftSSP2
 *
 * DESCRIPTION:Move cursor to left
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     bool true:設定成功、false:設定失敗
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern bool ST7032_bCursorLeftSSP2() {
    // カーソル位置判定
    if (stStateSSP2.u8CursorPos == 0) {
        return false;
    }
    return bSetCursorSSP2(stStateSSP2.u8CursorPos - 1);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_bCursorRightSSP1
 *
 * DESCRIPTION:Move cursor to right
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     bool true:設定成功、false:設定失敗
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern bool ST7032_bCursorRightSSP1() {
    // カーソル位置判定
    if (stStateSSP1.u8CursorPos >= 79) {
        return false;
    }
    return bSetCursorSSP1(stStateSSP1.u8CursorPos + 1);
}

/*******************************************************************************
 *
 * NAME: ST7032_bCursorRightSSP2
 *
 * DESCRIPTION:Move cursor to right
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     bool true:設定成功、false:設定失敗
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern bool ST7032_bCursorRightSSP2() {
    // カーソル位置判定
    if (stStateSSP2.u8CursorPos >= 79) {
        return false;
    }
    return bSetCursorSSP2(stStateSSP2.u8CursorPos + 1);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vWriteCGRAMSSP1
 *
 * DESCRIPTION:Write CGRAM
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8CharNo        R   Character No
 *       uint8*     pu8BitMap       R   Character Bit Map
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vWriteCGRAMSSP1(uint8 u8CharNo, uint8* pu8BitMap) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // ファンクション設定　IS(instruction table select)＝0
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // CGRAMアドレス設定
    vExecCmdSSP1(ST7032_CMD_SET_CGRAM | ((u8CharNo << 3) & 0x38));
    __delay_us(ST7032_DEF_WAIT);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // CGRAMデータの送信
    uint8 *pu8WkMap = pu8BitMap;
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 8; u8Idx++) {
        I2C_u8MstTxSSP1(*pu8WkMap & 0x1F);
        __delay_us(ST7032_DEF_WAIT);
        pu8WkMap++;
    }
    // ストップコンディション
    I2C_vMstStopSSP1();
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // カーソル再設定
    bSetCursorSSP1(stStateSSP1.u8CursorPos);
}

/*******************************************************************************
 *
 * NAME: ST7032_vWriteCGRAMSSP2
 *
 * DESCRIPTION:Write CGRAM
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8CharNo        R   Character No
 *       uint8*     pu8BitMap       R   Character Bit Map
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vWriteCGRAMSSP2(uint8 u8CharNo, uint8* pu8BitMap) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // ファンクション設定　IS(instruction table select)＝0
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // CGRAMアドレス設定
    vExecCmdSSP2(ST7032_CMD_SET_CGRAM | ((u8CharNo << 3) & 0x38));
    __delay_us(ST7032_DEF_WAIT);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // CGRAMデータの送信
    uint8 *pu8WkMap = pu8BitMap;
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 8; u8Idx++) {
        I2C_u8MstTxSSP2(*pu8WkMap & 0x1F);
        __delay_us(ST7032_DEF_WAIT);
        pu8WkMap++;
    }
    // ストップコンディション
    I2C_vMstStopSSP2();
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // カーソル再設定
    bSetCursorSSP2(stStateSSP2.u8CursorPos);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vWriteCharSSP1
 *
 * DESCRIPTION:Write Character
 *
 * PARAMETERS:      Name            RW  Usage
 *        char      cData           R   インストラクション
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vWriteCharSSP1(char cData) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // データの送信
    I2C_u8MstTxSSP1(cData);
    // ストップコンディション
    I2C_vMstStopSSP1();
    // カーソル位置の移動
    stStateSSP1.u8CursorPos = (stStateSSP1.u8CursorPos + 1) % 80;
}

/*******************************************************************************
 *
 * NAME: ST7032_vWriteCharSSP2
 *
 * DESCRIPTION:Write Character
 *
 * PARAMETERS:      Name            RW  Usage
 *        char      cData           R   インストラクション
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vWriteCharSSP2(char cData) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // データの送信
    I2C_u8MstTxSSP2(cData);
    // ストップコンディション
    I2C_vMstStopSSP2();
    // カーソル位置の移動
    stStateSSP2.u8CursorPos = (stStateSSP2.u8CursorPos + 1) % 80;
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vWriteStringSSP1
 *
 * DESCRIPTION:Write String
 *
 * PARAMETERS:      Name            RW  Usage
 *       char*      pcStr           R   文字列のポインタ
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vWriteStringSSP1(char* pcStr) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // データの送信
    char* pcWkData = pcStr;
    while (*pcWkData != '\0') {
        I2C_u8MstTxSSP1(*pcWkData);
        __delay_us(ST7032_DEF_WAIT);
        pcWkData++;
    }
    // ストップコンディション
    I2C_vMstStopSSP1();
    // カーソル位置の移動
    stStateSSP1.u8CursorPos =
            (stStateSSP1.u8CursorPos + (pcWkData - pcStr)) % 80;
}

/*******************************************************************************
 *
 * NAME: ST7032_vWriteStringSSP2
 *
 * DESCRIPTION:Write String
 *
 * PARAMETERS:      Name            RW  Usage
 *        char      pcStr           R   文字列のポインタ
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vWriteStringSSP2(char* pcStr) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // データの送信
    char* pcWkData = pcStr;
    while (*pcWkData != '\0') {
        I2C_u8MstTxSSP2(*pcWkData);
        __delay_us(ST7032_DEF_WAIT);
        pcWkData++;
    }
    // ストップコンディション
    I2C_vMstStopSSP2();    
    // カーソル位置の移動
    stStateSSP2.u8CursorPos =
            (stStateSSP2.u8CursorPos + (pcWkData - pcStr)) % 80;
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vWriteDataSSP1
 *
 * DESCRIPTION:Write Data
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8*      pcData          R   データのポインタ
 *      uint8       u8Len           R   配列サイズ 
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vWriteDataSSP1(uint8* pcData, uint8 u8Len) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // データの送信
    uint8* pu8WkData = pcData;
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < u8Len; u8Idx++) {
        I2C_u8MstTxSSP1(*pu8WkData);
        __delay_us(ST7032_DEF_WAIT);
        pu8WkData++;
    }
    // ストップコンディション
    I2C_vMstStopSSP1();
    // カーソル位置の移動
    stStateSSP1.u8CursorPos =
            (stStateSSP1.u8CursorPos + (pu8WkData - pcData)) % 80;
}

/*******************************************************************************
 *
 * NAME: ST7032_vWriteDataSSP2
 *
 * DESCRIPTION:Write Data
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8*      pcData          R   データのポインタ
 *      uint8       u8Len           R   配列サイズ 
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vWriteDataSSP2(uint8* pcData, uint8 u8Len) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // データの送信
    uint8* pu8WkData = pcData;
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < u8Len; u8Idx++) {
        I2C_u8MstTxSSP2(*pu8WkData);
        __delay_us(ST7032_DEF_WAIT);
        pu8WkData++;
    }
    // ストップコンディション
    I2C_vMstStopSSP2();
    // カーソル位置の移動
    stStateSSP2.u8CursorPos =
            (stStateSSP2.u8CursorPos + (pu8WkData - pcData)) % 80;
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vWriteIconSSP1
 *
 * DESCRIPTION:アイコンの書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Addr          R   ICON Address
 *       uint8      u8Map           R   ICON Bit Map
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vWriteIconSSP1(uint8 u8Addr, uint8 u8Map) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
	// インストラクションテーブルチェンジ（IS=1）
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
	// コマンドの送信（アイコンアドレス）
    vExecCmdSSP1(ST7032_CMD_SET_ICON_ADDR | (u8Addr & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // インストラクションテーブルチェンジ（IS=0）
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // データの送信
    I2C_u8MstTxSSP1(u8Map & 0x1F);
    // ストップビット;
    I2C_vMstStopSSP1();
    __delay_us(ST7032_DEF_WAIT);
    // カーソル位置を戻す
    bSetCursorSSP1(stStateSSP1.u8CursorPos);
}

/*******************************************************************************
 *
 * NAME: ST7032_vWriteIconSSP2
 *
 * DESCRIPTION:アイコンの書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Addr          R   ICON Address
 *       uint8      u8Map           R   ICON Bit Map
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vWriteIconSSP2(uint8 u8Addr, uint8 u8Map) {
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
	// インストラクションテーブルチェンジ（IS=1）
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
	// コマンドの送信（アイコンアドレス）
    vExecCmdSSP2(ST7032_CMD_SET_ICON_ADDR | (u8Addr & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // インストラクションテーブルチェンジ（IS=0）
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // コントロールバイト（データ）の送信
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // データの送信
    I2C_u8MstTxSSP2(u8Map & 0x1F);
    // ストップビット;
    I2C_vMstStopSSP2();
    __delay_us(ST7032_DEF_WAIT);
    // カーソル位置を戻す
    bSetCursorSSP2(stStateSSP2.u8CursorPos);
}
#endif

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: vExecCmdSSP1
 *
 * DESCRIPTION:LCDインストラクションの実行
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Cmd           R   インストラクション      
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
static void vExecCmdSSP1(uint8 u8Cmd) {
    // コントロールバイト（コマンド）の送信
    I2C_u8MstTxSSP1(ST7032_CNTR_CMD | 0x80);
    // コマンドの送信
    I2C_u8MstTxSSP1(u8Cmd);
}

/*******************************************************************************
 *
 * NAME: vExecCmdSSP2
 *
 * DESCRIPTION:LCDインストラクションの実行
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Cmd           R   インストラクション      
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
static void vExecCmdSSP2(uint8 u8Cmd) {
    // コントロールバイト（コマンド）の送信
    I2C_u8MstTxSSP2(ST7032_CNTR_CMD | 0x80);
    // コマンドの送信
    I2C_u8MstTxSSP2(u8Cmd);
}
#endif

/*******************************************************************************
 *
 * NAME: vExecCmdEndSSP1
 *
 * DESCRIPTION:Execute Command and Stop End
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Cmd           R   インストラクション      
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
static void vExecCmdEndSSP1(uint8 u8Cmd) {
    // コントロールバイト（コマンド）の送信
    I2C_u8MstTxSSP1(ST7032_CNTR_CMD);
    // コマンドの送信
    I2C_u8MstTxSSP1(u8Cmd);
    // ストップコンディション
    I2C_vMstStopSSP1();
}

/*******************************************************************************
 *
 * NAME: vExecCmdEndSSP2
 *
 * DESCRIPTION:Execute Command and Stop End
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Cmd           R   インストラクション      
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
static void vExecCmdEndSSP2(uint8 u8Cmd) {
    // コントロールバイト（コマンド）の送信
    I2C_u8MstTxSSP2(ST7032_CNTR_CMD);
    // コマンドの送信
    I2C_u8MstTxSSP2(u8Cmd);
    // ストップコンディション
    I2C_vMstStopSSP2();
}
#endif

/*******************************************************************************
 *
 * NAME: bSetCursorSSP1
 *
 * DESCRIPTION:Move cursor
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Pos           R   Cursor Position
 *
 * RETURNS:
 *     bool true:設定成功、false:設定失敗
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
static bool bSetCursorSSP1(uint8 u8Pos) {
    // カーソル設定の可否を判定
    stStateSSP1.u8CursorPos = u8Pos;
    // カーソルアドレスへ変換
    uint8 u8Addr = (u8Pos / 40) * 0x40 + (u8Pos % 40);
    // スタートコンディションの送信
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // カーソル設定
    vExecCmdEndSSP1(ST7032_CMD_SET_DD_ADDR | (u8Addr & 0x7F));
    __delay_us(ST7032_DEF_WAIT);
    // 設定成功
    return true;
}

/*******************************************************************************
 *
 * NAME: bSetCursorSSP2
 *
 * DESCRIPTION:Move cursor
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Pos           R   Cursor Position
 *
 * RETURNS:
 *     bool true:設定成功、false:設定失敗
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
static bool bSetCursorSSP2(uint8 u8Pos) {
    // カーソル設定の可否を判定
    stStateSSP2.u8CursorPos = u8Pos;
    // カーソルアドレスへ変換
    uint8 u8Addr = (u8Pos / 40) * 0x40 + (u8Pos % 40);
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // カーソル設定
    vExecCmdEndSSP2(ST7032_CMD_SET_DD_ADDR | (u8Addr & 0x7F));
    __delay_us(ST7032_DEF_WAIT);
    // 設定成功
    return true;    
}
#endif

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
