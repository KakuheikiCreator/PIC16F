/*******************************************************************************
 *
 * MODULE :I/O Interface Main functions source file
 *
 * CREATED:2019/03/10 03:38:00
 * AUTHOR :Nakanohito
 *
 * Target MCU :PIC16F1827
 * DESCRIPTION:I2C I/O Interface firmware
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
#include <stdio.h>
#include <string.h>
#include <pic16f1827.h>
#include "setting.h"            // 設定値
#include "libcom.h"             // 共通定義
#include "i2cUtil.h"            // I2C関数ライブラリ用
#include "keypad.h"             // キーパッド
#include "st7032.h"             // LCD

/******************************************************************************/
/***       Macro Definitions                                                ***/
/******************************************************************************/
// コンフィグ１
#pragma config FOSC     = INTOSC  // 内部クロック(INTOSC:16MHz)使用する
#pragma config WDTE     = OFF     // ウオッチドッグタイマー無し(OFF)
#pragma config PWRTE    = ON      // 電源ONから64ms後にプログラムを開始する(ON)
#pragma config MCLRE    = OFF     // 外部リセット信号は使用せずにデジタル入力(RA3)ピンとする(OFF)
#pragma config CP       = OFF     // プログラムメモリーを保護しない(OFF)
#pragma config CPD      = OFF     // EEPROMのデータ読み出し許可（OFF）
#pragma config BOREN    = ON      // 電源電圧降下(BORV設定以下)常時監視機能ON(ON)
#pragma config CLKOUTEN = OFF     // CLKOUTピンをRA4ピンで使用する(OFF)
#pragma config IESO     = OFF     // 内部から外部クロックへの切替えでの起動はなし(OFF)
#pragma config FCMEN    = OFF     // 部クロック監視しない(OFF)

// コンフィグ２
#pragma config WRT      = OFF     // Flashメモリーを保護しない(OFF)
#pragma config PLLEN    = OFF     // 動作クロックを32MHzでは動作させない(OFF)
#pragma config STVREN   = ON      // スタックがオーバフローやアンダーフローしたらリセットをする(ON)
#pragma config BORV     = HI      // 電源電圧降下常時監視電圧(2.7V)設定(HI)
#pragma config LVP      = OFF     // 低電圧プログラミング機能使用しない(OFF)

// I2C 7bit address
#ifndef I2C_ADDR
#define	I2C_ADDR            (0x08)
#endif

// ピン：電源関係
#define PIN_POWER           RB0
#define PIN_BACK_LIGHT      RB3

// LCDコントラスト
#define LCD_CONTRAST_DEF    (0x28)

// メモリマップサイズ
#define MAP_SIZE            (0xA7)
// メモリマップ状のデータサイズ
#define MAP_DATA_SIZE       (80)
#define MAP_CGRAM_SIZE      (64)
#define MAP_ICONRAM_SIZE    (16)
// メモリマップ位置
#define	MAP_ADDR_DISPLAY    (0x07)
#define	MAP_ADDR_CGRAM      (0x57)
#define	MAP_ADDR_ICONRAM    (0x97)

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
/**
 * ステータス
 */
typedef enum {
    MEM_STS_NORMAL      = 0x00, // 正常
    MEM_STS_PROCESSING  = 0x01  // 処理中
} teMapStatus;

/**
 * イベント種別
 */
typedef enum {
    EVT_NONE            = 0x00, // 通知イベント無し
    EVT_TIMER           = 0x01, // タイマー
    EVT_PW_CONTRAST     = 0x02, // 電源、コントラスト設定
    EVT_CURSOR_SET      = 0x04, // カーソル設定
    EVT_CURSOR_DRAW     = 0x08, // カーソル描画
    EVT_DRAW_LINE_0     = 0x10, // １行目描画
    EVT_DRAW_LINE_1     = 0x20, // ２行目描画
    EVT_SET_CGRAM       = 0x40, // CGRAM設定
    EVT_DRAW_ICON       = 0x80  // アイコン描画
} teEventType;

/**
 * アプリケーションステータス
 */
typedef struct {
    uint8 u8TimerCnt;           // タイマーカウンタ
    bool bWriteStartFlg;        // 書き込みスタートコンディション受信フラグ
    uint8 u8MapAddr;            // 現在メモリマップアドレス位置
    uint8 u8EventMap;           // イベントマップ
} tsAppStatus;

/**
 * メモリマップ
 */
typedef struct {
    teMapStatus eStatus;                    // ステータス
    uint8 u8KeyValue;                       // キー値（最終値）
    uint8 u8Power;                          // バックライト
    uint8 u8Contrast;                       // コントラスト
    uint8 u8CursorType;                     // カーソルタイプ
    uint8 u8CursorRow;                      // カーソル行
    uint8 u8CursorCol;                      // カーソル列
    uint8 u8DispRam[MAP_DATA_SIZE];         // 表示文字RAM
    uint8 u8CGRam[MAP_CGRAM_SIZE];          // ユーザー文字RAM
    uint8 u8IconRam[MAP_ICONRAM_SIZE];      // アイコンRAM
} tsMemoryMap;


/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
// イベントステータス設定
static void evt_vSetEventMap(teEventType eEvtStatus);
// イベントステータス設定
static void evt_vSetDrawEvent(uint8 u8Addr);
// イベント情報取得
static uint8 evt_u8GetEventMap();
// 電源設定処理
static void lcd_vPowerSetting(uint8 u8Settings);
// カーソル設定描画処理
static void lcd_vCursorSetting(uint8 u8CursorType);
// カーソル描画処理
static void lcd_vDrawCursor();
// 行描画処理
static void lcd_vDarwLine(uint8 u8RowNo);
// CGRAM書き込み
static void lcd_vDrawCGRAM();
// ICON Ram書き込み
static void lcd_vDrawIconRAM();
// タイマー割り込み処理
static void timer_vInterrupt();
// I2C割り込みのコールバック関数
static void ssp1_vCallback(uint8 u8BusNo, uint8 u8EvtType);
// 書き込みリクエスト処理
static void ssp1_vWriteData(uint8 u8Data);
// 読み込みリクエスト処理
static uint8 ssp1_u8ReadData();

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
//==============================================================================
// Variable definition
//==============================================================================
// デバッグ用ステータス
static uint8 u8Status = 0x00;
// アプリケーションステータス
static tsAppStatus sAppStatus;
// メモリーマップ
static tsMemoryMap sMemoryMap;

/******************************************************************************/
/***        Main Functions                                                  ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: main
 *
 * DESCRIPTION:主処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
void main() {
    //==========================================================================
    // タイマーおよびピン設定
    //==========================================================================
    OSCCON = 0b01111010;    // 内部クロックは16ＭＨｚとする
    ANSELA = 0b00000000;    // アナログ使用しない(すべてデジタルI/Oに割当てる)
    ANSELB = 0b00000000;    // アナログ使用しない(すべてデジタルI/Oに割当てる)
    TRISA  = 0b00011100;    // キーパッドのみ入力モードとする
    TRISB  = 0b10110110;    // ピンはSDA/SCLのみ入力
    PORTA  = 0b00000000;    // 出力ピンの初期化(全てLOWにする)
    PORTB  = 0b00000000;    // 出力ピンの初期化(全てLOWにする)

    //==========================================================================
    // アプリケーションステータスの初期化
    //==========================================================================
    sAppStatus.u8TimerCnt     = 0;          // タイマーカウンタ
    sAppStatus.bWriteStartFlg = false;      // スタートコンディション受信フラグ
    sAppStatus.u8MapAddr      = 0x00;       // マップ上のアドレス
    sAppStatus.u8EventMap     = 0x00;       // イベントマップ
    
    //==========================================================================
    // タイマー設定
    // FOSC = 16MHz / 4
    // カウントアップサイクル = FOSC / プリスケーラ = (16MHz / 4) / 128
    // 1秒間の割り込み回数 = カウントアップサイクル / 256 = 128回
    //==========================================================================
    OPTION_REG = 0b00000110;    // 内部クロックでTIMER0を使用、プリスケーラ 1:128
    TMR0   = 0;                 // タイマー0の初期化
    TMR0IF = 0;                 // タイマー0割込フラグ(T0IF)を0にする
    TMR0IE = 1;                 // タイマー0割込み(T0IE)を許可する
    
    //==========================================================================
    // メモリマップ関連の初期化
    //==========================================================================
    // メモリマップをゼロクリア
    memset(&sMemoryMap, 0x00, sizeof(tsMemoryMap));
    sMemoryMap.u8KeyValue = 0xFF;               // キー値
    sMemoryMap.u8Power    = 0x01;               // LCD電源
    sMemoryMap.u8Contrast = LCD_CONTRAST_DEF;   // LCDコントラスト
    memset(sMemoryMap.u8CGRam, 0xE0, MAP_CGRAM_SIZE);   // CGRAM
        
    //==========================================================================
    // I2C初期処理
    //==========================================================================
    // SSP1:スレーブモードでの初期化
    I2C_vInitSlaveSSP1(I2C_ADDR, I2C_SLAVE_STD, ssp1_vCallback);
    // SSP2:マスターモードで初期化
    I2C_vInitMasterSSP2(I2C_MASTER_STD, I2C_CLK_DIV_STD_8MHZ);
    
    //==========================================================================
    // キーパッド設定
    //==========================================================================
    tsKEYPAD_status keypadSts;
    // 各列のピン
    keypadSts.u16PinCols[0] = ID_PORTA | 0b00000100;
    keypadSts.u16PinCols[1] = ID_PORTA | 0b00001000;
    keypadSts.u16PinCols[2] = ID_PORTA | 0b00010000;
    keypadSts.u16PinCols[3] = ID_PORTB | 0b10000000;
    // 各行のピン
    keypadSts.u16PinRows[0] = ID_PORTA | 0b00000010;
    keypadSts.u16PinRows[1] = ID_PORTA | 0b00000001;
    keypadSts.u16PinRows[2] = ID_PORTA | 0b10000000;
    keypadSts.u16PinRows[3] = ID_PORTA | 0b01000000;
    
    // 対象のキーパッドを初期化する
    KEYPAD_vInit(&keypadSts);
    
    //==========================================================================
    // 割り込みの有効化
    //==========================================================================
    PEIE = 1;   // 周辺装置割り込み有効
    GIE  = 1;   // 全割込み処理を許可する

    //==========================================================================
    // LCDの初期化処理
    //==========================================================================
    // LCDピン設定初期化
    PIN_POWER      = ON;
    PIN_BACK_LIGHT = OFF;
    __delay_ms(40);
    // LCD初期化処理
    ST7032_vInitSSP2();
    
    //==========================================================================
    // 主処理ループ
    //==========================================================================
    uint8 u8EventMap;
    // メインループ
    while(true) {
        //----------------------------------------------------------------------
        // イベント処理
        //----------------------------------------------------------------------
        // イベント通知判定
        u8EventMap = evt_u8GetEventMap();
        if (u8EventMap == EVT_NONE) {
            continue;
        }
        // 電源コントラスト設定
        if ((u8EventMap & EVT_PW_CONTRAST) == EVT_PW_CONTRAST) {
            // 電源とコントラスト設定
            lcd_vPowerSetting(sMemoryMap.u8Power);
            // コントラスト変更
            ST7032_vSetContrastSSP2(sMemoryMap.u8Contrast);
        }
        // カーソル設定
        if ((u8EventMap & EVT_CURSOR_SET) == EVT_CURSOR_SET) {
            lcd_vCursorSetting(sMemoryMap.u8CursorType);
        }
        // カーソル描画判定
        if ((u8EventMap & EVT_CURSOR_DRAW) == EVT_CURSOR_DRAW) {
            lcd_vDrawCursor();
        }
        // １行目描画判定
        if ((u8EventMap & EVT_DRAW_LINE_0) == EVT_DRAW_LINE_0) {
            lcd_vDarwLine(0);
        }
        // ２行目描画判定
        if ((u8EventMap & EVT_DRAW_LINE_1) == EVT_DRAW_LINE_1) {
            lcd_vDarwLine(1);
        }
        // CGRAMへの書き込み判定
        if ((u8EventMap & EVT_SET_CGRAM) == EVT_SET_CGRAM) {
            // CGRAMへの書き込み
            lcd_vDrawCGRAM();
        }
        // ICON RAMへの書き込み判定
        if ((u8EventMap & EVT_DRAW_ICON) == EVT_DRAW_ICON) {
            // ICON RAMへの書き込み
            lcd_vDrawIconRAM();
        }
    }
}

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: evt_vSetEventMap
 *
 * DESCRIPTION:イベントマップ設定
 *
 * PARAMETERS:      Name            RW  Usage
 * teEventType      eEvtType        R   設定するイベントマップ
 * 
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void evt_vSetEventMap(teEventType eEvtType) {
    sMemoryMap.eStatus = MEM_STS_PROCESSING;
    sAppStatus.u8EventMap = sAppStatus.u8EventMap | eEvtType;
}

/*******************************************************************************
 *
 * NAME: evt_vSetDrawEvent
 *
 * DESCRIPTION:描画イベント設定
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Addr          R   描画依頼アドレス
 * 
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void evt_vSetDrawEvent(uint8 u8Addr) {
    // 指定された行の描画イベント
    if (u8Addr < 40) {
        evt_vSetEventMap(EVT_DRAW_LINE_0);
    } else {
        evt_vSetEventMap(EVT_DRAW_LINE_1);
    }    
}

/*******************************************************************************
 *
 * NAME: evt_u8GetEventMap
 *
 * DESCRIPTION:イベントマップ取得処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8:イベントマップ
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static uint8 evt_u8GetEventMap() {
    // クリティカルセクションの開始
    criticalSec_vBegin();
    // マップステータス更新
    uint8 u8EvtMap = sAppStatus.u8EventMap;
    if (sAppStatus.u8EventMap == EVT_NONE) {
        sMemoryMap.eStatus = MEM_STS_NORMAL;
    } else {
        sAppStatus.u8EventMap = EVT_NONE;
    }
    // クリティカルセクションの終了
    criticalSec_vEnd();
    // イベントマップを返却
    return u8EvtMap;
}

/*******************************************************************************
 *
 * NAME: lcd_vPowerSetting
 *
 * DESCRIPTION:電源設定処理
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Settings      R   カーソルタイプ
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void lcd_vPowerSetting(uint8 u8Settings) {
    //==========================================================================
    // クリティカルセクション
    //==========================================================================
    criticalSec_vBegin();
    // カーソル位置情報の取得
    uint8 u8Val = sMemoryMap.u8Power;
    sMemoryMap.u8Power = sMemoryMap.u8Power | 0x01;
    criticalSec_vEnd();
    
    //==========================================================================
    // 電源処理
    //==========================================================================
    // 電源オフ判定
    if ((u8Val & 0x01) == 0x00) {
        // リセット実行
        PIN_POWER = OFF;
        __delay_ms(1);
        PIN_POWER = ON;
        __delay_ms(40);
        // LCD初期化処理
        ST7032_vInitSSP2();
    }
    // バックライト設定
    if ((u8Val & 0x02) == 0x00) {
        PIN_BACK_LIGHT = OFF;
    } else {
        PIN_BACK_LIGHT = ON;
    }
}

/*******************************************************************************
 *
 * NAME: lcd_vCursorSetting
 *
 * DESCRIPTION:カーソル設定描画処理
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8CursorType    R   カーソルタイプ
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void lcd_vCursorSetting(uint8 u8CursorType) {
    // カーソル表示設定の取得
    bool bCursorDisp = u8CursorType & 0x01;
    bool bCursorBlink = (u8CursorType >> 1) & 0x01;
    // カーソル表示設定
    ST7032_vDispSettingSSP2(true, bCursorDisp, bCursorBlink);
}

/*******************************************************************************
 *
 * NAME: lcd_vDrawCursor
 *
 * DESCRIPTION:カーソル描画処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void lcd_vDrawCursor() {
    // クリティカルセクションの開始
    criticalSec_vBegin();
    // カーソル位置情報の取得
    uint8 u8CursorRow = sMemoryMap.u8CursorRow;
    uint8 u8CursorCol = sMemoryMap.u8CursorCol;
    // クリティカルセクションの終了
    criticalSec_vEnd();
    // カーソル再描画
    ST7032_bSetCursorSSP2(u8CursorRow, u8CursorCol);
}

/*******************************************************************************
 *
 * NAME: lcd_vDarwLine
 *
 * DESCRIPTION:行描画処理
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8RowNo         R   描画対象行番号
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void lcd_vDarwLine(uint8 u8RowNo) {
    //==========================================================================
    // マップからの描画データ取得
    //==========================================================================
    // クリティカルセクションの開始
    criticalSec_vBegin();
    // 文字描画処理
    uint8 u8Msg[16];
    memcpy(u8Msg, &sMemoryMap.u8DispRam[u8RowNo * 40], 16);
    // クリティカルセクションの終了
    criticalSec_vEnd();
    
    //==========================================================================
    // 行描画処理
    //==========================================================================
    // カーソル移動
    ST7032_bSetCursorSSP2(u8RowNo, 0);
    // 行描画処理
    ST7032_vWriteDataSSP2(u8Msg, 16);
    // カーソル再描画
    lcd_vDrawCursor();
}

/*******************************************************************************
 *
 * NAME: lcd_vDrawCGRAM
 *
 * DESCRIPTION:CGRAM書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void lcd_vDrawCGRAM() {
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 8; u8Idx++) {
        if (sMemoryMap.u8CGRam[u8Idx * 8] < 0x20) {
            ST7032_vWriteCGRAMSSP2(u8Idx, &sMemoryMap.u8CGRam[u8Idx * 8]);
        }
    }
}

/*******************************************************************************
 *
 * NAME: lcd_vDrawIconRAM
 *
 * DESCRIPTION:ICON RAM書き込み
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void lcd_vDrawIconRAM() {
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < MAP_ICONRAM_SIZE; u8Idx++) {
        ST7032_vWriteIconSSP2(u8Idx, sMemoryMap.u8IconRam[u8Idx]);
    }
}

/*******************************************************************************
 *
 * NAME: timer_vInterrupt
 *
 * DESCRIPTION:タイマー割り込み処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void timer_vInterrupt() {
    // タイマー０の割り込みの有無を判定
    if (TMR0IF != 1) {
        return;
    }
    // タイマーカウンタ
    TMR0   = 0;                 // タイマー0のカウンタ初期化
    TMR0IF = 0;                 // タイマー0割込フラグをリセット
    // 秒間128回の割り込みを想定
    // 割込み発生の回数をカウントする
    if (sAppStatus.u8TimerCnt < 128) {
        sAppStatus.u8TimerCnt++;
    } else {
        sAppStatus.u8TimerCnt = 0;
    }
    // タイマーイベント
    // 秒間128回の割り込みを想定し、秒間64回イベント発生
    if ((sAppStatus.u8TimerCnt % 2) == 0) {
        evt_vSetEventMap(EVT_TIMER);
    }
    // キー値更新
    KEYPAD_bUpdateBuffer();
    uint8 u8KeyNo = KEYPAD_u8Read();
    if (u8KeyNo != 0xFF) {
        sMemoryMap.u8KeyValue = u8KeyNo;
    }
}

/*******************************************************************************
 *
 * NAME: ssp1_vCallback
 *
 * DESCRIPTION:I2C割り込みのコールバック関数
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       u8BusNo         R   Bus number
 *      uint8       u8EvtType       R   Event Type
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void ssp1_vCallback(uint8 u8BusNo, uint8 u8EvtType) {
    uint8 u8Data = 0;
    switch (u8EvtType) {
        case I2C_SLV_EVT_WRITE_ADDR:
            // 書き込み要求アドレス受信
            // アドレスデータを空読みする
            u8Data = SSP1BUF;
            // スタート状態フラグ更新
            sAppStatus.bWriteStartFlg = true;
            break;
        case I2C_SLV_EVT_WRITE_DATA:
            // 書き込み要求データ受信
            ssp1_vWriteData((uint8)SSP1BUF);
            // スタート状態フラグ更新
            sAppStatus.bWriteStartFlg = false;
            break;
        case I2C_SLV_EVT_READ_ADDR:
            // 読み出し要求アドレス受信
            // アドレスデータを空読みする
            u8Data = SSP1BUF;
            // 読み出し処理
            SSP1BUF = ssp1_u8ReadData();
            break;
        case I2C_SLV_EVT_READ_ACK:
            // 読み出し要求ACK応答受信
            SSP1BUF = ssp1_u8ReadData();
            break;
        case I2C_SLV_EVT_READ_NACK:
            // 読み出し要求NACK応答受信
            // 読み出し完了なので返信しない
            break;
        default:
            // スタート状態フラグ更新
            sAppStatus.bWriteStartFlg = false;
            break;
    }
}

/*******************************************************************************
 *
 * NAME: ssp1_vWriteData
 *
 * DESCRIPTION:書き込みリクエスト処理
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       u8Data          R   書き込みデータ
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void ssp1_vWriteData(uint8 u8Data) {
    //==========================================================================
    // アドレス設定（スタート状態直後）の処理
    //==========================================================================
    if (sAppStatus.bWriteStartFlg) {
        // アドレスエラー判定
        if (u8Data < MAP_SIZE) {
            // 受信したメモリマップアドレスを設定(ACKはPICが自動的に返信する)
            sAppStatus.u8MapAddr = u8Data;
        } else {
            // メモリオーバーフロー
            // NACK返信する
            SSP1CON2bits.ACKDT = 0x01;
        }
        // 終了
        return;
    }

    //==========================================================================
    // メモリ領域への書き込み
    //==========================================================================
    // アドレスエラー判定
    if (sAppStatus.u8MapAddr >= MAP_SIZE) {
        // NACK返信する
        SSP1CON2bits.ACKDT = 0x01;
        // 終了
        return;
    }
    // アドレス
    uint8 u8Addr;
    // メモリ領域判定
    switch (sAppStatus.u8MapAddr) {
        case 0x00:
            // ステータス（読み込み専用）
            break;
        case 0x01:
            // キー値
            sMemoryMap.u8KeyValue = u8Data;
            break;
        case 0x02:
            // 電源設定
            if (u8Data > 0x03) {
                // NACK返信する
                SSP1CON2bits.ACKDT = 0x01;
                // 終了
                return;
            }
            // 値の更新判定
            if (sMemoryMap.u8Power != u8Data) {
                // 電源オフ判定
                if ((u8Data & 0x01) == 0x00) {
                    // 関連パラメータ初期化
                    sMemoryMap.u8Power      = 0x00;             // LCD電源
                    sMemoryMap.u8Contrast   = LCD_CONTRAST_DEF; // LCDコントラスト
                    sMemoryMap.u8CursorType = 0x00;             // カーソルタイプ
                    sMemoryMap.u8CursorRow  = 0x00;             // カーソル行
                    sMemoryMap.u8CursorCol  = 0x00;             // カーソル列
                    memset(sMemoryMap.u8DispRam, 0x00, MAP_DATA_SIZE);      // 表示文字RAM
                    memset(sMemoryMap.u8CGRam, 0xE0, MAP_CGRAM_SIZE);       // ユーザー文字RAM
                    memset(sMemoryMap.u8IconRam, 0x00, MAP_ICONRAM_SIZE);   // アイコンRAM
                } else {
                    // バックライトを更新
                    sMemoryMap.u8Power = u8Data;
                }
                // イベント情報の通知
                evt_vSetEventMap(EVT_PW_CONTRAST);
            }
            break;
        case 0x03:
            // コントラスト設定
            if (u8Data > ST7032_CONTRAST_MAX) {
                // NACK返信する
                SSP1CON2bits.ACKDT = 0x01;
                // 終了
                return;
            }
            // 値の更新判定
            if (sMemoryMap.u8Contrast != u8Data) {
                // コントラストを更新
                sMemoryMap.u8Contrast = u8Data;
                // イベント情報の通知
                evt_vSetEventMap(EVT_PW_CONTRAST);
            }
            break;
        case 0x04:
            // カーソルタイプ
            // 入力チェック
            if (u8Data > 0x03) {
                // NACK返信する
                SSP1CON2bits.ACKDT = 0x01;
                // 終了
                return;
            }
            // カーソルタイプ
            if (sMemoryMap.u8CursorType != u8Data) {
                // カーソル行を更新
                sMemoryMap.u8CursorType = u8Data;
                // イベント情報の通知
                evt_vSetEventMap(EVT_CURSOR_SET);
            }
            break;
        case 0x05:
            // カーソル行
            // 入力チェック
            if (u8Data > ST7032_ROW_MAX) {
                // NACK返信する
                SSP1CON2bits.ACKDT = 0x01;
                // 終了
                return;
            }
            // カーソル移動判定
            if (sMemoryMap.u8CursorRow != u8Data) {
                // カーソル行を更新
                sMemoryMap.u8CursorRow = u8Data;
                // イベント情報の通知
                evt_vSetEventMap(EVT_CURSOR_DRAW);
            }
            break;
        case 0x06:
            // カーソル列
            // 入力チェック
            if (u8Data > ST7032_COL_MAX) {
                // NACK返信する
                SSP1CON2bits.ACKDT = 0x01;
                // 終了
                return;
            }
            // カーソル移動判定
            if (sMemoryMap.u8CursorCol != u8Data) {
                // カーソル列を更新
                sMemoryMap.u8CursorCol = u8Data;
                // イベント情報の通知
                evt_vSetEventMap(EVT_CURSOR_DRAW);
            }
            break;
        default:
            // 対象データ判定
            if (sAppStatus.u8MapAddr < MAP_ADDR_CGRAM) {
                // 表示データ更新判定
                u8Addr = sAppStatus.u8MapAddr - MAP_ADDR_DISPLAY;
                if (sMemoryMap.u8DispRam[u8Addr] == u8Data) {
                    break;
                }
                // 表示データ設定
                sMemoryMap.u8DispRam[u8Addr] = u8Data;
                // イベント情報の通知
                evt_vSetDrawEvent(u8Addr);
            } else if (sAppStatus.u8MapAddr < MAP_ADDR_ICONRAM) {
                // CGRAM入力チェック
                if (u8Data > 0x1F) {
                    // NACK返信する
                    SSP1CON2bits.ACKDT = 0x01;
                    // 終了
                    return;
                }
                // CGRAM更新判定
                u8Addr = sAppStatus.u8MapAddr - MAP_ADDR_CGRAM;
                if (sMemoryMap.u8CGRam[u8Addr] == u8Data) {
                    break;
                }
                // 表示データ設定
                sMemoryMap.u8CGRam[u8Addr] = u8Data;
                // イベント情報の通知
                evt_vSetEventMap(EVT_SET_CGRAM);
            } else {
                // ICONRAM入力チェック
                if (u8Data > 0x1F) {
                    // NACK返信する
                    SSP1CON2bits.ACKDT = 0x01;
                    // 終了
                    return;
                }
                // ICONRAM更新判定
                u8Addr = sAppStatus.u8MapAddr - MAP_ADDR_ICONRAM;
                if (sMemoryMap.u8IconRam[u8Addr] == u8Data) {
                    break;
                }
                // 表示データ設定
                sMemoryMap.u8IconRam[u8Addr] = u8Data;
                // イベント情報の通知
                evt_vSetEventMap(EVT_DRAW_ICON);
            }
            break;
    }
    // メモリマップアドレスカウントアップ
    sAppStatus.u8MapAddr++;
}

/*******************************************************************************
 *
 * NAME: ssp1_u8ReadData
 *
 * DESCRIPTION:読み込みリクエスト処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     uint8:読み込みデータ
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static uint8 ssp1_u8ReadData() {
    //==========================================================================
    // 現在アドレスからの読み込み処理
    //==========================================================================
    // メモリオーバーフロー判定
    if (sAppStatus.u8MapAddr >= MAP_SIZE) {
        // メモリオーバーフロー
        // NACK返信する
        SSP1CON2bits.ACKDT = 0x01;
        // 終了
        return 0xFF;
    }
    // メモリ領域判定
    uint8 u8Data;
    uint8 u8Addr;
    switch (sAppStatus.u8MapAddr) {
        case 0x00:
            // ステータス（読み込み専用）
            u8Data = sMemoryMap.eStatus;
            break;
        case 0x01:
            // キー値（最終値モード）
            u8Data = sMemoryMap.u8KeyValue;
            sMemoryMap.u8KeyValue = 0xFF;
            break;
        case 0x02:
            // 電源設定
            u8Data = sMemoryMap.u8Power;
            break;
        case 0x03:
            // コントラスト
            u8Data = sMemoryMap.u8Contrast;
            break;
        case 0x04:
            // カーソルタイプ
            u8Data = sMemoryMap.u8CursorType;
            break;
        case 0x05:
            // カーソル行
            u8Data = sMemoryMap.u8CursorRow;
            break;
        case 0x06:
            // カーソル列
            u8Data = sMemoryMap.u8CursorCol;
            break;
        default:
            // 対象データ判定
            if (sAppStatus.u8MapAddr < MAP_ADDR_CGRAM) {
                // 表示データ
                u8Addr = sAppStatus.u8MapAddr - MAP_ADDR_DISPLAY;
                u8Data = sMemoryMap.u8DispRam[u8Addr];
            } else if (sAppStatus.u8MapAddr < MAP_ADDR_ICONRAM) {
                // Character Generator RAM
                u8Addr = sAppStatus.u8MapAddr - MAP_ADDR_CGRAM;
                u8Data = sMemoryMap.u8CGRam[u8Addr];
            } else {
                // ICON Ram
                u8Addr = sAppStatus.u8MapAddr - MAP_ADDR_ICONRAM;
                u8Data = sMemoryMap.u8IconRam[u8Addr];
            }
            break;
    }
    // メモリマップアドレスカウントアップ
    sAppStatus.u8MapAddr++;
    // 値を返信
    return u8Data;
}

/******************************************************************************/
/***        ISR Functions                                                 ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: __interrupt(high_priority) ISR
 *
 * DESCRIPTION:割り込み処理
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
void __interrupt(high_priority) ISR() {
    //==========================================================================
    // タイマー割り込み発生時の処理
    //==========================================================================
    // タイマー割り込み処理
    timer_vInterrupt();
    
    //==========================================================================
    // SSP(I2C)割り込み発生時の処理
    //==========================================================================
    I2C_vSlaveIsrSSP1();
}

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
