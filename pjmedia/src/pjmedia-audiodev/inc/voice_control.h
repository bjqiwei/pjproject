/*
* All Rights Reserved
*
* MARVELL CONFIDENTIAL
* Copyright 2012 Marvell International Ltd All Rights Reserved.
* The source code contained or described herein and all documents related to
* the source code ("Material") are owned by Marvell International Ltd or its
* suppliers or licensors. Title to the Material remains with Marvell International Ltd
* or its suppliers and licensors. The Material contains trade secrets and
* proprietary and confidential information of Marvell or its suppliers and
* licensors. The Material is protected by worldwide copyright and trade secret
* laws and treaty provisions. No part of the Material may be used, copied,
* reproduced, modified, published, uploaded, posted, transmitted, distributed,
* or disclosed in any way without Marvell's prior express written permission.
*
* No license under any patent, copyright, trade secret or other intellectual
* property right is granted to or conferred upon you by disclosure or delivery
* of the Materials, either expressly, by implication, inducement, estoppel or
* otherwise. Any license under such intellectual property rights must be
* express and approved by Marvell in writing.
*
*/

#ifndef __VOICE_CONTROL_MRVL_H__
#define __VOICE_CONTROL_MRVL_H__

#include "vcm.h"

#define VCM_EXTRA_VOL       0x00000001
#define VCM_BT_NREC_OFF     0x00000002
#define VCM_BT_WB           0x00000004
#define VCM_TTY_FULL        0x00000008
#define VCM_TTY_HCO         0x00000010
#define VCM_TTY_VCO         0x00000020
#define VCM_TTY_VCO_DUALMIC 0x00000040
#define VCM_DUAL_MIC        0x00000080

#define PCM_WB_BUF_SIZE     640
#define PCM_NARROW_BUF_SIZE 320

#define CONFIG_DSPGAIN_DOMAIN_MIN  (-36)
#define CONFIG_DSPGAIN_DOMAIN_MAX  (12)
#define CONFIG_DSPGAIN_SIDETONE_DISABLE  (-128)
#define CONFIG_DSPGAIN_MUTE_ON  (-100)
#define CONFIG_DSPGAIN_MUTE_OFF  (100)

typedef enum
{
	CONFIG_DSPGAIN_TX = 0, /* TX: near to far end */
	CONFIG_DSPGAIN_RX, /* RX: far to near end */
	CONFIG_DSPGAIN_SIDETONE,
	CONFIG_DSPGAIN_NUMBER = CONFIG_DSPGAIN_SIDETONE
} CONFIG_DSPGAIN_Direction;

typedef enum
{
	F697 = 0,
	F770,
	F852,
	F941,
	F1209,
	F1336,
	F1477,
	F1633,
	F450,
	F440,
	F480,
	NUM_OF_FREQUENCY,
} CONFIG_DTMFCONTROL_INDEX;

struct vcm_config 
{    
    VCM_StreamType      type;    
    VCM_SrcDst          srcdst;    
    VCM_CombWithCall    priority;  
    unsigned int        dest;
};


// definition of Voice Call Control interface
void vcm_select_path(unsigned int out_device, unsigned int in_device, unsigned int params);
int vcm_recording_start(struct vcm_config *config);
int vcm_recording_stop(void);
int vcm_recording_read(void *buffer, unsigned int bytes);
int vcm_playback_start(struct vcm_config *config);
int vcm_playback_stop(void);
int vcm_playback_write(const void *buffer, unsigned int bytes);
void vcm_setvolume(signed char input_gain, signed char input_gain_wb,
        signed char output_gain, signed char output_gain_wb, signed char sidetone_gain, signed char sidetone_gain_wb, unsigned char volume);
void vcm_mute_mic(bool mute_on, unsigned char ramp_level);
void vcm_mute_all(bool mute_on, unsigned char ramp_level);
void vcm_set_loopback(unsigned int out_device, bool loopback_on);
void vcm_set_user_eq(char *value, unsigned int len);
int vcm_ctl_read(void *buffer, unsigned int bytes);
int vcm_ecall(void *value);
int vcm_playback_drain(unsigned short timeoutSec);

#endif
