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
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/v4l2-dv-timings.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <media/v4l2-common.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-event.h>
#include <media/v4l2-rect.h>

#include "armcb_isp_driver.h"
#include "armcb_v4l2_core.h"
#include "armcb_v4l2_config.h"
#include "armcb_vb2.h"
#include "system_logger.h"
#include <media/videobuf2-dma-contig.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_ISP
#endif

static int armcb_vb2_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				 unsigned int *nplanes, unsigned int sizes[],
				 struct device *alloc_devs[])
{
	int i = 0;
	static unsigned long cnt;

	armcb_v4l2_stream_t *pstream = vb2_get_drv_priv(vq);
	struct v4l2_format vfmt;

	LOG(LOG_INFO, "Enter id:%d, cnt: %lu.", pstream->stream_id, cnt++);
	LOG(LOG_INFO, "vq: %p, *nplanes: %u.", vq, *nplanes);

	// get current format
	if (armcb_v4l2_stream_get_format(pstream, &vfmt) < 0) {
		LOG(LOG_ERR, "fail to get format from stream");
		return -EBUSY;
	}

	/* we just need to use one plane to store our image */
	if (vfmt.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (WARN_ON(vfmt.fmt.pix_mp.num_planes > VIDEO_MAX_PLANES))
			goto done;

		*nplanes = vfmt.fmt.pix_mp.num_planes;
		for (i = 0; i < vfmt.fmt.pix_mp.num_planes; i++)
			sizes[i] = vfmt.fmt.pix_mp.plane_fmt[i].sizeimage;
	} else {
		LOG(LOG_ERR, "Unsupported buf type :%d", vfmt.type);
		goto done;
	}

done:
	return 0;
}

static void armcb_vb2_buf_finish(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

	vbuf->flags |= V4L2_BUF_FLAG_TIMECODE;
}

static void armcb_vb2_buf_queue(struct vb2_buffer *vb)
{
	armcb_v4l2_stream_t *pstream = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vvb = to_vb2_v4l2_buffer(vb);
	armcb_v4l2_buffer_t *buf = container_of(vvb, armcb_v4l2_buffer_t, vvb);
	unsigned long flags;

	if(pstream) {
		spin_lock_irqsave(&pstream->slock, flags);
		list_add_tail(&buf->list, &pstream->stream_buffer_list);
		spin_unlock_irqrestore(&pstream->slock, flags);
	}

	return;
}
#ifndef V4L2_OPT
static void *armcb_vb2_cma_get_userptr(struct vb2_buffer *vb,
					   struct device *alloc_ctx,
					   unsigned long vaddr, unsigned long size)
{
	struct armcb_vb2_private_data *priv;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return ERR_PTR(-ENOMEM);
	LOG(LOG_DEBUG, "get userptr 0x%x size %d", vaddr, size);
	priv->vaddr = (void *)vaddr;
	priv->size = size;
	priv->alloc_ctx = alloc_ctx;
	return priv;
}

static void armcb_vb2_cma_put_userptr(void *buf_priv)
{
	kfree(buf_priv);
}

static void *armcb_vb2_cma_vaddr(struct vb2_buffer *vb, void *buf_priv)
{
	struct armcb_vb2_private_data *buf = buf_priv;

	if (!buf->vaddr) {
		LOG(LOG_ERR,
			"Address of an unallocated plane requested or cannot map user pointer");
		return NULL;
	}

	LOG(LOG_DEBUG, "addr=%p", buf->vaddr);
	return buf->vaddr;
}

static const struct vb2_mem_ops armcb_vb2_get_q_mem_op = {
	.get_userptr = armcb_vb2_cma_get_userptr,
	.put_userptr = armcb_vb2_cma_put_userptr,
	.vaddr = armcb_vb2_cma_vaddr,
};
#endif

const struct vb2_mem_ops *armcb_vb2_get_q_mem_ops(void)
{
#ifndef V4L2_OPT
	return &armcb_vb2_get_q_mem_op;
#else
	return &vb2_dma_contig_memops;
#endif
}

static int cap_vb2_start_streaming(struct vb2_queue *q, unsigned int count)
{
	LOG(LOG_DEBUG, "function enter");
	LOG(LOG_DEBUG, "function exit");
	return 0;
}

static void cap_vb2_stop_streaming(struct vb2_queue *q)
{
	armcb_v4l2_stream_t *pstream = vb2_get_drv_priv(q);
	LOG(LOG_INFO, "function enter");
	pstream->stream_started = 0;
	LOG(LOG_INFO, "function exit");

	return;
}

static struct vb2_ops armcb_vid_cap_qops = {
	.queue_setup = armcb_vb2_queue_setup,
	.buf_queue = armcb_vb2_buf_queue,
	.buf_finish = armcb_vb2_buf_finish,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,

	.start_streaming = cap_vb2_start_streaming,
	.stop_streaming = cap_vb2_stop_streaming,
};

struct vb2_ops *armcb_vb2_get_q_ops(void)
{
	return &armcb_vid_cap_qops;
}

extern struct device *mem_dev;
int isp_vb2_queue_init(struct vb2_queue *q, struct mutex *mlock,
			   armcb_v4l2_stream_t *pstream, struct device *dev)
{
	int ret = 0;
	memset(q, 0, sizeof(struct vb2_queue));

	/* start creating the vb2 queues */

	// all stream multiplanar
	q->type = pstream->cur_v4l2_fmt.type;

	LOG(LOG_DEBUG, "vb2 init for stream:%d type: %u.", pstream->stream_id,
		q->type);

	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	q->io_modes = VB2_USERPTR | VB2_MMAP;
	q->drv_priv = pstream;
	q->buf_struct_size = sizeof(armcb_v4l2_buffer_t);
	q->ops = armcb_vb2_get_q_ops();
	q->mem_ops = armcb_vb2_get_q_mem_ops();
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_buffers_needed = 1;
	q->lock = mlock;
	q->dev = mem_dev;
#ifndef V4L2_OPT
	q->io_modes = VB2_USERPTR | VB2_READ;
	q->dev = dev;
	q->non_coherent_mem = 1;
#endif

	ret = vb2_queue_init(q);
	q->memory = V4L2_MEMORY_MMAP;

	return ret;
}

extern armcb_v4l2_stream_t *g_outport_map[ARMCB_MAX_DEVS][ISP_OUTPUT_PORT_MAX];

int isp_vb2_queue_release(struct vb2_queue *q, struct file *file)
{
	int ret = 0, i = 0, j = 0;

	/* Set the g_outport_map to 0 to avoid the next loop outport is busy*/
	for(i = 0; i < ARMCB_MAX_DEVS; i++  )
	{
		for(j = 0; j < ISP_OUTPUT_PORT_MAX; j++)
			g_outport_map[i][j] = NULL;
	}

	LOG(LOG_INFO, "#### Armcb isp release all resource and exit !!!");

	return ret;
}
