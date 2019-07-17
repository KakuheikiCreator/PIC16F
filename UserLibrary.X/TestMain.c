/*******************************************************************************
 *
 * MODULE :Test Main functions source file
 *
 * CREATED:2019/04/29 22:42:00
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
#include "setting.h"            // 設定値
#include "libcom.h"             // 共通定義
#include "i2cUtil.h"            // I2C関数ライブラリ用
#include "keypad.h"             // キーパッド
#include "st7032.h"             // LCD

/******************************************************************************/
/***       Macro Definitions                                                ***/
/******************************************************************************/
// コンフィグ１
#pragma config FOSC  = INTOSC   // 内部クロック(INTOSC:8MHz)使用する
#pragma config WDTE  = OFF      // ウオッチドッグタイマー無し(OFF)
#pragma config PWRTE = ON       // 電源ONから64ms後にプログラムを開始する(ON)
#pragma config MCLRE = OFF      // 外部リセット信号は使用せずにデジタル入力(RA3)ピンとする(OFF)
#pragma config CP    = OFF      // プログラムメモリーを保護しない(OFF)
#pragma config CPD   = OFF      // EEPROMのデータ読み出し許可（OFF）
#pragma config BOREN = ON       // 電源電圧降下(BORV設定以下)常時監視機能ON(ON)
#pragma config CLKOUTEN = OFF   // CLKOUTピンをRA4ピンで使用する(OFF)
#pragma config IESO  = OFF      // 内部から外部クロックへの切替えでの起動はなし(OFF)
#pragma config FCMEN = OFF      // 部クロック監視しない(OFF)

// コンフィグ２
#pragma config WRT    = OFF     // Flashメモリーを保護しない(OFF)
#pragma config PLLEN  = OFF     // 動作クロックを32MHzでは動作させない(OFF)
#pragma config STVREN = ON      // スタックがオーバフローやアンダーフローしたらリセットをする(ON)
#pragma config BORV   = HI      // 電源電圧降下常時監視電圧(2.7V)設定(HI)
#pragma config LVP    = OFF     // 低電圧プログラミング機能使用しない(OFF)

// 16進数文字への変換
#define hexToChar(val) HEX_LIST[val & 0x0F]

// I2C 7bit address
#define	I2C_ADDR    (8)

// ピン：電源関係
#define PIN_POWER           RB0
#define PIN_BACK_LIGHT      RB3

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
/**
 * 入力モード
 */
typedef enum {
    INPUT_MODE_NONE         = 0x00, // 入力無効
    INPUT_MODE_CURRENT      = 0x01, // 現在値モード
    INPUT_MODE_FINAL        = 0x02, // 最終値モード
    INPUT_MODE_BUFFERING    = 0x03  // バッファリング
} teInputMode;


/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
// タイマー割り込み処理
static void timer_vInterrupt();
// I2C割り込みのコールバック関数
static void ssp1_vCallback(uint8 u8BusNo, uint8 u8EvtType);
// I2C Test 01:書き込み（１バイト単位）
static void ssp2_vI2CTest01();
// I2C Test 02:書き込み（複数バイト単位）
static void ssp2_vI2CTest02();
// I2C Test 03:読み込み（１バイト単位）
static void ssp2_vI2CTest03();
// I2C Test 03:読み込み（複数バイト単位）
static void ssp2_vI2CTest04();
// LCD Test 01:文字描画
static void ssp2_vLCDTest01();
// LCD Test 02:カーソル移動
static void ssp2_vLCDTest02();
// LCD Test 03:CGRAM
static void ssp2_vLCDTest03();
// LCD Test 04:ICON
static void ssp2_vLCDTest04();
// Keypad Test 01:
static void ssp2_vKeypadTest01();
// Keypad Test 02:
static void ssp2_vKeypadTest02();
// Keypad Test 03:
static void ssp2_vKeypadTest03();
// Keypad Test 04:
static void ssp2_vKeypadTest04();

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
// 文字配列
static const char HEX_LIST[] = "0123456789ABCDEF";
static const char KEY_LIST[] = "123A456B789C*0#D";
// I2Cデータ
static uint8 u8RxData;
// 入力モード
static teInputMode eInputMode;
// キー値
static uint8 u8KeyValue;

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
 *   None.
 ******************************************************************************/
void main() {
    //==========================================================================
    // タイマーおよびピン設定
    //==========================================================================
    OSCCON = 0b01111010;    // 内部クロックは16ＭＨｚとする
    ANSELA = 0b00000000;    // アナログ使用しない(すべてデジタルI/Oに割当てる)
    ANSELB = 0b00000000;    // アナログ使用しない(すべてデジタルI/Oに割当てる)
    TRISA  = 0b00011100;    // 全てを出力モードとする
    TRISB  = 0b10110110;    // ピンはRB1(SDA)/RB4(SCL)のみ入力
    PORTA  = 0b00000000;    // 出力ピンの初期化(全てLOWにする)
    PORTB  = 0b00000000;    // 出力ピンの初期化(全てLOWにする)

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
    // テストケース処理
    //==========================================================================
    // メインループ
    while(true) {
        // I2C Test 01:1バイト単位の書き込み
//        ssp2_vI2CTest01();
        // I2C Test 02:複数バイト単位の書き込み
//        ssp2_vI2CTest02();
        // I2C Test 03:1バイト単位の読み込み
//        ssp2_vI2CTest03();
        // I2C Test 04:複数バイト単位の読み込み
//        ssp2_vI2CTest04();
        // LCD Test 01
        ssp2_vLCDTest01();
        // LCD Test 02
        ssp2_vLCDTest02();
        // LCD Test 03
        ssp2_vLCDTest03();
        // LCD Test 04
        ssp2_vLCDTest04();
        // Keypad Test 01
//        ssp2_vKeypadTest01();
        // Keypad Test 02
//        ssp2_vKeypadTest02();
        // Keypad Test 03
//        ssp2_vKeypadTest03();
        // Keypad Test 04
//        ssp2_vKeypadTest04();
    }
}

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
 * None.
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
    // 入力モード判定
    switch (eInputMode) {
        // 入力無効
        case INPUT_MODE_NONE:
            break;
        // 現在値モード
        case INPUT_MODE_CURRENT:
            u8KeyValue = KEYPAD_u8Read();
            break;
        // 最終値モード
        case INPUT_MODE_FINAL:
            // バッファ更新
            KEYPAD_bUpdateBuffer();
            break;
        // バッファリングモード
        case INPUT_MODE_BUFFERING:
            // バッファ更新
            KEYPAD_bUpdateBuffer();
            break;
        default:
            break;
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
 * None.
 ******************************************************************************/
static void ssp1_vCallback(uint8 u8BusNo, uint8 u8EvtType) {
    uint8 u8Data;
    switch (u8EvtType) {
        case I2C_SLV_EVT_WRITE_ADDR:
            // 書き込み要求アドレス受信
            // アドレスデータを空読みする
            u8Data = SSP1BUF;
            break;
        case I2C_SLV_EVT_WRITE_DATA:
            // 書き込み要求データ受信
            // データを読込む(ACKはPICが自動的に返す)
            u8RxData = SSP1BUF;
            break;
        case I2C_SLV_EVT_READ_ADDR:
            // 読み出し要求アドレス受信
            // アドレスデータを空読みする
            u8Data = SSP1BUF;
            // データを返信
            SSP1BUF = u8RxData;
            u8RxData++;
            break;
        case I2C_SLV_EVT_READ_ACK:
            // 読み出し要求ACK応答受信
            // 受信データを送信バッファにセット
            SSP1BUF = u8RxData;
            u8RxData++;
            break;
        case I2C_SLV_EVT_READ_NACK:
            // 読み出し要求NACK応答受信
            // 読み出し完了により、何もしない
            break;
        default:
            break;
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vI2CTest01
 *
 * DESCRIPTION:I2Cテストケース
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vI2CTest01() { 
    //==========================================================================
    // キー読み込みテスト
    //==========================================================================
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // メッセージ
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:I2C 01     ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Tx 0x  ->Rx 0x  ");
    
    //==========================================================================
    // 1バイト単位の送信テスト
    //==========================================================================
    // データクリア
    u8RxData = 0xFF;
    uint8 u8TxData;
    for (u8TxData = 0; u8TxData < 255; u8TxData++) {
        // 送信データ表示
        ST7032_bSetCursorSSP2(1, 5);
        ST7032_vWriteCharSSP2(HEX_LIST[u8TxData / 16]);
        ST7032_vWriteCharSSP2(HEX_LIST[u8TxData % 16]);
        // スタートコンディションの送信
        I2C_u8MstStartSSP2(I2C_ADDR, false);
        I2C_u8MstTxSSP2(u8TxData);
        I2C_vMstStopSSP2();
        // 受信データ確認
        ST7032_bSetCursorSSP2(1, 14);
        if (u8RxData == u8TxData) {
            ST7032_vWriteCharSSP2(HEX_LIST[u8RxData / 16]);
            ST7032_vWriteCharSSP2(HEX_LIST[u8RxData % 16]);
        } else {
            ST7032_vWriteStringSSP2("XX");
        }
        // ウェイト
        __delay_ms(20);
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vI2CTest02
 *
 * DESCRIPTION:I2Cテストケース、複数バイト単位の書き込みテスト
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vI2CTest02() { 
    //==========================================================================
    // キー読み込みテスト
    //==========================================================================
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // メッセージ
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:I2C 02     ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Tx 0x00->0xFF   ");
    // ウェイト
    __delay_ms(1000);
    
    //==========================================================================
    // 複数バイト単位の送信テスト
    //==========================================================================
    // スタートコンディションの送信
    I2C_u8MstStartSSP2(I2C_ADDR, false);
    // データクリア
    u8RxData = 0xFF;
    uint8 u8TxData;
    for (u8TxData = 0; u8TxData < 255; u8TxData++) {
        I2C_u8MstTxSSP2(u8TxData);
        // 受信データ確認
        if (u8RxData != u8TxData) {
            break;
        }
    }
    I2C_vMstStopSSP2();
    // 結果表示
    ST7032_bSetCursorSSP2(1, 0);
    if (u8TxData == 255) {
        ST7032_vWriteStringSSP2("Tx 0x00->0xFF OK");
    } else {
        ST7032_vWriteStringSSP2("Tx 0x00->0xFF NG");
    }
    // ウェイト
    __delay_ms(1000);
}

/*******************************************************************************
 *
 * NAME: ssp2_vI2CTest03
 *
 * DESCRIPTION:I2Cテストケース、複数バイト単位の書き込みテスト
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vI2CTest03() { 
    //==========================================================================
    // キー読み込みテスト
    //==========================================================================
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // メッセージ
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:I2C 03     ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Rx 0x00->0xFF   ");
    
    //==========================================================================
    // 1バイト単位の受信テスト
    //==========================================================================
    // データクリア
    u8RxData = 0x00;
    uint8 u8Data;
    uint8 u8Cnt;
    for (u8Cnt = 0; u8Cnt < 255; u8Cnt++) {
        // 1バイト読み込み
        I2C_u8MstStartSSP2(I2C_ADDR, true);
        u8Data = I2C_u8MstRxSSP2(true);
        I2C_vMstStopSSP2();
        if (u8Data != u8Cnt) {
            break;
        }
    }
    // 1バイト読み込み
    I2C_u8MstStartSSP2(I2C_ADDR, true);
    u8Data = I2C_u8MstRxSSP2(true);
    I2C_vMstStopSSP2();
    __delay_ms(1000);
    // 結果表示
    ST7032_bSetCursorSSP2(1, 14);
    if (u8Data == 255) {
        ST7032_vWriteStringSSP2("OK");
    } else {
        ST7032_vWriteStringSSP2("NG");
    }
    // ウェイト
    __delay_ms(1000);
}

/*******************************************************************************
 *
 * NAME: ssp2_vI2CTest04
 *
 * DESCRIPTION:I2Cテストケース、複数バイト単位の書き込みテスト
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vI2CTest04() { 
    //==========================================================================
    // キー読み込みテスト
    //==========================================================================
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // メッセージ
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:I2C 04     ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Rx 0x00->0xFF   ");
    
    //==========================================================================
    // 複数バイト単位の受信テスト
    //==========================================================================
    // データクリア
    u8RxData = 0x00;
    // スタートコンディション
    I2C_u8MstStartSSP2(I2C_ADDR, true);
    uint8 u8Data;
    uint8 u8Cnt;
    for (u8Cnt = 0; u8Cnt < 255; u8Cnt++) {
        // 1バイト読み込み
        u8Data = I2C_u8MstRxSSP2(false);
        if (u8Data != u8Cnt) {
            break;
        }
    }
    // 1バイト読み込み
    u8Data = I2C_u8MstRxSSP2(true);
    // ストップコンディション
    I2C_vMstStopSSP2();
    __delay_ms(1000);
    // 結果表示
    ST7032_bSetCursorSSP2(1, 14);
    if (u8Data == 255) {
        ST7032_vWriteStringSSP2("OK");
    } else {
        ST7032_vWriteStringSSP2("NG");
    }
    // ウェイト
    __delay_ms(1000);
}

/*******************************************************************************
 *
 * NAME: ssp2_vLCDTest01
 *
 * DESCRIPTION:I2Cテストケース、複数バイト単位の書き込みテスト
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vLCDTest01() {
    //==========================================================================
    // 2行目に描画
    //==========================================================================
    // 画面クリア
    ST7032_vClearDispSSP2();
    // カーソル移動
    ST7032_vCursorTopSSP2();
    // 行描画処理
    uint8 val = (uint8)(rand() % 0xFF);
    ST7032_vWriteStringSSP2("Test:LCD 01 0x");
    ST7032_vWriteCharSSP2(hexToChar(val >> 4));
    ST7032_vWriteCharSSP2(hexToChar(val & 0x0F));
    // カーソル移動
    ST7032_bSetCursorSSP2(1, 0);
    // 文字描画
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx <16; u8Idx++) {
        // 行描画処理
        ST7032_vWriteCharSSP2(val);
        val = (val + 1) % 0xFF;
        // 0.5秒間隔で実行
        __delay_ms(500);
    }
    //==========================================================================
    // 1行目に描画
    //==========================================================================
    // 画面クリア
    ST7032_vClearDispSSP2();
    // カーソル移動
    ST7032_bSetCursorSSP2(1, 0);
    // 行描画処理
    ST7032_vWriteStringSSP2("Test:LCD 01 0x");
    ST7032_vWriteCharSSP2(hexToChar(val >> 4));
    ST7032_vWriteCharSSP2(hexToChar(val & 0x0F));
    // カーソル移動
    ST7032_bSetCursorSSP2(0, 0);
    // 文字描画
    for (u8Idx = 0; u8Idx <16; u8Idx++) {
        // 行描画処理
        ST7032_vWriteDataSSP2(&val, 1);
        val = (val + 1) % 0xFF;
        // 0.5秒間隔で実行
        __delay_ms(500);
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vLCDTest02
 *
 * DESCRIPTION:LCDテストケース、カーソル移動
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vLCDTest02() {
    //==========================================================================
    // 2行目に描画
    //==========================================================================
    // 画面クリア
    ST7032_vClearDispSSP2();
    // カーソル移動
    ST7032_bSetCursorSSP2(0, 0);
    // 行描画処理
    ST7032_vWriteStringSSP2("Test:LCD 02 0x0");
    ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorRowNoSSP2()));
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // 文字描画
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 16; u8Idx++) {
        // カーソル移動
        if ((u8Idx % 2) == 0) {
            if (ST7032_bSetCursorSSP2(1, u8Idx) == true) {
                ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorColNoSSP2()));
            }
        } else {
            ST7032_bCursorRightSSP2();
        }
        // 0.5秒間隔で実行
        __delay_ms(500);
    }
    for (u8Idx = 0; u8Idx < 16; u8Idx++) {
        // カーソル移動
        if ((u8Idx % 2) == 0) {
            if (ST7032_bSetCursorSSP2(1, 15 - u8Idx) == true) {
                ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorColNoSSP2()));
            }
        } else {
            ST7032_bCursorLeftSSP2();
        }
        // 0.5秒間隔で実行
        __delay_ms(500);
    }
    //==========================================================================
    // 1行目に描画
    //==========================================================================
    // 画面クリア
    ST7032_vClearDispSSP2();
    // カーソル移動
    ST7032_bSetCursorSSP2(1, 0);
    // 行描画処理
    ST7032_vWriteStringSSP2("Test:LCD 02 0x0");
    ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorRowNoSSP2()));
    // カーソル点滅
    ST7032_vDispSettingSSP2(true, true, true);
    // 文字描画
    for (u8Idx = 0; u8Idx <16; u8Idx++) {
        // カーソル移動
        if ((u8Idx % 2) == 0) {
            ST7032_bSetCursorSSP2(0, u8Idx);
            ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorColNoSSP2()));
        } else {
            ST7032_bCursorRightSSP2();
        }
        // 0.5秒間隔で実行
        __delay_ms(500);
    }
    for (u8Idx = 0; u8Idx <16; u8Idx++) {
        // カーソル移動
        if ((u8Idx % 2) == 0) {
            ST7032_bSetCursorSSP2(0, 15 - u8Idx);
            ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorColNoSSP2()));
        } else {
            ST7032_bCursorLeftSSP2();
        }
        // 0.5秒間隔で実行
        __delay_ms(500);
    }
    //==========================================================================
    // 2行目に描画
    //==========================================================================
    // 画面クリア
    ST7032_vClearDispSSP2();
    // カーソル移動
    ST7032_bSetCursorSSP2(0, 0);
    // 行描画処理
    ST7032_vWriteStringSSP2("Test:LCD 02 ");
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // カーソル移動
    ST7032_bSetCursorSSP2(1, 38);
    if (ST7032_bCursorRightSSP2() == true) {
        ST7032_bSetCursorSSP2(0, 12);
        ST7032_vWriteCharSSP2('A');
    }
    // 1秒間隔で実行
    __delay_ms(1000);
    ST7032_bSetCursorSSP2(1, 39);
    if (ST7032_bCursorRightSSP2() == false) {
        ST7032_bSetCursorSSP2(0, 13);
        ST7032_vWriteCharSSP2('B');
    }
    // 1秒間隔で実行
    __delay_ms(1000);
    ST7032_bSetCursorSSP2(0, 1);
    if (ST7032_bCursorLeftSSP2() == true) {
        ST7032_bSetCursorSSP2(0, 14);
        ST7032_vWriteCharSSP2('C');
    }
    // 1秒間隔で実行
    __delay_ms(1000);
    ST7032_bSetCursorSSP2(0, 0);
    if (ST7032_bCursorLeftSSP2() == false) {
        ST7032_bSetCursorSSP2(0, 15);
        ST7032_vWriteCharSSP2('D');
    }
    // 1秒間隔で実行
    __delay_ms(1000);
    if (ST7032_bSetCursorSSP2(1, 39) == true) {
        ST7032_bSetCursorSSP2(1, 0);
        ST7032_vWriteCharSSP2('E');
    }
    // 1秒間隔で実行
    __delay_ms(1000);
    if (ST7032_bSetCursorSSP2(1, 40) == false) {
        ST7032_bSetCursorSSP2(1, 1);
        ST7032_vWriteCharSSP2('F');
    }
    // 1秒間隔で実行
    __delay_ms(1000);
}

/*******************************************************************************
 *
 * NAME: ssp2_vLCDTest03
 *
 * DESCRIPTION:LCDテストケース、CGRAM
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vLCDTest03() {
    //==========================================================================
    // CGRAMへの書き込み
    //==========================================================================
    uint8 u8BitMap[] = {0x11, 0x0A, 0x04, 0x15, 0x15, 0x04, 0x0A, 0x11};
    ST7032_vWriteCGRAMSSP2(0x00, u8BitMap);
    u8BitMap[0] = 0x04;
    u8BitMap[1] = 0x0A;
    u8BitMap[2] = 0x11;
    u8BitMap[3] = 0x0A;
    u8BitMap[4] = 0x04;
    u8BitMap[5] = 0x0A;
    u8BitMap[6] = 0x11;
    u8BitMap[7] = 0x0E;
    ST7032_vWriteCGRAMSSP2(0x01, u8BitMap);
    u8BitMap[0] = 0x11;
    u8BitMap[1] = 0x15;
    u8BitMap[2] = 0x15;
    u8BitMap[3] = 0x04;
    u8BitMap[4] = 0x0A;
    u8BitMap[5] = 0x0A;
    u8BitMap[6] = 0x0A;
    u8BitMap[7] = 0x04;
    ST7032_vWriteCGRAMSSP2(0x02, u8BitMap);
    u8BitMap[0] = 0x1F;
    u8BitMap[1] = 0x00;
    u8BitMap[2] = 0x0E;
    u8BitMap[3] = 0x00;
    u8BitMap[4] = 0x1F;
    u8BitMap[5] = 0x00;
    u8BitMap[6] = 0x15;
    u8BitMap[7] = 0x15;
    ST7032_vWriteCGRAMSSP2(0x03, u8BitMap);
    u8BitMap[0] = 0x1F;
    u8BitMap[1] = 0x11;
    u8BitMap[2] = 0x15;
    u8BitMap[3] = 0x11;
    u8BitMap[4] = 0x1F;
    u8BitMap[5] = 0x15;
    u8BitMap[6] = 0x15;
    u8BitMap[7] = 0x15;
    ST7032_vWriteCGRAMSSP2(0x04, u8BitMap);
    u8BitMap[0] = 0x15;
    u8BitMap[1] = 0x15;
    u8BitMap[2] = 0x0E;
    u8BitMap[3] = 0x15;
    u8BitMap[4] = 0x0A;
    u8BitMap[5] = 0x15;
    u8BitMap[6] = 0x0A;
    u8BitMap[7] = 0x15;
    ST7032_vWriteCGRAMSSP2(0x05, u8BitMap);
    u8BitMap[0] = 0x1F;
    u8BitMap[1] = 0x11;
    u8BitMap[2] = 0x15;
    u8BitMap[3] = 0x1D;
    u8BitMap[4] = 0x01;
    u8BitMap[5] = 0x1F;
    u8BitMap[6] = 0x00;
    u8BitMap[7] = 0x1F;
    ST7032_vWriteCGRAMSSP2(0x06, u8BitMap);
    u8BitMap[0] = 0x1F;
    u8BitMap[1] = 0x11;
    u8BitMap[2] = 0x10;
    u8BitMap[3] = 0x1F;
    u8BitMap[4] = 0x01;
    u8BitMap[5] = 0x15;
    u8BitMap[6] = 0x11;
    u8BitMap[7] = 0x1F;
    ST7032_vWriteCGRAMSSP2(0x07, u8BitMap);
    
    //==========================================================================
    // 2行目に描画
    //==========================================================================
    // 画面クリア
    ST7032_vClearDispSSP2();
    // カーソル移動
    ST7032_bSetCursorSSP2(0, 0);
    // 行描画処理
    ST7032_vWriteStringSSP2("Test:LCD 03     ");
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // カーソル移動
    ST7032_bSetCursorSSP2(1, 0);
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 16; u8Idx++) {
        // 描画処理
        ST7032_vWriteCharSSP2(u8Idx);
        // 0.5秒間隔で実行
        __delay_ms(500);
    }
    //==========================================================================
    // 1行目に描画
    //==========================================================================
    // 画面クリア
    ST7032_vClearDispSSP2();
    // カーソル移動
    ST7032_bSetCursorSSP2(1, 0);
    // 行描画処理
    ST7032_vWriteStringSSP2("Test:LCD 03     ");
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // カーソル移動
    ST7032_bSetCursorSSP2(0, 0);
    for (u8Idx = 0; u8Idx < 16; u8Idx++) {
        // 描画処理
        ST7032_vWriteCharSSP2(u8Idx);
        // 0.5秒間隔で実行
        __delay_ms(500);
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vLCDTest04
 *
 * DESCRIPTION:LCDテストケース、アイコン
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vLCDTest04() {
    //==========================================================================
    // アイコンを描画
    //==========================================================================
    // 画面クリア
    ST7032_vClearDispSSP2();
    // アイコンクリア
    ST7032_vClearIconSSP2();
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // カーソル移動
    ST7032_bSetCursorSSP2(0, 0);
    // 行描画処理
    ST7032_vWriteStringSSP2("Test:LCD 04 0:00");
    // アイコン
    uint8 u8Addr;
    uint8 u8SVal;
    for (u8Addr = 0; u8Addr < 16; u8Addr++) {
        // アドレス
        ST7032_bSetCursorSSP2(0, 12);
        ST7032_vWriteCharSSP2(hexToChar(u8Addr));
        // 値
        for (u8SVal = 0; u8SVal < 32; u8SVal++) {
            // 値出力
            ST7032_bSetCursorSSP2(0, 14);
            ST7032_vWriteCharSSP2(hexToChar(u8SVal >> 4));
            ST7032_vWriteCharSSP2(hexToChar(u8SVal & 0x0F));
            // アイコン描画
            ST7032_vWriteIconSSP2(u8Addr, u8SVal);
            // 0.05秒間隔で実行
            __delay_ms(50);
        }
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vKeypadTest01
 *
 * DESCRIPTION:キーパッドテストケース
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vKeypadTest01() {
    //==========================================================================
    // キー読み込みテスト
    //==========================================================================
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // メッセージ
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:Keypad 01  ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("None   :        ");
    //==========================================================================
    // 入力モード：入力無効
    //==========================================================================
    eInputMode = INPUT_MODE_NONE;
    KEYPAD_vClearBuffer();
    // 10秒間入力無効
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 100; u8Idx++) {
        if (KEYPAD_u8ReadBuffer() == 0xFF) {
            ST7032_bSetCursorSSP2(1, 5);
            ST7032_vWriteCharSSP2(HEX_LIST[10 - (u8Idx / 10)]);
        } else {
            ST7032_bSetCursorSSP2(1, 0);
            ST7032_vWriteStringSSP2("None:Error      ");
        }
        __delay_ms(100);
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vKeypadTest02
 *
 * DESCRIPTION:キーパッドテストケース
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vKeypadTest02() {
    //==========================================================================
    // キー読み込みテスト
    //==========================================================================
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // メッセージ
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:Keypad 02  ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Current:Input 1 ");
    //==========================================================================
    // 入力モード：現在値モード
    //==========================================================================
    // 現在値モード
    eInputMode = INPUT_MODE_CURRENT;
    KEYPAD_vClearBuffer();
    u8KeyValue = 0xFF;
    // キー入力判定
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 16; u8Idx++) {
        while (u8KeyValue != u8Idx) {
            __delay_ms(10);
        }
        ST7032_bSetCursorSSP2(1, 14);
        ST7032_vWriteCharSSP2(KEY_LIST[u8Idx + 1]);
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vKeypadTest03
 *
 * DESCRIPTION:キーパッドテストケース
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vKeypadTest03() {
    //==========================================================================
    // キー読み込みテスト
    //==========================================================================
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // メッセージ
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:Keypad 03  ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Final  :Input 1 ");
    //==========================================================================
    // 入力モード：最終値モード
    //==========================================================================
    // 現在値モード
    eInputMode = INPUT_MODE_FINAL;
    KEYPAD_vClearBuffer();
    u8KeyValue = 0xFF;
    // キー入力判定
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 16; u8Idx++) {
        while (KEYPAD_u8ReadFinal() != u8Idx) {
            __delay_ms(2000);
        }
        ST7032_bSetCursorSSP2(1, 14);
        ST7032_vWriteCharSSP2(KEY_LIST[u8Idx + 1]);
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vKeypadTest04
 *
 * DESCRIPTION:キーパッドテストケース
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void ssp2_vKeypadTest04() {
    //==========================================================================
    // キー読み込みテスト
    //==========================================================================
    // カーソル表示
    ST7032_vDispSettingSSP2(true, true, false);
    // メッセージ
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:Keypad 04  ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Buffer :Input 1 ");
    //==========================================================================
    // 入力モード：バッファリングモード
    //==========================================================================
    // 現在値モード
    eInputMode = INPUT_MODE_BUFFERING;
    KEYPAD_vClearBuffer();
    u8KeyValue = 0xFF;
    // キー入力判定
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 16; u8Idx++) {
        while (KEYPAD_u8ReadBuffer() != u8Idx) {
            __delay_ms(2000);
        }
        ST7032_bSetCursorSSP2(1, 14);
        ST7032_vWriteCharSSP2(KEY_LIST[u8Idx + 1]);
    }
}

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
