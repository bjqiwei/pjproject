/*--------------------------------------------------------------------------------------------------------------------
   (C) Copyright 2006, 2007 Marvell DSPC Ltd. All Rights Reserved.
   -------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------
 *  INTEL CONFIDENTIAL
 *  Copyright 2006 Intel Corporation All Rights Reserved.
 *  The source code contained or described herein and all documents related to the source code (Material are owned
 *  by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or
 *  its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of
 *  Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and
 *  treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted,
 *  transmitted, distributed, or disclosed in any way without Intels prior express written permission.
 *
 *  No license under any patent, copyright, trade secret or other intellectual property right is granted to or
 *  conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement,
 *  estoppel or otherwise. Any license under such intellectual property rights must be express and approved by
 *  Intel in writing.
 *  -------------------------------------------------------------------------------------------------------------------
 *
 *  Filename: telatparamdef_ubus.h
 *
 *  Authors:  tzahi stern
 *
 *  Description: Defines for AT Command Parameters that use ubus i/f
 *               these defines are in seperated header file than telatparamdef.h to make sync simpler
 *
 *  History:
 *   June 24, 2006 - Creation of file
 *
 *  Notes:
 *
 ******************************************************************************/



#ifndef TELATPARAMDEF_UBUS_H
#define TELATPARAMDEF_UBUS_H

#ifndef NO_AUDIO
/* AT*AUDIOMODE */
#define TEL_AT_AUDIO_MODE_VAL_MIN                0
#define TEL_AT_AUDIO_MODE_VAL_MAX                4
#define TEL_AT_AUDIO_MODE_VAL_DEFAULT            0

/* AT*AUDIODEVICE */
#define TEL_AT_AUDIO_DEVICE_VAL_MIN                0
#define TEL_AT_AUDIO_DEVICE_VAL_MAX                2
#define TEL_AT_AUDIO_DEVICE_VAL_DEFAULT            0

/* AT*AUDIOVOL */
#define TEL_AT_AUDIO_VOL_VAL_MIN               0
#define TEL_AT_AUDIO_VOL_VAL_MAX               100
#define TEL_AT_AUDIO_VOL_VAL_DEFAULT           80

/* AT*AUDIOMUTE */
#define TEL_AT_AUDIO_MUTE_VAL_MIN              0
#define TEL_AT_AUDIO_MUTE_VAL_MAX              1
#define TEL_AT_AUDIO_MUTE_VAL_DEFAULT          0

//ECALLDATA
#define TEL_AT_ECALLDATA_OP_VAL_MIN                     (0)
#define TEL_AT_ECALLDATA_OP_VAL_MAX                     (7)
#define TEL_AT_ECALLDATA_OP_VAL_DEFAULT                 (TEL_AT_ECALLDATA_OP_VAL_MAX+1)

//ECALLDATA OP=1
#define TEL_AT_ECALLDATA_OP1_PULLPUSH_MODE_STR_DEFAULT  "\0"
#define TEL_AT_ECALLDATA_OP1_PULLPUSH_MODE_STR_MIN_LEN  (1)
#define TEL_AT_ECALLDATA_OP1_PULLPUSH_MODE_STR_MAX_LEN  (1)
#define TEL_AT_ECALLDATA_OP1_PULLPUSH_MODE_VAL_MIN      (0)
#define TEL_AT_ECALLDATA_OP1_PULLPUSH_MODE_VAL_MAX      (1)

#define TEL_AT_ECALLDATA_OP1_DATA_STR_DEFAULT           "\0"
#define TEL_AT_ECALLDATA_OP1_DATA_STR_MIN_LEN           (280)
#define TEL_AT_ECALLDATA_OP1_DATA_STR_MAX_LEN           (280)

//ECALLDATA OP=2
#define TEL_AT_ECALLDATA_OP2_UPDATE_MODE_STR_DEFAULT    "\0"
#define TEL_AT_ECALLDATA_OP2_UPDATE_MODE_STR_MIN_LEN    (1)
#define TEL_AT_ECALLDATA_OP2_UPDATE_MODE_STR_MAX_LEN    (1)
#define TEL_AT_ECALLDATA_OP2_UPDATE_MODE_VAL_MIN        (0)
#define TEL_AT_ECALLDATA_OP2_UPDATE_MODE_VAL_MAX        (1)

#define TEL_AT_ECALLDATA_OP2_DATA_STR_DEFAULT           "\0"
#define TEL_AT_ECALLDATA_OP2_DATA_STR_MIN_LEN           (280)
#define TEL_AT_ECALLDATA_OP2_DATA_STR_MAX_LEN           (280)

//ECALLDATA OP=3
#define TEL_AT_ECALLDATA_OP3_URC_MODE_STR_DEFAULT       "\0"
#define TEL_AT_ECALLDATA_OP3_URC_MODE_STR_MIN_LEN       (8)
#define TEL_AT_ECALLDATA_OP3_URC_MODE_STR_MAX_LEN       (8)

//ECALLDATA OP=4
#define TEL_AT_ECALLDATA_OP4_ENABLE_STR_DEFAULT         "\0"
#define TEL_AT_ECALLDATA_OP4_ENABLE_STR_MIN_LEN         (1)
#define TEL_AT_ECALLDATA_OP4_ENABLE_STR_MAX_LEN         (1)
#define TEL_AT_ECALLDATA_OP4_ENABLE_VAL_MIN             (0)
#define TEL_AT_ECALLDATA_OP4_ENABLE_VAL_MAX             (1)

//ECALLDATA OP=5
#define TEL_AT_ECALLDATA_OP5_TIMER_STR_DEFAULT          "\0"
#define TEL_AT_ECALLDATA_OP5_TIMER_STR_MIN_LEN          (1)
#define TEL_AT_ECALLDATA_OP5_TIMER_STR_MAX_LEN          (1)
#define TEL_AT_ECALLDATA_OP5_TIMER_VAL_MIN              (0)
#define TEL_AT_ECALLDATA_OP5_TIMER_VAL_MAX              (6)

#define TEL_AT_ECALLDATA_OP5_DATA_STR_DEFAULT           "\0"
#define TEL_AT_ECALLDATA_OP5_DATA_STR_MIN_LEN           (1)
#define TEL_AT_ECALLDATA_OP5_DATA_STR_MAX_LEN           (5)     //65535=0xFFFF
#define TEL_AT_ECALLDATA_OP5_DATA_VAL_MIN               (1)
#define TEL_AT_ECALLDATA_OP5_DATA_VAL_MAX               (65535)

//ECALLDATA OP=6
#define TEL_AT_ECALLDATA_OP6_MSD_DATA_SRC_STR_DEFAULT   "\0"
#define TEL_AT_ECALLDATA_OP6_MSD_DATA_SRC_STR_MIN_LEN   (1)
#define TEL_AT_ECALLDATA_OP6_MSD_DATA_SRC_STR_MAX_LEN   (1)
#define TEL_AT_ECALLDATA_OP6_MSD_DATA_SRC_VAL_MIN       (0)
#define TEL_AT_ECALLDATA_OP6_MSD_DATA_SRC_VAL_MAX       (1)

//ECALLVOICE
#define TEL_AT_ECALLVOICE_CMDID_VAL_MIN         0
#define TEL_AT_ECALLVOICE_CMDID_VAL_MAX         4
#define TEL_AT_ECALLVOICE_CMDID_VAL_DEFAULT     0

#define TEL_AT_ECALLVOICE_RESID_VAL_MIN         0
#define TEL_AT_ECALLVOICE_RESID_VAL_MAX         1
#define TEL_AT_ECALLVOICE_RESID_VAL_DEFAULT     0

#define TEL_AT_ECALLVOICE_PARAM2_VAL_MIN        0
#define TEL_AT_ECALLVOICE_PARAM2_VAL_MAX        1
#define TEL_AT_ECALLVOICE_PARAM2_VAL_DEFAULT    0

#endif // NO_AUDIO

#endif
/* END OF FILE */
