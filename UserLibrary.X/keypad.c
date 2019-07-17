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
// �b��128��̃`�F�b�N��z��
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
// �{�^������������Ă����̓ǂݍ���
static uint8 readColumn();

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
// �f�o�C�X���
static tsKEYPAD_status *spKEYPAD_status;

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: KEYPAD_vInit
 *
 * DESCRIPTION:�L�[�p�b�h�̏���������
 *
 * PARAMETERS:      Name            RW  Usage
 * tsKEYPAD_status  *spStatus       R   �f�o�C�X�X�e�[�^�X���
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
    // �L�[���̓X�e�[�^�X������
    spStatus->u8BeforeKeyNo  = 0;
    spStatus->u16KeyChkCnt    = 0;
     // �o�b�t�@�X�e�[�^�X������
    spStatus->u8BuffSize     = 0;
    spStatus->u8BuffBeginIdx = 0;
    spStatus->u8BuffEndIdx   = KEYPAD_BUFF_SIZE - 1;
    // �L�[�o�b�t�@�̏�����
    memset(spStatus->u8KeyBuffer, 0x00, KEYPAD_BUFF_SIZE);
    // �f�o�C�X�X�e�[�^�X���
    spKEYPAD_status = spStatus;
}

/*******************************************************************************
 *
 * NAME: KEYPAD_bUpdateBuffer
 *
 * DESCRIPTION:�o�b�t�@�X�V
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    bool_t        TRUE:�X�V����
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern bool KEYPAD_bUpdateBuffer() {
    // �o�b�t�@�����O����
    if (spKEYPAD_status->u8BuffSize >= KEYPAD_BUFF_SIZE) {
        // �o�b�t�@����t�̏ꍇ
        return false;
    }
    // �L�[�ԍ��ǂݍ���
    uint8 u8KeyNo = KEYPAD_u8Read();
    if (u8KeyNo == 0xFF) {
        return false;
    }
    // �N���e�B�J���Z�N�V�����J�n
    criticalSec_vBegin();
    // �o�b�t�@�����O
    uint8 u8Idx = (spKEYPAD_status->u8BuffEndIdx + 1) % KEYPAD_BUFF_SIZE;
    spKEYPAD_status->u8KeyBuffer[u8Idx] = u8KeyNo;
    spKEYPAD_status->u8BuffEndIdx = u8Idx;
    spKEYPAD_status->u8BuffSize++;
    // �N���e�B�J���Z�N�V�����I��
    criticalSec_vEnd();
    return true;
}

/*******************************************************************************
 *
 * NAME: KEYPAD_u8BufferSize
 *
 * DESCRIPTION:�o�b�t�@�T�C�Y�擾
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8         �o�b�t�@�T�C�Y
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
 * DESCRIPTION:�o�b�t�@�N���A
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t true:�o�b�t�@�N���A����
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern void KEYPAD_vClearBuffer() {
    // �N���e�B�J���Z�N�V�����J�n
    criticalSec_vBegin();
    // �o�b�t�@�N���A
    spKEYPAD_status->u8BuffSize     = 0;
    spKEYPAD_status->u8BuffBeginIdx = 0;
    spKEYPAD_status->u8BuffEndIdx   = KEYPAD_BUFF_SIZE - 1;
    // �N���e�B�J���Z�N�V�����I��
    criticalSec_vEnd();
}

/*******************************************************************************
 *
 * NAME:  KEYPAD_u8Read
 *
 * DESCRIPTION:���݃L�[�l�̓ǂݍ���
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8*      pu8Key          W   �ǂݍ��݃L�[
 *
 * RETURNS:
 *   uint8 ���݂̃L�[�ԍ��A�����͎��ɂ�0xFF��ԋp
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 KEYPAD_u8Read() {
    // �|�[�g�`�F�b�N
    uint8 u8PortA = PORTA;
#ifdef PORTB
    uint8 u8PortB = PORTB;
#endif
#ifdef PORTC
    uint8 u8PortC = PORTC;
#endif
    // �L�[���̓`�F�b�N
    uint16 u16PinMap;
    uint8 u8Col;
    uint8 u8Row;
    for (u8Row = 0; u8Row < KEYPAD_ROW_SIZE; u8Row++) {
        // �P�s���ƂɃ{�^���𑖍�
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
        // �L�[���͔���
        if (u8Col != 0xFF) {
            break;
        }
    }
    // �L�[���͔���
    if (u8Col == 0xFF) {
        spKEYPAD_status->u8BeforeKeyNo = 0xFF;
        spKEYPAD_status->u16KeyChkCnt   = 0;
        return 0xFF;
    }
    // �L�[�ԍ��擾
    uint8 u8KeyNo = u8Row * KEYPAD_ROW_SIZE + u8Col;
    // ����L�[���͔���
    if (u8KeyNo != spKEYPAD_status->u8BeforeKeyNo) {
        spKEYPAD_status->u8BeforeKeyNo = u8KeyNo;
        spKEYPAD_status->u16KeyChkCnt   = 1;
        return 0xFF;
    }
    // ����L�[�`�F�b�N�񐔃J�E���g�A�b�v
    spKEYPAD_status->u16KeyChkCnt++;
    // �`���^�����O�Ɖ������ςȂ��̔���i���̓^�C�~���O����j
    if (spKEYPAD_status->u16KeyChkCnt == KEYPAD_CHK_CNT_0 ||
        spKEYPAD_status->u16KeyChkCnt == KEYPAD_CHK_CNT_1 ||
        spKEYPAD_status->u16KeyChkCnt == KEYPAD_CHK_CNT_2) {
        // �{�^���ԍ���ԋp
        return u8KeyNo;
    } else if (spKEYPAD_status->u16KeyChkCnt == KEYPAD_CHK_CNT_3) {
        spKEYPAD_status->u16KeyChkCnt = KEYPAD_CHK_CNT_2;
        // �{�^���ԍ���ԋp
        return u8KeyNo;
    }
    // 
    return 0xFF;
}

/*******************************************************************************
 *
 * NAME: KEYPAD_u8ReadBuffer
 *
 * DESCRIPTION:�o�b�t�@�擪�����̓ǂݍ���
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8*      pu8Key          W   �ǂݍ��݃L�[
 *
 * RETURNS:
 *   uint8 �o�b�t�@�̐擪�L�[�ԍ��A�o�b�t�@����Ă��Ȃ��ꍇ�ɂ�0xFF��ԋp
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 KEYPAD_u8ReadBuffer() {
    // �o�b�t�@�����̗L������
    if (spKEYPAD_status->u8BuffSize == 0) {
        return 0xFF;
    }
    // �N���e�B�J���Z�N�V�����J�n
    criticalSec_vBegin();
    // �o�b�t�@�ǂݍ���
    uint8 u8Idx = spKEYPAD_status->u8BuffBeginIdx;
    uint8 u8KeyNo = spKEYPAD_status->u8KeyBuffer[u8Idx];
    spKEYPAD_status->u8BuffBeginIdx = (u8Idx + 1) % KEYPAD_BUFF_SIZE;
    spKEYPAD_status->u8BuffSize--;
    // �N���e�B�J���Z�N�V�����I��
    criticalSec_vEnd();
    return u8KeyNo;
}

/*******************************************************************************
 *
 * NAME: KEYPAD_u8ReadFinal
 *
 * DESCRIPTION:�o�b�t�@�I�[�����̓ǂݍ���
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   bool_t         TRUE:�ǂݍ��ݐ���
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern uint8 KEYPAD_u8ReadFinal() {
    // �o�b�t�@�����̗L������
    if (spKEYPAD_status->u8BuffSize == 0) {
        return 0xFF;
    }
    // �N���e�B�J���Z�N�V�����J�n
    criticalSec_vBegin();
    // �o�b�t�@�ǂݍ���
    uint8 u8KeyNo = spKEYPAD_status->u8KeyBuffer[spKEYPAD_status->u8BuffEndIdx];
    // �o�b�t�@�N���A
    spKEYPAD_status->u8BuffBeginIdx = 0;
    spKEYPAD_status->u8BuffEndIdx   = KEYPAD_BUFF_SIZE - 1;
    spKEYPAD_status->u8BuffSize     = 0;
    // �N���e�B�J���Z�N�V�����I��
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
 * DESCRIPTION:�{�^����������Ă����̓ǂݍ���
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *   uint8 ���͂��ꂽ��ԍ��A�����͎��ɂ�0xFF��ԋp
 *
 * NOTES:
 * None.
 ******************************************************************************/
static uint8 readColumn() {
    // �|�[�g�`�F�b�N
    uint8 portA = PORTA;
#ifdef PORTB
    uint8 portB = PORTB;
#endif
#ifdef PORTC
    uint8 portC = PORTC;
#endif
    // �{�^�����`�F�b�N
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
                // ��������Ă���L�[��������Ȃ�����
                return 0xFF;
        }
        // ���͔���
        if ((u8Port & (uint8)u16PinMap) != 0x00) {
            return u8Col;
        }
    }
    // ��������Ă���L�[��������Ȃ�����
    return 0xFF;
}

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
