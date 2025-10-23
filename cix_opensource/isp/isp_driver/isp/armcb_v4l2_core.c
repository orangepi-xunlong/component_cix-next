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

#include "armcb_v4l2_core.h"
#include "armcb_camera_io_drv.h"
#include "armcb_isp.h"
#include "armcb_isp_driver.h"
#include "armcb_platform.h"
#include "armcb_register.h"
#include "armcb_v4l2_config.h"
#include "armcb_v4l2_stream.h"
#include "armcb_v4l_sd.h"
#include "armcb_vb2.h"
#include "asm-generic/errno-base.h"
#include "isp_hw_ops.h"
#include "linux/types.h"
#include "media/media-device.h"
#include "media/media-entity.h"
#include "system_dma.h"
#include "system_logger.h"
#include <linux/errno.h>
#include <linux/font.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/v4l2-dv-timings.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-vmalloc.h>
#include <linux/soc/cix/rdr_pub.h>
#include <linux/ktime.h>
#include <linux/soc/cix/cix_ddr_lp.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_ISP
#endif

#define ARMCB_MODULE_NAME "armcb_isp_v4l2"
#define MAX_PLANES_NUM 2
#define READY_TIME 3500

extern int armcb_multi_cam;
void * discard_buf_addr;
u32 discard_buf_size;
dma_addr_t discard_buf_addr_dma;
dma_addr_t discard_dma_handle;

static armcb_v4l2_dev_t *g_isp_v4l2_devs[ARMCB_MAX_DEVS] = { 0 };
static uint32_t outport_array[ARMCB_MAX_DEVS][V4L2_STREAM_TYPE_MAX];
static int g_adev_idx;
armcb_v4l2_stream_t
	*g_outport_map[ARMCB_MAX_DEVS][ISP_OUTPUT_PORT_MAX] = { 0 };


static const int vout_idx[5] = {
	ISP_OUTPUT_PORT_VOUT0,
	ISP_OUTPUT_PORT_VOUT1,
	ISP_OUTPUT_PORT_VOUT3,
	ISP_OUTPUT_PORT_VOUT5,
	ISP_OUTPUT_PORT_VOUT7
};

/// define isp port token list
static char *const g_IspPortToken[] = {
	[ISP_OUTPUT_PORT_VIN] = (char *const) "VIN",
	[ISP_OUTPUT_PORT_3A] = (char *const) "3A",
	[ISP_OUTPUT_PORT_VOUT0] = (char *const) "VOUT0",
	[ISP_OUTPUT_PORT_VOUT1] = (char *const) "VOUT1",
	[ISP_OUTPUT_PORT_VOUT2] = (char *const) "VOUT2",
	[ISP_OUTPUT_PORT_VOUT3] = (char *const) "VOUT3",
	[ISP_OUTPUT_PORT_VOUT4] = (char *const) "VOUT4",
	[ISP_OUTPUT_PORT_VOUT5] = (char *const) "VOUT5",
	[ISP_OUTPUT_PORT_VOUT6] = (char *const) "VOUT6",
	[ISP_OUTPUT_PORT_VOUT7] = (char *const) "VOUT7",
	[ISP_OUTPUT_PORT_VOUT8] = (char *const) "VOUT8",
	[ISP_OUTPUT_PORT_VOUT9] = (char *const) "VOUT9",

	[ISP_OUTPUT_PORT_MAX] = NULL,
};

static int armcb_isp_vb2_streamoff(struct file *file);

pid_t isp_getpid(void)
{
	return current->tgid;
}

struct isp_v4l2_fmt isp_src_formats[] = {
	{
		.name = "NV12M",
		.fourcc = V4L2_PIX_FMT_NV12M,
		.depth = { 8, 8 },
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

armcb_v4l2_stream_t *armcb_v4l2_get_stream(uint32_t ctx_id, int stream_id)
{
	armcb_v4l2_stream_t *pstream = NULL;

	pstream = g_isp_v4l2_devs[ctx_id]->pstreams[stream_id];

	return pstream;
}

/* ----------------------------------------------------------------
 * stream finder utility function
 */
int armcb_v4l2_find_stream(armcb_v4l2_stream_t **ppstream, uint32_t ctx_id,
			   int stream_type)
{
	int stream_id = 0;
	*ppstream = NULL;

	if (stream_type >= V4L2_STREAM_TYPE_MAX || stream_type < 0 ||
		ctx_id >= ARMCB_MAX_DEVS) {
		LOG(LOG_ERR, "stream_id=%d, ctx_id=%d", stream_id, ctx_id);
		return -EINVAL;
	}

	if (g_isp_v4l2_devs[ctx_id] == NULL) {
		LOG(LOG_ERR, "ctx %d dev is NULL", ctx_id);
		return -EBUSY;
	}

	stream_id = g_isp_v4l2_devs[ctx_id]->stream_id_index[stream_type];
	if (stream_id < 0 || stream_id >= V4L2_STREAM_TYPE_MAX ||
		g_isp_v4l2_devs[ctx_id]->pstreams[stream_id] == NULL) {
		LOG(LOG_DEBUG, "stream_type:%d stream_id:%d", stream_type,
			stream_id);
		return -ENODEV;
	}

	*ppstream = g_isp_v4l2_devs[ctx_id]->pstreams[stream_id];
	LOG(LOG_DEBUG, "ctx_id=%d stream_id=%d stream=%p", ctx_id, stream_id,
		*ppstream);

	return 0;
}

int armcb_v4l2_find_ctx_stream_by_outport(uint32_t outport, uint32_t *p_ctx_id,
					  uint32_t *p_stream_id)
{
	uint32_t ctx_id = 0;
	uint32_t stream_id = 0;

	if (outport < 0 || p_ctx_id == NULL || p_stream_id == NULL) {
		LOG(LOG_ERR, "invalid parameter");
		return -EINVAL;
	}

	for (ctx_id = 0; ctx_id < ARMCB_MAX_DEVS; ctx_id++) {
		for (stream_id = 0; stream_id < V4L2_STREAM_TYPE_MAX;
			 stream_id++) {
			if (outport & outport_array[ctx_id][stream_id])
				goto exit;
		}
	}

	if (ctx_id == ARMCB_MAX_DEVS) {
		*p_ctx_id = -1;
		*p_stream_id = -1;
		LOG(LOG_ERR,
			"failed to find a valid ctx_id and stream_id for outport:%d",
			outport);
		return -EINVAL;
	}

exit:
	*p_ctx_id = ctx_id;
	*p_stream_id = stream_id;

	LOG(LOG_DEBUG, "success find ctx_id:%d stream_id:%d for outport:%d",
		ctx_id, stream_id, outport);
	return 0;
}

int armcb_v4l2_find_stream_by_outport_ctx(uint32_t outport, uint32_t ctx_id,
					  uint32_t *p_stream_id)
{
	uint32_t stream_id = 0;

	if (outport < 0 || ctx_id < 0 || p_stream_id == NULL) {
		LOG(LOG_ERR, "invalid parameter");
		return -EINVAL;
	}

	for (stream_id = 0; stream_id < V4L2_STREAM_TYPE_MAX; stream_id++) {
		if (outport & outport_array[ctx_id][stream_id])
			goto exit;
	}

	if (stream_id == V4L2_STREAM_TYPE_MAX) {
		*p_stream_id = -1;
		LOG(LOG_DEBUG,
			"failed to find a valid stream_id for outport:%d and ctx_id:%d",
			outport, ctx_id);
		return -EINVAL;
	}

exit:
	*p_stream_id = stream_id;

	LOG(LOG_DEBUG, "success find stream_id:%d for outport:%d and ctx_id:%d",
		stream_id, outport, ctx_id);
	return 0;
}

extern struct device *mem_dev;
void armcb_isp_invalid_cache (struct vb2_buffer *vb,
					armcb_v4l2_buffer_t *pbuf)
{
	struct sg_table sgt0;
	struct sg_table sgt1;
	struct device *dev = mem_dev;

	void* dma_vaddr_0;
	void* dma_vaddr_1;
	dma_addr_t dma_addr_0;
	dma_addr_t dma_addr_1;

	if((!dev) || (pbuf->vvb.vb2_buf.num_planes > MAX_PLANES_NUM))
		return;

	dma_vaddr_0 = vb2_plane_vaddr(&pbuf->vvb.vb2_buf, 0);
	dma_addr_0 = vb2_dma_contig_plane_dma_addr(vb, 0);

	dma_get_sgtable_attrs(dev, &sgt0, dma_vaddr_0, dma_addr_0,
			vb->planes[0].length, 0);

	dma_sync_sgtable_for_cpu(dev, &sgt0, DMA_FROM_DEVICE);

	if (pbuf->vvb.vb2_buf.num_planes > 1) {
		dma_vaddr_1 = vb2_plane_vaddr(&pbuf->vvb.vb2_buf, 1);
		dma_addr_1 = vb2_dma_contig_plane_dma_addr(vb, 1);

		dma_get_sgtable_attrs(dev, &sgt1, dma_vaddr_1, dma_addr_1,
				vb->planes[1].length, 0);

		dma_sync_sgtable_for_cpu(dev, &sgt1, DMA_FROM_DEVICE);
	}
}

void armcb_isp_put_frame(uint32_t ctx_id, int stream_id, isp_output_port_t port)
{
	int rc = 0;
	unsigned long flags;
	armcb_v4l2_stream_t *pstream = NULL;
	armcb_v4l2_buffer_t *pbuf = NULL;
	struct vb2_buffer *vb = NULL;
	armcb_v4l2_dev_t *dev = NULL;

	static armcb_v4l2_buffer_t *splastbuf;
	armcb_v4l2_buffer_t *plastbuf = NULL;

	dev = armcb_v4l2_core_get_dev(ctx_id);

	if (stream_id < 0 && port < ISP_OUTPUT_PORT_MAX) {
		pstream = g_outport_map[ctx_id][port];
	} else {
		/* find stream pointer */
		rc = armcb_v4l2_find_stream(&pstream, ctx_id, stream_id);
		if (rc < 0) {
			LOG(LOG_WARN,
				"can't find stream on ctx %d stream_id %d (errno = %d)",
				ctx_id, stream_id, rc);
			return;
		}
	}

	LOG(LOG_DEBUG,
		"ctx_id:%d Stream#%d fmt(%d*%d %d %d) outport(%d %s) streamType(%d) "
		"reserved_buf_addr(0x%x)",
		ctx_id, pstream->stream_id, pstream->cur_v4l2_fmt.fmt.pix_mp.width,
		pstream->cur_v4l2_fmt.fmt.pix_mp.height,
		pstream->cur_v4l2_fmt.fmt.pix_mp.pixelformat,
		pstream->cur_v4l2_fmt.type, pstream->outport, g_IspPortToken[port],
		pstream->stream_type, pstream->reserved_buf_addr);

	/* check if stream is on */
	if (!pstream || !pstream->stream_started) {
		LOG(LOG_DEBUG, "[Stream#%d] is not started yet on ctx %d",
			stream_id, ctx_id);
		return;
	}

	LOG(LOG_DEBUG, "ctx_id:%d [Stream#%d] %p", ctx_id, pstream->stream_id,
		pstream);


	spin_lock_irqsave(&pstream->slock, flags);

	plastbuf = list_last_entry(&(pstream->stream_buffer_list_busy),
				   armcb_v4l2_buffer_t, list);

	/* try to get an active buffer from vb2 queue  */
	if (!list_empty(&pstream->stream_buffer_list_busy)) {
		if (!list_is_singular(&pstream->stream_buffer_list_busy) ||
			(plastbuf == splastbuf)) {
			pbuf = list_entry(pstream->stream_buffer_list_busy.next,
					  armcb_v4l2_buffer_t, list);
			if(pbuf)
				list_del(&pbuf->list);
		}
	}

	if (!pbuf) {
		/// @TODO: need to use reserved buffer to hw output.
		LOG(LOG_DEBUG, "[Stream#%d] type: %d no buffers, use reserved buffer",
			pstream->stream_id, V4L2_STREAM_TYPE_VIDEO);
		goto spin_unlock;
	}

	splastbuf = list_last_entry(&(pstream->stream_buffer_list_busy),
					armcb_v4l2_buffer_t, list);

	spin_unlock_irqrestore(&pstream->slock, flags);

	vb = &pbuf->vvb.vb2_buf;

	/*To sync the write done interrupts and the active buffer status*/
	if((vb->state != VB2_BUF_STATE_ACTIVE) ||
		(vb->memory != VB2_MEMORY_MMAP))
	{
		LOG(LOG_ERR, "VB buffer is not active");
		goto exit;
	}

	if (!pbuf->vvb.vb2_buf.vb2_queue)
		goto exit;

	vb->planes[0].bytesused = vb->planes[0].length;
	vb->planes[1].bytesused = vb->planes[1].length;

	if(atomic_read(&dev->upload_streamoff) == 1)
		goto exit;

	/* Invalid cache before CPU read buffer to avoid the cache
	 * line issue */
	armcb_isp_invalid_cache(vb, pbuf);
	vb->timestamp = ktime_get_ns();
	vb2_buffer_done(vb, VB2_BUF_STATE_DONE);

	LOG(LOG_DEBUG, "%s put frame success ctx_id:%d stream_id:%d",
               g_IspPortToken[port], ctx_id, stream_id);
	return;

spin_unlock:
	spin_unlock_irqrestore(&pstream->slock, flags);

exit:
	return;

}

static int armcb_v4l2_querycap(struct file *file, void *priv,
				   struct v4l2_capability *cap)
{
	armcb_v4l2_dev_t *dev = video_drvdata(file);

	strcpy(cap->driver, "arm-china-isp");
	strcpy(cap->card, "linlon isp v4l2");
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev->v4l2_dev.name);

	/* V4L2_CAP_VIDEO_CAPTURE_MPLANE */
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING |
			   V4L2_CAP_READWRITE;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	// cap->capabilities = dev->vid_cap_caps | V4L2_CAP_DEVICE_CAPS;
	LOG(LOG_DEBUG, "capabilities(0x%x)", cap->capabilities);

	return 0;
}

static int armcb_v4l2_log_status(struct file *file, void *fh)
{
	return v4l2_ctrl_log_status(file, fh);
}

static int armcb_v4l2_fh_release(struct file *file)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct v4l2_event_subscription sub;
	int ret = 0;

	LOG(LOG_DEBUG, "isp_v4l2 close: ctx_id: %d, called for sid:%d.",
		dev->ctx_id, sp->stream_id);
	if (sp) {
		/* unsubscribe event when close file */
		memset(&sub, 0, sizeof(sub));
		sub.type = V4L2_EVENT_ALL;
		ret = v4l2_event_unsubscribe(&sp->fh, &sub);
		LOG(LOG_DEBUG,
				"armcb_fop_release v4l2_event_unsubscribe, ret = %d", ret);
		v4l2_fh_del(&sp->fh);
		v4l2_fh_exit(&sp->fh);
	}

	kfree(sp);

	return ret;
}

int armcb_disable_irq(void)
{
	int ret = 0;

	armcb_i7_disable_int();
	armcb_i7_disable_vin();

	return ret;
}

static int armcb_release_output_port(void)
{
    int rc = 0, i = 0, j = 0;

    for(i = 0; i < ARMCB_MAX_DEVS; i++  )
    {
        for(j = 0; j < ISP_OUTPUT_PORT_MAX; j++)
            g_outport_map[i][j] = NULL;
    }

    LOG(LOG_INFO, "#### Armcb release all port resource !!!");
    return rc;
}

// Get current application PID

static int find_user_process_by_name(const char *name)
{
	int rc = -1;
	struct task_struct *curr;
	curr = current;

	if (curr->mm != NULL) {
		printk(KERN_INFO "The PID is tgid:%d, user_pid: %s\n",
               curr->tgid, curr->comm);
		rc = 0;
	} else {
		printk(KERN_INFO "The PID is kernel_pid: %s\n", curr->comm);
		rc = -1;
	}

	rc = strncmp(curr->comm, name, 7);
	LOG(LOG_INFO, "The pid is %s, and the rc:%d \n",curr->comm, rc);

	return rc;
}

static int armcb_v4l2_fop_release(struct file *file)
{
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct video_device *vdev = &dev->vid_cap_dev;
	struct vb2_queue *q = vdev->queue;
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	armcb_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];
	int outport_idx = -1;
	int rc = 0;

	mutex_lock(&dev->queue_lock);
	atomic_sub_return(1, &dev->opened);

	/*There the isp_app and the 3rd application will do close fd, and will
	 * meet follow cases when 3rd app close the fd,
	 * default: 3rd app just close, the release file handle.
	 * case 1: 3rd app is streaming, and close the fd, upload event to isp_app.
	 * case 2: 3rd app is s_fmt, but not start streaming, release buf and clear flags.
	 * case 3: 3rd app is streamoff, and close the fd, clear the flags.
	 * */

	/* case1. The stream is started, and need to close the fd,
	 * report the event to isp_app to do deinit.
	 * */
	if ((atomic_read(&dev->stream_on_cnt) == STREAM_ON) && (vdev->queue)) {
		unsigned int *pData = NULL;
		struct v4l2_event ev;
		memset(&ev, 0, sizeof(struct v4l2_event));
		ev.id = ISP_DAEMON_EVENT_STREAM_OFF;
		ev.type = V4L2_EVENT_CTRL;

		/* Queue a empty event to notify userspace */
		pData = (unsigned int *)ev.u.data;
		pData[0] = 0xff;
		v4l2_event_queue(&dev->vid_cap_dev, &ev);
		LOG(LOG_INFO, "Upload streamoff event");

		atomic_set(&dev->upload_streamoff, 1);

		goto exit;
	}

	LOG(LOG_INFO, "##### %s, stream_on_cnt:%d, port_idx_release:%d\n",
			dev_name(&vdev->dev),
			atomic_read(&dev->stream_on_cnt),
			atomic_read(&dev->port_idx_release));

	/* case2. The stream is just s_fmt but not started, and need to close
	 * the fd, just release buf and outport port, clear the atomic flags
	 * to avoid the outport busy and wrong status.
	 */
	if((atomic_read(&dev->stream_on_cnt) == STREAM_DEFAULT) && (vdev->queue)
			&& (armcb_multi_cam == 0)
			&& (atomic_read(&dev->port_idx_release) == OUTPORT_IDX_IS_FREE)) {
		rc = find_user_process_by_name("isp_app");
		if(!rc) {
			LOG(LOG_INFO, "#### The pid is isp_app !");
			goto exit;
		}

		if(dev->buf_type == q->type) {
			LOG(LOG_DEBUG, " ##### %s, vb2 release\n", dev_name(&vdev->dev));
			rc = destroy_buf_queue(q, VB2_BUF_STATE_ERROR);
			if(!rc) {
				vb2_queue_release(q);
				vdev->queue = NULL;
			}
		}

		dev->stream_mask &= ~(1 << sp->stream_id);

		/* deinit stream */
		if (pstream) {
			outport_idx = armcb_outport_bits_to_idx(pstream->outport);
			if (outport_idx >= 0 && outport_idx < ISP_OUTPUT_PORT_MAX)
				g_outport_map[sp->ctx_id][outport_idx] = NULL;
			if (pstream->stream_type < V4L2_STREAM_TYPE_MAX)
				dev->stream_id_index[pstream->stream_type] = -1;
			armcb_v4l2_stream_deinit(pstream, dev);
			dev->pstreams[sp->stream_id] = NULL;
		}

		msleep(READY_TIME);
		/*deinit the stream*/
		dev->is_streaming = 0;
		goto deinit;
	}

	/* When the stream is stream off and the buffer is released, it should
	 * set the g_outport_map to 0 to avoid the next loop outport is busy
	 */
	/* case3. The stream is streamoff, and need to close the fd, clear
	 * the outport port, clear the atomic flags to avoid the outport busy.
	 */
	if ((atomic_read(&dev->stream_on_cnt) == STREAM_OFF)
			&& (atomic_read(&dev->port_idx_release) == OUTPORT_IDX_IS_BUSY)
			&& (!vdev->queue))
	{
		atomic_set(&dev->upload_streamoff, 0);
		goto deinit;
	}

	if ((atomic_read(&dev->stream_on_cnt) == STREAM_OFF)
			&& (armcb_multi_cam == 0)
			&& (atomic_read(&dev->port_idx_release) == OUTPORT_IDX_IS_FREE))
	{
		goto deinit;
	}

	armcb_v4l2_fh_release(file);
	LOG(LOG_INFO, "release v4l2 fp success");
	mutex_unlock(&dev->queue_lock);
	return rc;

deinit:
	armcb_release_output_port();
	atomic_set(&dev->stream_on_cnt, STREAM_DEFAULT);
	atomic_set(&dev->port_idx_release, OUTPORT_IDX_DEFAULT);

exit:
	/* release file handle */
	armcb_v4l2_fh_release(file);
	LOG(LOG_INFO, "release v4l2 fp success");
	mutex_unlock(&dev->queue_lock);
	return rc;
}

static int armcb_v4l2_fh_open(struct file *file)
{
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct armcb_isp_v4l2_fh *sp = NULL;
	int stream_opened = 0;

	sp = kzalloc(sizeof(struct armcb_isp_v4l2_fh), GFP_KERNEL);
	if (!sp)
		return -ENOMEM;

	stream_opened = atomic_read(&dev->opened);
	if (stream_opened >= V4L2_STREAM_TYPE_MAX) {
		LOG(LOG_ERR,
			"too many open streams, stream_opened: %d, max: %d.",
			stream_opened, V4L2_STREAM_TYPE_MAX);
		kfree(sp);
		return -EBUSY;
	}

	file->private_data = &sp->fh;

	v4l2_fh_init(&sp->fh, &dev->vid_cap_dev);
	v4l2_fh_add(&sp->fh);

	LOG(LOG_INFO, "open v4l2 fp success");
	return 0;
}

static int armcb_v4l2_fop_open(struct file *filp)
{
	armcb_v4l2_dev_t *dev = video_drvdata(filp);
	int ret = -1;

	ret = armcb_v4l2_fh_open(filp);
	if (ret < 0) {
		LOG(LOG_ERR, "Error, file handle open fail (rc=%d)", ret);
		goto open_fail;
	}

	atomic_add(1, &dev->opened);

	LOG(LOG_DEBUG, "open v4l2 fp success");
open_fail:
	return ret;
}

static ssize_t armcb_v4l2_fop_write(struct file *filep, const char __user *buf,
					size_t count, loff_t *ppos)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(filep->private_data);
	int rc = 0;

	if (sp->vb2_q.lock && mutex_lock_interruptible(sp->vb2_q.lock))
		return -ERESTARTSYS;

	rc = vb2_write(&sp->vb2_q, buf, count, ppos,
			   filep->f_flags & O_NONBLOCK);

	if (sp->vb2_q.lock)
		mutex_unlock(sp->vb2_q.lock);
	return rc;
}

static ssize_t armcb_v4l2_fop_read(struct file *filep, char __user *buf,
				   size_t count, loff_t *ppos)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(filep->private_data);
	int rc = 0;

	if (sp->vb2_q.lock && mutex_lock_interruptible(sp->vb2_q.lock))
		return -ERESTARTSYS;

	rc = vb2_read(&sp->vb2_q, buf, count, ppos,
			  filep->f_flags & O_NONBLOCK);

	if (sp->vb2_q.lock)
		mutex_unlock(sp->vb2_q.lock);
	return rc;
}

static unsigned int armcb_v4l2_fop_poll(struct file *filep,
					struct poll_table_struct *wait)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(filep->private_data);
	int rc = 0;

	if (sp->vb2_q.lock && mutex_lock_interruptible(sp->vb2_q.lock))
		return POLLERR;

	rc = vb2_poll(&sp->vb2_q, filep, wait);

	if (sp->vb2_q.lock)
		mutex_unlock(sp->vb2_q.lock);

	return rc;
}

static int armcb_v4l2_fop_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	int rc = 0;

	rc = vb2_mmap(&sp->vb2_q, vma);

	return rc;
}

static const struct v4l2_file_operations armcb_fops = {
	.owner = THIS_MODULE,
	.open = armcb_v4l2_fop_open,
	.release = armcb_v4l2_fop_release,
	.read = armcb_v4l2_fop_read,
	.write = armcb_v4l2_fop_write,
	.poll = armcb_v4l2_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = armcb_v4l2_fop_mmap,
};

/* Per-stream control operations */
static inline bool armcb_v4l2_is_q_busy(struct vb2_queue *queue,
					struct file *file)
{
	if (queue->owner && queue->owner != file->private_data)
		LOG(LOG_ERR, "vb2_queue %p is busy!", queue);

	return queue->owner && queue->owner != file->private_data;
}

static int armcb_v4l2_streamon(struct file *file, void *priv,
				   enum v4l2_buf_type buf_type)
{
	int rc = -1;
	struct v4l2_event ev;
	unsigned int *pData = NULL;
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct video_device *vdev = &dev->vid_cap_dev;

	struct armcb_isp_v4l2_fh *sp = fh_to_private(priv);
	armcb_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];

	if (dev->is_streaming) {
		LOG(LOG_ERR, "%s streaming is busy\n", dev_name(&vdev->dev));
		return -EBUSY;
	}

	if (armcb_v4l2_is_q_busy(&sp->vb2_q, file))
		return -EBUSY;

	LOG(LOG_DEBUG, "buf_type:%d\n", buf_type);
	memset(&ev, 0, sizeof(struct v4l2_event));
	ev.id = ISP_DAEMON_EVENT_STREAM_ON;
	ev.type = V4L2_EVENT_CTRL;

	pData = (unsigned int *)ev.u.data;
	pData[0] = 0xff;
	v4l2_event_queue(&dev->vid_cap_dev, &ev);

	mutex_lock(&dev->queue_lock);
	dev->buf_type = buf_type;
	mutex_unlock(&dev->queue_lock);

	rc = vb2_streamon(&sp->vb2_q, buf_type);
	if (rc != 0) {
		LOG(LOG_ERR, "fail to vb2_streamon :%d. (rc=%d)", sp->stream_id, rc);
		return rc;
	}

	/*config first frame output address*/
#ifndef V4L2_OPT
	rc = armcb_v4l2_config_update_stream_vin_addr(pstream);
	if (rc != 0) {
		LOG(LOG_ERR,
			"fail to update stream vin addr. (stream_id = %d, rc=%d)",
			sp->stream_id, rc);
	}

	rc = armcb_v4l2_config_update_stream_hw_addr(pstream);
	if (rc != 0) {
		LOG(LOG_ERR,
			"fail to update stream output addr. (stream_id = %d, rc=%d)",
			sp->stream_id, rc);
	}
#endif

	rc = armcb_v4l2_stream_on(pstream);
	if (rc != 0) {
		LOG(LOG_ERR, "fail to isp_stream_on. (stream_id = %d, rc=%d)",
			sp->stream_id, rc);
		return rc;
	}

	dev->is_streaming = 1;
	dev->streaming_pid = isp_getpid();

	atomic_set(&dev->upload_streamoff, 0);

	if (dev->ddr_lp_mode == 1) {
		cix_set_ddrlp(0);
		dev->ddr_lp_mode = 0;
		usleep_range(1000,1000 + 100);
	}

	return rc;
}

static int armcb_isp_vb2_streamoff(struct file *file)
{
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct video_device *vdev = &dev->vid_cap_dev;
	struct vb2_queue *q = vdev->queue;
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	armcb_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];
	int outport_idx = -1;
	int loop;
	int ret = 0;

	mutex_lock(&dev->queue_lock);

	if (!pstream) {
		goto exit;
	}

	/* To keep the q is not null before do release buffer*/
	if((!q) || (!q->ops) || (!q->dev)) {
		LOG(LOG_ERR, "vb2_queue dev is NULL\n");
		goto exit;
	}

	pstream->stream_started = 0;

	/* Check isp_app stream off the isp hardware, then release the buffer*/
	if((atomic_read(&dev->stream_on_cnt) == 0) && (vdev->queue))
	{
		/* Revert the dev->stream_on_cnt to -1 for check the isp_app
		 * streamon/off status, the file->private_data shoule be the
		 * vdev->queue->owner, if not, the vb2_fos_release will not call
		 * vb2_queue_release. So the V4L2 alloc buffer release by isp_app
		 * need to use vb2_queue_release buffer to avoid this issue.*/
		if(dev->buf_type == q->type) {
			LOG(LOG_INFO, " ##### %s, vb2 release\n", dev_name(&vdev->dev));

			/* destroy buf queue*/
			ret = destroy_buf_queue(q, VB2_BUF_STATE_ERROR);
			if(!ret) {
				vb2_queue_release(q);
				vdev->queue = NULL;
			}

			if (dev->ddr_lp_mode == 0) {
				cix_set_ddrlp(1);
				dev->ddr_lp_mode = 1;
				usleep_range(100,100 + 100);
			}

		}
	}

	/*deinit the stream*/
	for(loop = 0;loop < V4L2_STREAM_TYPE_MAX;loop++) {
		armcb_v4l2_stream_t *pstream = dev->pstreams[loop];
		if(pstream != NULL) {
			dev->stream_mask &= ~(1 << loop);
			/* deinit stream */
			if (pstream) {
				outport_idx = armcb_outport_bits_to_idx(pstream->outport);
				if (outport_idx >= 0 && outport_idx < ISP_OUTPUT_PORT_MAX)
					g_outport_map[dev->ctx_id][outport_idx] = NULL;
				if (pstream->stream_type < V4L2_STREAM_TYPE_MAX)
					dev->stream_id_index[pstream->stream_type] = -1;
				armcb_v4l2_stream_deinit(pstream, dev);
				dev->pstreams[loop] = NULL;
				dev->is_streaming = 0;
			}
		}
	}

exit:
	mutex_unlock(&dev->queue_lock);
	return ret;
}

static int armcb_v4l2_streamoff(struct file *file, void *priv,
				enum v4l2_buf_type buf_type)
{
	int ret = 0;

	return ret;
}

int armcb_v4l2_g_fmt_vid_cap_mplane(struct file *file, void *priv,
					struct v4l2_format *f)
{
	/*we get fmt from userspace*/
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;

	LOG(LOG_DEBUG,
		"GET FMT: width = %d, height = %d, colorspace = %d, pixel_format = %d, " \
		"sizeimage0 = %d, sizeimage1 = %d, bytesperline0 = %d, bytesperline1 = %d",
		pix->width, pix->height, pix->colorspace, pix->pixelformat,
		pix->plane_fmt[0].sizeimage, pix->plane_fmt[1].sizeimage,
		pix->plane_fmt[0].bytesperline, pix->plane_fmt[1].bytesperline);

	return 0;
}

int armcb_v4l2_s_fmt_vid_cap_mplane(struct file *file, void *priv,
					struct v4l2_format *f)
{
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	armcb_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];
	struct vb2_queue *q = &sp->vb2_q;

	int outport_idx = -1;
	int i;
	int rc = 0;
	struct v4l2_event ev;
	unsigned int *pData = NULL;

	LOG(LOG_INFO, "### ctx_id:%d stream_id:%d", dev->ctx_id, sp->stream_id);

	if (vb2_is_busy(q)) {
		LOG(LOG_INFO, "Buffer is busy and set fmt failed!");
		return -EBUSY;
	}

	mutex_lock(&dev->queue_lock);

	for (i = 0; i < V4L2_STREAM_TYPE_MAX; i++) {
		if ((dev->stream_mask & (1 << i)) == 0) {
			dev->stream_mask |= (1 << i);
			sp->stream_id = i;
			sp->ctx_id = dev->ctx_id;
			break;
		}
	}

	sp = fh_to_private(file->private_data);
	LOG(LOG_INFO, "ctx_id: %d, called for stream id:%d.",
						dev->ctx_id, sp->stream_id);

	/* init stream */
	armcb_v4l2_stream_init(&dev->pstreams[sp->stream_id], sp->stream_id,
				   dev->ctx_id);
	pstream = dev->pstreams[sp->stream_id];
	if (pstream == NULL) {
		LOG(LOG_ERR, "stream alloc failed\n");
		mutex_unlock(&dev->queue_lock);
		return -ENOMEM;
	}

	/* init vb2 queue */
	rc = isp_vb2_queue_init(&sp->vb2_q, &dev->mutex,
				dev->pstreams[sp->stream_id],
				dev->v4l2_dev.dev);
	if (rc < 0) {
		LOG(LOG_ERR, "Error, vb2 queue init fail (rc=%d)", rc);
		//goto vb2_q_fail;
	}

	/* update stream pointer */
	rc = armcb_v4l2_stream_set_format(pstream, f);
	if (rc < 0) {
		LOG(LOG_ERR, "set format failed.");
		mutex_unlock(&dev->queue_lock);
		return rc;
	}

	atomic_set(&dev->port_idx_release, OUTPORT_IDX_IS_FREE);
	/* update stream pointer index */
	dev->stream_id_index[pstream->stream_type] = pstream->stream_id;
	outport_array[sp->ctx_id][sp->stream_id] = pstream->outport;
	outport_idx = armcb_outport_bits_to_idx(pstream->outport);
	if (outport_idx < 0 || outport_idx >= ISP_OUTPUT_PORT_MAX) {
		rc = -EINVAL;
		LOG(LOG_ERR, "invalid outport idx:%d, bits:%#x\n", outport_idx,
			pstream->outport);
		mutex_unlock(&dev->queue_lock);
		return rc;
	}

	if (g_outport_map[sp->ctx_id][outport_idx]) {
		rc = -EINVAL;
		LOG(LOG_ERR, "busy outport idx:%d, bits:%#x\n", outport_idx,
			pstream->outport);
		mutex_unlock(&dev->queue_lock);
		return rc;
	}

	g_outport_map[sp->ctx_id][outport_idx] = pstream;
	memset(&ev, 0, sizeof(struct v4l2_event));
	ev.id = ISP_DAEMON_EVENT_SET_IMG_SIZE;
	ev.type = V4L2_EVENT_CTRL;

	pData = (unsigned int *)ev.u.data;
	pData[0] = dev->ctx_id;
	pData[1] = f->fmt.pix_mp.width;
	pData[2] = f->fmt.pix_mp.height;
	pData[3] = f->fmt.pix_mp.pixelformat;
	pData[4] = 30;
	v4l2_event_queue(&dev->vid_cap_dev, &ev);

    LOG(LOG_DEBUG, "pstream:%p, ctx_id:%d stream_id:%d stream_type:%d, outport_idx:%d, outport:%d",
		pstream, pstream->ctx_id, pstream->stream_id,
		pstream->stream_type, outport_idx, pstream->outport);

	mutex_unlock(&dev->queue_lock);

	return 0;
}

int armcb_v4l2_try_fmt_vid_cap_mplane(struct file *file, void *priv,
					  struct v4l2_format *f)
{
	/*empty function*/
	/*we get fmt from userspace*/
	return 0;
}

/* vb2 customization for multi-stream support */
int armcb_v4l2_reqbufs(struct file *file, void *priv,
				  struct v4l2_requestbuffers *p)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	struct video_device *vdev = video_devdata(file);
	int rc = 0;

	vdev->queue = &sp->vb2_q;

	if (armcb_v4l2_is_q_busy(&sp->vb2_q, file))
		return -EBUSY;

	rc = vb2_reqbufs(&sp->vb2_q, p);
	if (rc == 0)
		sp->vb2_q.owner = p->count ? file->private_data : NULL;

	return rc;
}

/*
static int armcb_v4l2_expbuf(struct file *file, void *priv,
				 struct v4l2_exportbuffer *p)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	int rc = 0;

	if (armcb_v4l2_is_q_busy(&sp->vb2_q, file))
		return -EBUSY;

	rc = vb2_expbuf(&sp->vb2_q, p);
	LOG(LOG_DEBUG, "expbuf sid:%d type:%d index:%d plane:%d rc: %d",
		sp->stream_id, p->type, p->index, p->plane, rc);

	return rc;
}
*/

static int armcb_v4l2_querybuf(struct file *file, void *priv,
				   struct v4l2_buffer *p)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	int rc = 0;

	rc = vb2_querybuf(&sp->vb2_q, p);
	LOG(LOG_DEBUG, "sid:%d querybuf p->type:%d p->index:%d , rc %d",
		sp->stream_id, p->type, p->index, rc);
	return rc;
}

static int armcb_v4l2_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct video_device *vdev = video_devdata(file);
	int rc = 0;
#ifndef V4L2_OPT
	armcb_v4l2_stream_t *pstream = NULL;
#endif
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	armcb_v4l2_dev_t *dev = video_drvdata(file);
#endif
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);

	LOG(LOG_DEBUG, "ctx_id:%d stream_id = %d, stream_q: %p, device_q: %p", sp->ctx_id,
	sp->stream_id, &sp->vb2_q, vdev->queue);

#ifndef V4L2_OPT
	//find stream pointer
	pstream = armcb_v4l2_get_stream(sp->ctx_id, sp->stream_id);
	if (pstream) {
		if (pstream->stream_started == 0) {
			if (p->reserved) {
				LOG(LOG_WARN,
					"set reserved buffer %p userptr:%p", p,
					p->m.planes->m.userptr);
				pstream->reserved_buf_addr =
					(u32)p->m.planes->m.userptr;
				return 0;
			}
		}
	}
#endif

	LOG(LOG_DEBUG, "ctx_id:%d stream_id = %d, ownermatch=%d", sp->ctx_id,
		sp->stream_id, armcb_v4l2_is_q_busy(&sp->vb2_q, file));
	if (armcb_v4l2_is_q_busy(&sp->vb2_q, file))
		return -EBUSY;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	rc = vb2_qbuf(&sp->vb2_q, dev->v4l2_dev.mdev, p);
#else
	rc = vb2_qbuf(&sp->vb2_q, p);
#endif
	LOG(LOG_DEBUG, "ctx_id:%d stream_id:%d qbuf p->type:%d p->index:%d, rc %d",
		sp->ctx_id, sp->stream_id, p->type, p->index, rc);
	return rc;
}

static int armcb_v4l2_dqbuf(struct file *file, void *priv,
				struct v4l2_buffer *p)
{
	struct armcb_isp_v4l2_fh *sp = fh_to_private(file->private_data);
	struct video_device *vdev = video_devdata(file);
	int rc = 0;

	LOG(LOG_DEBUG, "ctx_id:%d stream_id = %d, stream_q: %p, device_q: %p", sp->ctx_id,
		sp->stream_id, &sp->vb2_q, vdev->queue);

	if (armcb_v4l2_is_q_busy(&sp->vb2_q, file))
		return -EBUSY;

	rc = vb2_dqbuf(&sp->vb2_q, p, file->f_flags & O_NONBLOCK);
	LOG_RATELIMITED(
		LOG_DEBUG,
		"ctx_id:%d stream_id:%d dqbuf p->type:%d p->index:%d, rc %d",
		sp->ctx_id, sp->stream_id, p->type, p->index, rc);

	return rc;
}

int armcb_v4l2_enum_fmt_vid_cap(struct file *file, void *priv,
				   struct v4l2_fmtdesc *f)
{
	struct isp_v4l2_fmt *fmt;

	if (f->index >= (int)ARRAY_SIZE(isp_src_formats))
		return -EINVAL;

	fmt = &isp_src_formats[f->index];
	strncpy(f->description, fmt->name, sizeof(f->description) - 1);
	f->pixelformat = fmt->fourcc;
	f->mbus_code = fmt->mbus_code;

	LOG(LOG_INFO, "function exit : %d", f->pixelformat);

	return 0;
}

static int armcb_v4l2_enum_framesizes(struct file *file, void *priv,
					 struct v4l2_frmsizeenum *fsize)
{
	static const struct v4l2_frmsize_discrete frame_sizes[] = {
		{ .width = 1920, .height = 1080 }, // 1080p
		{ .width = 1280, .height = 720 },  // 720p
		{ .width = 640, .height = 480 }    // 480p
	};

	if (fsize->index >= ARRAY_SIZE(frame_sizes))
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete = frame_sizes[fsize->index];

	return 0;
}

/* Create a buffer for discard operation, before frist frame we need to an
 * address for all the vout port
 * */
int armcb_v4l2_alloc_discard_buffer(u32 reserved_size, struct device *dev)
{
	struct device *dma_dev = mem_dev;

	dma_addr_t dma_handle;
	void *cpu_buf_addr = NULL;
	int rc = 0;

	LOG(LOG_DEBUG, "Allocating %u bytes\n", reserved_size);

	if ((!dma_dev) || (!dev)) {
		LOG(LOG_ERR, "DMA device not initialized\n");
		return -ENODEV;
	}

	/* Alloc DMA buffer for reserved buffer */
	cpu_buf_addr = dma_alloc_coherent(mem_dev,PAGE_ALIGN(reserved_size),
			&dma_handle, GFP_KERNEL);
	if (!cpu_buf_addr) {
		LOG(LOG_ERR, "DMA alloc failed for size %u\n", reserved_size);
		return -ENOMEM;
	}

	/* Update the buffer information */

	discard_buf_addr_dma = dma_handle ;
	discard_buf_addr = cpu_buf_addr;
	discard_buf_size = PAGE_ALIGN(reserved_size);
	discard_dma_handle = dma_handle;

	LOG(LOG_DEBUG, "Allocated: virt=%p, dma=0x%0x, size=%u\n",
			discard_buf_addr, discard_buf_addr_dma, reserved_size);

	return rc;
}

int armcb_v4l2_release_discard_buffer(u32 reserved_size, struct device *dev)
{
	struct device *dma_dev = mem_dev;
	int ret = 0;

	dma_free_coherent(dma_dev, reserved_size,
					discard_buf_addr, discard_dma_handle);
	return ret;
}

int armcb_v4l2_config_init_update_stream_hw_addr(armcb_v4l2_dev_t *dev)
{
	u32 vout_reg1 = 0;
	u32 vout_reg2 = 0;
	u32 startaddr = 0;
	int i = 0, rc = 0;

	for(i = 0; i < sizeof(vout_idx)/sizeof(vout_idx[0]); i++) {
		armcb_init_output_addr_by_i7(&vout_reg1, &vout_reg2, vout_idx[i]);
		startaddr = discard_buf_addr_dma;
		if (vout_reg1 && startaddr) {
			armcb_isp_write_reg(vout_reg1, startaddr);
		}

		if (vout_reg2 && startaddr) {
			armcb_isp_write_reg(vout_reg2, startaddr);
		}
	}

	return rc;
}

//Sync the daemon and other application
static int armcb_v4l2_s_ctrl(struct file *file, void *fh,
                            struct v4l2_control *ctrl)
{
	int rc = -1;
	armcb_v4l2_dev_t *dev = video_drvdata(file);
	struct armcb_isp_v4l2_fh *sp = fh_to_private(fh);
	armcb_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];
	struct video_device *vdev = &dev->vid_cap_dev;

	switch (ctrl->id) {
		case ISP_DAEMON_SET_STREAM_ON:
			LOG(LOG_DEBUG, "ISP_DAEMON_SET_STREAM_ON\n");
			/* Just frist time and frist few frame use reserved buffer*/
			armcb_v4l2_config_init_update_stream_hw_addr(dev);

			rc = armcb_isp_hw_apply_list(CMD_TYPE_STREAMON);
			atomic_set(&dev->stream_on_cnt, STREAM_ON);
			if (rc < 0) {
				LOG(LOG_ERR, "armcb_isp_hw_apply_list failed ret(%d)", rc);
			} else {
				LOG(LOG_INFO, "pstream:%p, armcb_isp_hw_apply_list success",
						pstream);
			}
			break;
		case ISP_DAEMON_SET_MULTI_CAM:
			armcb_multi_cam = ctrl->value;
			LOG(LOG_INFO,"set the multi camera %d", armcb_multi_cam);
			rc = 0;
			break;
		case ISP_DAEMON_SET_STREAM_OFF:
				LOG(LOG_INFO, "devname:%s, ISP_DAEMON_SET_STREAM_OFF\n", dev_name(&vdev->dev));
				/*disbale the stream operate ram,if not smmu error maybe occur */
				armcb_disable_irq();
				rc = armcb_isp_hw_apply_list(CMD_TYPE_STREAMOFF);
				if (rc < 0)
					LOG(LOG_ERR, "armcb_isp_hw_apply_list failed ret(%d)", rc);
				rc = armcb_isp_hw_apply_list(CMD_TYPE_POWERDOWN);
				if (rc < 0)
					LOG(LOG_ERR, "armcb_isp_hw_apply_list failed ret(%d)", rc);
				atomic_set(&dev->stream_on_cnt, STREAM_OFF);
				armcb_isp_vb2_streamoff(file);
			break;
		default:
			break;
	}

	return rc;
}

static int
armcb_v4l2_core_subscribe_event(struct v4l2_fh *fh,
				  const struct v4l2_event_subscription *sub)
{
	int ret = -1;

	LOG(LOG_DEBUG, "type:%d id:%d", sub->type, sub->id);
	ret = v4l2_event_subscribe(fh, sub, CORE_NEVENTS, NULL);
	if (ret < 0)
		LOG(LOG_ERR, "armcb_v4l2_subscribe_event failed ret(%d)", ret);
	return ret;
}

static int
armcb_v4l2_core_unsubscribe_event(struct v4l2_fh *fh,
					const struct v4l2_event_subscription *sub)
{
	struct v4l2_event ev;

	memset(&ev, 0, sizeof(struct v4l2_event));
	ev.id = ISP_MIXTURE_INT;
	ev.type = V4L2_EVENT_CTRL;
	/// Queue a empty event to notify userspace
	v4l2_event_queue(fh->vdev, &ev);

	LOG(LOG_INFO, "unsubscribe event: type:%d id:%d", sub->type, sub->id);
	v4l2_event_unsubscribe(fh, sub);

	return 0;
}


static int armcb_v4l2_g_selection(struct file *file, void *fh,
				 struct v4l2_selection *s)
{
	LOG(LOG_INFO, "enter: target = %d, type = %d",
		s->target, s->type);

	/* need fix: hard code for NV12M 1080p fmt */
	switch (s->target) {
	case V4L2_SEL_TGT_CROP:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = 1920;
		s->r.height = 1080;
		s->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		LOG(LOG_DEBUG, "function exit");
		return 0;
	}

	return -EINVAL;
}

static int armcb_v4l2_s_selection(struct file *file, void *fh,
				 struct v4l2_selection *s)
{
	LOG(LOG_DEBUG, "function enter");
	return 0;
}

static int armcb_v4l2_enum_frameintervals(struct file *file, void *fh,
                     struct v4l2_frmivalenum *interval) {
    /* Check the supporting formats*/
    if (interval->pixel_format != V4L2_PIX_FMT_NV12M && interval->pixel_format != V4L2_PIX_FMT_RGB24) {
        return -EINVAL;
    }

	switch (interval->index) {
		case 0:
			interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
			interval->discrete.numerator = 1;
			interval->discrete.denominator = 30; // 30fps
			return 0;
		case 1:
			interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
			interval->discrete.numerator = 1;
			interval->discrete.denominator = 15; // 15fps
			return 0;
		default:
			return -EINVAL;
	}

    LOG(LOG_ERR, "Unsupported resolution: %dx%d",
        interval->width, interval->height);
    return -EINVAL;
}

static int armcb_g_param(struct file *file, void *fh,
			      struct v4l2_streamparm *a)

{
	LOG(LOG_INFO, "g_param enter");
	return 0;
}

static const struct v4l2_ioctl_ops armcb_ioctl_ops = {
	.vidioc_querycap = armcb_v4l2_querycap,

	.vidioc_g_fmt_vid_cap_mplane = armcb_v4l2_g_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_cap_mplane = armcb_v4l2_s_fmt_vid_cap_mplane,
	.vidioc_try_fmt_vid_cap_mplane = armcb_v4l2_try_fmt_vid_cap_mplane,

	.vidioc_reqbufs = armcb_v4l2_reqbufs,
	.vidioc_querybuf = armcb_v4l2_querybuf,
	.vidioc_qbuf = armcb_v4l2_qbuf,
	.vidioc_dqbuf = armcb_v4l2_dqbuf,

	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_expbuf = NULL,

	.vidioc_enum_fmt_vid_cap = armcb_v4l2_enum_fmt_vid_cap,
	.vidioc_enum_framesizes = armcb_v4l2_enum_framesizes,

	.vidioc_streamon = armcb_v4l2_streamon,
	.vidioc_streamoff = armcb_v4l2_streamoff,

	.vidioc_log_status = armcb_v4l2_log_status,
	.vidioc_subscribe_event = armcb_v4l2_core_subscribe_event,
	.vidioc_unsubscribe_event = armcb_v4l2_core_unsubscribe_event,

	.vidioc_g_selection = armcb_v4l2_g_selection,
	.vidioc_s_selection = armcb_v4l2_s_selection,
	.vidioc_enum_frameintervals = armcb_v4l2_enum_frameintervals,
	.vidioc_g_parm = armcb_g_param,

	//sync
	.vidioc_s_ctrl = armcb_v4l2_s_ctrl,
};

/*-----------------------------------------------------------------
 *	Initialization and module stuff
 *  ---------------------------------------------------------------
 */
armcb_v4l2_dev_t *armcb_v4l2_core_get_dev(uint32_t ctx_id)
{
	LOG(LOG_DEBUG, "get ctx_id:%d pdev:%p", ctx_id,
		g_isp_v4l2_devs[ctx_id]);
	return g_isp_v4l2_devs[ctx_id];
}

uint32_t armcb_v4l2_core_find_1st_opened_dev(void)
{
	uint32_t i = 0;
	armcb_v4l2_dev_t *pdev = NULL;

	for (i = 0; i < ARMCB_MAX_DEVS; i++) {
		pdev = armcb_v4l2_core_get_dev(i);
		if (pdev && atomic_read(&pdev->opened) > 0)
			break;
	}

	if (i == ARMCB_MAX_DEVS)
		LOG(LOG_WARN, "No v4l2 device opened");
	return i;
}

static void armcb_v4l2_dev_release(struct v4l2_device *v4l2_dev)
{
	armcb_v4l2_dev_t *dev =
		container_of(v4l2_dev, armcb_v4l2_dev_t, v4l2_dev);

	v4l2_device_unregister(&dev->v4l2_dev);
	kfree(dev);
}

static armcb_v4l2_dev_t *
armcb_v4l2_create_instance(struct platform_device *pdev, int ctx_id,
			   struct device *devnode)
{
	armcb_v4l2_dev_t *dev = NULL;
	struct video_device *vfd = NULL;
	int ret = 0;
	int i = 0;

	LOG(LOG_INFO, " ctx_id(%d) +", ctx_id);
	/* allocate main vivid state structure */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		LOG(LOG_ERR, "failed to alloc memory for armcb dev.");
		return NULL;
	}

	dev->ctx_id = ctx_id;
	if (pdev)
		dev->pvdev = pdev;
	else if (devnode)
		dev->pvdev = (struct platform_device *)devnode;
	else {
		LOG(LOG_ERR, "invalid device for drivers.");
		kfree(dev);
		return NULL;
	}

#ifdef CONFIG_MEDIA_CONTROLLER
	dev->v4l2_dev.mdev = &dev->mdev;

	/* Initialize media device */
	(void)strscpy(dev->mdev.model, ARMCB_MODULE_NAME,
			  sizeof(dev->mdev.model));
	snprintf(dev->mdev.bus_info, sizeof(dev->mdev.bus_info),
		 "platform:%s-%03d", ARMCB_MODULE_NAME, ctx_id);
	dev->mdev.dev = devnode;
	devnode->coherent_dma_mask = -1;
	media_device_init(&dev->mdev);
#endif

	/* register v4l2_device */
	snprintf(dev->v4l2_dev.name, sizeof(dev->v4l2_dev.name), "%s-%02d",
		 ARMCB_MODULE_NAME, ctx_id);
	LOG(LOG_INFO, "dev->v4l2_dev.name[%s]", dev->v4l2_dev.name);
	ret = v4l2_device_register(devnode, &dev->v4l2_dev);
	if (ret) {
		kfree(dev);
		return NULL;
	}
	dev->v4l2_dev.release = armcb_v4l2_dev_release;
	dev->v4l2_dev.notify = armcb_v4l2_subdev_notify;

	/* set up the capabilities of the video capture device */
	dev->vid_cap_caps = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING |
				V4L2_CAP_READWRITE;
	/* initialize locks */
	spin_lock_init(&dev->slock);
	spin_lock_init(&dev->v4l2_event_slock);

	dev->ddr_lp_mode = 1;

	mutex_init(&dev->mutex);
	mutex_init(&dev->v4l2_event_mutex);
	mutex_init(&dev->ordered_sd_mutex);
	mutex_init(&dev->queue_lock);

	/* init subdev list */
	INIT_LIST_HEAD(&dev->ordered_sd_list);

	/* defualt video device*/
	dev->has_vid_cap = 1;

	/* initialize stream id table */
	for (i = 0; i < V4L2_STREAM_TYPE_MAX; i++)
		dev->stream_id_index[i] = -1;
	/* initialize open counter */
	atomic_set(&dev->stream_on_cnt, STREAM_DEFAULT);
	atomic_set(&dev->port_idx_release, OUTPORT_IDX_DEFAULT);
	atomic_set(&dev->opened, 0);
	atomic_set(&dev->upload_streamoff, 0);

	/* Buf queue status*/
	dev->is_streaming = 0;

	/* finally start creating the device nodes */
	if (dev->has_vid_cap) {
		vfd = &dev->vid_cap_dev;
		snprintf(vfd->name, sizeof(vfd->name), "armcb-%02d-vid-cap",
			 ctx_id);
		vfd->fops = &armcb_fops;
		vfd->ioctl_ops = &armcb_ioctl_ops;
		vfd->device_caps = dev->vid_cap_caps;
		vfd->release = video_device_release_empty;
		vfd->v4l2_dev = &dev->v4l2_dev;
		vfd->queue = &dev->vb2_q;
		vfd->tvnorms = 0;

		/*
		 * Provide a mutex to v4l2 core. It will be used to protect
		 * all fops and v4l2 ioctls.
		 */
		vfd->lock = &dev->mutex;
		video_set_drvdata(vfd, dev);
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ret = video_register_device(vfd, VFL_TYPE_VIDEO, (9 - ctx_id));
#else
		ret = video_register_device(vfd, VFL_TYPE_GRABBER, (9 - ctx_id));
#endif
		if (ret < 0)
			goto unreg_dev;
		LOG(LOG_INFO, "V4L2 capture device registered as %s",
			video_device_node_name(vfd));
		LOG(LOG_INFO,
			"[has_vid_cap] vfd->name[%s] v4l2_dev.name[%s] dev_name[%s]",
			vfd->name, dev->v4l2_dev.name, video_device_node_name(vfd));
	}

	/* Now that everything is fine, let's add it to device list */
	LOG(LOG_INFO, "create video device instance success");
	return dev;

unreg_dev:
	video_unregister_device(&dev->vid_cap_dev);
	v4l2_device_put(&dev->v4l2_dev);
	kfree(dev);
	LOG(LOG_ERR, "create video device instance failed. ret = %d", ret);
	return NULL;
}

armcb_v4l2_dev_t *armcb_register_instance(struct platform_device *pdev,
					  struct device *devnode,
					  unsigned int cam_id)
{
	armcb_v4l2_dev_t *adev = NULL;

	if (cam_id + 1 >= ARMCB_MAX_DEVS) {
		LOG(LOG_ERR, "too many instance, current is %d.", cam_id);
		return NULL;
	}
	if (g_isp_v4l2_devs[cam_id] != NULL) {
		LOG(LOG_ERR, "camera %d has probe.", cam_id);
		return NULL;
	}

    adev = armcb_v4l2_create_instance(pdev, cam_id, devnode);
	if (adev == NULL) {
		LOG(LOG_ERR, "too many instance, current is %d.", cam_id);
		return NULL;
	}

	LOG(LOG_INFO, "register v4l2 video instance %d %p", cam_id, adev);
	g_isp_v4l2_devs[cam_id] = adev;
	return adev;
}

void armcb_cam_instance_destroy(void)
{
	int i = 0;

	for (; i < ARMCB_MAX_DEVS; i++) {
		if (g_isp_v4l2_devs[i] == NULL)
			continue;
		v4l2_async_nf_unregister(&g_isp_v4l2_devs[i]->dts_notifier);
#if (KERNEL_VERSION(4, 17, 0) <= LINUX_VERSION_CODE)
		v4l2_async_nf_cleanup(&g_isp_v4l2_devs[i]->dts_notifier);
#endif
		video_unregister_device(&g_isp_v4l2_devs[i]->vid_cap_dev);
		v4l2_device_put(&g_isp_v4l2_devs[i]->v4l2_dev);
		media_device_unregister(&g_isp_v4l2_devs[i]->mdev);
		LOG(LOG_INFO, "release armcb instance %d (%p)", i,
			g_isp_v4l2_devs[i]);
		g_isp_v4l2_devs[i] = NULL;
	}
	g_adev_idx = 0;
}
