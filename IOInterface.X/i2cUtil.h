/*******************************************************************************
 *
 * MODULE :I2C utility functions header file
 *
 * CREATED:2019/03/10 14:50:00
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
#ifndef I2CUTIL_H
#define	I2CUTIL_H

#ifdef	__cplusplus
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
#define I2C_MASTER_1 8                  // 送信データバッファのサイズ
#define SND_DATA_LEN 8                  // 送信データバッファのサイズ
#define RCV_DATA_LEN 8                  // 受信データバッファのサイズ

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/    
//====================================================================
// I2Cマスターモード
//====================================================================
enum I2C_MasterMode {
    I2C_MASTER_STD          // I2Cスレーブ標準モード（100KHz）
   ,I2C_MASTER_HIGH         // I2Cスレーブ高速モード（400KHz）
};

//====================================================================
// I2Cスレーブモード
//====================================================================
enum I2C_SlaveMode {
    I2C_SLAVE_STD           // SSP1 I2Cスレーブ標準モード（100KHz）
   ,I2C_SLAVE_HIGH          // SSP1 I2Cスレーブ高速モード（400KHz）
};

//====================================================================
// I2Cマスタークロック分割値
//====================================================================
enum I2C_ClockDiv {
    I2C_CLK_DIV_STD_4MHZ   = 0x09,  // 100Kbps |  4MHz(_XTAL_FREQ= 4000000)
    I2C_CLK_DIV_STD_8MHZ   = 0x13,  // 100Kbps |  8MHz(_XTAL_FREQ= 8000000)
    I2C_CLK_DIV_STD_16MHZ  = 0x27,  // 100Kbps | 16MHz(_XTAL_FREQ=16000000)
    I2C_CLK_DIV_STD_32MHZ  = 0x4F,  // 100Kbps | 32MHz(_XTAL_FREQ=32000000)
    I2C_CLK_DIV_HIGH_8MHZ  = 0x04,  // 400Kbps |  8MHz(_XTAL_FREQ= 8000000)
    I2C_CLK_DIV_HIGH_16MHZ = 0x09,  // 400Kbps | 16MHz(_XTAL_FREQ=16000000)
    I2C_CLK_DIV_HIGH_32MHZ = 0x13,  // 400Kbps | 32MHz(_XTAL_FREQ=32000000)
};

//====================================================================
// I2Cスレーブイベント種別
// SSPxSTATのR/W、D/A、BF、SSPxCON2レジスタのACKSTATから生成
//====================================================================
enum I2C_SlaveEvent {
    I2C_SLV_EVT_WRITE_ADDR = 0b00000001,    // 書き込み要求アドレス受信
    I2C_SLV_EVT_WRITE_DATA = 0b00100001,    // 書き込み要求データ受信
    I2C_SLV_EVT_READ_ADDR  = 0b00000101,    // 読み出し要求アドレス受信
    I2C_SLV_EVT_READ_ACK   = 0b00100100,    // 読み出し要求ACK応答受信
    I2C_SLV_EVT_READ_NACK  = 0b01100100,    // 読み出し要求NACK応答受信
    I2C_SLV_EVT_BUS_ERROR  = 0b11111111     // バスエラー（バス衝突等）
};

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

/** I2Cのマスターモードで初期化する */
extern void I2C_vInitMasterSSP1(enum I2C_MasterMode eMode, uint8 u8ClkDiv);

#ifdef SSP2STAT
/** I2Cのマスターモードで初期化する */
extern void I2C_vInitMasterSSP2(enum I2C_MasterMode eMode, uint8 u8ClkDiv);
#endif

/** I2Cのスレーブモードで初期化する */
extern void I2C_vInitSlaveSSP1(uint8 u8Address,
                            enum I2C_SlaveMode eMode,
                            void (*pvCallback)(uint8 u8BusNo, uint8 u8EvtType));

#ifdef SSP2STAT
/** I2Cのスレーブモードで初期化する */
extern void I2C_vInitSlaveSSP2(uint8 u8Address,
                            enum I2C_SlaveMode eMode,
                            void (*pvCallback)(uint8 u8BusNo, uint8 u8EvtType));
#endif

/** I2Cのマスターのスタート処理 */
extern uint8 I2C_u8MstStartSSP1(uint8 u8Address, bool bReadFlg);

#ifdef SSP2STAT
/** I2Cのマスターのスタート処理 */
extern uint8 I2C_u8MstStartSSP2(uint8 u8Address, bool bReadFlg);
#endif

/** I2Cのマスターのストップ処理 */
extern void I2C_vMstStopSSP1();

#ifdef SSP2STAT
/** I2Cのマスターのストップ処理 */
extern void I2C_vMstStopSSP2();
#endif

/** I2Cのマスターの送信処理 */
extern uint8 I2C_u8MstTxSSP1(uint8 u8Data);

#ifdef SSP2STAT
/** I2Cのマスターの送信処理 */
extern uint8 I2C_u8MstTxSSP2(uint8 u8Data);
#endif

/** I2Cのマスターの受信処理 */
extern uint8 I2C_u8MstRxSSP1(bool bAckFlg);

#ifdef SSP2STAT
/** I2Cのマスターの受信処理 */
extern uint8 I2C_u8MstRxSSP2(bool bAckFlg);
#endif

/** SSP1 interrupt processing */
extern void I2C_vSlaveIsrSSP1();

#ifdef SSP2STAT
/** SSP2 interrupt processing */
extern void I2C_vSlaveIsrSSP2();
#endif

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

#ifdef	__cplusplus
}
#endif

#endif	/* I2CUTIL_H */

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
