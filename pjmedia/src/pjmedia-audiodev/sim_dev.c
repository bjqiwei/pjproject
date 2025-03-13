/*
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <pjmedia-audiodev/audiodev_imp.h>
#include <pj/assert.h>
#include <pj/log.h>
#include <pj/os.h>
#include <stdbool.h>
#if PJMEDIA_AUDIO_DEV_HAS_SIM_AUDIO
/*sim  Include files*/
#include "audio_if_types.h"
#include "audio_if_ubus.h"
#include "audio_if_parameter.h"
#include "audio_if.h"
#include "audio_if_api.h"
#include "audio_if_audio_hw_mrvl.h"
#include "audio_hw_mrvl.h"
#include <cutils/str_parms.h>
#include "vcm.h"



#define THIS_FILE               "sim_dev.c"

/* sim_audio device info */
struct sim_audio_dev_info
{
    pjmedia_aud_dev_info         info;
    unsigned                     dev_id;
};

/* sim_audio factory */
struct sim_audio_factory
{
    pjmedia_aud_dev_factory      base;
    pj_pool_t                   *pool;
    pj_pool_factory             *pf;

    unsigned                     dev_count;
    struct sim_audio_dev_info  *dev_info;
};

/* Individual sim capture/playback stream descriptor */
struct sim_channel
{
    pj_timestamp  timestamp;
};


/* Sound stream. */
struct sim_audio_stream
{
    pjmedia_aud_stream   base;              /**< Base stream           */
    pjmedia_aud_param    param;             /**< Settings              */
    pj_pool_t           *pool;              /**< Memory pool.          */

    pjmedia_aud_rec_cb   rec_cb;            /**< Capture callback.     */
    pjmedia_aud_play_cb  play_cb;           /**< Playback callback.    */
    void                *user_data;         /**< Application data.     */

    struct sim_channel  play_strm;         /**< Playback stream.      */
    struct sim_channel  rec_strm;          /**< Capture stream.       */

    void*               buffer;            /**< Temp. frame buffer.   */
    unsigned            bytes_per_frame;   /**< Bytes per frame       */

    pj_thread_t* thread;            /**< Thread handle.        */
};


/* Prototypes */
static pj_status_t sim_factory_init(pjmedia_aud_dev_factory *f);
static pj_status_t sim_factory_destroy(pjmedia_aud_dev_factory *f);
static pj_status_t sim_factory_refresh(pjmedia_aud_dev_factory *f);
static unsigned    sim_factory_get_dev_count(pjmedia_aud_dev_factory *f);
static pj_status_t sim_factory_get_dev_info(pjmedia_aud_dev_factory *f,
                                             unsigned index,
                                             pjmedia_aud_dev_info *info);
static pj_status_t sim_factory_default_param(pjmedia_aud_dev_factory *f,
                                              unsigned index,
                                              pjmedia_aud_param *param);
static pj_status_t sim_factory_create_stream(pjmedia_aud_dev_factory *f,
                                              const pjmedia_aud_param *param,
                                              pjmedia_aud_rec_cb rec_cb,
                                              pjmedia_aud_play_cb play_cb,
                                              void *user_data,
                                              pjmedia_aud_stream **p_aud_strm);

static pj_status_t sim_stream_get_param(pjmedia_aud_stream *strm,
                                         pjmedia_aud_param *param);
static pj_status_t sim_stream_get_cap(pjmedia_aud_stream *strm,
                                       pjmedia_aud_dev_cap cap,
                                       void *value);
static pj_status_t sim_stream_set_cap(pjmedia_aud_stream *strm,
                                       pjmedia_aud_dev_cap cap,
                                       const void *value);
static pj_status_t sim_stream_start(pjmedia_aud_stream *strm);
static pj_status_t sim_stream_stop(pjmedia_aud_stream *strm);
static pj_status_t sim_stream_destroy(pjmedia_aud_stream *strm);

/* Operations */
static pjmedia_aud_dev_factory_op factory_op =
{
    &sim_factory_init,
    &sim_factory_destroy,
    &sim_factory_get_dev_count,
    &sim_factory_get_dev_info,
    &sim_factory_default_param,
    &sim_factory_create_stream,
    &sim_factory_refresh
};

static pjmedia_aud_stream_op stream_op =
{
    &sim_stream_get_param,
    &sim_stream_get_cap,
    &sim_stream_set_cap,
    &sim_stream_start,
    &sim_stream_stop,
    &sim_stream_destroy
};


/****************************************************************************
 * Factory operations
 */
/*
 * Init sim_audio audio driver.
 */
pjmedia_aud_dev_factory* pjmedia_sim_audio_factory(pj_pool_factory *pf)
{
    struct sim_audio_factory *f;
    pj_pool_t *pool;

    pool = pj_pool_create(pf, "sim audio", 1000, 1000, NULL);
    f = PJ_POOL_ZALLOC_T(pool, struct sim_audio_factory);
    f->pf = pf;
    f->pool = pool;
    f->base.op = &factory_op;

    return &f->base;
}


/* API: init factory */
static pj_status_t sim_factory_init(pjmedia_aud_dev_factory *f)
{
    struct sim_audio_factory* sf = (struct sim_audio_factory*)f;
    struct sim_audio_dev_info* sdi;

    sf->dev_count = 1;
    sf->dev_info = (struct sim_audio_dev_info*)
        pj_pool_calloc(sf->pool, sf->dev_count,
            sizeof(struct sim_audio_dev_info));

    sdi = &sf->dev_info[0];
    pj_bzero(sdi, sizeof(*sdi));
    pj_ansi_strxcpy(sdi->info.name, "sim device", sizeof(sdi->info.name));
    pj_ansi_strxcpy(sdi->info.driver, "sim", sizeof(sdi->info.driver));
    sdi->info.input_count = 1;
    sdi->info.output_count = 1;
    sdi->info.default_samples_per_sec = 8000;
    /* Set the device capabilities here */
    sdi->info.caps = 0;
    sdi->info.caps |= PJMEDIA_AUD_DEV_CAP_INPUT_LATENCY;
    sdi->info.caps |= PJMEDIA_AUD_DEV_CAP_OUTPUT_LATENCY;

    /* Extended formats */
    //sdi->info.caps |= PJMEDIA_AUD_DEV_CAP_EXT_FORMAT;
    //sdi->info.ext_fmt_cnt = 2;
    //pjmedia_format_init_audio(&sdi->info.ext_fmt[0],
    //    PJMEDIA_FORMAT_PCMU, 8000, 1, 8,
    //    20000, 64000, 64000);
    //pjmedia_format_init_audio(&sdi->info.ext_fmt[0],
    //    PJMEDIA_FORMAT_PCMA, 8000, 1, 8,
    //    20000, 64000, 64000);

    PJ_LOG(4, (THIS_FILE, "sim found %d devices:",
        sf->dev_count));
    for (int c = 0; c < sf->dev_count; ++c) {
        PJ_LOG(4, (THIS_FILE, " dev_id %d: %s  (in=%d, out=%d)",
            c,
            sf->dev_info[c].info.name,
            sf->dev_info[c].info.input_count,
            sf->dev_info[c].info.output_count));
    }

    PJ_LOG(4, (THIS_FILE, "sim audio initialized"));

    return PJ_SUCCESS;
}

/* API: destroy factory */
static pj_status_t sim_factory_destroy(pjmedia_aud_dev_factory *f)
{
    struct sim_audio_factory *sf = (struct sim_audio_factory*)f;

    pj_pool_safe_release(&sf->pool);

    return PJ_SUCCESS;
}

/* API: refresh the list of devices */
static pj_status_t sim_factory_refresh(pjmedia_aud_dev_factory *f)
{
    PJ_UNUSED_ARG(f);
    return PJ_SUCCESS;
}

/* API: get number of devices */
static unsigned sim_factory_get_dev_count(pjmedia_aud_dev_factory *f)
{
    struct sim_audio_factory *sf = (struct sim_audio_factory*)f;
    return sf->dev_count;
}

/* API: get device info */
static pj_status_t sim_factory_get_dev_info(pjmedia_aud_dev_factory *f,
                                             unsigned index,
                                             pjmedia_aud_dev_info *info)
{
    struct sim_audio_factory *sf = (struct sim_audio_factory*)f;

    PJ_ASSERT_RETURN(index < sf->dev_count, PJMEDIA_EAUD_INVDEV);

    pj_memcpy(info, &sf->dev_info[index].info, sizeof(*info));

    return PJ_SUCCESS;
}

/* API: create default device parameter */
static pj_status_t sim_factory_default_param(pjmedia_aud_dev_factory *f,
                                              unsigned index,
                                              pjmedia_aud_param *param)
{
    struct sim_audio_factory *sf = (struct sim_audio_factory*)f;
    struct sim_audio_dev_info *di = &sf->dev_info[index];

    PJ_ASSERT_RETURN(index < sf->dev_count, PJMEDIA_EAUD_INVDEV);

    pj_bzero(param, sizeof(*param));
    if (di->info.input_count && di->info.output_count) {
        param->dir = PJMEDIA_DIR_CAPTURE_PLAYBACK;
        param->rec_id = index;
        param->play_id = index;
    } else if (di->info.input_count) {
        param->dir = PJMEDIA_DIR_CAPTURE;
        param->rec_id = index;
        param->play_id = PJMEDIA_AUD_INVALID_DEV;
    } else if (di->info.output_count) {
        param->dir = PJMEDIA_DIR_PLAYBACK;
        param->play_id = index;
        param->rec_id = PJMEDIA_AUD_INVALID_DEV;
    } else {
        return PJMEDIA_EAUD_INVDEV;
    }

    /* Set the mandatory settings here */
    /* The values here are just some examples */
    param->clock_rate = di->info.default_samples_per_sec;
    param->channel_count = 1;
    param->samples_per_frame = di->info.default_samples_per_sec * 20 / 1000;
    param->bits_per_sample = 16;

    /* Set the device capabilities here */
    param->flags = 0;

    return PJ_SUCCESS;
}

static audio_hw_device_t* play_ring_tone_ahw_dev_ubus;

static struct audio_stream_in* stream_in = NULL;
static struct audio_stream_out* stream_out = NULL;
static bool go_on_pcmloopback = true;
static bool stream_started = false;
static unsigned int pcm_record_size = 0;       //320:NB, 640:WB
static unsigned int pcm_playback_size = 0;     //320:NB, 640:WB

/* Internal: create sim player device. */
static pj_status_t init_player_stream(struct sim_audio_stream* parent, struct sim_channel* sim_strm)
{
    parent->bytes_per_frame = pcm_playback_size;
    sim_strm->timestamp.u64 = 0;
    return PJ_SUCCESS;
}

/* Internal: create sim recorder device */
static pj_status_t init_capture_stream(struct sim_audio_stream* parent, struct sim_channel* sim_strm)
{
    parent->bytes_per_frame = pcm_record_size;
    sim_strm->timestamp.u64 = 0;
    return PJ_SUCCESS;
}

/*******************************************************************************\
 *	Function:	sim_dev_thread
 *   Description:This function will main thread to record the PCM stream from
 *               ASR1826 and playback the PCM stream to ASR1826.
 *
 *   NOTE:
 sample rate is 8000
 channel number is 1
 bit of sample is 16
 Due to modem limited, only support 8k/16k sample rate
 *	Returns:	void
 \*******************************************************************************/

static int config_parameters(int in_out)
{
    unsigned int direction = 0xFF, type, srcdst, priority, dest;
    char kvpair[128];
    struct str_parms* param = NULL;
    int data[5];
    const char* key = NULL;
    bool update_vcm = false;

    direction = in_out;/* 0-play, 1-record */
    type = 0; /* 0:PCM_NB_BUF_SIZE, 1:PCM_WB_BUF_SIZE */
    srcdst = 2;/* 0-None, 1-Near end, 2-Far end, 3-Both ends */
    priority = 1;/* 0-Do not combine(override), 1-Combine */
    dest = 0;/* 0-Near codec, 1-Near Vocoder */

    if (direction == 0) {//output
        if (type == 0)
            pcm_playback_size = PCM_NB_BUF_SIZE;
        else
            pcm_playback_size = PCM_WB_BUF_SIZE;

        PJ_LOG(4, (THIS_FILE, "config playback parameters."));
    }
    else if (direction == 1) {//input
        if (type == 0)
            pcm_record_size = PCM_NB_BUF_SIZE;
        else
            pcm_record_size = PCM_WB_BUF_SIZE;

        PJ_LOG(4, (THIS_FILE, "config record parameters."));
    }

    memset(kvpair, 0x00, sizeof(kvpair));
    sprintf(kvpair, "%s=%d;%s=%d;%s=%d;%s=%d;%s=%d", VCM_CONFIG_DIRECTION, direction,
        VCM_CONFIG_TYPE, type, VCM_CONFIG_SRC_DST, srcdst,
        VCM_CONFIG_PRIORITY, priority, VCM_CONFIG_DEST, dest);

    PJ_LOG(4, (THIS_FILE, "%s: config information kvpair is %s.", __FUNCTION__, kvpair));

    //extract the parameter and config from string
    param = str_parms_create_str(kvpair);
    if (!param) {
        PJ_LOG(3, (THIS_FILE, "%s: param create str is null!", __FUNCTION__));
        return -1;
    }

    //set vcm configurations
    key = VCM_CONFIG_DIRECTION;
    if (str_parms_get_int(param, key, &data[0]) == 0) {
        update_vcm = true;
        str_parms_del(param, key);
    }
    key = VCM_CONFIG_TYPE;
    if (str_parms_get_int(param, key, &data[1]) == 0) {
        update_vcm = true;
        str_parms_del(param, key);
    }
    key = VCM_CONFIG_SRC_DST;
    if (str_parms_get_int(param, key, &data[2]) == 0) {
        update_vcm = true;
        str_parms_del(param, key);
    }
    key = VCM_CONFIG_PRIORITY;
    if (str_parms_get_int(param, key, &data[3]) == 0) {
        update_vcm = true;
        str_parms_del(param, key);
    }
    key = VCM_CONFIG_DEST;
    if (str_parms_get_int(param, key, &data[4]) == 0) {
        update_vcm = true;
        str_parms_del(param, key);
    }

    //printf("Direction is %d, Type is %d, Src_Dst is %d, Priority is %d, Dest is %d. \n",data[0], data[1], data[2], data[3], data[4]);

    if (update_vcm) {
        configure_vcm(data);   /*TODO check if all inputs got all values successfully*/
    }

    return 0;
}

static void sim_dev_thread(void* arg)
{
    struct sim_audio_stream* strm = (struct sim_audio_stream*)arg;
    int rc, len, cap_len;
    char buffer[PCM_WB_BUF_SIZE];
    unsigned int frames = 0;
    pj_status_t status = PJ_SUCCESS;

    PJ_LOG(4, (THIS_FILE, "enter play_ring_tone_thread."));

    //Must set vcm_configure before pcmloopback
    if ((pcm_record_size != PCM_NB_BUF_SIZE) && (pcm_record_size != PCM_WB_BUF_SIZE)) {
        PJ_LOG(3, (THIS_FILE, "%s: Please use vcm_configure to set pcm_record_size!!", __FUNCTION__));
        return;
    }

    //Must set vcm_configure before playback
    if ((pcm_playback_size != PCM_NB_BUF_SIZE) && (pcm_playback_size != PCM_WB_BUF_SIZE)) {
        PJ_LOG(3, (THIS_FILE, "%s: Please use vcm_configure to set pcm_playback_size!!", __FUNCTION__));
        return;
    }

    //Must set vcm_configure before playback
    if (pcm_record_size != pcm_playback_size) {
        PJ_LOG(3, (THIS_FILE, "%s: Please configure pcm_record_size = pcm_playback_size!!", __FUNCTION__));
        return;
    }

    //open the audiostub_ctl, prepare for record and playback
    VCMInit();

    //open record stream 
    rc = play_ring_tone_ahw_dev_ubus->open_input_stream(play_ring_tone_ahw_dev_ubus, 0,
        play_ring_tone_ahw_dev_ubus->get_supported_devices(play_ring_tone_ahw_dev_ubus),
        NULL, &stream_in, 0, 0, AUDIO_SOURCE_VOICE_CALL);
    if (rc < 0) {
        PJ_LOG(3, (THIS_FILE, "%s: error opening input device. rc = %d!", __FUNCTION__, rc));
        goto bad_stream;
    }

    //open playback stream
    rc = play_ring_tone_ahw_dev_ubus->open_output_stream(play_ring_tone_ahw_dev_ubus, 0,
        play_ring_tone_ahw_dev_ubus->get_supported_devices(play_ring_tone_ahw_dev_ubus),
        AUDIO_OUTPUT_FLAG_DIRECT, NULL, &stream_out, 0);
    if (rc < 0) {
        PJ_LOG(3, (THIS_FILE, "%s: error opening output device. rc = %d!", __FUNCTION__, rc));
        goto bad_stream;
    }

    PJ_LOG(3, (THIS_FILE, "%s: starting pcmloopback %d bytes every 20ms!", __FUNCTION__, pcm_record_size));
    go_on_pcmloopback = true;
    while (go_on_pcmloopback) {
        //record the needed format stream from the device.
        //only read pcm stream, no send command.
        if (!stream_started)
        {
            //sleep 20 millisecond
            pj_thread_sleep(20);
            continue;
        }
        {
            cap_len = stream_in->read(stream_in, buffer, pcm_record_size);
            if (len < 0) {
                PJ_LOG(3, (THIS_FILE, "%s: error reading!", __FUNCTION__));
                goto end_pcmloopback;
            }
            pjmedia_frame pcm_frame, * frame;

            /* PCM mode */

            /* Copy the audio data out of the wave buffer. */
            pj_memcpy(strm->buffer, buffer, pcm_record_size);

            /* Prepare frame */
            frame = &pcm_frame;
            frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
            frame->buf = strm->buffer;
            frame->size = pcm_record_size;
            frame->timestamp.u64 = strm->rec_strm.timestamp.u64;
            frame->bit_info = 0;

            status = (*strm->rec_cb)(strm->user_data, frame);
            strm->rec_strm.timestamp.u64 += strm->param.samples_per_frame /
                strm->param.channel_count;

            PJ_LOG(4, (THIS_FILE, "%s: record from Dev size is %d!", __FUNCTION__, len));
        }


        //
        //TODO:send the above IP package to far-end
        //

        //playback the needed format stream to device.
        //only write pcm stream, no send command.

        {
            pjmedia_frame pcm_frame, * frame;
            frame = &pcm_frame;

            frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
            frame->size = strm->bytes_per_frame;
            frame->buf = buffer;
            frame->timestamp.u64 = strm->play_strm.timestamp.u64;
            frame->bit_info = 0;
            status = (*strm->play_cb)(strm->user_data, frame);

            rc = stream_out->write(stream_out, buffer, len);
            strm->play_strm.timestamp.u64 += strm->param.samples_per_frame /
                strm->param.channel_count;

            if (rc < 0) {
                PJ_LOG(3, (THIS_FILE, "%s: error writing (child)!", __FUNCTION__));
                goto end_pcmloopback;
            } else if (rc < len) {
                PJ_LOG(3, (THIS_FILE, "%s: wrote less than buffer size!", __FUNCTION__));
                goto end_pcmloopback;
            }
            PJ_LOG(4, (THIS_FILE, "%s: playback to Dev len is %d.", __FUNCTION__, rc));
        }

        PJ_LOG(4, (THIS_FILE, "%s: No.%d frame loopback!", __FUNCTION__, ++frames));
    }

end_pcmloopback:
    stream_in->common.standby(&stream_in->common);
    play_ring_tone_ahw_dev_ubus->close_input_stream(play_ring_tone_ahw_dev_ubus, stream_in);
    stream_out->common.standby(&stream_out->common);
    play_ring_tone_ahw_dev_ubus->close_output_stream(play_ring_tone_ahw_dev_ubus, stream_out);
    VCMDeinit();//close the fd of audiostub_ctl when exit the thread.

    go_on_pcmloopback = false;
bad_stream:
    PJ_LOG(4, (THIS_FILE, "%s: finished pcm loopback!", __FUNCTION__));
    PJ_LOG(4, (THIS_FILE, "exit play_ring_tone_thread!"));
    return;
}

/* API: create stream */
static pj_status_t sim_factory_create_stream(pjmedia_aud_dev_factory *f,
                                              const pjmedia_aud_param *param,
                                              pjmedia_aud_rec_cb rec_cb,
                                              pjmedia_aud_play_cb play_cb,
                                              void *user_data,
                                              pjmedia_aud_stream **p_aud_strm)
{
    struct sim_audio_factory *sf = (struct sim_audio_factory*)f;
    pj_pool_t *pool;
    struct sim_audio_stream *strm;
    pj_status_t status;

    /* Create and Initialize stream descriptor */
    pool = pj_pool_create(sf->pf, "sim_audio-dev", 1000, 1000, NULL);
    PJ_ASSERT_RETURN(pool != NULL, PJ_ENOMEM);

    strm = PJ_POOL_ZALLOC_T(pool, struct sim_audio_stream);
    pj_memcpy(&strm->param, param, sizeof(*param));
    strm->pool = pool;
    strm->rec_cb = rec_cb;
    strm->play_cb = play_cb;
    strm->user_data = user_data;

    //init global variables
    play_ring_tone_ahw_dev_ubus = audio_hal_install();
    if (play_ring_tone_ahw_dev_ubus == NULL) {
        printf("%s: audio_hal_install failed!\n", __FUNCTION__);
        exit(-1);
    }

    //The following config parameters are needed for main thread.

    /* Create player stream here */
    if (param->dir & PJMEDIA_DIR_PLAYBACK) {
        config_parameters(0);
        status = init_player_stream(strm, &strm->play_strm);
        //config playback parameters.
        if (status != PJ_SUCCESS) {
            sim_stream_destroy(&strm->base);
            return status;
        }
    }

    /* Create capture stream here */
    if (param->dir & PJMEDIA_DIR_CAPTURE) {
        config_parameters(1);
        status = init_capture_stream(strm, &strm->rec_strm);
        //config record parameters.
        if (status != PJ_SUCCESS) {
            sim_stream_destroy(&strm->base);
            return status;
        }
    }

    strm->buffer = pj_pool_alloc(pool, strm->bytes_per_frame);
    if (!strm->buffer) {
        pj_pool_release(pool);
        return PJ_ENOMEM;
    }

    /* Create and start the thread */
    status = pj_thread_create(pool, "sim_dev", &sim_dev_thread, strm, 0, 0,
        &strm->thread);
    if (status != PJ_SUCCESS) {
        sim_stream_destroy(&strm->base);
        return status;
    }

    /* Apply the remaining settings */
    /* Below is an example if you want to set the output volume */
    if (param->flags & PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING) {
        sim_stream_set_cap(&strm->base,
                            PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING,
                            &param->output_vol);
    }


    /* Done */
    strm->base.op = &stream_op;
    *p_aud_strm = &strm->base;

    return PJ_SUCCESS;
}

/* API: Get stream info. */
static pj_status_t sim_stream_get_param(pjmedia_aud_stream *s,
                                         pjmedia_aud_param *pi)
{
    struct sim_audio_stream *strm = (struct sim_audio_stream*)s;

    PJ_ASSERT_RETURN(strm && pi, PJ_EINVAL);

    pj_memcpy(pi, &strm->param, sizeof(*pi));

    /* Example: Update the volume setting */
    if (sim_stream_get_cap(s, PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING,
                            &pi->output_vol) == PJ_SUCCESS)
    {
        pi->flags |= PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING;
    }

    return PJ_SUCCESS;
}

/* API: get capability */
static pj_status_t sim_stream_get_cap(pjmedia_aud_stream *s,
                                       pjmedia_aud_dev_cap cap,
                                       void *pval)
{
    struct sim_audio_stream *strm = (struct sim_audio_stream*)s;

    PJ_UNUSED_ARG(strm);

    PJ_ASSERT_RETURN(s && pval, PJ_EINVAL);

    /* Example: Get the output's volume setting */
    if (cap==PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING)
    {
        /* Output volume setting */
        *(unsigned*)pval = 0; // retrieve output device's volume here
        return PJ_SUCCESS;
    } else {
        return PJMEDIA_EAUD_INVCAP;
    }
}

/* API: set capability */
static pj_status_t sim_stream_set_cap(pjmedia_aud_stream *s,
                                       pjmedia_aud_dev_cap cap,
                                       const void *pval)
{
    struct sim_audio_stream *strm = (struct sim_audio_stream*)s;

    PJ_UNUSED_ARG(strm);

    PJ_ASSERT_RETURN(s && pval, PJ_EINVAL);

    /* Example */
    if (cap==PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING)
    {
        /* Output volume setting */
        // set output's volume level here
        return PJ_SUCCESS;
    }

    return PJMEDIA_EAUD_INVCAP;
}

/* API: Start stream. */
static pj_status_t sim_stream_start(pjmedia_aud_stream *strm)
{
    struct sim_audio_stream *stream = (struct sim_audio_stream*)strm;

    PJ_UNUSED_ARG(stream);
    stream_started = true;
    PJ_LOG(4, (THIS_FILE, "Starting sim audio stream"));

    return PJ_SUCCESS;
}

/* API: Stop stream. */
static pj_status_t sim_stream_stop(pjmedia_aud_stream *strm)
{
    struct sim_audio_stream *stream = (struct sim_audio_stream*)strm;

    PJ_UNUSED_ARG(stream);
    stream_started = false;
    PJ_LOG(4, (THIS_FILE, "Stopping sim audio stream"));

    return PJ_SUCCESS;
}


/* API: Destroy stream. */
static pj_status_t sim_stream_destroy(pjmedia_aud_stream *strm)
{
    struct sim_audio_stream *stream = (struct sim_audio_stream*)strm;

    PJ_ASSERT_RETURN(stream != NULL, PJ_EINVAL);

    sim_stream_stop(strm);
    /* Stop the stream thread */
    if (stream->thread)
    {
        go_on_pcmloopback = false;
        pj_thread_join(stream->thread);
        pj_thread_destroy(stream->thread);
        stream->thread = NULL;
    }

    pj_pool_release(stream->pool);

    return PJ_SUCCESS;
}

#endif  /* PJMEDIA_AUDIO_DEV_HAS_SIM_AUDIO */
