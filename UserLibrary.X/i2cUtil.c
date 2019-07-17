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
#include "i2cUtil.h"        // I2C�֐����C�u�����[�p
#include "libcom.h"         // ���ʒ�`
#include "keypad.h"         // �L�[�p�b�h

/******************************************************************************/
/***       Macro Definitions                                                ***/
/******************************************************************************/

//==========================================================
// I2C�o�X���̒�`
//==========================================================
// SSPxSTAT��R/W�AD/A�ABF�ASSPxCON2���W�X�^��ACKSTAT���琶��
#define u8GetSSP1EvtType() ((SSP1STAT & 0b00100101) | (SSP1CON2 & 0b01000000))

#ifdef SSP2STAT
// SSPxSTAT��R/W�AD/A�ABF�ASSPxCON2���W�X�^��ACKSTAT���琶��
#define u8GetSSP2EvtType() ((SSP2STAT & 0b00100101) | (SSP2CON2 & 0b01000000))
#endif

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
/** I2C�̃}�X�^�[�̃E�F�C�g���� */
static void vMasterWaitSSP1();

#ifdef SSP2STAT
/** I2C�̃}�X�^�[�̃E�F�C�g���� */
static void vMasterWaitSSP2();
#endif

/** I2C�̃}�X�^�[�̃E�F�C�g���� */
static void vSlaveWaitSSP1();

#ifdef SSP2STAT
/** I2C�̃}�X�^�[�̃E�F�C�g���� */
static void vSlaveWaitSSP2();
#endif

/** �R�[���o�b�N�֐��̃_�~�[ */
static void vDmyCallback(uint8 u8BusNo, uint8 u8EvtType);


/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
// �X�^�[�g�t���O
static bool bMstStartFlgSSP1 = false;
#ifdef SSP2STAT
static bool bMstStartFlgSSP2 = false;
#endif

// �R�[���o�b�N�֐��̃|�C���^
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
 * DESCRIPTION:�w�肳�ꂽI2C���[�h�ŏ���������
 *
 * PARAMETERS:    Name      RW  Usage
 * I2C_MasterMode eMode     R   �}�X�^�[���[�h
 *      uint8     u8ClkDiv  R   �N���b�N�����l
 *
 * RETURNS:
 *
 * NOTES:
 *   �}�X�^�ݒ莞�̃N���b�N�����l�iu8ClkDiv�j�̗�
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
    // �X�^�[�g�t���O�iSSP1�j�̏�����
    bMstStartFlgSSP1 = false;
    // SDA/SCL�s����7bit�A�h���X��I2C�Ŏg�p���A�}�X�^�[���[�h�Ƃ���
    if (eMode == I2C_MASTER_STD) {
        // �W�����[�h�ɐݒ肷��(100kHz)
        SSP1STAT= 0b10000000;
    } else {
        // �������[�h�ɐݒ肷��(400kHz)
        SSP1STAT= 0b00000000;
    }
    // Baud Rate=(_XTAL_FREQ / (4 * (SSPxADD + 1)));
    SSP1ADD = u8ClkDiv;
    // SDA/SCL�s����7bit�A�h���X��I2C�Ŏg�p���A�}�X�^�[���[�h�Ƃ���
    SSP1CON1 = 0b00101000;
    SSP1CON2 = 0b10000000;
}

/*******************************************************************************
 *
 * NAME: I2C_vInitMasterSSP2
 *
 * DESCRIPTION:�w�肳�ꂽI2C���[�h�ŏ���������
 *
 * PARAMETERS:    Name      RW  Usage
 * I2C_MasterMode eMode     R   �}�X�^�[���[�h
 *      uint8     u8ClkDiv  R   �N���b�N�����l
 *
 * RETURNS:
 *
 * NOTES:
 *   �}�X�^�ݒ莞�̃N���b�N�����l�iu8ClkDiv�j�̗�
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
    // �X�^�[�g�t���O�iSSP2�j�̏�����
    bMstStartFlgSSP2 = false;
    // SDA/SCL�s����7bit�A�h���X��I2C�Ŏg�p���A�}�X�^�[���[�h�Ƃ���
    if (eMode == I2C_MASTER_STD) {
        // �W�����[�h�ɐݒ肷��(100kHz)
        SSP2STAT= 0b10000000;
    } else {
        // �������[�h�ɐݒ肷��(400kHz)
        SSP2STAT= 0b00000000;
    }
    // Baud Rate=(_XTAL_FREQ / (4 * (SSPxADD + 1)));
    SSP2ADD = u8ClkDiv;
    // SDA/SCL�s����7bit�A�h���X��I2C�Ŏg�p���A�}�X�^�[���[�h�Ƃ���
    SSP2CON1 = 0b00101000;
    SSP2CON2 = 0b10000000;
}

/*******************************************************************************
 *
 * NAME: I2C_vInitSlaveSSP1
 *
 * DESCRIPTION:SSP1���w�肳�ꂽI2C�X���[�u���[�h�ŏ���������
 *
 * PARAMETERS:   Name         RW  Usage
 *      uint8    u8Address    R   I2C�A�h���X
 * I2C_SlaveMode eMode        R   �X���[�u���[�h
 *      void     (*pvCallback)(uint8 u8BusNo, uint8 u8EvtType)
 *
 * RETURNS:
 *
 * NOTES:
 * 
 ******************************************************************************/
extern void I2C_vInitSlaveSSP1(uint8 u8Address, enum I2C_SlaveMode eMode,
                            void (*pvCallback)(uint8 u8BusNo, uint8 u8EvtType)) {
    // �R�[���o�b�N�֐��̐ݒ�
    if (pvCallback != NULL) {
        pvSSP1Func = pvCallback;
    }
    // �ʐM���x�̐ݒ�
    if (eMode == I2C_SLAVE_STD) {
        // �W�����[�h�ɐݒ肷��(100kHz)
        SSP1STAT = 0b10000000;
    } else {
        // �������[�h�ɐݒ肷��(400kHz)
        SSP1STAT = 0b00000000;
    }
    // SDA/SCL�s����7bit�A�h���X��I2C�Ŏg�p���A�X���[�u���[�h�Ƃ���
    SSP1CON1 = 0b00110110;
    // �ꊇ�Ăяo���̓���ʒm��L���ɂ���
    // SCL����(�N���b�N�X�g���b�`)���s��
    SSP1CON2 = 0b10000001;
    // �o�X�Փˊ��荞�݂�L����
    SSP1CON3 = 0b00000101;
    SSP1ADD  = u8Address << 1;  // �}�C�A�h���X�̐ݒ�
    SSP1MSK  = 0b11111110;      // �A�h���X��r���̃}�X�N�f�[�^
    SSP1IF   = 0;               // SSP(I2C)���荞�݃t���O���N���A����
    BCL1IF   = 0;               // MSSP(I2C)�o�X�Փˊ��荞�݃t���O���N���A����
    SSP1IE   = 1;               // SSP(I2C)���荞�݂�������
    BCL1IE   = 1;               // MSSP(I2C)�o�X�Փˊ��荞�݂�������
}

/*******************************************************************************
 *
 * NAME: I2C_vInitSlaveSSP2
 *
 * DESCRIPTION:SSP2���w�肳�ꂽI2C�X���[�u���[�h�ŏ���������
 *
 * PARAMETERS:   Name         RW  Usage
 *      uint8    u8Address    R   I2C�A�h���X
 * I2C_SlaveMode eMode        R   �X���[�u���[�h
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
    // �R�[���o�b�N�֐��̐ݒ�
    if (pvCallback != NULL) {
        pvSSP2Func = pvCallback;
    }
    // �ʐM���x�̐ݒ�
    if (eMode == I2C_SLAVE_STD) {
        // �W�����[�h�ɐݒ肷��(100kHz)
        SSP2STAT = 0b10000000;
    } else {
        // �������[�h�ɐݒ肷��(400kHz)
        SSP2STAT = 0b00000000;
    }
    // SDA/SCL�s����7bit�A�h���X��I2C�Ŏg�p���A�X���[�u���[�h�Ƃ���
    SSP2CON1 = 0b00110110;
    // �ꊇ�Ăяo���̓���ʒm��L���ɂ���
    // SCL����(�N���b�N�X�g���b�`)���s��
    SSP2CON2 = 0b10000001;
    // �o�X�Փˊ��荞�݂�L����
    SSP2CON3 = 0b00000101;
    SSP2ADD  = u8Address << 1;  // �}�C�A�h���X�̐ݒ�
    SSP2MSK  = 0b11111110;      // �A�h���X��r���̃}�X�N�f�[�^
    SSP2IF   = 0;               // SSP(I2C)���荞�݃t���O���N���A����
    BCL2IF   = 0;               // MSSP(I2C)�o�X�Փˊ��荞�݃t���O���N���A����
    SSP2IE   = 1;               // SSP(I2C)���荞�݂�������
    BCL2IE   = 1;               // MSSP(I2C)�o�X�Փˊ��荞�݂�������
}
#endif

/*******************************************************************************
 *
 * NAME: I2C_u8MstStartSSP1
 *
 * DESCRIPTION:I2C�̃}�X�^�[�̃X�^�[�g����
 *
 * PARAMETERS:  Name            RW  Usage
 *       uint8  u8Address       R   I2C�A�h���X
 *        bool  bReadFlg        R   �ǂݍ��݃t���O
 *
 * RETURNS:
 *    uint8 ACKSTAT
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 I2C_u8MstStartSSP1(uint8 u8Address, bool bReadFlg) {
    // �X�^�[�g��Ԃ̔���
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
 * DESCRIPTION:I2C�̃}�X�^�[�̃X�^�[�g����
 *
 * PARAMETERS:  Name            RW  Usage
 *       uint8  u8Address       R   I2C�A�h���X
 *        bool  bReadFlg        R   �ǂݍ��݃t���O
 *
 * RETURNS:
 *    uint8 ACKSTAT
 *
 * NOTES:
 * None.
 ******************************************************************************/
#ifdef SSP2STAT
extern uint8 I2C_u8MstStartSSP2(uint8 u8Address, bool bReadFlg) {
    // �X�^�[�g��Ԃ̔���
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
 * DESCRIPTION:I2C�̃}�X�^�[�̃X�g�b�v����
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
 * DESCRIPTION:I2C�̃}�X�^�[�̃X�g�b�v����
 *
 * PARAMETERS:  Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
#ifdef SSP2STAT
/** I2C�̃}�X�^�[�̃X�g�b�v���� */
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
 * DESCRIPTION:I2C�̃}�X�^�[�̑��M����
 *
 * PARAMETERS:  Name            RW  Usage
 *       uint8  u8Data          R   ���M�f�[�^
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
 * DESCRIPTION:I2C�̃}�X�^�[�̑��M����
 *
 * PARAMETERS:  Name            RW  Usage
 *       uint8  u8Data          R   ���M�f�[�^
 *
 * RETURNS:
 *    uint8 ACKSTAT
 *
 * NOTES:
 * None.
 ******************************************************************************/
#ifdef SSP2STAT
/** I2C�̃}�X�^�[�̑��M���� */
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
 * DESCRIPTION:I2C�̃}�X�^�[�̎�M����
 *
 * PARAMETERS:  Name          RW  Usage
 *        bool  bNackFlg      R   NACK�ԐM�t���O
 *
 * RETURNS:
 *    uint8 ��M�f�[�^
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 I2C_u8MstRxSSP1(bool bNackFlg) {
    // ��M����
    vMasterWaitSSP1();
    SSP1CON2bits.RCEN = 1;
    // ��M�f�[�^��荞��
    vMasterWaitSSP1();
    uint8 u8Data = SSP1BUF;
    // ACK/NACK�ԐM����
    vMasterWaitSSP1();
    SSP1CON2bits.ACKDT = bNackFlg & 0x01;
    SSP1CON2bits.ACKEN = 1;
    return u8Data;
}


/*******************************************************************************
 *
 * NAME: I2C_u8MstRxSSP2
 *
 * DESCRIPTION:I2C�̃}�X�^�[�̎�M����
 *
 * PARAMETERS:  Name            RW  Usage
 *        bool  bNackFlg        R   NACK�ԐM�t���O
 *
 * RETURNS:
 *    uint8 ��M�f�[�^
 *
 * NOTES:
 * None.
 ******************************************************************************/
#ifdef SSP2STAT
extern uint8 I2C_u8MstRxSSP2(bool bNackFlg) {
    // ��M����
    vMasterWaitSSP2();
    SSP2CON2bits.RCEN = 1;
    // ��M�f�[�^��荞��
    vMasterWaitSSP2();
    uint8 u8Data = SSP2BUF;
    // ACK/NACK�ԐM����
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
 * DESCRIPTION:I2C�X���[�u�̊��荞�ݏ���
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
    // SSP(I2C)���荞�ݔ������̏���
    //==========================================================================
    // SSP�̊��荞�݂̗L���𔻒�
    if (SSP1IF == 1) {
        SSP1CON2bits.ACKDT = 0x00;
        pvSSP1Func(1, u8GetSSP1EvtType());
        SSP1IF = 0;             // �����݃t���O�N���A
        SSP1CON1bits.CKP = 1;   // SCL���C�����J������(�ʐM�̍ĊJ)
    }
    // MSSP(I2C)�o�X�Փˊ��荞�������̏���
    if (BCL1IF == 1) {
        pvSSP1Func(1, I2C_SLV_EVT_BUS_ERROR);
        // �t���O�N���A
        BCL1IF = 0;
    }
}

/*******************************************************************************
 *
 * NAME: I2C_vSlaveIsrSSP2
 *
 * DESCRIPTION:I2C�X���[�u�̊��荞�ݏ���
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
    // SSP(I2C)���荞�ݔ������̏���
    //==========================================================================
    // SSP�̊��荞�݂̗L���𔻒�
    if (SSP2IF == 1) {
        SSP2CON2bits.ACKDT = 0x00;
        pvSSP2Func(2, u8GetSSP2EvtType());
        SSP2IF = 0;             // �����݃t���O�N���A
        SSP2CON1bits.CKP = 1;   // SCL���C�����J������(�ʐM�̍ĊJ)
    }
    // MSSP(I2C)�o�X�Փˊ��荞�������̏���
    if (BCL2IF == 1) {
        pvSSP2Func(2, I2C_SLV_EVT_BUS_ERROR);
        // �t���O�N���A
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
 * DESCRIPTION:I2C�}�X�^�[�̃E�F�C�g����
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
 * DESCRIPTION:I2C�}�X�^�[�̃E�F�C�g����
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
 * DESCRIPTION:I2C�}�X�^�[�̃E�F�C�g����
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
 * DESCRIPTION:I2C�}�X�^�[�̃E�F�C�g����
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
 * DESCRIPTION:�R�[���o�b�N�֐��̃_�~�[
 *
 * PARAMETERS:  Name            RW  Usage
 *       uint8  u8BusNo         R   SPP�ԍ�
 *       uint8  u8EvtType       R   �C�x���g�^�C�v
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
