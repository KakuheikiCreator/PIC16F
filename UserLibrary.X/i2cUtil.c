/*******************************************************************************
 *
 * MODULE :I2C utility functions source file
 *
 * CREATED:2019/03/11 23:25:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:I2C Slave driver
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
/***       Include files                                                    ***/
/******************************************************************************/
#include <xc.h>
#include "i2cUtil.h"        // I2C関数ライブラリー用
#include "libcom.h"         // 共通定義
#include "keypad.h"         // キーパッド

/******************************************************************************/
/***       Macro Definitions                                                ***/
/******************************************************************************/

//==========================================================
// I2Cバス毎の定義
//==========================================================
// SSPxSTATのR/W、D/A、BF、SSPxCON2レジスタのACKSTATから生成
#define u8GetSSP1EvtType() ((SSP1STAT & 0b00100101) | (SSP1CON2 & 0b01000000))

#ifdef SSP2STAT
// SSPxSTATのR/W、D/A、BF、SSPxCON2レジスタのACKSTATから生成
#define u8GetSSP2EvtType() ((SSP2STAT & 0b00100101) | (SSP2CON2 & 0b01000000))
#endif

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
/** I2Cのマスターのウェイト処理 */
static void vMasterWaitSSP1();

#ifdef SSP2STAT
/** I2Cのマスターのウェイト処理 */
static void vMasterWaitSSP2();
#endif

/** I2Cのマスターのウェイト処理 */
static void vSlaveWaitSSP1();

#ifdef SSP2STAT
/** I2Cのマスターのウェイト処理 */
static void vSlaveWaitSSP2();
#endif

/** コールバック関数のダミー */
static void vDmyCallback(uint8 u8BusNo, uint8 u8EvtType);


/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
// スタートフラグ
static bool bMstStartFlgSSP1 = false;
#ifdef SSP2STAT
static bool bMstStartFlgSSP2 = false;
#endif

// コールバック関数のポインタ
static void (*pvSSP1Func)(uint8 u8BusNo, uint8 u8EvtType) = vDmyCallback;
#ifdef SSP2STAT
static void (*pvSSP2Func)(uint8 u8BusNo, uint8 u8EvtType) = vDmyCallback;
#endif

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/
/*******************************************************************************
 *
 * NAME: I2C_vInitMasterSSP1
 *
 * DESCRIPTION:指定されたI2Cモードで初期化する
 *
 * PARAMETERS:    Name      RW  Usage
 * I2C_MasterMode eMode     R   マスターモード
 *      uint8     u8ClkDiv  R   クロック分割値
 *
 * RETURNS:
 *
 * NOTES:
 *   マスタ設定時のクロック分割値（u8ClkDiv）の例
 *     Baud Rate = (_XTAL_FREQ / (4 * (u8ClkDiv + 1)))
 *     100Kbps |  4MHz(_XTAL_FREQ= 4000000) | u8AddReg=0x09
 *     100Kbps |  8MHz(_XTAL_FREQ= 8000000) | u8AddReg=0x13
 *     100Kbps | 16MHz(_XTAL_FREQ=16000000) | u8AddReg=0x27
 *     100Kbps | 32MHz(_XTAL_FREQ=32000000) | u8AddReg=0x4F
 *     400Kbps |  8MHz(_XTAL_FREQ= 8000000) | u8AddReg=0x04
 *     400Kbps | 16MHz(_XTAL_FREQ=16000000) | u8AddReg=0x09
 *     400Kbps | 32MHz(_XTAL_FREQ=32000000) | u8AddReg=0x13
 * 
 ******************************************************************************/
extern void I2C_vInitMasterSSP1(enum I2C_MasterMode eMode, uint8 u8ClkDiv) {
    // スタートフラグ（SSP1）の初期化
    bMstStartFlgSSP1 = false;
    // SDA/SCLピンは7bitアドレスのI2Cで使用し、マスターモードとする
    if (eMode == I2C_MASTER_STD) {
        // 標準モードに設定する(100kHz)
        SSP1STAT= 0b10000000;
    } else {
        // 高速モードに設定する(400kHz)
        SSP1STAT= 0b00000000;
    }
    // Baud Rate=(_XTAL_FREQ / (4 * (SSPxADD + 1)));
    SSP1ADD = u8ClkDiv;
    // SDA/SCLピンは7bitアドレスのI2Cで使用し、マスターモードとする
    SSP1CON1 = 0b00101000;
    SSP1CON2 = 0b10000000;
}

/*******************************************************************************
 *
 * NAME: I2C_vInitMasterSSP2
 *
 * DESCRIPTION:指定されたI2Cモードで初期化する
 *
 * PARAMETERS:    Name      RW  Usage
 * I2C_MasterMode eMode     R   マスターモード
 *      uint8     u8ClkDiv  R   クロック分割値
 *
 * RETURNS:
 *
 * NOTES:
 *   マスタ設定時のクロック分割値（u8ClkDiv）の例
 *     Baud Rate = (_XTAL_FREQ / (4 * (u8ClkDiv + 1)))
 *     100Kbps |  4MHz(_XTAL_FREQ= 4000000) | u8AddReg=0x09
 *     100Kbps |  8MHz(_XTAL_FREQ= 8000000) | u8AddReg=0x13
 *     100Kbps | 16MHz(_XTAL_FREQ=16000000) | u8AddReg=0x27
 *     100Kbps | 32MHz(_XTAL_FREQ=32000000) | u8AddReg=0x4F
 *     400Kbps |  8MHz(_XTAL_FREQ= 8000000) | u8AddReg=0x04
 *     400Kbps | 16MHz(_XTAL_FREQ=16000000) | u8AddReg=0x09
 *     400Kbps | 32MHz(_XTAL_FREQ=32000000) | u8AddReg=0x13
 * 
 ******************************************************************************/
extern void I2C_vInitMasterSSP2(enum I2C_MasterMode eMode, uint8 u8ClkDiv) {
    // スタートフラグ（SSP2）の初期化
    bMstStartFlgSSP2 = false;
    // SDA/SCLピンは7bitアドレスのI2Cで使用し、マスターモードとする
    if (eMode == I2C_MASTER_STD) {
        // 標準モードに設定する(100kHz)
        SSP2STAT= 0b10000000;
    } else {
        // 高速モードに設定する(400kHz)
        SSP2STAT= 0b00000000;
    }
    // Baud Rate=(_XTAL_FREQ / (4 * (SSPxADD + 1)));
    SSP2ADD = u8ClkDiv;
    // SDA/SCLピンは7bitアドレスのI2Cで使用し、マスターモードとする
    SSP2CON1 = 0b00101000;
    SSP2CON2 = 0b10000000;
}

/*******************************************************************************
 *
 * NAME: I2C_vInitSlaveSSP1
 *
 * DESCRIPTION:SSP1を指定されたI2Cスレーブモードで初期化する
 *
 * PARAMETERS:   Name         RW  Usage
 *      uint8    u8Address    R   I2Cアドレス
 * I2C_SlaveMode eMode        R   スレーブモード
 *      void     (*pvCallback)(uint8 u8BusNo, uint8 u8EvtType)
 *
 * RETURNS:
 *
 * NOTES:
 * 
 ******************************************************************************/
extern void I2C_vInitSlaveSSP1(uint8 u8Address, enum I2C_SlaveMode eMode,
                            void (*pvCallback)(uint8 u8BusNo, uint8 u8EvtType)) {
    // コールバック関数の設定
    if (pvCallback != NULL) {
        pvSSP1Func = pvCallback;
    }
    // 通信速度の設定
    if (eMode == I2C_SLAVE_STD) {
        // 標準モードに設定する(100kHz)
        SSP1STAT = 0b10000000;
    } else {
        // 高速モードに設定する(400kHz)
        SSP1STAT = 0b00000000;
    }
    // SDA/SCLピンは7bitアドレスのI2Cで使用し、スレーブモードとする
    SSP1CON1 = 0b00110110;
    // 一括呼び出しの同報通知を有効にする
    // SCL制御(クロックストレッチ)を行う
    SSP1CON2 = 0b10000001;
    // バス衝突割り込みを有効化
    SSP1CON3 = 0b00000101;
    SSP1ADD  = u8Address << 1;  // マイアドレスの設定
    SSP1MSK  = 0b11111110;      // アドレス比較時のマスクデータ
    SSP1IF   = 0;               // SSP(I2C)割り込みフラグをクリアする
    BCL1IF   = 0;               // MSSP(I2C)バス衝突割り込みフラグをクリアする
    SSP1IE   = 1;               // SSP(I2C)割り込みを許可する
    BCL1IE   = 1;               // MSSP(I2C)バス衝突割り込みを許可する
}

/*******************************************************************************
 *
 * NAME: I2C_vInitSlaveSSP2
 *
 * DESCRIPTION:SSP2を指定されたI2Cスレーブモードで初期化する
 *
 * PARAMETERS:   Name         RW  Usage
 *      uint8    u8Address    R   I2Cアドレス
 * I2C_SlaveMode eMode        R   スレーブモード
 *      void     (*pvCallback)(uint8 u8BusNo, uint8 u8EvtType)
 *
 * RETURNS:
 *
 * NOTES:
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void I2C_vInitSlaveSSP2(uint8 u8Address, enum I2C_SlaveMode eMode,
                            void (*pvCallback)(uint8 u8BusNo, uint8 u8EvtType)) {
    // コールバック関数の設定
    if (pvCallback != NULL) {
        pvSSP2Func = pvCallback;
    }
    // 通信速度の設定
    if (eMode == I2C_SLAVE_STD) {
        // 標準モードに設定する(100kHz)
        SSP2STAT = 0b10000000;
    } else {
        // 高速モードに設定する(400kHz)
        SSP2STAT = 0b00000000;
    }
    // SDA/SCLピンは7bitアドレスのI2Cで使用し、スレーブモードとする
    SSP2CON1 = 0b00110110;
    // 一括呼び出しの同報通知を有効にする
    // SCL制御(クロックストレッチ)を行う
    SSP2CON2 = 0b10000001;
    // バス衝突割り込みを有効化
    SSP2CON3 = 0b00000101;
    SSP2ADD  = u8Address << 1;  // マイアドレスの設定
    SSP2MSK  = 0b11111110;      // アドレス比較時のマスクデータ
    SSP2IF   = 0;               // SSP(I2C)割り込みフラグをクリアする
    BCL2IF   = 0;               // MSSP(I2C)バス衝突割り込みフラグをクリアする
    SSP2IE   = 1;               // SSP(I2C)割り込みを許可する
    BCL2IE   = 1;               // MSSP(I2C)バス衝突割り込みを許可する
}
#endif

/*******************************************************************************
 *
 * NAME: I2C_u8MstStartSSP1
 *
 * DESCRIPTION:I2Cのマスターのスタート処理
 *
 * PARAMETERS:  Name            RW  Usage
 *       uint8  u8Address       R   I2Cアドレス
 *        bool  bReadFlg        R   読み込みフラグ
 *
 * RETURNS:
 *    uint8 ACKSTAT
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 I2C_u8MstStartSSP1(uint8 u8Address, bool bReadFlg) {
    // スタート状態の判定
    vMasterWaitSSP1();
    if (bMstStartFlgSSP1 == true) {
        SSP1CON2bits.RSEN = 1;
    } else {
        bMstStartFlgSSP1 = true;
        SSP1CON2bits.SEN = 1;
    }
    vMasterWaitSSP1();
    SSP1BUF = (u8Address << 1) | (bReadFlg & 0x01);
    vMasterWaitSSP1();
    return SSP1CON2bits.ACKSTAT;
}

/*******************************************************************************
 *
 * NAME: I2C_u8MstStartSSP2
 *
 * DESCRIPTION:I2Cのマスターのスタート処理
 *
 * PARAMETERS:  Name            RW  Usage
 *       uint8  u8Address       R   I2Cアドレス
 *        bool  bReadFlg        R   読み込みフラグ
 *
 * RETURNS:
 *    uint8 ACKSTAT
 *
 * NOTES:
 * None.
 ******************************************************************************/
#ifdef SSP2STAT
extern uint8 I2C_u8MstStartSSP2(uint8 u8Address, bool bReadFlg) {
    // スタート状態の判定
    vMasterWaitSSP2();
    if (bMstStartFlgSSP2 == true) {
        SSP2CON2bits.RSEN = 1;
    } else {
        bMstStartFlgSSP2 = true;
        SSP2CON2bits.SEN = 1;
    }
    vMasterWaitSSP2();
    SSP2BUF = (u8Address << 1) | (bReadFlg & 0x01);
    vMasterWaitSSP2();
    return SSP2CON2bits.ACKSTAT;
}
#endif

/*******************************************************************************
 *
 * NAME: I2C_vMstStopSSP1
 *
 * DESCRIPTION:I2Cのマスターのストップ処理
 *
 * PARAMETERS:  Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern void I2C_vMstStopSSP1() {
    vMasterWaitSSP1();
    bMstStartFlgSSP1 = false;
    SSP1CON2bits.PEN = 0x01;    
}

/*******************************************************************************
 *
 * NAME: I2C_vMstStopSSP2
 *
 * DESCRIPTION:I2Cのマスターのストップ処理
 *
 * PARAMETERS:  Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
#ifdef SSP2STAT
/** I2Cのマスターのストップ処理 */
extern void I2C_vMstStopSSP2() {
    vMasterWaitSSP2();
    bMstStartFlgSSP2 = false;
    SSP2CON2bits.PEN = 0x01;
}
#endif

/*******************************************************************************
 *
 * NAME: I2C_u8MstTxSSP1
 *
 * DESCRIPTION:I2Cのマスターの送信処理
 *
 * PARAMETERS:  Name            RW  Usage
 *       uint8  u8Data          R   送信データ
 *
 * RETURNS:
 *    uint8 ACKSTAT
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 I2C_u8MstTxSSP1(uint8 u8Data) {
    SSP1BUF = u8Data;
    vMasterWaitSSP1();
    return SSP1CON2bits.ACKSTAT;
}

/*******************************************************************************
 *
 * NAME: I2C_u8MstTxSSP2
 *
 * DESCRIPTION:I2Cのマスターの送信処理
 *
 * PARAMETERS:  Name            RW  Usage
 *       uint8  u8Data          R   送信データ
 *
 * RETURNS:
 *    uint8 ACKSTAT
 *
 * NOTES:
 * None.
 ******************************************************************************/
#ifdef SSP2STAT
/** I2Cのマスターの送信処理 */
extern uint8 I2C_u8MstTxSSP2(uint8 u8Data) {
    SSP2BUF = u8Data;
    vMasterWaitSSP2();
    return SSP2CON2bits.ACKSTAT;
}
#endif

/*******************************************************************************
 *
 * NAME: I2C_u8MstRxSSP1
 *
 * DESCRIPTION:I2Cのマスターの受信処理
 *
 * PARAMETERS:  Name          RW  Usage
 *        bool  bNackFlg      R   NACK返信フラグ
 *
 * RETURNS:
 *    uint8 受信データ
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 I2C_u8MstRxSSP1(bool bNackFlg) {
    // 受信許可
    vMasterWaitSSP1();
    SSP1CON2bits.RCEN = 1;
    // 受信データ取り込み
    vMasterWaitSSP1();
    uint8 u8Data = SSP1BUF;
    // ACK/NACK返信する
    vMasterWaitSSP1();
    SSP1CON2bits.ACKDT = bNackFlg & 0x01;
    SSP1CON2bits.ACKEN = 1;
    return u8Data;
}


/*******************************************************************************
 *
 * NAME: I2C_u8MstRxSSP2
 *
 * DESCRIPTION:I2Cのマスターの受信処理
 *
 * PARAMETERS:  Name            RW  Usage
 *        bool  bNackFlg        R   NACK返信フラグ
 *
 * RETURNS:
 *    uint8 受信データ
 *
 * NOTES:
 * None.
 ******************************************************************************/
#ifdef SSP2STAT
extern uint8 I2C_u8MstRxSSP2(bool bNackFlg) {
    // 受信許可
    vMasterWaitSSP2();
    SSP2CON2bits.RCEN = 1;
    // 受信データ取り込み
    vMasterWaitSSP2();
    uint8 u8Data = SSP2BUF;
    // ACK/NACK返信する
    vMasterWaitSSP2();
    SSP2CON2bits.ACKDT = bNackFlg & 0x01;
    SSP2CON2bits.ACKEN = 1;
    return u8Data;
}
#endif

/*******************************************************************************
 *
 * NAME: I2C_vSlaveIsrSSP1
 *
 * DESCRIPTION:I2Cスレーブの割り込み処理
 *
 * PARAMETERS:  Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern void I2C_vSlaveIsrSSP1() {
    //==========================================================================
    // SSP(I2C)割り込み発生時の処理
    //==========================================================================
    // SSPの割り込みの有無を判定
    if (SSP1IF == 1) {
        SSP1CON2bits.ACKDT = 0x00;
        pvSSP1Func(1, u8GetSSP1EvtType());
        SSP1IF = 0;             // 割込みフラグクリア
        SSP1CON1bits.CKP = 1;   // SCLラインを開放する(通信の再開)
    }
    // MSSP(I2C)バス衝突割り込発生時の処理
    if (BCL1IF == 1) {
        pvSSP1Func(1, I2C_SLV_EVT_BUS_ERROR);
        // フラグクリア
        BCL1IF = 0;
    }
}

/*******************************************************************************
 *
 * NAME: I2C_vSlaveIsrSSP2
 *
 * DESCRIPTION:I2Cスレーブの割り込み処理
 *
 * PARAMETERS:  Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
#ifdef SSP2STAT
extern void I2C_vSlaveIsrSSP2() {
    //==========================================================================
    // SSP(I2C)割り込み発生時の処理
    //==========================================================================
    // SSPの割り込みの有無を判定
    if (SSP2IF == 1) {
        SSP2CON2bits.ACKDT = 0x00;
        pvSSP2Func(2, u8GetSSP2EvtType());
        SSP2IF = 0;             // 割込みフラグクリア
        SSP2CON1bits.CKP = 1;   // SCLラインを開放する(通信の再開)
    }
    // MSSP(I2C)バス衝突割り込発生時の処理
    if (BCL2IF == 1) {
        pvSSP2Func(2, I2C_SLV_EVT_BUS_ERROR);
        // フラグクリア
        BCL2IF = 0;
    }
}
#endif

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: vMasterWaitSSP1
 *
 * DESCRIPTION:I2Cマスターのウェイト処理
 *
 * PARAMETERS:  Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void vMasterWaitSSP1() {
    while ((SSP1STAT & 0x04) || (SSP1CON2 & 0x1F));
}

/*******************************************************************************
 *
 * NAME: vMasterWaitSSP2
 *
 * DESCRIPTION:I2Cマスターのウェイト処理
 *
 * PARAMETERS:  Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
#ifdef SSP2STAT
static void vMasterWaitSSP2() {
    while ((SSP2STAT & 0x04) || (SSP2CON2 & 0x1F));
}
#endif

/*******************************************************************************
 *
 * NAME: vSlaveWaitSSP1
 *
 * DESCRIPTION:I2Cマスターのウェイト処理
 *
 * PARAMETERS:  Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void vSlaveWaitSSP1() {
    while((SSP1STATbits.BF) || (SSP1CON1bits.CKP));
}

/*******************************************************************************
 *
 * NAME: vSlaveWaitSSP2
 *
 * DESCRIPTION:I2Cマスターのウェイト処理
 *
 * PARAMETERS:  Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
#ifdef SSP2STAT
static void vSlaveWaitSSP2() {
    while((SSP2STATbits.BF) || (SSP2CON1bits.CKP));
}
#endif

/*******************************************************************************
 *
 * NAME: vDmyCallback
 *
 * DESCRIPTION:コールバック関数のダミー
 *
 * PARAMETERS:  Name            RW  Usage
 *       uint8  u8BusNo         R   SPP番号
 *       uint8  u8EvtType       R   イベントタイプ
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
static void vDmyCallback(uint8 u8BusNo, uint8 u8EvtType) {
    return;
}

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
