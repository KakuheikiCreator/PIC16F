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
#include "setting.h"            // �ݒ�l
#include "libcom.h"             // ���ʒ�`
#include "i2cUtil.h"            // I2C�֐����C�u�����p
#include "keypad.h"             // �L�[�p�b�h
#include "st7032.h"             // LCD

/******************************************************************************/
/***       Macro Definitions                                                ***/
/******************************************************************************/
// �R���t�B�O�P
#pragma config FOSC  = INTOSC   // �����N���b�N(INTOSC:8MHz)�g�p����
#pragma config WDTE  = OFF      // �E�I�b�`�h�b�O�^�C�}�[����(OFF)
#pragma config PWRTE = ON       // �d��ON����64ms��Ƀv���O�������J�n����(ON)
#pragma config MCLRE = OFF      // �O�����Z�b�g�M���͎g�p�����Ƀf�W�^������(RA3)�s���Ƃ���(OFF)
#pragma config CP    = OFF      // �v���O�����������[��ی삵�Ȃ�(OFF)
#pragma config CPD   = OFF      // EEPROM�̃f�[�^�ǂݏo�����iOFF�j
#pragma config BOREN = ON       // �d���d���~��(BORV�ݒ�ȉ�)�펞�Ď��@�\ON(ON)
#pragma config CLKOUTEN = OFF   // CLKOUT�s����RA4�s���Ŏg�p����(OFF)
#pragma config IESO  = OFF      // ��������O���N���b�N�ւ̐ؑւ��ł̋N���͂Ȃ�(OFF)
#pragma config FCMEN = OFF      // ���N���b�N�Ď����Ȃ�(OFF)

// �R���t�B�O�Q
#pragma config WRT    = OFF     // Flash�������[��ی삵�Ȃ�(OFF)
#pragma config PLLEN  = OFF     // ����N���b�N��32MHz�ł͓��삳���Ȃ�(OFF)
#pragma config STVREN = ON      // �X�^�b�N���I�[�o�t���[��A���_�[�t���[�����烊�Z�b�g������(ON)
#pragma config BORV   = HI      // �d���d���~���펞�Ď��d��(2.7V)�ݒ�(HI)
#pragma config LVP    = OFF     // ��d���v���O���~���O�@�\�g�p���Ȃ�(OFF)

// 16�i�������ւ̕ϊ�
#define hexToChar(val) HEX_LIST[val & 0x0F]

// I2C 7bit address
#define	I2C_ADDR    (8)

// �s���F�d���֌W
#define PIN_POWER           RB0
#define PIN_BACK_LIGHT      RB3

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
/**
 * ���̓��[�h
 */
typedef enum {
    INPUT_MODE_NONE         = 0x00, // ���͖���
    INPUT_MODE_CURRENT      = 0x01, // ���ݒl���[�h
    INPUT_MODE_FINAL        = 0x02, // �ŏI�l���[�h
    INPUT_MODE_BUFFERING    = 0x03  // �o�b�t�@�����O
} teInputMode;


/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
// �^�C�}�[���荞�ݏ���
static void timer_vInterrupt();
// I2C���荞�݂̃R�[���o�b�N�֐�
static void ssp1_vCallback(uint8 u8BusNo, uint8 u8EvtType);
// I2C Test 01:�������݁i�P�o�C�g�P�ʁj
static void ssp2_vI2CTest01();
// I2C Test 02:�������݁i�����o�C�g�P�ʁj
static void ssp2_vI2CTest02();
// I2C Test 03:�ǂݍ��݁i�P�o�C�g�P�ʁj
static void ssp2_vI2CTest03();
// I2C Test 03:�ǂݍ��݁i�����o�C�g�P�ʁj
static void ssp2_vI2CTest04();
// LCD Test 01:�����`��
static void ssp2_vLCDTest01();
// LCD Test 02:�J�[�\���ړ�
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
// �����z��
static const char HEX_LIST[] = "0123456789ABCDEF";
static const char KEY_LIST[] = "123A456B789C*0#D";
// I2C�f�[�^
static uint8 u8RxData;
// ���̓��[�h
static teInputMode eInputMode;
// �L�[�l
static uint8 u8KeyValue;

/*******************************************************************************
 *
 * NAME: main
 *
 * DESCRIPTION:�又��
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
    // �^�C�}�[����уs���ݒ�
    //==========================================================================
    OSCCON = 0b01111010;    // �����N���b�N��16�l�g���Ƃ���
    ANSELA = 0b00000000;    // �A�i���O�g�p���Ȃ�(���ׂăf�W�^��I/O�Ɋ����Ă�)
    ANSELB = 0b00000000;    // �A�i���O�g�p���Ȃ�(���ׂăf�W�^��I/O�Ɋ����Ă�)
    TRISA  = 0b00011100;    // �S�Ă��o�̓��[�h�Ƃ���
    TRISB  = 0b10110110;    // �s����RB1(SDA)/RB4(SCL)�̂ݓ���
    PORTA  = 0b00000000;    // �o�̓s���̏�����(�S��LOW�ɂ���)
    PORTB  = 0b00000000;    // �o�̓s���̏�����(�S��LOW�ɂ���)

    //==========================================================================
    // �^�C�}�[�ݒ�
    // FOSC = 16MHz / 4
    // �J�E���g�A�b�v�T�C�N�� = FOSC / �v���X�P�[�� = (16MHz / 4) / 128
    // 1�b�Ԃ̊��荞�݉� = �J�E���g�A�b�v�T�C�N�� / 256 = 128��
    //==========================================================================
    OPTION_REG = 0b00000110;    // �����N���b�N��TIMER0���g�p�A�v���X�P�[�� 1:128
    TMR0   = 0;                 // �^�C�}�[0�̏�����
    TMR0IF = 0;                 // �^�C�}�[0�����t���O(T0IF)��0�ɂ���
    TMR0IE = 1;                 // �^�C�}�[0������(T0IE)��������
    
    //==========================================================================
    // I2C��������
    //==========================================================================
    // SSP1:�X���[�u���[�h�ł̏�����
    I2C_vInitSlaveSSP1(I2C_ADDR, I2C_SLAVE_STD, ssp1_vCallback);
    // SSP2:�}�X�^�[���[�h�ŏ�����
    I2C_vInitMasterSSP2(I2C_MASTER_STD, I2C_CLK_DIV_STD_8MHZ);
    
    //==========================================================================
    // �L�[�p�b�h�ݒ�
    //==========================================================================
    tsKEYPAD_status keypadSts;
    // �e��̃s��
    keypadSts.u16PinCols[0] = ID_PORTA | 0b00000100;
    keypadSts.u16PinCols[1] = ID_PORTA | 0b00001000;
    keypadSts.u16PinCols[2] = ID_PORTA | 0b00010000;
    keypadSts.u16PinCols[3] = ID_PORTB | 0b10000000;
    // �e�s�̃s��
    keypadSts.u16PinRows[0] = ID_PORTA | 0b00000010;
    keypadSts.u16PinRows[1] = ID_PORTA | 0b00000001;
    keypadSts.u16PinRows[2] = ID_PORTA | 0b10000000;
    keypadSts.u16PinRows[3] = ID_PORTA | 0b01000000;
    
    // �Ώۂ̃L�[�p�b�h������������
    KEYPAD_vInit(&keypadSts);
    
    //==========================================================================
    // ���荞�݂̗L����
    //==========================================================================
    PEIE = 1;   // ���ӑ��u���荞�ݗL��
    GIE  = 1;   // �S�����ݏ�����������

    //==========================================================================
    // LCD�̏���������
    //==========================================================================
    // LCD�s���ݒ菉����
    PIN_POWER      = ON;
    PIN_BACK_LIGHT = OFF;
    __delay_ms(40);
    // LCD����������
    ST7032_vInitSSP2();

    //==========================================================================
    // �e�X�g�P�[�X����
    //==========================================================================
    // ���C�����[�v
    while(true) {
        // I2C Test 01:1�o�C�g�P�ʂ̏�������
//        ssp2_vI2CTest01();
        // I2C Test 02:�����o�C�g�P�ʂ̏�������
//        ssp2_vI2CTest02();
        // I2C Test 03:1�o�C�g�P�ʂ̓ǂݍ���
//        ssp2_vI2CTest03();
        // I2C Test 04:�����o�C�g�P�ʂ̓ǂݍ���
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
 * DESCRIPTION:���荞�ݏ���
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
    // �^�C�}�[���荞�ݔ������̏���
    //==========================================================================
    // �^�C�}�[���荞�ݏ���
    timer_vInterrupt();
    
    //==========================================================================
    // SSP(I2C)���荞�ݔ������̏���
    //==========================================================================
    I2C_vSlaveIsrSSP1();
}

/*******************************************************************************
 *
 * NAME: timer_vInterrupt
 *
 * DESCRIPTION:�^�C�}�[���荞�ݏ���
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void timer_vInterrupt() {
    // �^�C�}�[�O�̊��荞�݂̗L���𔻒�
    if (TMR0IF != 1) {
        return;
    }
    // �^�C�}�[�J�E���^
    TMR0   = 0;                 // �^�C�}�[0�̃J�E���^������
    TMR0IF = 0;                 // �^�C�}�[0�����t���O�����Z�b�g
    // ���̓��[�h����
    switch (eInputMode) {
        // ���͖���
        case INPUT_MODE_NONE:
            break;
        // ���ݒl���[�h
        case INPUT_MODE_CURRENT:
            u8KeyValue = KEYPAD_u8Read();
            break;
        // �ŏI�l���[�h
        case INPUT_MODE_FINAL:
            // �o�b�t�@�X�V
            KEYPAD_bUpdateBuffer();
            break;
        // �o�b�t�@�����O���[�h
        case INPUT_MODE_BUFFERING:
            // �o�b�t�@�X�V
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
 * DESCRIPTION:I2C���荞�݂̃R�[���o�b�N�֐�
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
            // �������ݗv���A�h���X��M
            // �A�h���X�f�[�^����ǂ݂���
            u8Data = SSP1BUF;
            break;
        case I2C_SLV_EVT_WRITE_DATA:
            // �������ݗv���f�[�^��M
            // �f�[�^��Ǎ���(ACK��PIC�������I�ɕԂ�)
            u8RxData = SSP1BUF;
            break;
        case I2C_SLV_EVT_READ_ADDR:
            // �ǂݏo���v���A�h���X��M
            // �A�h���X�f�[�^����ǂ݂���
            u8Data = SSP1BUF;
            // �f�[�^��ԐM
            SSP1BUF = u8RxData;
            u8RxData++;
            break;
        case I2C_SLV_EVT_READ_ACK:
            // �ǂݏo���v��ACK������M
            // ��M�f�[�^�𑗐M�o�b�t�@�ɃZ�b�g
            SSP1BUF = u8RxData;
            u8RxData++;
            break;
        case I2C_SLV_EVT_READ_NACK:
            // �ǂݏo���v��NACK������M
            // �ǂݏo�������ɂ��A�������Ȃ�
            break;
        default:
            break;
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vI2CTest01
 *
 * DESCRIPTION:I2C�e�X�g�P�[�X
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
    // �L�[�ǂݍ��݃e�X�g
    //==========================================================================
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // ���b�Z�[�W
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:I2C 01     ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Tx 0x  ->Rx 0x  ");
    
    //==========================================================================
    // 1�o�C�g�P�ʂ̑��M�e�X�g
    //==========================================================================
    // �f�[�^�N���A
    u8RxData = 0xFF;
    uint8 u8TxData;
    for (u8TxData = 0; u8TxData < 255; u8TxData++) {
        // ���M�f�[�^�\��
        ST7032_bSetCursorSSP2(1, 5);
        ST7032_vWriteCharSSP2(HEX_LIST[u8TxData / 16]);
        ST7032_vWriteCharSSP2(HEX_LIST[u8TxData % 16]);
        // �X�^�[�g�R���f�B�V�����̑��M
        I2C_u8MstStartSSP2(I2C_ADDR, false);
        I2C_u8MstTxSSP2(u8TxData);
        I2C_vMstStopSSP2();
        // ��M�f�[�^�m�F
        ST7032_bSetCursorSSP2(1, 14);
        if (u8RxData == u8TxData) {
            ST7032_vWriteCharSSP2(HEX_LIST[u8RxData / 16]);
            ST7032_vWriteCharSSP2(HEX_LIST[u8RxData % 16]);
        } else {
            ST7032_vWriteStringSSP2("XX");
        }
        // �E�F�C�g
        __delay_ms(20);
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vI2CTest02
 *
 * DESCRIPTION:I2C�e�X�g�P�[�X�A�����o�C�g�P�ʂ̏������݃e�X�g
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
    // �L�[�ǂݍ��݃e�X�g
    //==========================================================================
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // ���b�Z�[�W
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:I2C 02     ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Tx 0x00->0xFF   ");
    // �E�F�C�g
    __delay_ms(1000);
    
    //==========================================================================
    // �����o�C�g�P�ʂ̑��M�e�X�g
    //==========================================================================
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(I2C_ADDR, false);
    // �f�[�^�N���A
    u8RxData = 0xFF;
    uint8 u8TxData;
    for (u8TxData = 0; u8TxData < 255; u8TxData++) {
        I2C_u8MstTxSSP2(u8TxData);
        // ��M�f�[�^�m�F
        if (u8RxData != u8TxData) {
            break;
        }
    }
    I2C_vMstStopSSP2();
    // ���ʕ\��
    ST7032_bSetCursorSSP2(1, 0);
    if (u8TxData == 255) {
        ST7032_vWriteStringSSP2("Tx 0x00->0xFF OK");
    } else {
        ST7032_vWriteStringSSP2("Tx 0x00->0xFF NG");
    }
    // �E�F�C�g
    __delay_ms(1000);
}

/*******************************************************************************
 *
 * NAME: ssp2_vI2CTest03
 *
 * DESCRIPTION:I2C�e�X�g�P�[�X�A�����o�C�g�P�ʂ̏������݃e�X�g
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
    // �L�[�ǂݍ��݃e�X�g
    //==========================================================================
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // ���b�Z�[�W
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:I2C 03     ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Rx 0x00->0xFF   ");
    
    //==========================================================================
    // 1�o�C�g�P�ʂ̎�M�e�X�g
    //==========================================================================
    // �f�[�^�N���A
    u8RxData = 0x00;
    uint8 u8Data;
    uint8 u8Cnt;
    for (u8Cnt = 0; u8Cnt < 255; u8Cnt++) {
        // 1�o�C�g�ǂݍ���
        I2C_u8MstStartSSP2(I2C_ADDR, true);
        u8Data = I2C_u8MstRxSSP2(true);
        I2C_vMstStopSSP2();
        if (u8Data != u8Cnt) {
            break;
        }
    }
    // 1�o�C�g�ǂݍ���
    I2C_u8MstStartSSP2(I2C_ADDR, true);
    u8Data = I2C_u8MstRxSSP2(true);
    I2C_vMstStopSSP2();
    __delay_ms(1000);
    // ���ʕ\��
    ST7032_bSetCursorSSP2(1, 14);
    if (u8Data == 255) {
        ST7032_vWriteStringSSP2("OK");
    } else {
        ST7032_vWriteStringSSP2("NG");
    }
    // �E�F�C�g
    __delay_ms(1000);
}

/*******************************************************************************
 *
 * NAME: ssp2_vI2CTest04
 *
 * DESCRIPTION:I2C�e�X�g�P�[�X�A�����o�C�g�P�ʂ̏������݃e�X�g
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
    // �L�[�ǂݍ��݃e�X�g
    //==========================================================================
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // ���b�Z�[�W
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:I2C 04     ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Rx 0x00->0xFF   ");
    
    //==========================================================================
    // �����o�C�g�P�ʂ̎�M�e�X�g
    //==========================================================================
    // �f�[�^�N���A
    u8RxData = 0x00;
    // �X�^�[�g�R���f�B�V����
    I2C_u8MstStartSSP2(I2C_ADDR, true);
    uint8 u8Data;
    uint8 u8Cnt;
    for (u8Cnt = 0; u8Cnt < 255; u8Cnt++) {
        // 1�o�C�g�ǂݍ���
        u8Data = I2C_u8MstRxSSP2(false);
        if (u8Data != u8Cnt) {
            break;
        }
    }
    // 1�o�C�g�ǂݍ���
    u8Data = I2C_u8MstRxSSP2(true);
    // �X�g�b�v�R���f�B�V����
    I2C_vMstStopSSP2();
    __delay_ms(1000);
    // ���ʕ\��
    ST7032_bSetCursorSSP2(1, 14);
    if (u8Data == 255) {
        ST7032_vWriteStringSSP2("OK");
    } else {
        ST7032_vWriteStringSSP2("NG");
    }
    // �E�F�C�g
    __delay_ms(1000);
}

/*******************************************************************************
 *
 * NAME: ssp2_vLCDTest01
 *
 * DESCRIPTION:I2C�e�X�g�P�[�X�A�����o�C�g�P�ʂ̏������݃e�X�g
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
    // 2�s�ڂɕ`��
    //==========================================================================
    // ��ʃN���A
    ST7032_vClearDispSSP2();
    // �J�[�\���ړ�
    ST7032_vCursorTopSSP2();
    // �s�`�揈��
    uint8 val = (uint8)(rand() % 0xFF);
    ST7032_vWriteStringSSP2("Test:LCD 01 0x");
    ST7032_vWriteCharSSP2(hexToChar(val >> 4));
    ST7032_vWriteCharSSP2(hexToChar(val & 0x0F));
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(1, 0);
    // �����`��
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx <16; u8Idx++) {
        // �s�`�揈��
        ST7032_vWriteCharSSP2(val);
        val = (val + 1) % 0xFF;
        // 0.5�b�Ԋu�Ŏ��s
        __delay_ms(500);
    }
    //==========================================================================
    // 1�s�ڂɕ`��
    //==========================================================================
    // ��ʃN���A
    ST7032_vClearDispSSP2();
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(1, 0);
    // �s�`�揈��
    ST7032_vWriteStringSSP2("Test:LCD 01 0x");
    ST7032_vWriteCharSSP2(hexToChar(val >> 4));
    ST7032_vWriteCharSSP2(hexToChar(val & 0x0F));
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(0, 0);
    // �����`��
    for (u8Idx = 0; u8Idx <16; u8Idx++) {
        // �s�`�揈��
        ST7032_vWriteDataSSP2(&val, 1);
        val = (val + 1) % 0xFF;
        // 0.5�b�Ԋu�Ŏ��s
        __delay_ms(500);
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vLCDTest02
 *
 * DESCRIPTION:LCD�e�X�g�P�[�X�A�J�[�\���ړ�
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
    // 2�s�ڂɕ`��
    //==========================================================================
    // ��ʃN���A
    ST7032_vClearDispSSP2();
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(0, 0);
    // �s�`�揈��
    ST7032_vWriteStringSSP2("Test:LCD 02 0x0");
    ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorRowNoSSP2()));
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // �����`��
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 16; u8Idx++) {
        // �J�[�\���ړ�
        if ((u8Idx % 2) == 0) {
            if (ST7032_bSetCursorSSP2(1, u8Idx) == true) {
                ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorColNoSSP2()));
            }
        } else {
            ST7032_bCursorRightSSP2();
        }
        // 0.5�b�Ԋu�Ŏ��s
        __delay_ms(500);
    }
    for (u8Idx = 0; u8Idx < 16; u8Idx++) {
        // �J�[�\���ړ�
        if ((u8Idx % 2) == 0) {
            if (ST7032_bSetCursorSSP2(1, 15 - u8Idx) == true) {
                ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorColNoSSP2()));
            }
        } else {
            ST7032_bCursorLeftSSP2();
        }
        // 0.5�b�Ԋu�Ŏ��s
        __delay_ms(500);
    }
    //==========================================================================
    // 1�s�ڂɕ`��
    //==========================================================================
    // ��ʃN���A
    ST7032_vClearDispSSP2();
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(1, 0);
    // �s�`�揈��
    ST7032_vWriteStringSSP2("Test:LCD 02 0x0");
    ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorRowNoSSP2()));
    // �J�[�\���_��
    ST7032_vDispSettingSSP2(true, true, true);
    // �����`��
    for (u8Idx = 0; u8Idx <16; u8Idx++) {
        // �J�[�\���ړ�
        if ((u8Idx % 2) == 0) {
            ST7032_bSetCursorSSP2(0, u8Idx);
            ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorColNoSSP2()));
        } else {
            ST7032_bCursorRightSSP2();
        }
        // 0.5�b�Ԋu�Ŏ��s
        __delay_ms(500);
    }
    for (u8Idx = 0; u8Idx <16; u8Idx++) {
        // �J�[�\���ړ�
        if ((u8Idx % 2) == 0) {
            ST7032_bSetCursorSSP2(0, 15 - u8Idx);
            ST7032_vWriteCharSSP2(hexToChar(ST7032_u8GetCursorColNoSSP2()));
        } else {
            ST7032_bCursorLeftSSP2();
        }
        // 0.5�b�Ԋu�Ŏ��s
        __delay_ms(500);
    }
    //==========================================================================
    // 2�s�ڂɕ`��
    //==========================================================================
    // ��ʃN���A
    ST7032_vClearDispSSP2();
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(0, 0);
    // �s�`�揈��
    ST7032_vWriteStringSSP2("Test:LCD 02 ");
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(1, 38);
    if (ST7032_bCursorRightSSP2() == true) {
        ST7032_bSetCursorSSP2(0, 12);
        ST7032_vWriteCharSSP2('A');
    }
    // 1�b�Ԋu�Ŏ��s
    __delay_ms(1000);
    ST7032_bSetCursorSSP2(1, 39);
    if (ST7032_bCursorRightSSP2() == false) {
        ST7032_bSetCursorSSP2(0, 13);
        ST7032_vWriteCharSSP2('B');
    }
    // 1�b�Ԋu�Ŏ��s
    __delay_ms(1000);
    ST7032_bSetCursorSSP2(0, 1);
    if (ST7032_bCursorLeftSSP2() == true) {
        ST7032_bSetCursorSSP2(0, 14);
        ST7032_vWriteCharSSP2('C');
    }
    // 1�b�Ԋu�Ŏ��s
    __delay_ms(1000);
    ST7032_bSetCursorSSP2(0, 0);
    if (ST7032_bCursorLeftSSP2() == false) {
        ST7032_bSetCursorSSP2(0, 15);
        ST7032_vWriteCharSSP2('D');
    }
    // 1�b�Ԋu�Ŏ��s
    __delay_ms(1000);
    if (ST7032_bSetCursorSSP2(1, 39) == true) {
        ST7032_bSetCursorSSP2(1, 0);
        ST7032_vWriteCharSSP2('E');
    }
    // 1�b�Ԋu�Ŏ��s
    __delay_ms(1000);
    if (ST7032_bSetCursorSSP2(1, 40) == false) {
        ST7032_bSetCursorSSP2(1, 1);
        ST7032_vWriteCharSSP2('F');
    }
    // 1�b�Ԋu�Ŏ��s
    __delay_ms(1000);
}

/*******************************************************************************
 *
 * NAME: ssp2_vLCDTest03
 *
 * DESCRIPTION:LCD�e�X�g�P�[�X�ACGRAM
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
    // CGRAM�ւ̏�������
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
    // 2�s�ڂɕ`��
    //==========================================================================
    // ��ʃN���A
    ST7032_vClearDispSSP2();
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(0, 0);
    // �s�`�揈��
    ST7032_vWriteStringSSP2("Test:LCD 03     ");
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(1, 0);
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 16; u8Idx++) {
        // �`�揈��
        ST7032_vWriteCharSSP2(u8Idx);
        // 0.5�b�Ԋu�Ŏ��s
        __delay_ms(500);
    }
    //==========================================================================
    // 1�s�ڂɕ`��
    //==========================================================================
    // ��ʃN���A
    ST7032_vClearDispSSP2();
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(1, 0);
    // �s�`�揈��
    ST7032_vWriteStringSSP2("Test:LCD 03     ");
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(0, 0);
    for (u8Idx = 0; u8Idx < 16; u8Idx++) {
        // �`�揈��
        ST7032_vWriteCharSSP2(u8Idx);
        // 0.5�b�Ԋu�Ŏ��s
        __delay_ms(500);
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vLCDTest04
 *
 * DESCRIPTION:LCD�e�X�g�P�[�X�A�A�C�R��
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
    // �A�C�R����`��
    //==========================================================================
    // ��ʃN���A
    ST7032_vClearDispSSP2();
    // �A�C�R���N���A
    ST7032_vClearIconSSP2();
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(0, 0);
    // �s�`�揈��
    ST7032_vWriteStringSSP2("Test:LCD 04 0:00");
    // �A�C�R��
    uint8 u8Addr;
    uint8 u8SVal;
    for (u8Addr = 0; u8Addr < 16; u8Addr++) {
        // �A�h���X
        ST7032_bSetCursorSSP2(0, 12);
        ST7032_vWriteCharSSP2(hexToChar(u8Addr));
        // �l
        for (u8SVal = 0; u8SVal < 32; u8SVal++) {
            // �l�o��
            ST7032_bSetCursorSSP2(0, 14);
            ST7032_vWriteCharSSP2(hexToChar(u8SVal >> 4));
            ST7032_vWriteCharSSP2(hexToChar(u8SVal & 0x0F));
            // �A�C�R���`��
            ST7032_vWriteIconSSP2(u8Addr, u8SVal);
            // 0.05�b�Ԋu�Ŏ��s
            __delay_ms(50);
        }
    }
}

/*******************************************************************************
 *
 * NAME: ssp2_vKeypadTest01
 *
 * DESCRIPTION:�L�[�p�b�h�e�X�g�P�[�X
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
    // �L�[�ǂݍ��݃e�X�g
    //==========================================================================
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // ���b�Z�[�W
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:Keypad 01  ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("None   :        ");
    //==========================================================================
    // ���̓��[�h�F���͖���
    //==========================================================================
    eInputMode = INPUT_MODE_NONE;
    KEYPAD_vClearBuffer();
    // 10�b�ԓ��͖���
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
 * DESCRIPTION:�L�[�p�b�h�e�X�g�P�[�X
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
    // �L�[�ǂݍ��݃e�X�g
    //==========================================================================
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // ���b�Z�[�W
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:Keypad 02  ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Current:Input 1 ");
    //==========================================================================
    // ���̓��[�h�F���ݒl���[�h
    //==========================================================================
    // ���ݒl���[�h
    eInputMode = INPUT_MODE_CURRENT;
    KEYPAD_vClearBuffer();
    u8KeyValue = 0xFF;
    // �L�[���͔���
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
 * DESCRIPTION:�L�[�p�b�h�e�X�g�P�[�X
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
    // �L�[�ǂݍ��݃e�X�g
    //==========================================================================
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // ���b�Z�[�W
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:Keypad 03  ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Final  :Input 1 ");
    //==========================================================================
    // ���̓��[�h�F�ŏI�l���[�h
    //==========================================================================
    // ���ݒl���[�h
    eInputMode = INPUT_MODE_FINAL;
    KEYPAD_vClearBuffer();
    u8KeyValue = 0xFF;
    // �L�[���͔���
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
 * DESCRIPTION:�L�[�p�b�h�e�X�g�P�[�X
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
    // �L�[�ǂݍ��݃e�X�g
    //==========================================================================
    // �J�[�\���\��
    ST7032_vDispSettingSSP2(true, true, false);
    // ���b�Z�[�W
    ST7032_bSetCursorSSP2(0, 0);
    ST7032_vWriteStringSSP2("Test:Keypad 04  ");
    ST7032_bSetCursorSSP2(1, 0);
    ST7032_vWriteStringSSP2("Buffer :Input 1 ");
    //==========================================================================
    // ���̓��[�h�F�o�b�t�@�����O���[�h
    //==========================================================================
    // ���ݒl���[�h
    eInputMode = INPUT_MODE_BUFFERING;
    KEYPAD_vClearBuffer();
    u8KeyValue = 0xFF;
    // �L�[���͔���
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
