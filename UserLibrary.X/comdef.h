/*******************************************************************************
 *
 * MODULE :Common definition header file
 *
 * CREATED:2019/03/02 06:51:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:Definition of basic variable types
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

#ifndef COMDEF_H
#define	COMDEF_H

#ifdef	__cplusplus
extern "C" {
#endif

/******************************************************************************/
/***        Include files                                                   ***/
/******************************************************************************/
#include <xc.h>

/******************************************************************************/
/***        Macro Definitions                                               ***/
/******************************************************************************/

// 基本データ型の定義
#define bool   unsigned char
#define int8   char
#define int16  int
#define int32  long
#define uint8  unsigned char
#define uint16 unsigned int
#define uint32 unsigned long

// ブール値
#define true  (1)
#define false (0) 
// ON/OFF
#define ON  (1)
#define OFF (0) 

    // ポートの識別子
#ifdef PORTA
#define ID_PORTA  (0x0000)
#endif
#ifdef PORTB
#define ID_PORTB  (0x0100)
#endif
#ifdef PORTC
#define ID_PORTC  (0x0200)
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* COMDEF_H */

