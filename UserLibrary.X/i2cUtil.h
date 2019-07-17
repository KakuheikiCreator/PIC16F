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
#define I2C_MASTER_1 8                  // ���M�f�[�^�o�b�t�@�̃T�C�Y
#define SND_DATA_LEN 8                  // ���M�f�[�^�o�b�t�@�̃T�C�Y
#define RCV_DATA_LEN 8                  // ��M�f�[�^�o�b�t�@�̃T�C�Y

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/    
//====================================================================
// I2C�}�X�^�[���[�h
//====================================================================
enum I2C_MasterMode {
    I2C_MASTER_STD          // I2C�X���[�u�W�����[�h�i100KHz�j
   ,I2C_MASTER_HIGH         // I2C�X���[�u�������[�h�i400KHz�j
};

//====================================================================
// I2C�X���[�u���[�h
//====================================================================
enum I2C_SlaveMode {
    I2C_SLAVE_STD           // SSP1 I2C�X���[�u�W�����[�h�i100KHz�j
   ,I2C_SLAVE_HIGH          // SSP1 I2C�X���[�u�������[�h�i400KHz�j
};

//====================================================================
// I2C�}�X�^�[�N���b�N�����l
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
// I2C�X���[�u�C�x���g���
// SSPxSTAT��R/W�AD/A�ABF�ASSPxCON2���W�X�^��ACKSTAT���琶��
//====================================================================
enum I2C_SlaveEvent {
    I2C_SLV_EVT_WRITE_ADDR = 0b00000001,    // �������ݗv���A�h���X��M
    I2C_SLV_EVT_WRITE_DATA = 0b00100001,    // �������ݗv���f�[�^��M
    I2C_SLV_EVT_READ_ADDR  = 0b00000101,    // �ǂݏo���v���A�h���X��M
    I2C_SLV_EVT_READ_ACK   = 0b00100100,    // �ǂݏo���v��ACK������M
    I2C_SLV_EVT_READ_NACK  = 0b01100100,    // �ǂݏo���v��NACK������M
    I2C_SLV_EVT_BUS_ERROR  = 0b11111111     // �o�X�G���[�i�o�X�Փ˓��j
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

/** I2C�̃}�X�^�[���[�h�ŏ��������� */
extern void I2C_vInitMasterSSP1(enum I2C_MasterMode eMode, uint8 u8ClkDiv);

#ifdef SSP2STAT
/** I2C�̃}�X�^�[���[�h�ŏ��������� */
extern void I2C_vInitMasterSSP2(enum I2C_MasterMode eMode, uint8 u8ClkDiv);
#endif

/** I2C�̃X���[�u���[�h�ŏ��������� */
extern void I2C_vInitSlaveSSP1(uint8 u8Address,
                            enum I2C_SlaveMode eMode,
                            void (*pvCallback)(uint8 u8BusNo, uint8 u8EvtType));

#ifdef SSP2STAT
/** I2C�̃X���[�u���[�h�ŏ��������� */
extern void I2C_vInitSlaveSSP2(uint8 u8Address,
                            enum I2C_SlaveMode eMode,
                            void (*pvCallback)(uint8 u8BusNo, uint8 u8EvtType));
#endif

/** I2C�̃}�X�^�[�̃X�^�[�g���� */
extern uint8 I2C_u8MstStartSSP1(uint8 u8Address, bool bReadFlg);

#ifdef SSP2STAT
/** I2C�̃}�X�^�[�̃X�^�[�g���� */
extern uint8 I2C_u8MstStartSSP2(uint8 u8Address, bool bReadFlg);
#endif

/** I2C�̃}�X�^�[�̃X�g�b�v���� */
extern void I2C_vMstStopSSP1();

#ifdef SSP2STAT
/** I2C�̃}�X�^�[�̃X�g�b�v���� */
extern void I2C_vMstStopSSP2();
#endif

/** I2C�̃}�X�^�[�̑��M���� */
extern uint8 I2C_u8MstTxSSP1(uint8 u8Data);

#ifdef SSP2STAT
/** I2C�̃}�X�^�[�̑��M���� */
extern uint8 I2C_u8MstTxSSP2(uint8 u8Data);
#endif

/** I2C�̃}�X�^�[�̎�M���� */
extern uint8 I2C_u8MstRxSSP1(bool bAckFlg);

#ifdef SSP2STAT
/** I2C�̃}�X�^�[�̎�M���� */
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
