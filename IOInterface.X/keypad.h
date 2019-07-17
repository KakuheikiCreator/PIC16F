/*******************************************************************************
 *
 * MODULE :Keypad Driver functions header file
 *
 * CREATED:2019/02/24 17:07:00
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
#ifndef KEYPAD_H
#define KEYPAD_H

#if defined __cplusplus
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
// KeyPad Setting
#ifndef KEYPAD_ROW_SIZE
#define KEYPAD_ROW_SIZE        (4)      // �s�T�C�Y
#endif

#ifndef KEYPAD_COL_SIZE
#define KEYPAD_COL_SIZE        (4)      // ��T�C�Y
#endif

#ifndef KEYPAD_BUFF_SIZE
#define KEYPAD_BUFF_SIZE       (4)      // �o�b�t�@�T�C�Y
#endif

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
/**
 * KEYPAD�p�̃X�e�[�^�X���
 */
typedef struct {
    uint8 u8CheckCnt;                       // 1�b������̃`�F�b�N��
    uint16 u16PinCols[KEYPAD_COL_SIZE];     // ��s���}�b�v�z��
    uint16 u16PinRows[KEYPAD_ROW_SIZE];     // �s�s���}�b�v�z��
    uint8 u8BeforeKeyNo;                    // �O��L�[�ԍ�
    uint16 u16KeyChkCnt;                    // ����L�[�A�����o��
    uint8 u8KeyBuffer[KEYPAD_BUFF_SIZE];    // �L�[�o�b�t�@
    uint8 u8BuffSize;                       // �L�[�o�b�t�@�T�C�Y
    uint8 u8BuffBeginIdx;                   // �L�[�o�b�t�@�C���f�b�N�X
    uint8 u8BuffEndIdx;                     // �L�[�o�b�t�@�C���f�b�N�X
} tsKEYPAD_status;

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
/** ���������� */
extern void KEYPAD_vInit(tsKEYPAD_status *spStatus);
/** �o�b�t�@�X�V */
extern bool KEYPAD_bUpdateBuffer();
/** �o�b�t�@�T�C�Y�擾 */
extern uint8 KEYPAD_u8BufferSize();
/** �o�b�t�@�N���A */
extern void KEYPAD_vClearBuffer();
/** ���݃L�[�l�̓ǂݍ��� */
extern uint8 KEYPAD_u8Read();
/** �o�b�t�@����̃L�[�ǂݍ��� */
extern uint8 KEYPAD_u8ReadBuffer();
/** �ŏI�o�b�t�@�L�[�ǂݍ��� */
extern uint8 KEYPAD_u8ReadFinal();

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* KEYPAD_H */

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
