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
#include "setting.h"            // �ݒ�l
#include "libcom.h"             // ���ʒ�`
#include "i2cUtil.h"            // I2C�֐����C�u�����p
#include "keypad.h"             // �L�[�p�b�h
#include "st7032.h"             // LCD

/******************************************************************************/
/***       Macro Definitions                                                ***/
/******************************************************************************/
// �R���t�B�O�P
#pragma config FOSC     = INTOSC  // �����N���b�N(INTOSC:16MHz)�g�p����
#pragma config WDTE     = OFF     // �E�I�b�`�h�b�O�^�C�}�[����(OFF)
#pragma config PWRTE    = ON      // �d��ON����64ms��Ƀv���O�������J�n����(ON)
#pragma config MCLRE    = OFF     // �O�����Z�b�g�M���͎g�p�����Ƀf�W�^������(RA3)�s���Ƃ���(OFF)
#pragma config CP       = OFF     // �v���O�����������[��ی삵�Ȃ�(OFF)
#pragma config CPD      = OFF     // EEPROM�̃f�[�^�ǂݏo�����iOFF�j
#pragma config BOREN    = ON      // �d���d���~��(BORV�ݒ�ȉ�)�펞�Ď��@�\ON(ON)
#pragma config CLKOUTEN = OFF     // CLKOUT�s����RA4�s���Ŏg�p����(OFF)
#pragma config IESO     = OFF     // ��������O���N���b�N�ւ̐ؑւ��ł̋N���͂Ȃ�(OFF)
#pragma config FCMEN    = OFF     // ���N���b�N�Ď����Ȃ�(OFF)

// �R���t�B�O�Q
#pragma config WRT      = OFF     // Flash�������[��ی삵�Ȃ�(OFF)
#pragma config PLLEN    = OFF     // ����N���b�N��32MHz�ł͓��삳���Ȃ�(OFF)
#pragma config STVREN   = ON      // �X�^�b�N���I�[�o�t���[��A���_�[�t���[�����烊�Z�b�g������(ON)
#pragma config BORV     = HI      // �d���d���~���펞�Ď��d��(2.7V)�ݒ�(HI)
#pragma config LVP      = OFF     // ��d���v���O���~���O�@�\�g�p���Ȃ�(OFF)

// I2C 7bit address
#ifndef I2C_ADDR
#define	I2C_ADDR            (0x08)
#endif

// �s���F�d���֌W
#define PIN_POWER           RB0
#define PIN_BACK_LIGHT      RB3

// LCD�R���g���X�g
#define LCD_CONTRAST_DEF    (0x28)

// �������}�b�v�T�C�Y
#define MAP_SIZE            (0xA7)
// �������}�b�v��̃f�[�^�T�C�Y
#define MAP_DATA_SIZE       (80)
#define MAP_CGRAM_SIZE      (64)
#define MAP_ICONRAM_SIZE    (16)
// �������}�b�v�ʒu
#define	MAP_ADDR_DISPLAY    (0x07)
#define	MAP_ADDR_CGRAM      (0x57)
#define	MAP_ADDR_ICONRAM    (0x97)

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
/**
 * �X�e�[�^�X
 */
typedef enum {
    MEM_STS_NORMAL      = 0x00, // ����
    MEM_STS_PROCESSING  = 0x01  // ������
} teMapStatus;

/**
 * �C�x���g���
 */
typedef enum {
    EVT_NONE            = 0x00, // �ʒm�C�x���g����
    EVT_TIMER           = 0x01, // �^�C�}�[
    EVT_PW_CONTRAST     = 0x02, // �d���A�R���g���X�g�ݒ�
    EVT_CURSOR_SET      = 0x04, // �J�[�\���ݒ�
    EVT_CURSOR_DRAW     = 0x08, // �J�[�\���`��
    EVT_DRAW_LINE_0     = 0x10, // �P�s�ڕ`��
    EVT_DRAW_LINE_1     = 0x20, // �Q�s�ڕ`��
    EVT_SET_CGRAM       = 0x40, // CGRAM�ݒ�
    EVT_DRAW_ICON       = 0x80  // �A�C�R���`��
} teEventType;

/**
 * �A�v���P�[�V�����X�e�[�^�X
 */
typedef struct {
    uint8 u8TimerCnt;           // �^�C�}�[�J�E���^
    bool bWriteStartFlg;        // �������݃X�^�[�g�R���f�B�V������M�t���O
    uint8 u8MapAddr;            // ���݃������}�b�v�A�h���X�ʒu
    uint8 u8EventMap;           // �C�x���g�}�b�v
} tsAppStatus;

/**
 * �������}�b�v
 */
typedef struct {
    teMapStatus eStatus;                    // �X�e�[�^�X
    uint8 u8KeyValue;                       // �L�[�l�i�ŏI�l�j
    uint8 u8Power;                          // �o�b�N���C�g
    uint8 u8Contrast;                       // �R���g���X�g
    uint8 u8CursorType;                     // �J�[�\���^�C�v
    uint8 u8CursorRow;                      // �J�[�\���s
    uint8 u8CursorCol;                      // �J�[�\����
    uint8 u8DispRam[MAP_DATA_SIZE];         // �\������RAM
    uint8 u8CGRam[MAP_CGRAM_SIZE];          // ���[�U�[����RAM
    uint8 u8IconRam[MAP_ICONRAM_SIZE];      // �A�C�R��RAM
} tsMemoryMap;


/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
// �C�x���g�X�e�[�^�X�ݒ�
static void evt_vSetEventMap(teEventType eEvtStatus);
// �C�x���g�X�e�[�^�X�ݒ�
static void evt_vSetDrawEvent(uint8 u8Addr);
// �C�x���g���擾
static uint8 evt_u8GetEventMap();
// �d���ݒ菈��
static void lcd_vPowerSetting(uint8 u8Settings);
// �J�[�\���ݒ�`�揈��
static void lcd_vCursorSetting(uint8 u8CursorType);
// �J�[�\���`�揈��
static void lcd_vDrawCursor();
// �s�`�揈��
static void lcd_vDarwLine(uint8 u8RowNo);
// CGRAM��������
static void lcd_vDrawCGRAM();
// ICON Ram��������
static void lcd_vDrawIconRAM();
// �^�C�}�[���荞�ݏ���
static void timer_vInterrupt();
// I2C���荞�݂̃R�[���o�b�N�֐�
static void ssp1_vCallback(uint8 u8BusNo, uint8 u8EvtType);
// �������݃��N�G�X�g����
static void ssp1_vWriteData(uint8 u8Data);
// �ǂݍ��݃��N�G�X�g����
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
// �f�o�b�O�p�X�e�[�^�X
static uint8 u8Status = 0x00;
// �A�v���P�[�V�����X�e�[�^�X
static tsAppStatus sAppStatus;
// �������[�}�b�v
static tsMemoryMap sMemoryMap;

/******************************************************************************/
/***        Main Functions                                                  ***/
/******************************************************************************/

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
 *  None.
 ******************************************************************************/
void main() {
    //==========================================================================
    // �^�C�}�[����уs���ݒ�
    //==========================================================================
    OSCCON = 0b01111010;    // �����N���b�N��16�l�g���Ƃ���
    ANSELA = 0b00000000;    // �A�i���O�g�p���Ȃ�(���ׂăf�W�^��I/O�Ɋ����Ă�)
    ANSELB = 0b00000000;    // �A�i���O�g�p���Ȃ�(���ׂăf�W�^��I/O�Ɋ����Ă�)
    TRISA  = 0b00011100;    // �L�[�p�b�h�̂ݓ��̓��[�h�Ƃ���
    TRISB  = 0b10110110;    // �s����SDA/SCL�̂ݓ���
    PORTA  = 0b00000000;    // �o�̓s���̏�����(�S��LOW�ɂ���)
    PORTB  = 0b00000000;    // �o�̓s���̏�����(�S��LOW�ɂ���)

    //==========================================================================
    // �A�v���P�[�V�����X�e�[�^�X�̏�����
    //==========================================================================
    sAppStatus.u8TimerCnt     = 0;          // �^�C�}�[�J�E���^
    sAppStatus.bWriteStartFlg = false;      // �X�^�[�g�R���f�B�V������M�t���O
    sAppStatus.u8MapAddr      = 0x00;       // �}�b�v��̃A�h���X
    sAppStatus.u8EventMap     = 0x00;       // �C�x���g�}�b�v
    
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
    // �������}�b�v�֘A�̏�����
    //==========================================================================
    // �������}�b�v���[���N���A
    memset(&sMemoryMap, 0x00, sizeof(tsMemoryMap));
    sMemoryMap.u8KeyValue = 0xFF;               // �L�[�l
    sMemoryMap.u8Power    = 0x01;               // LCD�d��
    sMemoryMap.u8Contrast = LCD_CONTRAST_DEF;   // LCD�R���g���X�g
    memset(sMemoryMap.u8CGRam, 0xE0, MAP_CGRAM_SIZE);   // CGRAM
        
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
    // �又�����[�v
    //==========================================================================
    uint8 u8EventMap;
    // ���C�����[�v
    while(true) {
        //----------------------------------------------------------------------
        // �C�x���g����
        //----------------------------------------------------------------------
        // �C�x���g�ʒm����
        u8EventMap = evt_u8GetEventMap();
        if (u8EventMap == EVT_NONE) {
            continue;
        }
        // �d���R���g���X�g�ݒ�
        if ((u8EventMap & EVT_PW_CONTRAST) == EVT_PW_CONTRAST) {
            // �d���ƃR���g���X�g�ݒ�
            lcd_vPowerSetting(sMemoryMap.u8Power);
            // �R���g���X�g�ύX
            ST7032_vSetContrastSSP2(sMemoryMap.u8Contrast);
        }
        // �J�[�\���ݒ�
        if ((u8EventMap & EVT_CURSOR_SET) == EVT_CURSOR_SET) {
            lcd_vCursorSetting(sMemoryMap.u8CursorType);
        }
        // �J�[�\���`�攻��
        if ((u8EventMap & EVT_CURSOR_DRAW) == EVT_CURSOR_DRAW) {
            lcd_vDrawCursor();
        }
        // �P�s�ڕ`�攻��
        if ((u8EventMap & EVT_DRAW_LINE_0) == EVT_DRAW_LINE_0) {
            lcd_vDarwLine(0);
        }
        // �Q�s�ڕ`�攻��
        if ((u8EventMap & EVT_DRAW_LINE_1) == EVT_DRAW_LINE_1) {
            lcd_vDarwLine(1);
        }
        // CGRAM�ւ̏������ݔ���
        if ((u8EventMap & EVT_SET_CGRAM) == EVT_SET_CGRAM) {
            // CGRAM�ւ̏�������
            lcd_vDrawCGRAM();
        }
        // ICON RAM�ւ̏������ݔ���
        if ((u8EventMap & EVT_DRAW_ICON) == EVT_DRAW_ICON) {
            // ICON RAM�ւ̏�������
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
 * DESCRIPTION:�C�x���g�}�b�v�ݒ�
 *
 * PARAMETERS:      Name            RW  Usage
 * teEventType      eEvtType        R   �ݒ肷��C�x���g�}�b�v
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
 * DESCRIPTION:�`��C�x���g�ݒ�
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Addr          R   �`��˗��A�h���X
 * 
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void evt_vSetDrawEvent(uint8 u8Addr) {
    // �w�肳�ꂽ�s�̕`��C�x���g
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
 * DESCRIPTION:�C�x���g�}�b�v�擾����
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8:�C�x���g�}�b�v
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static uint8 evt_u8GetEventMap() {
    // �N���e�B�J���Z�N�V�����̊J�n
    criticalSec_vBegin();
    // �}�b�v�X�e�[�^�X�X�V
    uint8 u8EvtMap = sAppStatus.u8EventMap;
    if (sAppStatus.u8EventMap == EVT_NONE) {
        sMemoryMap.eStatus = MEM_STS_NORMAL;
    } else {
        sAppStatus.u8EventMap = EVT_NONE;
    }
    // �N���e�B�J���Z�N�V�����̏I��
    criticalSec_vEnd();
    // �C�x���g�}�b�v��ԋp
    return u8EvtMap;
}

/*******************************************************************************
 *
 * NAME: lcd_vPowerSetting
 *
 * DESCRIPTION:�d���ݒ菈��
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Settings      R   �J�[�\���^�C�v
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void lcd_vPowerSetting(uint8 u8Settings) {
    //==========================================================================
    // �N���e�B�J���Z�N�V����
    //==========================================================================
    criticalSec_vBegin();
    // �J�[�\���ʒu���̎擾
    uint8 u8Val = sMemoryMap.u8Power;
    sMemoryMap.u8Power = sMemoryMap.u8Power | 0x01;
    criticalSec_vEnd();
    
    //==========================================================================
    // �d������
    //==========================================================================
    // �d���I�t����
    if ((u8Val & 0x01) == 0x00) {
        // ���Z�b�g���s
        PIN_POWER = OFF;
        __delay_ms(1);
        PIN_POWER = ON;
        __delay_ms(40);
        // LCD����������
        ST7032_vInitSSP2();
    }
    // �o�b�N���C�g�ݒ�
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
 * DESCRIPTION:�J�[�\���ݒ�`�揈��
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8CursorType    R   �J�[�\���^�C�v
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void lcd_vCursorSetting(uint8 u8CursorType) {
    // �J�[�\���\���ݒ�̎擾
    bool bCursorDisp = u8CursorType & 0x01;
    bool bCursorBlink = (u8CursorType >> 1) & 0x01;
    // �J�[�\���\���ݒ�
    ST7032_vDispSettingSSP2(true, bCursorDisp, bCursorBlink);
}

/*******************************************************************************
 *
 * NAME: lcd_vDrawCursor
 *
 * DESCRIPTION:�J�[�\���`�揈��
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void lcd_vDrawCursor() {
    // �N���e�B�J���Z�N�V�����̊J�n
    criticalSec_vBegin();
    // �J�[�\���ʒu���̎擾
    uint8 u8CursorRow = sMemoryMap.u8CursorRow;
    uint8 u8CursorCol = sMemoryMap.u8CursorCol;
    // �N���e�B�J���Z�N�V�����̏I��
    criticalSec_vEnd();
    // �J�[�\���ĕ`��
    ST7032_bSetCursorSSP2(u8CursorRow, u8CursorCol);
}

/*******************************************************************************
 *
 * NAME: lcd_vDarwLine
 *
 * DESCRIPTION:�s�`�揈��
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8RowNo         R   �`��Ώۍs�ԍ�
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void lcd_vDarwLine(uint8 u8RowNo) {
    //==========================================================================
    // �}�b�v����̕`��f�[�^�擾
    //==========================================================================
    // �N���e�B�J���Z�N�V�����̊J�n
    criticalSec_vBegin();
    // �����`�揈��
    uint8 u8Msg[16];
    memcpy(u8Msg, &sMemoryMap.u8DispRam[u8RowNo * 40], 16);
    // �N���e�B�J���Z�N�V�����̏I��
    criticalSec_vEnd();
    
    //==========================================================================
    // �s�`�揈��
    //==========================================================================
    // �J�[�\���ړ�
    ST7032_bSetCursorSSP2(u8RowNo, 0);
    // �s�`�揈��
    ST7032_vWriteDataSSP2(u8Msg, 16);
    // �J�[�\���ĕ`��
    lcd_vDrawCursor();
}

/*******************************************************************************
 *
 * NAME: lcd_vDrawCGRAM
 *
 * DESCRIPTION:CGRAM��������
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
 * DESCRIPTION:ICON RAM��������
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
    // �b��128��̊��荞�݂�z��
    // �����ݔ����̉񐔂��J�E���g����
    if (sAppStatus.u8TimerCnt < 128) {
        sAppStatus.u8TimerCnt++;
    } else {
        sAppStatus.u8TimerCnt = 0;
    }
    // �^�C�}�[�C�x���g
    // �b��128��̊��荞�݂�z�肵�A�b��64��C�x���g����
    if ((sAppStatus.u8TimerCnt % 2) == 0) {
        evt_vSetEventMap(EVT_TIMER);
    }
    // �L�[�l�X�V
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
 * DESCRIPTION:I2C���荞�݂̃R�[���o�b�N�֐�
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
            // �������ݗv���A�h���X��M
            // �A�h���X�f�[�^����ǂ݂���
            u8Data = SSP1BUF;
            // �X�^�[�g��ԃt���O�X�V
            sAppStatus.bWriteStartFlg = true;
            break;
        case I2C_SLV_EVT_WRITE_DATA:
            // �������ݗv���f�[�^��M
            ssp1_vWriteData((uint8)SSP1BUF);
            // �X�^�[�g��ԃt���O�X�V
            sAppStatus.bWriteStartFlg = false;
            break;
        case I2C_SLV_EVT_READ_ADDR:
            // �ǂݏo���v���A�h���X��M
            // �A�h���X�f�[�^����ǂ݂���
            u8Data = SSP1BUF;
            // �ǂݏo������
            SSP1BUF = ssp1_u8ReadData();
            break;
        case I2C_SLV_EVT_READ_ACK:
            // �ǂݏo���v��ACK������M
            SSP1BUF = ssp1_u8ReadData();
            break;
        case I2C_SLV_EVT_READ_NACK:
            // �ǂݏo���v��NACK������M
            // �ǂݏo�������Ȃ̂ŕԐM���Ȃ�
            break;
        default:
            // �X�^�[�g��ԃt���O�X�V
            sAppStatus.bWriteStartFlg = false;
            break;
    }
}

/*******************************************************************************
 *
 * NAME: ssp1_vWriteData
 *
 * DESCRIPTION:�������݃��N�G�X�g����
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8       u8Data          R   �������݃f�[�^
 *
 * RETURNS:
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static void ssp1_vWriteData(uint8 u8Data) {
    //==========================================================================
    // �A�h���X�ݒ�i�X�^�[�g��Ԓ���j�̏���
    //==========================================================================
    if (sAppStatus.bWriteStartFlg) {
        // �A�h���X�G���[����
        if (u8Data < MAP_SIZE) {
            // ��M�����������}�b�v�A�h���X��ݒ�(ACK��PIC�������I�ɕԐM����)
            sAppStatus.u8MapAddr = u8Data;
        } else {
            // �������I�[�o�[�t���[
            // NACK�ԐM����
            SSP1CON2bits.ACKDT = 0x01;
        }
        // �I��
        return;
    }

    //==========================================================================
    // �������̈�ւ̏�������
    //==========================================================================
    // �A�h���X�G���[����
    if (sAppStatus.u8MapAddr >= MAP_SIZE) {
        // NACK�ԐM����
        SSP1CON2bits.ACKDT = 0x01;
        // �I��
        return;
    }
    // �A�h���X
    uint8 u8Addr;
    // �������̈攻��
    switch (sAppStatus.u8MapAddr) {
        case 0x00:
            // �X�e�[�^�X�i�ǂݍ��ݐ�p�j
            break;
        case 0x01:
            // �L�[�l
            sMemoryMap.u8KeyValue = u8Data;
            break;
        case 0x02:
            // �d���ݒ�
            if (u8Data > 0x03) {
                // NACK�ԐM����
                SSP1CON2bits.ACKDT = 0x01;
                // �I��
                return;
            }
            // �l�̍X�V����
            if (sMemoryMap.u8Power != u8Data) {
                // �d���I�t����
                if ((u8Data & 0x01) == 0x00) {
                    // �֘A�p�����[�^������
                    sMemoryMap.u8Power      = 0x00;             // LCD�d��
                    sMemoryMap.u8Contrast   = LCD_CONTRAST_DEF; // LCD�R���g���X�g
                    sMemoryMap.u8CursorType = 0x00;             // �J�[�\���^�C�v
                    sMemoryMap.u8CursorRow  = 0x00;             // �J�[�\���s
                    sMemoryMap.u8CursorCol  = 0x00;             // �J�[�\����
                    memset(sMemoryMap.u8DispRam, 0x00, MAP_DATA_SIZE);      // �\������RAM
                    memset(sMemoryMap.u8CGRam, 0xE0, MAP_CGRAM_SIZE);       // ���[�U�[����RAM
                    memset(sMemoryMap.u8IconRam, 0x00, MAP_ICONRAM_SIZE);   // �A�C�R��RAM
                } else {
                    // �o�b�N���C�g���X�V
                    sMemoryMap.u8Power = u8Data;
                }
                // �C�x���g���̒ʒm
                evt_vSetEventMap(EVT_PW_CONTRAST);
            }
            break;
        case 0x03:
            // �R���g���X�g�ݒ�
            if (u8Data > ST7032_CONTRAST_MAX) {
                // NACK�ԐM����
                SSP1CON2bits.ACKDT = 0x01;
                // �I��
                return;
            }
            // �l�̍X�V����
            if (sMemoryMap.u8Contrast != u8Data) {
                // �R���g���X�g���X�V
                sMemoryMap.u8Contrast = u8Data;
                // �C�x���g���̒ʒm
                evt_vSetEventMap(EVT_PW_CONTRAST);
            }
            break;
        case 0x04:
            // �J�[�\���^�C�v
            // ���̓`�F�b�N
            if (u8Data > 0x03) {
                // NACK�ԐM����
                SSP1CON2bits.ACKDT = 0x01;
                // �I��
                return;
            }
            // �J�[�\���^�C�v
            if (sMemoryMap.u8CursorType != u8Data) {
                // �J�[�\���s���X�V
                sMemoryMap.u8CursorType = u8Data;
                // �C�x���g���̒ʒm
                evt_vSetEventMap(EVT_CURSOR_SET);
            }
            break;
        case 0x05:
            // �J�[�\���s
            // ���̓`�F�b�N
            if (u8Data > ST7032_ROW_MAX) {
                // NACK�ԐM����
                SSP1CON2bits.ACKDT = 0x01;
                // �I��
                return;
            }
            // �J�[�\���ړ�����
            if (sMemoryMap.u8CursorRow != u8Data) {
                // �J�[�\���s���X�V
                sMemoryMap.u8CursorRow = u8Data;
                // �C�x���g���̒ʒm
                evt_vSetEventMap(EVT_CURSOR_DRAW);
            }
            break;
        case 0x06:
            // �J�[�\����
            // ���̓`�F�b�N
            if (u8Data > ST7032_COL_MAX) {
                // NACK�ԐM����
                SSP1CON2bits.ACKDT = 0x01;
                // �I��
                return;
            }
            // �J�[�\���ړ�����
            if (sMemoryMap.u8CursorCol != u8Data) {
                // �J�[�\������X�V
                sMemoryMap.u8CursorCol = u8Data;
                // �C�x���g���̒ʒm
                evt_vSetEventMap(EVT_CURSOR_DRAW);
            }
            break;
        default:
            // �Ώۃf�[�^����
            if (sAppStatus.u8MapAddr < MAP_ADDR_CGRAM) {
                // �\���f�[�^�X�V����
                u8Addr = sAppStatus.u8MapAddr - MAP_ADDR_DISPLAY;
                if (sMemoryMap.u8DispRam[u8Addr] == u8Data) {
                    break;
                }
                // �\���f�[�^�ݒ�
                sMemoryMap.u8DispRam[u8Addr] = u8Data;
                // �C�x���g���̒ʒm
                evt_vSetDrawEvent(u8Addr);
            } else if (sAppStatus.u8MapAddr < MAP_ADDR_ICONRAM) {
                // CGRAM���̓`�F�b�N
                if (u8Data > 0x1F) {
                    // NACK�ԐM����
                    SSP1CON2bits.ACKDT = 0x01;
                    // �I��
                    return;
                }
                // CGRAM�X�V����
                u8Addr = sAppStatus.u8MapAddr - MAP_ADDR_CGRAM;
                if (sMemoryMap.u8CGRam[u8Addr] == u8Data) {
                    break;
                }
                // �\���f�[�^�ݒ�
                sMemoryMap.u8CGRam[u8Addr] = u8Data;
                // �C�x���g���̒ʒm
                evt_vSetEventMap(EVT_SET_CGRAM);
            } else {
                // ICONRAM���̓`�F�b�N
                if (u8Data > 0x1F) {
                    // NACK�ԐM����
                    SSP1CON2bits.ACKDT = 0x01;
                    // �I��
                    return;
                }
                // ICONRAM�X�V����
                u8Addr = sAppStatus.u8MapAddr - MAP_ADDR_ICONRAM;
                if (sMemoryMap.u8IconRam[u8Addr] == u8Data) {
                    break;
                }
                // �\���f�[�^�ݒ�
                sMemoryMap.u8IconRam[u8Addr] = u8Data;
                // �C�x���g���̒ʒm
                evt_vSetEventMap(EVT_DRAW_ICON);
            }
            break;
    }
    // �������}�b�v�A�h���X�J�E���g�A�b�v
    sAppStatus.u8MapAddr++;
}

/*******************************************************************************
 *
 * NAME: ssp1_u8ReadData
 *
 * DESCRIPTION:�ǂݍ��݃��N�G�X�g����
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     uint8:�ǂݍ��݃f�[�^
 *
 * NOTES:
 *  None.
 ******************************************************************************/
static uint8 ssp1_u8ReadData() {
    //==========================================================================
    // ���݃A�h���X����̓ǂݍ��ݏ���
    //==========================================================================
    // �������I�[�o�[�t���[����
    if (sAppStatus.u8MapAddr >= MAP_SIZE) {
        // �������I�[�o�[�t���[
        // NACK�ԐM����
        SSP1CON2bits.ACKDT = 0x01;
        // �I��
        return 0xFF;
    }
    // �������̈攻��
    uint8 u8Data;
    uint8 u8Addr;
    switch (sAppStatus.u8MapAddr) {
        case 0x00:
            // �X�e�[�^�X�i�ǂݍ��ݐ�p�j
            u8Data = sMemoryMap.eStatus;
            break;
        case 0x01:
            // �L�[�l�i�ŏI�l���[�h�j
            u8Data = sMemoryMap.u8KeyValue;
            sMemoryMap.u8KeyValue = 0xFF;
            break;
        case 0x02:
            // �d���ݒ�
            u8Data = sMemoryMap.u8Power;
            break;
        case 0x03:
            // �R���g���X�g
            u8Data = sMemoryMap.u8Contrast;
            break;
        case 0x04:
            // �J�[�\���^�C�v
            u8Data = sMemoryMap.u8CursorType;
            break;
        case 0x05:
            // �J�[�\���s
            u8Data = sMemoryMap.u8CursorRow;
            break;
        case 0x06:
            // �J�[�\����
            u8Data = sMemoryMap.u8CursorCol;
            break;
        default:
            // �Ώۃf�[�^����
            if (sAppStatus.u8MapAddr < MAP_ADDR_CGRAM) {
                // �\���f�[�^
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
    // �������}�b�v�A�h���X�J�E���g�A�b�v
    sAppStatus.u8MapAddr++;
    // �l��ԐM
    return u8Data;
}

/******************************************************************************/
/***        ISR Functions                                                 ***/
/******************************************************************************/

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
 *  None.
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

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
