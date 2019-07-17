/*******************************************************************************
 *
 * MODULE :ST7032 Driver functions source file
 *
 * CREATED:2019/03/11 23:25:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:ST7032I LCD I2C draiver
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
#include "st7032.h"             // LCD���C�u����
#include "i2cUtil.h"            // I2C�֐����C�u�����p

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/
// Address Read/Write
#define ST7032_I2C_ADDR             (0x3E)

// �R���g���[���o�C�g�i�R�}���h�j
#define ST7032_CNTR_CMD             (0x00)
// �R���g���[���o�C�g�i�f�[�^�j
#define ST7032_CNTR_DATA            (0x40)

// ST7032�C���X�g���N�V����
#define ST7032_CMD_CLEAR_DISP       (0b00000001)    // �N���A�f�B�X�v���C
#define ST7032_CMD_ENTRY_MODE_SET   (0b00000100)    // ���̓��[�h�ݒ�
#define ST7032_CMD_DISP_CNTR_DEF    (0b00001000)    // �f�B�X�v���C�ݒ�
#define ST7032_CMD_OSC_FREQ         (0b00010100)    // �����I�V���[�^�ݒ�
#define ST7032_CMD_FUNC_SET_DEF     (0b00111000)    // �t�@���N�V�����ݒ�@IS(instruction table select)��0
#define ST7032_CMD_FUNC_SET_EX      (0b00111001)    // �t�@���N�V�����ݒ�@IS(instruction table select)��1
#define ST7032_CMD_DISP_CNTR_EX     (0b01011000)    // �g���f�B�X�v���C�ݒ�A�A�C�R���\��ON�A�u�[�X�^�[��HOFF�A�R���g���X�g�ݒ�i��2���j
#define ST7032_CMD_FOLLOWER_CNTR    (0b01101100)    // �t�H���A��HON�ARab0?Rab2(0b100)
#define ST7032_CMD_CONTRAST_LOW     (0b01110000)    // �R���g���X�g�ݒ�i��4���j
#define ST7032_CMD_SET_CGRAM        (0b01000000)    // CGRAM�A�h���X�ݒ�
#define ST7032_CMD_SET_ICON_ADDR    (0b01000000)    // �A�C�R���A�h���X�ݒ�
#define ST7032_CMD_SET_DD_ADDR      (0b10000000)    // �J�[�\���A�h���X�ݒ�

// �҂�����
#ifndef ST7032_DEF_WAIT
#define ST7032_DEF_WAIT             (26)
#endif
#ifndef ST7032_EX_WAIT
#define ST7032_EX_WAIT              (1080)
#endif

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/
/**
 * �\���́FLCD�̏�ԏ��
 */
typedef struct {
	// �J�[�\���ʒu
	uint8 u8CursorPos;
    // ICON display(1bit). booster circuit(1bit). Contrast(6bit).
	uint8 u8Settings;
} ST7032_state;

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/
// Execute Command
static void vExecCmdSSP1(uint8 u8Cmd);
#ifdef SSP2STAT
static void vExecCmdSSP2(uint8 u8Cmd);
#endif

// Execute Command and Stop End
static void vExecCmdEndSSP1(uint8 u8Cmd);
#ifdef SSP2STAT
static void vExecCmdEndSSP2(uint8 u8Cmd);
#endif

// Move cursor
static bool bSetCursorSSP1(uint8 u8Pos);
#ifdef SSP2STAT
static bool bSetCursorSSP2(uint8 u8Pos);
#endif

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
/** �J�[�\���ʒu(SSP1) */
static ST7032_state stStateSSP1;
/** �J�[�\���ʒu(SSP2) */
#ifdef SSP2STAT
static ST7032_state stStateSSP2;
#endif


/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/
/*******************************************************************************
 *
 * NAME: ST7032_vInitSSP1
 *
 * DESCRIPTION:Initialize LCD
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vInitSSP1() {
    // �J�[�\���ʒu
    stStateSSP1.u8CursorPos = 0x00;
    // ICON,Booster,Contrast
    stStateSSP1.u8Settings = 0xE8;
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��1
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // �����I�V���[�^�ݒ�
    vExecCmdSSP1(ST7032_CMD_OSC_FREQ);
    __delay_us(ST7032_DEF_WAIT);
    // �R���g���X�g�ݒ�i��4���j
    vExecCmdSSP1(ST7032_CMD_CONTRAST_LOW | (stStateSSP1.u8Settings & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // �g���f�B�X�v���C�ݒ�A�A�C�R���\��ON�A�u�[�X�^�[��HON�A�R���g���X�g�ݒ�i��2���j
    vExecCmdSSP1(ST7032_CMD_DISP_CNTR_EX | (stStateSSP1.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // �t�H���A��HON�ARab0?Rab2(0b100)
    vExecCmdSSP1(ST7032_CMD_FOLLOWER_CNTR);
    __delay_us(ST7032_DEF_WAIT);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��0
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // �f�B�X�v���C�ݒ�A�\��ON�A�J�[�\���\��OFF�A�J�[�\���_��OFF
    vExecCmdSSP1(ST7032_CMD_DISP_CNTR_DEF | 0x04);
    __delay_us(ST7032_DEF_WAIT);
    // �J�[�\���ݒ�
    vExecCmdSSP1(ST7032_CMD_SET_DD_ADDR);
    __delay_us(ST7032_DEF_WAIT);
    // �N���A�f�B�X�v���C
    vExecCmdEndSSP1(ST7032_CMD_CLEAR_DISP);
    __delay_us(ST7032_EX_WAIT);
}

/*******************************************************************************
 *
 * NAME: ST7032_vInitSSP2
 *
 * DESCRIPTION:Initialize LCD
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vInitSSP2() {
    // �J�[�\���ʒu
    stStateSSP2.u8CursorPos = 0x00;
    // ICON,Booster,Contrast
    stStateSSP2.u8Settings = 0xE8;
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��1
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // �����I�V���[�^�ݒ�
    vExecCmdSSP2(ST7032_CMD_OSC_FREQ);
    __delay_us(ST7032_DEF_WAIT);
    // �R���g���X�g�ݒ�i��4���j
    vExecCmdSSP2(ST7032_CMD_CONTRAST_LOW | (stStateSSP2.u8Settings & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // �g���f�B�X�v���C�ݒ�A�A�C�R���\��ON�A�u�[�X�^�[��HON�A�R���g���X�g�ݒ�i��2���j
    vExecCmdSSP2(ST7032_CMD_DISP_CNTR_EX | (stStateSSP2.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // �t�H���A��HON�ARab0?Rab2(0b100)
    vExecCmdSSP2(ST7032_CMD_FOLLOWER_CNTR);
    __delay_us(ST7032_DEF_WAIT);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��0
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // �f�B�X�v���C�ݒ�A�\��ON�A�J�[�\���\��OFF�A�J�[�\���_��OFF
    vExecCmdSSP2(ST7032_CMD_DISP_CNTR_DEF | 0x04);
    __delay_us(ST7032_DEF_WAIT);
    vExecCmdSSP2(ST7032_CMD_ENTRY_MODE_SET | 0x02);
    __delay_us(ST7032_DEF_WAIT);
    // �J�[�\���ݒ�
    vExecCmdSSP2(ST7032_CMD_SET_DD_ADDR);
    __delay_us(ST7032_DEF_WAIT);
    // �N���A�f�B�X�v���C
    vExecCmdEndSSP2(ST7032_CMD_CLEAR_DISP);
    __delay_us(ST7032_EX_WAIT);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vSetContrastSSP1
 *
 * DESCRIPTION:LCD Contrast Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        uint8     u8Contrast      R   Display Contrast
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vSetContrastSSP1(uint8 u8Contrast) {
    // ���̓`�F�b�N
    uint8 u8Val = u8Contrast & 0x3F;
    if ((stStateSSP1.u8Settings & 0x3F) == u8Val) {
        return;
    }
    // �ݒ�X�V
    stStateSSP1.u8Settings = (stStateSSP1.u8Settings & 0xC0) | u8Val;
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��1
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // �R���g���X�g�ݒ�i��4���j
    vExecCmdSSP1(ST7032_CMD_CONTRAST_LOW | (stStateSSP1.u8Settings & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // �g���f�B�X�v���C�ݒ�A�A�C�R���\��ON�A�u�[�X�^�[��HOFF�A�R���g���X�g�ݒ�i��2���j
    vExecCmdSSP1(ST7032_CMD_DISP_CNTR_EX | (stStateSSP1.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��0
    vExecCmdEndSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
}

/*******************************************************************************
 *
 * NAME: ST7032_vSetContrastSSP2
 *
 * DESCRIPTION:LCD Contrast Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        uint8     u8Contrast      R   Display Contrast
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vSetContrastSSP2(uint8 u8Contrast) {
    // ���̓`�F�b�N
    uint8 u8Val = u8Contrast & 0x3F;
    if ((stStateSSP2.u8Settings & 0x3F) == u8Val) {
        return;
    }
    // �ݒ�X�V
    stStateSSP2.u8Settings = (stStateSSP2.u8Settings & 0xC0) | u8Val;
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��1
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // �R���g���X�g�ݒ�i��4���j
    vExecCmdSSP2(ST7032_CMD_CONTRAST_LOW | (stStateSSP2.u8Settings & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // �g���f�B�X�v���C�ݒ�A�A�C�R���\��ON�A�u�[�X�^�[��HOFF�A�R���g���X�g�ݒ�i��2���j
    vExecCmdSSP2(ST7032_CMD_DISP_CNTR_EX | (stStateSSP2.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��0
    vExecCmdEndSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vClearDispSSP1
 *
 * DESCRIPTION:Clear Display
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vClearDispSSP1() {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // LCD�N���A
    vExecCmdEndSSP1(ST7032_CMD_CLEAR_DISP);
    // �J�[�\���ʒu�̏�����
    stStateSSP1.u8CursorPos = 0;
    __delay_us(ST7032_EX_WAIT);
}

/*******************************************************************************
 *
 * NAME: ST7032_vClearDispSSP2
 *
 * DESCRIPTION:Clear Display
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vClearDispSSP2() {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // LCD�N���A
    vExecCmdEndSSP2(ST7032_CMD_CLEAR_DISP);
    // �J�[�\���ʒu�̏�����
    stStateSSP2.u8CursorPos = 0;
    __delay_us(ST7032_EX_WAIT);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vClearIconSSP1
 *
 * DESCRIPTION:Clear Icon
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vClearIconSSP1() {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
	// �C���X�g���N�V�����e�[�u���`�F���W�iIS=1�j
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
	// �R�}���h�̑��M�i�A�C�R���A�h���X�j
    vExecCmdSSP1(ST7032_CMD_SET_ICON_ADDR);
    __delay_us(ST7032_DEF_WAIT);
    // �C���X�g���N�V�����e�[�u���`�F���W�iIS=0�j
    vExecCmdEndSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // �f�[�^�̑��M
    uint8 u8Addr;
    for (u8Addr = 0x00; u8Addr < 0x10; u8Addr++) {
        I2C_u8MstTxSSP1(0x00);
        __delay_us(ST7032_DEF_WAIT);
    }
    // �X�g�b�v�r�b�g;
    I2C_vMstStopSSP1();
    // �J�[�\���A�h���X�֕ϊ�
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �J�[�\���ʒu���Đݒ�
    bSetCursorSSP1(stStateSSP1.u8CursorPos);
}

/*******************************************************************************
 *
 * NAME: ST7032_vClearIconSSP2
 *
 * DESCRIPTION:Clear Icon
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vClearIconSSP2() {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
	// �C���X�g���N�V�����e�[�u���`�F���W�iIS=1�j
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
	// �R�}���h�̑��M�i�A�C�R���A�h���X�j
    vExecCmdSSP2(ST7032_CMD_SET_ICON_ADDR);
    __delay_us(ST7032_DEF_WAIT);
    // �C���X�g���N�V�����e�[�u���`�F���W�iIS=0�j
    vExecCmdEndSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // �f�[�^�̑��M
    uint8 u8Addr;
    for (u8Addr = 0x00; u8Addr < 0x10; u8Addr++) {
        I2C_u8MstTxSSP2(0x00);
        __delay_us(ST7032_DEF_WAIT);
    }
    // �X�g�b�v�r�b�g;
    I2C_vMstStopSSP2();
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �J�[�\���ʒu���Đݒ�
    bSetCursorSSP2(stStateSSP2.u8CursorPos);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vDispIconSSP1
 *
 * DESCRIPTION:Icon Display Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        bool      bDisp           R   �\���ݒ�
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vDispIconSSP1(bool bDisp) {
    // �X�V����
    if ((stStateSSP1.u8Settings >> 7) == bDisp) {
        return;
    }
    // �A�C�R���\���ݒ�̍X�V
    stStateSSP1.u8Settings = (stStateSSP1.u8Settings & 0x7F) | (bDisp << 7);
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��1
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // �g���f�B�X�v���C�ݒ�A�A�C�R���\��ON�A�u�[�X�^�[��HON�A�R���g���X�g�ݒ�i��2���j
    vExecCmdSSP1(ST7032_CMD_DISP_CNTR_EX | (stStateSSP1.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��0
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);    
}

/*******************************************************************************
 *
 * NAME: ST7032_vDispIconSSP2
 *
 * DESCRIPTION:Icon Display Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        bool      bDisp           R   �\���ݒ�
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vDispIconSSP2(bool bDisp) {
    // �X�V����
    if ((stStateSSP2.u8Settings >> 7) == bDisp) {
        return;
    }
    // �A�C�R���\���ݒ�̍X�V
    stStateSSP2.u8Settings = (stStateSSP2.u8Settings & 0x7F) | (bDisp << 7);
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��1
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
    // �g���f�B�X�v���C�ݒ�A�A�C�R���\��ON�A�u�[�X�^�[��HON�A�R���g���X�g�ݒ�i��2���j
    vExecCmdSSP2(ST7032_CMD_DISP_CNTR_EX | (stStateSSP2.u8Settings >> 4));
    __delay_us(ST7032_DEF_WAIT);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��0
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vDispSettingSSP1
 *
 * DESCRIPTION:LCD Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        bool      bDisp           R   Display LCD
 *        bool      bCursor         R   Cursor Display
 *        bool      bBlink          R   Blink Cursor
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vDispSettingSSP1(bool bDisp, bool bCursor, bool bBlink) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �f�B�X�v���C�ݒ�A�\��ON�A�J�[�\���\��OFF�A�J�[�\���_��OFF
    vExecCmdEndSSP1(ST7032_CMD_DISP_CNTR_DEF | (bDisp << 2) | (bCursor << 1) | bBlink);
    __delay_us(ST7032_DEF_WAIT);
}

/*******************************************************************************
 *
 * NAME: ST7032_vDispSettingSSP2
 *
 * DESCRIPTION:LCD Setting
 *
 * PARAMETERS:      Name            RW  Usage
 *        bool      bDisp           R   Display LCD
 *        bool      bCursor         R   Cursor Display
 *        bool      bBlink          R   Blink Cursor
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vDispSettingSSP2(bool bDisp, bool bCursor, bool bBlink) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �f�B�X�v���C�ݒ�A�\��ON�A�J�[�\���\��OFF�A�J�[�\���_��OFF
    vExecCmdEndSSP2(ST7032_CMD_DISP_CNTR_DEF | (bDisp << 2) | (bCursor << 1) | bBlink);
    __delay_us(ST7032_DEF_WAIT);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_u8GetCursorRowNoSSP1
 *
 * DESCRIPTION:get cursor row no
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8 �J�[�\���s
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern uint8 ST7032_u8GetCursorRowNoSSP1() {
    return stStateSSP1.u8CursorPos / 40;
}

/*******************************************************************************
 *
 * NAME: ST7032_u8GetCursorRowNoSSP2
 *
 * DESCRIPTION:get cursor row no
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8 �J�[�\���s
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern uint8 ST7032_u8GetCursorRowNoSSP2() {
    return stStateSSP2.u8CursorPos / 40;
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_u8GetCursorColNoSSP1
 *
 * DESCRIPTION:get cursor column no
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8 �J�[�\����A�h���X
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern uint8 ST7032_u8GetCursorColNoSSP1() {
    return stStateSSP1.u8CursorPos % 40;
}

/*******************************************************************************
 *
 * NAME: ST7032_u8GetCursorColNoSSP2
 *
 * DESCRIPTION:get cursor column no
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *    uint8 �J�[�\����ԍ�
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern uint8 ST7032_u8GetCursorColNoSSP2() {
    return stStateSSP2.u8CursorPos % 40;
}
#endif


/*******************************************************************************
 *
 * NAME: ST7032_bSetCursorSSP1
 *
 * DESCRIPTION:Move cursor
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8RowNo         R   Row No
 *       uint8      u8ColNo         R   Column No
 *
 * RETURNS:
 *     bool true:�ݒ萬���Afalse:�ݒ莸�s
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern bool ST7032_bSetCursorSSP1(uint8 u8RowNo, uint8 u8ColNo) {
    // �ʒu����
    if (u8RowNo > ST7032_ROW_MAX || u8ColNo > ST7032_COL_MAX) {
        return false;
    }
    // �J�[�\���ړ�����
    uint8 u8Addr = (u8RowNo * 40) + u8ColNo;
    if (u8Addr == stStateSSP1.u8CursorPos) {
        return true;
    }
    // �J�[�\���ړ�
    return bSetCursorSSP1(u8Addr);
}

/*******************************************************************************
 *
 * NAME: ST7032_bSetCursorSSP2
 *
 * DESCRIPTION:Move cursor
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8RowNo         R   Row No
 *       uint8      u8ColNo         R   Column No
 *
 * RETURNS:
 *     bool true:�ݒ萬���Afalse:�ݒ莸�s
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern bool ST7032_bSetCursorSSP2(uint8 u8RowNo, uint8 u8ColNo) {
    // �ʒu����
    if (u8RowNo > ST7032_ROW_MAX || u8ColNo > ST7032_COL_MAX) {
        return false;
    }
    // �J�[�\���ړ�����
    uint8 u8Addr = (u8RowNo * 40) + u8ColNo;
    if (u8Addr == stStateSSP2.u8CursorPos) {
        return true;
    }
    // �J�[�\���ړ�
    return bSetCursorSSP2(u8Addr);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_bCursorLeftSSP1
 *
 * DESCRIPTION:Move cursor to left
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     bool true:�ݒ萬���Afalse:�ݒ莸�s
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern bool ST7032_bCursorLeftSSP1() {
    // �J�[�\���ʒu����
    if (stStateSSP1.u8CursorPos == 0) {
        return false;
    }
    return bSetCursorSSP1(stStateSSP1.u8CursorPos - 1);
}

/*******************************************************************************
 *
 * NAME: ST7032_bCursorLeftSSP2
 *
 * DESCRIPTION:Move cursor to left
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     bool true:�ݒ萬���Afalse:�ݒ莸�s
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern bool ST7032_bCursorLeftSSP2() {
    // �J�[�\���ʒu����
    if (stStateSSP2.u8CursorPos == 0) {
        return false;
    }
    return bSetCursorSSP2(stStateSSP2.u8CursorPos - 1);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_bCursorRightSSP1
 *
 * DESCRIPTION:Move cursor to right
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     bool true:�ݒ萬���Afalse:�ݒ莸�s
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern bool ST7032_bCursorRightSSP1() {
    // �J�[�\���ʒu����
    if (stStateSSP1.u8CursorPos >= 79) {
        return false;
    }
    return bSetCursorSSP1(stStateSSP1.u8CursorPos + 1);
}

/*******************************************************************************
 *
 * NAME: ST7032_bCursorRightSSP2
 *
 * DESCRIPTION:Move cursor to right
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *     bool true:�ݒ萬���Afalse:�ݒ莸�s
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern bool ST7032_bCursorRightSSP2() {
    // �J�[�\���ʒu����
    if (stStateSSP2.u8CursorPos >= 79) {
        return false;
    }
    return bSetCursorSSP2(stStateSSP2.u8CursorPos + 1);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vWriteCGRAMSSP1
 *
 * DESCRIPTION:Write CGRAM
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8CharNo        R   Character No
 *       uint8*     pu8BitMap       R   Character Bit Map
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vWriteCGRAMSSP1(uint8 u8CharNo, uint8* pu8BitMap) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��0
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // CGRAM�A�h���X�ݒ�
    vExecCmdSSP1(ST7032_CMD_SET_CGRAM | ((u8CharNo << 3) & 0x38));
    __delay_us(ST7032_DEF_WAIT);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // CGRAM�f�[�^�̑��M
    uint8 *pu8WkMap = pu8BitMap;
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 8; u8Idx++) {
        I2C_u8MstTxSSP1(*pu8WkMap & 0x1F);
        __delay_us(ST7032_DEF_WAIT);
        pu8WkMap++;
    }
    // �X�g�b�v�R���f�B�V����
    I2C_vMstStopSSP1();
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �J�[�\���Đݒ�
    bSetCursorSSP1(stStateSSP1.u8CursorPos);
}

/*******************************************************************************
 *
 * NAME: ST7032_vWriteCGRAMSSP2
 *
 * DESCRIPTION:Write CGRAM
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8CharNo        R   Character No
 *       uint8*     pu8BitMap       R   Character Bit Map
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vWriteCGRAMSSP2(uint8 u8CharNo, uint8* pu8BitMap) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �t�@���N�V�����ݒ�@IS(instruction table select)��0
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // CGRAM�A�h���X�ݒ�
    vExecCmdSSP2(ST7032_CMD_SET_CGRAM | ((u8CharNo << 3) & 0x38));
    __delay_us(ST7032_DEF_WAIT);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // CGRAM�f�[�^�̑��M
    uint8 *pu8WkMap = pu8BitMap;
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < 8; u8Idx++) {
        I2C_u8MstTxSSP2(*pu8WkMap & 0x1F);
        __delay_us(ST7032_DEF_WAIT);
        pu8WkMap++;
    }
    // �X�g�b�v�R���f�B�V����
    I2C_vMstStopSSP2();
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �J�[�\���Đݒ�
    bSetCursorSSP2(stStateSSP2.u8CursorPos);
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vWriteCharSSP1
 *
 * DESCRIPTION:Write Character
 *
 * PARAMETERS:      Name            RW  Usage
 *        char      cData           R   �C���X�g���N�V����
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vWriteCharSSP1(char cData) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // �f�[�^�̑��M
    I2C_u8MstTxSSP1(cData);
    // �X�g�b�v�R���f�B�V����
    I2C_vMstStopSSP1();
    // �J�[�\���ʒu�̈ړ�
    stStateSSP1.u8CursorPos = (stStateSSP1.u8CursorPos + 1) % 80;
}

/*******************************************************************************
 *
 * NAME: ST7032_vWriteCharSSP2
 *
 * DESCRIPTION:Write Character
 *
 * PARAMETERS:      Name            RW  Usage
 *        char      cData           R   �C���X�g���N�V����
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vWriteCharSSP2(char cData) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // �f�[�^�̑��M
    I2C_u8MstTxSSP2(cData);
    // �X�g�b�v�R���f�B�V����
    I2C_vMstStopSSP2();
    // �J�[�\���ʒu�̈ړ�
    stStateSSP2.u8CursorPos = (stStateSSP2.u8CursorPos + 1) % 80;
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vWriteStringSSP1
 *
 * DESCRIPTION:Write String
 *
 * PARAMETERS:      Name            RW  Usage
 *       char*      pcStr           R   ������̃|�C���^
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vWriteStringSSP1(char* pcStr) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // �f�[�^�̑��M
    char* pcWkData = pcStr;
    while (*pcWkData != '\0') {
        I2C_u8MstTxSSP1(*pcWkData);
        __delay_us(ST7032_DEF_WAIT);
        pcWkData++;
    }
    // �X�g�b�v�R���f�B�V����
    I2C_vMstStopSSP1();
    // �J�[�\���ʒu�̈ړ�
    stStateSSP1.u8CursorPos =
            (stStateSSP1.u8CursorPos + (pcWkData - pcStr)) % 80;
}

/*******************************************************************************
 *
 * NAME: ST7032_vWriteStringSSP2
 *
 * DESCRIPTION:Write String
 *
 * PARAMETERS:      Name            RW  Usage
 *        char      pcStr           R   ������̃|�C���^
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vWriteStringSSP2(char* pcStr) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // �f�[�^�̑��M
    char* pcWkData = pcStr;
    while (*pcWkData != '\0') {
        I2C_u8MstTxSSP2(*pcWkData);
        __delay_us(ST7032_DEF_WAIT);
        pcWkData++;
    }
    // �X�g�b�v�R���f�B�V����
    I2C_vMstStopSSP2();    
    // �J�[�\���ʒu�̈ړ�
    stStateSSP2.u8CursorPos =
            (stStateSSP2.u8CursorPos + (pcWkData - pcStr)) % 80;
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vWriteDataSSP1
 *
 * DESCRIPTION:Write Data
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8*      pcData          R   �f�[�^�̃|�C���^
 *      uint8       u8Len           R   �z��T�C�Y 
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vWriteDataSSP1(uint8* pcData, uint8 u8Len) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // �f�[�^�̑��M
    uint8* pu8WkData = pcData;
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < u8Len; u8Idx++) {
        I2C_u8MstTxSSP1(*pu8WkData);
        __delay_us(ST7032_DEF_WAIT);
        pu8WkData++;
    }
    // �X�g�b�v�R���f�B�V����
    I2C_vMstStopSSP1();
    // �J�[�\���ʒu�̈ړ�
    stStateSSP1.u8CursorPos =
            (stStateSSP1.u8CursorPos + (pu8WkData - pcData)) % 80;
}

/*******************************************************************************
 *
 * NAME: ST7032_vWriteDataSSP2
 *
 * DESCRIPTION:Write Data
 *
 * PARAMETERS:      Name            RW  Usage
 *      uint8*      pcData          R   �f�[�^�̃|�C���^
 *      uint8       u8Len           R   �z��T�C�Y 
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vWriteDataSSP2(uint8* pcData, uint8 u8Len) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // �f�[�^�̑��M
    uint8* pu8WkData = pcData;
    uint8 u8Idx;
    for (u8Idx = 0; u8Idx < u8Len; u8Idx++) {
        I2C_u8MstTxSSP2(*pu8WkData);
        __delay_us(ST7032_DEF_WAIT);
        pu8WkData++;
    }
    // �X�g�b�v�R���f�B�V����
    I2C_vMstStopSSP2();
    // �J�[�\���ʒu�̈ړ�
    stStateSSP2.u8CursorPos =
            (stStateSSP2.u8CursorPos + (pu8WkData - pcData)) % 80;
}
#endif

/*******************************************************************************
 *
 * NAME: ST7032_vWriteIconSSP1
 *
 * DESCRIPTION:�A�C�R���̏�������
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Addr          R   ICON Address
 *       uint8      u8Map           R   ICON Bit Map
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
extern void ST7032_vWriteIconSSP1(uint8 u8Addr, uint8 u8Map) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
	// �C���X�g���N�V�����e�[�u���`�F���W�iIS=1�j
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
	// �R�}���h�̑��M�i�A�C�R���A�h���X�j
    vExecCmdSSP1(ST7032_CMD_SET_ICON_ADDR | (u8Addr & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // �C���X�g���N�V�����e�[�u���`�F���W�iIS=0�j
    vExecCmdSSP1(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP1(ST7032_CNTR_DATA);
    // �f�[�^�̑��M
    I2C_u8MstTxSSP1(u8Map & 0x1F);
    // �X�g�b�v�r�b�g;
    I2C_vMstStopSSP1();
    __delay_us(ST7032_DEF_WAIT);
    // �J�[�\���ʒu��߂�
    bSetCursorSSP1(stStateSSP1.u8CursorPos);
}

/*******************************************************************************
 *
 * NAME: ST7032_vWriteIconSSP2
 *
 * DESCRIPTION:�A�C�R���̏�������
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Addr          R   ICON Address
 *       uint8      u8Map           R   ICON Bit Map
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
extern void ST7032_vWriteIconSSP2(uint8 u8Addr, uint8 u8Map) {
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
	// �C���X�g���N�V�����e�[�u���`�F���W�iIS=1�j
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_EX);
    __delay_us(ST7032_DEF_WAIT);
	// �R�}���h�̑��M�i�A�C�R���A�h���X�j
    vExecCmdSSP2(ST7032_CMD_SET_ICON_ADDR | (u8Addr & 0x0F));
    __delay_us(ST7032_DEF_WAIT);
    // �C���X�g���N�V�����e�[�u���`�F���W�iIS=0�j
    vExecCmdSSP2(ST7032_CMD_FUNC_SET_DEF);
    __delay_us(ST7032_DEF_WAIT);
    // �R���g���[���o�C�g�i�f�[�^�j�̑��M
    I2C_u8MstTxSSP2(ST7032_CNTR_DATA);
    // �f�[�^�̑��M
    I2C_u8MstTxSSP2(u8Map & 0x1F);
    // �X�g�b�v�r�b�g;
    I2C_vMstStopSSP2();
    __delay_us(ST7032_DEF_WAIT);
    // �J�[�\���ʒu��߂�
    bSetCursorSSP2(stStateSSP2.u8CursorPos);
}
#endif

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: vExecCmdSSP1
 *
 * DESCRIPTION:LCD�C���X�g���N�V�����̎��s
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Cmd           R   �C���X�g���N�V����      
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
static void vExecCmdSSP1(uint8 u8Cmd) {
    // �R���g���[���o�C�g�i�R�}���h�j�̑��M
    I2C_u8MstTxSSP1(ST7032_CNTR_CMD | 0x80);
    // �R�}���h�̑��M
    I2C_u8MstTxSSP1(u8Cmd);
}

/*******************************************************************************
 *
 * NAME: vExecCmdSSP2
 *
 * DESCRIPTION:LCD�C���X�g���N�V�����̎��s
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Cmd           R   �C���X�g���N�V����      
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
static void vExecCmdSSP2(uint8 u8Cmd) {
    // �R���g���[���o�C�g�i�R�}���h�j�̑��M
    I2C_u8MstTxSSP2(ST7032_CNTR_CMD | 0x80);
    // �R�}���h�̑��M
    I2C_u8MstTxSSP2(u8Cmd);
}
#endif

/*******************************************************************************
 *
 * NAME: vExecCmdEndSSP1
 *
 * DESCRIPTION:Execute Command and Stop End
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Cmd           R   �C���X�g���N�V����      
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
static void vExecCmdEndSSP1(uint8 u8Cmd) {
    // �R���g���[���o�C�g�i�R�}���h�j�̑��M
    I2C_u8MstTxSSP1(ST7032_CNTR_CMD);
    // �R�}���h�̑��M
    I2C_u8MstTxSSP1(u8Cmd);
    // �X�g�b�v�R���f�B�V����
    I2C_vMstStopSSP1();
}

/*******************************************************************************
 *
 * NAME: vExecCmdEndSSP2
 *
 * DESCRIPTION:Execute Command and Stop End
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Cmd           R   �C���X�g���N�V����      
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
static void vExecCmdEndSSP2(uint8 u8Cmd) {
    // �R���g���[���o�C�g�i�R�}���h�j�̑��M
    I2C_u8MstTxSSP2(ST7032_CNTR_CMD);
    // �R�}���h�̑��M
    I2C_u8MstTxSSP2(u8Cmd);
    // �X�g�b�v�R���f�B�V����
    I2C_vMstStopSSP2();
}
#endif

/*******************************************************************************
 *
 * NAME: bSetCursorSSP1
 *
 * DESCRIPTION:Move cursor
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Pos           R   Cursor Position
 *
 * RETURNS:
 *     bool true:�ݒ萬���Afalse:�ݒ莸�s
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
static bool bSetCursorSSP1(uint8 u8Pos) {
    // �J�[�\���ݒ�̉ۂ𔻒�
    stStateSSP1.u8CursorPos = u8Pos;
    // �J�[�\���A�h���X�֕ϊ�
    uint8 u8Addr = (u8Pos / 40) * 0x40 + (u8Pos % 40);
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP1(ST7032_I2C_ADDR, false);
    // �J�[�\���ݒ�
    vExecCmdEndSSP1(ST7032_CMD_SET_DD_ADDR | (u8Addr & 0x7F));
    __delay_us(ST7032_DEF_WAIT);
    // �ݒ萬��
    return true;
}

/*******************************************************************************
 *
 * NAME: bSetCursorSSP2
 *
 * DESCRIPTION:Move cursor
 *
 * PARAMETERS:      Name            RW  Usage
 *       uint8      u8Pos           R   Cursor Position
 *
 * RETURNS:
 *     bool true:�ݒ萬���Afalse:�ݒ莸�s
 *
 * NOTES:
 * None.
 * 
 ******************************************************************************/
#ifdef SSP2STAT
static bool bSetCursorSSP2(uint8 u8Pos) {
    // �J�[�\���ݒ�̉ۂ𔻒�
    stStateSSP2.u8CursorPos = u8Pos;
    // �J�[�\���A�h���X�֕ϊ�
    uint8 u8Addr = (u8Pos / 40) * 0x40 + (u8Pos % 40);
    // �X�^�[�g�R���f�B�V�����̑��M
    I2C_u8MstStartSSP2(ST7032_I2C_ADDR, false);
    // �J�[�\���ݒ�
    vExecCmdEndSSP2(ST7032_CMD_SET_DD_ADDR | (u8Addr & 0x7F));
    __delay_us(ST7032_DEF_WAIT);
    // �ݒ萬��
    return true;    
}
#endif

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
