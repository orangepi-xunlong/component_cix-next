// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "system_logger.h"
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "armcb_v4l2_stream.h"
#include "armcb_v4l2_core.h"
#include "armcb_v4l2_config.h"
#include "isp_hw_ops.h"

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_ISP
#endif

#define ISP_V4L2_METADATA_SIZE 4096

/* sensor static informations */
static armcb_v4l2_stream_common g_stream_common[ARMCB_MAX_DEVS];
/* default size & format */
#define ISP_DEFAULT_FORMAT V4L2_PIX_FMT_NV12M

typedef struct _armcb_v4l2_fmt {
	const char *name;
	uint32_t fourcc;
	uint8_t depth;
	bool is_yuv;
	uint8_t planes;
} armcb_v4l2_fmt_t;

struct isp_v4l2_fmt isp_stream_formats[] = {
	{
		.name = "NV12M",
		.fourcc = V4L2_PIX_FMT_NV12M,
		.depth = { 8, 4 },
		.memplanes = 2,
		.mbus_code = MEDIA_BUS_FMT_YUYV8_1_5X8,
	},
	{
		.name = "RGB888",
		.fourcc = V4L2_PIX_FMT_RGB24,
		.depth = { 24 },
		.memplanes = 1,
		.mbus_code = MEDIA_BUS_FMT_RGB888_1X24,
	}
};

static struct isp_v4l2_fmt *armcb_v4l2_stream_find_format(uint32_t pixelformat)
{
	struct isp_v4l2_fmt *fmt = NULL;
	unsigned int i = 0;

	for (i = 0; i < ARRAY_SIZE(isp_stream_formats); i++) {
		fmt = &isp_stream_formats[i];
		if (fmt->fourcc == pixelformat)
			return fmt;
	}

	return NULL;
}

int armcb_v4l2_stream_try_format(armcb_v4l2_stream_t *pstream,
				 struct v4l2_format *f)
{
	struct isp_v4l2_fmt *tfmt;
	int i;

	LOG(LOG_INFO,
	    "[Stream#%d] try fmt type: %u, pixelformat: 0x%x, planeNum:%u, width: " \
	    "%u, height: %u, field: %u, pixelformat: 0x%x",
	    pstream->stream_id, f->type, f->fmt.pix_mp.pixelformat,
	    f->fmt.pix_mp.num_planes, f->fmt.pix_mp.width, f->fmt.pix_mp.height,
	    f->fmt.pix_mp.field, f->fmt.pix_mp.pixelformat);

	/* check format and modify */
	tfmt = armcb_v4l2_stream_find_format(f->fmt.pix_mp.pixelformat);
	if (!tfmt) {
		LOG(LOG_WARN,
		    "[Stream#%d] format 0x%08x is not supported, setting default format " \
		    "0x%08x.",
		    pstream->stream_id, f->fmt.pix_mp.pixelformat,
		    ISP_DEFAULT_FORMAT);
		f->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		f->fmt.pix_mp.pixelformat = ISP_DEFAULT_FORMAT;
		tfmt = armcb_v4l2_stream_find_format(f->fmt.pix_mp.pixelformat);
	}

#ifndef V4L2_OPT
	/* adjust width, height for META stream */
	if (f->fmt.pix_mp.pixelformat == ISP_V4L2_PIX_FMT_META) {
		f->fmt.pix_mp.width = ISP_V4L2_METADATA_SIZE;
		f->fmt.pix_mp.height = 1;
	} else if (f->fmt.pix_mp.pixelformat == ISP_V4L2_PIX_FMT_STATIS) {
		LOG(LOG_INFO,
		    "[Stream#%d] format is ISP_V4L2_PIX_FMT_STATIS:0x%08x",
		    pstream->stream_id, ISP_V4L2_PIX_FMT_STATIS);
		f->fmt.pix_mp.width = f->fmt.pix_mp.width;
		f->fmt.pix_mp.height = 1;
	} else {
		if (f->fmt.pix_mp.width == 0 || f->fmt.pix_mp.height == 0) {
			f->fmt.pix_mp.width = 1920;
			f->fmt.pix_mp.height = 1080;
		}
	}
#else
	if (f->fmt.pix_mp.width == 0 || f->fmt.pix_mp.height == 0) {
		f->fmt.pix_mp.width = 1920;
		f->fmt.pix_mp.height = 1080;
	}
#endif

	// all stream multiplanar
	f->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	f->fmt.pix_mp.num_planes = tfmt->memplanes;
	f->fmt.pix_mp.colorspace = V4L2_COLORSPACE_SRGB;

	for (i = 0; i < tfmt->memplanes; i++) {
		//hardcode
		f->fmt.pix_mp.plane_fmt[i].bytesperline = f->fmt.pix_mp.width;
		f->fmt.pix_mp.plane_fmt[i].sizeimage =
			f->fmt.pix_mp.height *
			f->fmt.pix_mp.plane_fmt[i].bytesperline * tfmt->depth[i] / 8;

		memset(f->fmt.pix_mp.plane_fmt[i].reserved, 0,
		       sizeof(f->fmt.pix_mp.plane_fmt[i].reserved));
		memset(f->fmt.pix_mp.reserved, 0,
		       sizeof(f->fmt.pix_mp.reserved));
	}

	return 0;
}

int armcb_v4l2_stream_init(armcb_v4l2_stream_t **ppstream, int stream_id,
			   int ctx_id)
{
	armcb_v4l2_stream_t *new_stream = NULL;
	// int current_sensor_preset;
	LOG(LOG_INFO, "ctx_id:%d [Stream#%d] Initializing stream ...", ctx_id,
	    stream_id);

	/* allocate armcb_v4l2_stream_t */
	new_stream = kzalloc(sizeof(armcb_v4l2_stream_t), GFP_KERNEL);
	if (new_stream == NULL) {
		LOG(LOG_ERR,
		    "[Stream#%d] Failed to allocate armcb_v4l2_stream_t.",
		    stream_id);
		return -ENOMEM;
	}

	/*all stream multiplanar*/
	new_stream->cur_v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	/* set input stream info */
	new_stream->stream_common = &(g_stream_common[ctx_id]);

	/* init control fields */
	new_stream->ctx_id = ctx_id;
	new_stream->stream_id = stream_id;
	new_stream->stream_type = V4L2_STREAM_TYPE_MAX;
	new_stream->stream_started = 0;
	new_stream->last_frame_id = 0xFFFFFFFF;

	/* init list */
	INIT_LIST_HEAD(&new_stream->stream_buffer_list);
	INIT_LIST_HEAD(&new_stream->stream_buffer_list_busy);

	/* init locks */
	spin_lock_init(&new_stream->slock);

	/* return stream private ptr to caller */
	*ppstream = new_stream;

	return 0;
}

void
armcb_v4l2_stream_buffer_list_release(armcb_v4l2_stream_t *pstream,
				      struct list_head *stream_buffer_list)
{
	armcb_v4l2_buffer_t *buf;
	struct vb2_v4l2_buffer *vvb;
	struct vb2_buffer *vb;
	unsigned int buf_index;

	while (!list_empty(stream_buffer_list)) {
		buf = list_entry(stream_buffer_list->next, armcb_v4l2_buffer_t,
				 list);
		list_del(&buf->list);

		vvb = &buf->vvb;
		vb = &vvb->vb2_buf;
		buf_index = vb->index;

		vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);

		LOG(LOG_INFO, "[Stream#%d] vid_cap buffer %d done",
		    pstream->stream_id, buf_index);
	}
}

void armcb_v4l2_stream_deinit(armcb_v4l2_stream_t *pstream,
                                        armcb_v4l2_dev_t *dev)
{
	if (!pstream) {
		LOG(LOG_ERR, "Null stream passed");
		return;
	}

	LOG(LOG_INFO, "ctx_id:%d [Stream#%d] Deinitializing stream %p ...",
	    pstream->ctx_id, pstream->stream_id, pstream);

	/* stop hardware do stream-off first if it's on */
	armcb_v4l2_stream_off(pstream, dev);

	/* release fw_info */
	kfree(pstream);
	pstream = NULL;
}

int armcb_v4l2_stream_on(armcb_v4l2_stream_t *pstream)
{
	if (!pstream) {
		LOG(LOG_ERR, "Null stream passed");
		return -EINVAL;
	}

	LOG(LOG_INFO, "ctx_id:%d [Stream#%d] %p called", pstream->ctx_id,
	    pstream->stream_id, pstream);

	if (pstream->stream_type != V4L2_STREAM_TYPE_META) {
		/* Resets frame counters */
		pstream->fw_frame_seq_count = 0;
	} else {
		atomic_set(&pstream->running, 0);
	}

	/* control fields update */
	pstream->stream_started = 1;
	return 0;
}

void armcb_v4l2_stream_off(armcb_v4l2_stream_t *pstream,
                                            armcb_v4l2_dev_t *dev)
{
	if (!pstream) {
		LOG(LOG_ERR, "Null stream passed");
		return;
	}

	// control fields update
	if (pstream->stream_type == V4L2_STREAM_TYPE_META) {
		while (atomic_read(&pstream->running) > 0)
		{ // metadata has no thread
			LOG(LOG_INFO, "[Stream#%d] still running %d !",
			    pstream->stream_id, atomic_read(&pstream->running));
			schedule();
		}
		atomic_set(&pstream->running, -1);
	}
}

int armcb_v4l2_stream_get_format(armcb_v4l2_stream_t *pstream,
				 struct v4l2_format *f)
{
	if (!pstream) {
		LOG(LOG_ERR, "Null stream passed");
		return -EINVAL;
	}

	*f = pstream->cur_v4l2_fmt;

	LOG(LOG_INFO,
	    "[Stream#%d]   - GET fmt - width: %4u, height: %4u, format: 0x%x.",
	    pstream->stream_id, f->fmt.pix_mp.width, f->fmt.pix_mp.height,
	    f->fmt.pix_mp.pixelformat);

	if (f->fmt.pix_mp.width == 0 || f->fmt.pix_mp.height == 0 ||
	    f->fmt.pix_mp.pixelformat == 0) {
		LOG(LOG_NOTICE, "Compliance error, uninitialized format");
	}

	return 0;
}

int armcb_multi_cam = 0;

int armcb_v4l2_stream_set_format(armcb_v4l2_stream_t *pstream,
				 struct v4l2_format *f)
{
	int rc = 0;

	if (!pstream) {
		LOG(LOG_ERR, "Null stream passed");
		return -EINVAL;
	}

	LOG(LOG_INFO, "[Stream#%d] VIDIOC_S_FMT operation", pstream->stream_id);

	LOG(LOG_INFO,
	    "[Stream#%d]   - SET fmt - width: %4u, height: %4u, format: 0x%x.",
	    pstream->stream_id, f->fmt.pix_mp.width, f->fmt.pix_mp.height,
	    f->fmt.pix_mp.pixelformat);

	/* try format first */
	armcb_v4l2_stream_try_format(pstream, f);

	/* set stream type*/
	switch (f->fmt.pix_mp.pixelformat) {
	case V4L2_PIX_FMT_RGB24:
	case V4L2_PIX_FMT_RGB32:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_X016:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV12M:
	case ISP_V4L2_PIX_FMT_RAW10:
	case ISP_V4L2_PIX_FMT_RAW12:
	case ISP_V4L2_PIX_FMT_RAW16:
	case ISP_V4L2_PIX_FMT_STATIS:
		pstream->stream_type = pstream->stream_id;
		break;
	case ISP_V4L2_PIX_FMT_META:
		pstream->stream_type = V4L2_STREAM_TYPE_META;
		break;
	default:
		LOG(LOG_ERR, "Shouldn't be here after try_format().");
		return -EINVAL;
	}

	/* update format field */
	pstream->cur_v4l2_fmt = *f;
	if (armcb_multi_cam)
	{
		switch (pstream->ctx_id) {
			case 0:
				f->fmt.pix_mp.field =
					(1 << ISP_OUTPUT_PORT_VOUT1) |
					(1 << ISP_OUTPUT_PORT_VOUT2);
				pstream->outport =
					(1 << ISP_OUTPUT_PORT_VOUT1) |
					(1 << ISP_OUTPUT_PORT_VOUT2);
				break;
			case 1:
				f->fmt.pix_mp.field =
					(1 << ISP_OUTPUT_PORT_VOUT3) |
					(1 << ISP_OUTPUT_PORT_VOUT4);
				pstream->outport =
					(1 << ISP_OUTPUT_PORT_VOUT3) |
					(1 << ISP_OUTPUT_PORT_VOUT4);
				break;
			case 2:
				f->fmt.pix_mp.field =
					(1 << ISP_OUTPUT_PORT_VOUT5) |
					(1 << ISP_OUTPUT_PORT_VOUT6);
				pstream->outport =
					(1 << ISP_OUTPUT_PORT_VOUT5) |
					(1 << ISP_OUTPUT_PORT_VOUT6);
				break;
			case 3:
				f->fmt.pix_mp.field =
					(1 << ISP_OUTPUT_PORT_VOUT7) |
					(1 << ISP_OUTPUT_PORT_VOUT8);
				pstream->outport =
					(1 << ISP_OUTPUT_PORT_VOUT7) |
					(1 << ISP_OUTPUT_PORT_VOUT8);
				break;
			default:
				f->fmt.pix_mp.field =
					(1 << ISP_OUTPUT_PORT_VOUT1) |
					(1 << ISP_OUTPUT_PORT_VOUT2);
				pstream->outport =
					(1 << ISP_OUTPUT_PORT_VOUT1) |
					(1 << ISP_OUTPUT_PORT_VOUT2);
				break;
		}
	} else {
		switch(f->fmt.pix_mp.pixelformat)
		{
			case V4L2_PIX_FMT_RGB24:
			case V4L2_PIX_FMT_RGB32:
			case V4L2_PIX_FMT_BGR24:
			case V4L2_PIX_FMT_BGR32:
				f->fmt.pix_mp.field =
					(1 << ISP_OUTPUT_PORT_VOUT0) |
					(1 << ISP_OUTPUT_PORT_VOUT1) |
					(1 << ISP_OUTPUT_PORT_VOUT2);
				pstream->outport =
					(1 << ISP_OUTPUT_PORT_VOUT0) |
					(1 << ISP_OUTPUT_PORT_VOUT1) |
					(1 << ISP_OUTPUT_PORT_VOUT2);
				break;
			case V4L2_PIX_FMT_NV12M:
				f->fmt.pix_mp.field =
					(1 << ISP_OUTPUT_PORT_VOUT1) |
					(1 << ISP_OUTPUT_PORT_VOUT2);
				pstream->outport =
					(1 << ISP_OUTPUT_PORT_VOUT1) |
					(1 << ISP_OUTPUT_PORT_VOUT2);
				break;
			default:
				f->fmt.pix_mp.field =
					(1 << ISP_OUTPUT_PORT_VOUT1) |
					(1 << ISP_OUTPUT_PORT_VOUT2);
				pstream->outport =
					(1 << ISP_OUTPUT_PORT_VOUT1) |
					(1 << ISP_OUTPUT_PORT_VOUT2);
				break;
		}
	}

	LOG(LOG_DEBUG, "[Stream#%d] - New fmt - width: %4u, height: %4u, format: 0x%x, type: " \
	    "%5u. outport: %u[%u]",
	    pstream->stream_id, pstream->cur_v4l2_fmt.fmt.pix_mp.width,
	    pstream->cur_v4l2_fmt.fmt.pix_mp.height,
	    pstream->cur_v4l2_fmt.fmt.pix_mp.pixelformat,
	    pstream->cur_v4l2_fmt.type, pstream->outport, f->fmt.pix_mp.field);

	return rc;
}
