/*******************************************************************************
 *
 * MODULE :ST7032 Driver functions header file
 *
 * CREATED:2019/03/17 16:16:00
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
#ifndef ST7032_H
#define	ST7032_H


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
/** コントラスト最大値 */
#define ST7032_CONTRAST_MAX     (63)
#define ST7032_ROW_MAX          (1)
#define ST7032_COL_MAX          (39)

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/
// LCD Initialize
extern void ST7032_vInitSSP1();
#ifdef SSP2STAT
extern void ST7032_vInitSSP2();
#endif

// LCD Setting
extern void ST7032_vDispSettingSSP1(bool bDisp, bool bCursor, bool bBlink);
#ifdef SSP2STAT
extern void ST7032_vDispSettingSSP2(bool bDisp, bool bCursor, bool bBlink);
#endif

// Icon Display Setting
extern void ST7032_vDispIconSSP1(bool bDisp);
#ifdef SSP2STAT
extern void ST7032_vDispIconSSP2(bool bDisp);
#endif

// LCD Contrast
extern void ST7032_vSetContrastSSP1(uint8 u8Contrast);
#ifdef SSP2STAT
extern void ST7032_vSetContrastSSP2(uint8 u8Contrast);
#endif

// Clear Display
extern void ST7032_vClearDispSSP1();
#ifdef SSP2STAT
extern void ST7032_vClearDispSSP2();
#endif

// Clear Icon
extern void ST7032_vClearIconSSP1();
#ifdef SSP2STAT
extern void ST7032_vClearIconSSP2();
#endif

// Get cursor row no
extern uint8 ST7032_u8GetCursorRowNoSSP1();
#ifdef SSP2STAT
extern uint8 ST7032_u8GetCursorRowNoSSP2();
#endif

// Get cursor column no
extern uint8 ST7032_u8GetCursorColNoSSP1();
#ifdef SSP2STAT
extern uint8 ST7032_u8GetCursorColNoSSP2();
#endif

// Move cursor
extern bool ST7032_bSetCursorSSP1(uint8 u8RowNo, uint8 u8ColNo);
#ifdef SSP2STAT
extern bool ST7032_bSetCursorSSP2(uint8 u8RowNo, uint8 u8ColNo);
#endif

// Move cursor to left
extern bool ST7032_bCursorLeftSSP1();
#ifdef SSP2STAT
extern bool ST7032_bCursorLeftSSP2();
#endif

// Move cursor to right
extern bool ST7032_bCursorRightSSP1();
#ifdef SSP2STAT
extern bool ST7032_bCursorRightSSP2();
#endif

// Move cursor to top
#define ST7032_vCursorTopSSP1() ST7032_bSetCursorSSP1(0, 0)
#ifdef SSP2STAT
#define ST7032_vCursorTopSSP2() ST7032_bSetCursorSSP2(0, 0)
#endif

// Write CGRAM
extern void ST7032_vWriteCGRAMSSP1(uint8 u8CharNo, uint8* pu8BitMap);
#ifdef SSP2STAT
extern void ST7032_vWriteCGRAMSSP2(uint8 u8CharNo, uint8* pu8BitMap);
#endif

// Write Character
extern void ST7032_vWriteCharSSP1(char cData);
#ifdef SSP2STAT
extern void ST7032_vWriteCharSSP2(char cData);
#endif

// Write String
extern void ST7032_vWriteStringSSP1(char* pcStr);
#ifdef SSP2STAT
extern void ST7032_vWriteStringSSP2(char* pcStr);
#endif

// Write Data
extern void ST7032_vWriteDataSSP1(uint8* pu8Data, uint8 u8Len);
#ifdef SSP2STAT
extern void ST7032_vWriteDataSSP2(uint8* pu8Data, uint8 u8Len);
#endif

// Write ICON
extern void ST7032_vWriteIconSSP1(uint8 u8Addr, uint8 u8Map);
#ifdef SSP2STAT
extern void ST7032_vWriteIconSSP2(uint8 u8Addr, uint8 u8Map);
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* ST7032_H */

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/
