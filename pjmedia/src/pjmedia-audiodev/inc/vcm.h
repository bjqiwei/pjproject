/*--------------------------------------------------------------------------------------------------------------------
 (C) Copyright 2006, 2007 Marvell DSPC Ltd. All Rights Reserved.
 -------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------
 MARVELL CONFIDENTIAL
 Copyright 2006 Marvell Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code (?Material? are owned
 by MARVELL Corporation or its suppliers or licensors. Title to the Material remains with MARVELL Corporation or
 its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of
 MARVELL or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and
 treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted,
 transmitted, distributed, or disclosed in any way without marvell?s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or
 conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement,
 estoppel or otherwise. Any license under such intellectual property rights must be express and approved by
 MARVELL in writing.
 -------------------------------------------------------------------------------------------------------------------*/

/******************************************************************************
 *               MODULE IMPLEMENTATION FILE
 *******************************************************************************
 * Title: Voice Call Manager (VCM)
 *
 * Filename: vcm.h
 *
 * Authors: APSE
 *
 * Description: Header file for VCM.
 *
 * Last Updated :
 *
 * Notes:
 ******************************************************************************/
#ifndef _VCM_H_
#define _VCM_H_

typedef unsigned char VCM_MSAGain;
typedef unsigned char VCM_RampLevel;
typedef unsigned int VCM_MiscParam;
typedef unsigned int VCM_StreamID;

typedef enum
{
	VCM_PARAMSET_PKTSIZE,
	VCM_PARAMSET_TX_THRESHOLD,
	VCM_PARAMSET_CALLSTART,
	VCM_PARAMSET_EQ
} VCM_ParameterID;

typedef enum
{
	/* first device must be '0' - used by 'for' loops */
	VCM_PROFILE_DEFAULT = 0,
	VCM_PROFILE_HANDSET,
	VCM_PROFILE_HEADSET,
	VCM_PROFILE_HANDSFREE,
	VCM_PROFILE_BLUETOOTH,
	VCM_PROFILE_STEREO_BT,
	VCM_PROFILE_SPEAKERPHONE,
	VCM_PROFILE_HEADPHONE,
	VCM_PROFILE_BT_NREC_OFF,
	VCM_PROFILE_BLUETOOTH_WB,
	VCM_PROFILE_BT_NREC_OFF_WB,
	VCM_PROFILE_HANDSET_DUALMIC,
	VCM_PROFILE_HEADSET_DUALMIC,
	VCM_PROFILE_HANDSFREE_DUALMIC,
	VCM_PROFILE_HANDSET_EXTRAVOLUME,
	VCM_PROFILE_HANDSFREE_EXTRAVOLUME,
	VCM_PROFILE_HANDSET_DUALMIC_EXTRAVOLUME,
	VCM_PROFILE_HANDSFREE_DUALMIC_EXTRAVOLUME,

	VCM_PROFILE_TTY,
	VCM_PROFILE_TTY_HCO,
	VCM_PROFILE_TTY_VCO,
	VCM_PROFILE_TTY_VCO_DUALMIC,

	VCM_PROFILE_HANDSET_LOOP,
	VCM_PROFILE_HEADSET_LOOP,
	VCM_PROFILE_HANDSFREE_LOOP,
	VCM_PROFILE_BLUETOOTH_LOOP,
	VCM_PROFILE_STEREO_BT_LOOP,

	VCM_PROFILE_HANDSET_ENH_OFF,
	VCM_PROFILE_HEADSET_ENH_OFF,
	VCM_PROFILE_HANDSFREE_ENH_OFF,
	VCM_PROFILE_BLUETOOTH_ENH_OFF,
	VCM_PROFILE_STEREO_BT_ENH_OFF,

	/* Must be at the end */
	VCM_NUM_OF_PROFILES,

	VCM_NOT_CONNECTED = 0x7FFFFFFF,
	VCM_PROFILE_ENUM_32_BIT = VCM_NOT_CONNECTED //32bit enum compiling enforcemen
} VCM_AudioProfile;

typedef enum
{
	VCM_APP_VOICE_CALL,

	/* Must be at the end */
	VCM_NO_APP, VCM_NUM_OF_APPS = VCM_NO_APP,

	VCM_APP_ENUM_32_BIT = VCM_NOT_CONNECTED //32bit enum compiling enforcement
} VCM_AudioApp;

typedef enum
{
	VCM_RC_INVALID = 0,
	VCM_RC_OK,
	VCM_RC_CP_NOT_RESPONSE,
	VCM_RC_PROFILE_ALREADY_ENABLED,
	VCM_RC_PROFILE_ALREADY_DISABLED,
	VCM_RC_NO_MUTE_CHANGE_NEEDED,
	VCM_RC_INVALID_VOLUME_CHANGE,
	VCM_RC_PROFILE_NOT_FOUND,
	VCM_RC_BUFFER_GET_FUNC_INVALID,
	VCM_RC_STREAM_OUT_NOT_PERFORMED,
	VCM_RC_STREAM_IN_NOT_PERFORMED,
	VCM_RC_STREAM_OUT_TO_BE_STOPPED_NOT_ACTIVE,
	VCM_RC_STREAM_IN_TO_BE_STOPPED_NOT_ACTIVE,
	VCM_RC_IO_ERROR,
	VCM_RC_PARAMETER_ID_INVALID,

	VCM_RC_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} VCM_ReturnCode;

typedef enum
{
	VCM_VOICE_CALL,
	VCM_TONE,
	VCM_PCM,
	VCM_PCM_WB,
	VCM_HR,
	VCM_EFR,
	VCM_FR,
	VCM_AMR_MR475,
	VCM_AMR_MR515,
	VCM_AMR_MR59,
	VCM_AMR_MR67,
	VCM_AMR_MR74,
	VCM_AMR_MR795,
	VCM_AMR_MR102,
	VCM_AMR_MR122,
	VCM_AMR_MRCNF,
	VCM_AMR_MRNO_TX_RX,
	VCM_AMR_WB_6_60,
	VCM_AMR_WB_8_85,
	VCM_AMR_WB_12_65,
	VCM_AMR_WB_14_25,
	VCM_AMR_WB_15_85,
	VCM_AMR_WB_18_25,
	VCM_AMR_WB_19_85,
	VCM_AMR_WB_23_05,
	VCM_AMR_WB_23_85,
	VCM_AMR_WB_SID,

	VCM_DUMMY,
	/* Must be at the end */
	VCM_NO_STREAM_TYPE,
	VCM_NUM_OF_STREAM_TYPES = VCM_NO_STREAM_TYPE,

	VCM_STREAM_TYPE_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} VCM_StreamType;

typedef enum
{
	VCM_MUTE_OFF = 0, VCM_MUTE_ON = 1,

	VCM_AUDIO_MUTE_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} VCM_AudioMute;

typedef enum
{
	VCM_NO_END = 0,
	VCM_NEAR_END, VCM_FAR_END, /* Lowest priority */
	VCM_BOTH_ENDS, /* Highest priority */

	VCM_SRC_DST_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} VCM_SrcDst;

typedef enum
{
	VCM_NOT_COMB_WITH_CALL = 0, /* Lowest priority */
	VCM_COMB_WITH_CALL = 1, /* Highest priority */

	VCM_COMB_WITH_CALL_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} VCM_CombWithCall;

typedef enum
{
	VCM_PATH_IN = 0, /* TX: near to far end */
	VCM_PATH_OUT, /* RX: far to near end */
	VCM_PATH_SIDETONE,

	/* Must be at the end */
	VCM_PATH_NOT_CONNECTED, VCM_NUM_OF_PATHS = VCM_PATH_NOT_CONNECTED,

	VCM_PATH_DIRECTION_ENUM_32_BIT = 0x7FFFFFFF //32bit enum compiling enforcement
} VCM_PathDirection;

typedef enum
{
	VCM_NEAR_CODEC = 1,
	VCM_NEAR_VOCODER = 2
} VCM_CodecVocoder;

typedef enum
{
	VCM_REC_DEFAULT = 1, /* depend on CP band rate */
	VCM_REC_8K = 3,
	VCM_REC_16K = 7
} VCM_RecRate;

typedef enum
{
	VCM_LOOPBACK_OFF = 0,
	VCM_LOOPBACK_PCM = 1,
	VCM_LOOPBACK_PACKET = 2,
} VCM_LoopbackMode;

#define PARAM_PROFILE_LOOPBACK_OFF (1)
#define PARAM_PROFILE_LOOPBACK_PCM (2)
#define PARAM_PROFILE_LOOPBACK_PACKET (3)
#define PARAM_PROFILE_LOOPBACK_MASK (3)

#define PARAM_STREAM_NEAR_CODEC (0 << 0)
#define PARAM_STREAM_NEAR_VOCODER (1 << 0)

#define UNUSEDPARAM(param) (void)param;

extern VCM_ReturnCode VCMAudioProfileSet(VCM_AudioApp app,
		VCM_AudioProfile profile, VCM_MiscParam miscParam);

extern VCM_ReturnCode VCMAudioProfileVolumeSet(VCM_PathDirection direction,
		VCM_MSAGain gain, VCM_MSAGain hd_gain, VCM_MiscParam miscParam);

extern VCM_ReturnCode VCMAudioProfileMute(VCM_PathDirection direction,
		VCM_AudioMute mute, VCM_RampLevel ramp);

extern VCM_ReturnCode VCMAudioSwitchPCM(unsigned int pcm);

extern VCM_ReturnCode VCMAudioConfigPCM(unsigned int pcm);

extern VCM_ReturnCode VCMAudioConfigPCMExpert(unsigned int pcm);

extern VCM_ReturnCode VCMAudioStreamOutStart(VCM_StreamType streamType,
		VCM_SrcDst srcDst, VCM_CombWithCall combWithCall,
		VCM_MiscParam miscParam, VCM_StreamID *streamID);

extern VCM_ReturnCode VCMAudioStreamOutStop(VCM_StreamID id);

extern VCM_ReturnCode VCMAudioStreamInStart(VCM_StreamType streamType,
		VCM_SrcDst srcDst, VCM_MiscParam miscParam,
		VCM_StreamID *streamID);

extern VCM_ReturnCode VCMAudioStreamInStop(VCM_StreamID id);

extern VCM_ReturnCode VCMAudioStreamWrite(VCM_StreamID id, unsigned char *data,
		unsigned int *len);

extern VCM_ReturnCode VCMAudioStreamDrain(VCM_StreamID id, unsigned short timeoutSec);

extern VCM_ReturnCode VCMAudioStreamRead(VCM_StreamID id, unsigned char *data,
		unsigned int *len);

extern VCM_ReturnCode VCMAudioParameterSet(VCM_ParameterID id, void * para,
		unsigned int len);

extern VCM_ReturnCode VCMAudioParameterGet(VCM_ParameterID id, void * para,
		unsigned int *len);

extern VCM_ReturnCode VCMAudioControlRead(unsigned char *data, unsigned int *len);

extern VCM_ReturnCode VCMAudioEcall(void * para);

extern VCM_ReturnCode VCMInit(void);
extern void VCMDeinit(void);
extern void config_VoIP_device(int VoIP_enable);

#endif  /* _VCM_H_ */

