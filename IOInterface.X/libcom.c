/*******************************************************************************
 *
 * MODULE :Library common definition source file
 *
 * CREATED:2019/03/17 03:27:00
 * AUTHOR :Nakanohito
 *
 * DESCRIPTION:Microchip PIC Library Common definition 
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
#include "libcom.h"           // ���ʒ�`

/******************************************************************************/
/***       Macro Definitions                                                ***/
/******************************************************************************/

/******************************************************************************/
/***        Type Definitions                                                ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Function Prototypes                                       ***/
/******************************************************************************/

/******************************************************************************/
/***        Exported Variables                                              ***/
/******************************************************************************/

/******************************************************************************/
/***        Local Variables                                                 ***/
/******************************************************************************/
/** �N���e�B�J���Z�N�V�����̊K�w�J�E���^ */
static volatile uint8 u8Depth = 0;

/******************************************************************************/
/***        Exported Functions                                              ***/
/******************************************************************************/

/*******************************************************************************
 *
 * NAME: criticalSec_vBegin
 *
 * DESCRIPTION:�N���e�B�J���Z�N�V�����̊J�n
 *
 * PARAMETERS:  Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern void criticalSec_vBegin() {
    GIE = 0;
    u8Depth++;
}

/*******************************************************************************
 *
 * NAME: criticalSec_vEnd
 *
 * DESCRIPTION:�N���e�B�J���Z�N�V�����̏I��
 *
 * PARAMETERS:  Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 * None.
 ******************************************************************************/
extern void criticalSec_vEnd() {
    u8Depth--;
    if (u8Depth == 0) {
        GIE = 1;
    }
}

/******************************************************************************/
/***        Local Functions                                                 ***/
/******************************************************************************/

/******************************************************************************/
/***        END OF FILE                                                     ***/
/******************************************************************************/