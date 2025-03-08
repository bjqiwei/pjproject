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

#if PJMEDIA_AUDIO_DEV_HAS_SIM_AUDIO

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

/* Sound stream. */
struct sim_audio_stream
{
    pjmedia_aud_stream   base;              /**< Base stream           */
    pjmedia_aud_param    param;             /**< Settings              */
    pj_pool_t           *pool;              /**< Memory pool.          */

    pjmedia_aud_rec_cb   rec_cb;            /**< Capture callback.     */
    pjmedia_aud_play_cb  play_cb;           /**< Playback callback.    */
    void                *user_data;         /**< Application data.     */
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

    /* Create and Initialize stream descriptor */
    pool = pj_pool_create(sf->pf, "sim_audio-dev", 1000, 1000, NULL);
    PJ_ASSERT_RETURN(pool != NULL, PJ_ENOMEM);

    strm = PJ_POOL_ZALLOC_T(pool, struct sim_audio_stream);
    pj_memcpy(&strm->param, param, sizeof(*param));
    strm->pool = pool;
    strm->rec_cb = rec_cb;
    strm->play_cb = play_cb;
    strm->user_data = user_data;

    /* Create player stream here */
    if (param->dir & PJMEDIA_DIR_PLAYBACK) {
    }

    /* Create capture stream here */
    if (param->dir & PJMEDIA_DIR_CAPTURE) {
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

    PJ_LOG(4, (THIS_FILE, "Starting null audio stream"));

    return PJ_SUCCESS;
}

/* API: Stop stream. */
static pj_status_t sim_stream_stop(pjmedia_aud_stream *strm)
{
    struct sim_audio_stream *stream = (struct sim_audio_stream*)strm;

    PJ_UNUSED_ARG(stream);

    PJ_LOG(4, (THIS_FILE, "Stopping null audio stream"));

    return PJ_SUCCESS;
}


/* API: Destroy stream. */
static pj_status_t sim_stream_destroy(pjmedia_aud_stream *strm)
{
    struct sim_audio_stream *stream = (struct sim_audio_stream*)strm;

    PJ_ASSERT_RETURN(stream != NULL, PJ_EINVAL);

    sim_stream_stop(strm);

    pj_pool_release(stream->pool);

    return PJ_SUCCESS;
}

#endif  /* PJMEDIA_AUDIO_DEV_HAS_NULL_AUDIO */
