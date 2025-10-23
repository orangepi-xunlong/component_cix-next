# import onnxruntime
import cv2
import numpy as np
import os
import sys
import time
import ctypes
import mmap
import posix_ipc

from libaipudrv import *

COCO_CLASSES = (
    "person",
    "bicycle",
    "car",
    "motorcycle",
    "airplane",
    "bus",
    "train",
    "truck",
    "boat",
    "traffic light",
    "fire hydrant",
    "stop sign",
    "parking meter",
    "bench",
    "bird",
    "cat",
    "dog",
    "horse",
    "sheep",
    "cow",
    "elephant",
    "bear",
    "zebra",
    "giraffe",
    "backpack",
    "umbrella",
    "handbag",
    "tie",
    "suitcase",
    "frisbee",
    "skis",
    "snowboard",
    "sports ball",
    "kite",
    "baseball bat",
    "baseball glove",
    "skateboard",
    "surfboard",
    "tennis racket",
    "bottle",
    "wine glass",
    "cup",
    "fork",
    "knife",
    "spoon",
    "bowl",
    "banana",
    "apple",
    "sandwich",
    "orange",
    "broccoli",
    "carrot",
    "hot dog",
    "pizza",
    "donut",
    "cake",
    "chair",
    "couch",
    "potted plant",
    "bed",
    "dining table",
    "toilet",
    "tv",
    "laptop",
    "mouse",
    "remote",
    "keyboard",
    "cell phone",
    "microwave",
    "oven",
    "toaster",
    "sink",
    "refrigerator",
    "book",
    "clock",
    "vase",
    "scissors",
    "teddy bear",
    "hair drier",
    "toothbrush",
)

_COLORS = np.array(
    [
        0.000, 0.447, 0.741,
        0.850, 0.325, 0.098,
        0.929, 0.694, 0.125,
        0.494, 0.184, 0.556,
        0.466, 0.674, 0.188,
        0.301, 0.745, 0.933,
        0.635, 0.078, 0.184,
        0.300, 0.300, 0.300,
        0.600, 0.600, 0.600,
        1.000, 0.000, 0.000,
        1.000, 0.500, 0.000,
        0.749, 0.749, 0.000,
        0.000, 1.000, 0.000,
        0.000, 0.000, 1.000,
        0.667, 0.000, 1.000,
        0.333, 0.333, 0.000,
        0.333, 0.667, 0.000,
        0.333, 1.000, 0.000,
        0.667, 0.333, 0.000,
        0.667, 0.667, 0.000,
        0.667, 1.000, 0.000,
        1.000, 0.333, 0.000,
        1.000, 0.667, 0.000,
        1.000, 1.000, 0.000,
        0.000, 0.333, 0.500,
        0.000, 0.667, 0.500,
        0.000, 1.000, 0.500,
        0.333, 0.000, 0.500,
        0.333, 0.333, 0.500,
        0.333, 0.667, 0.500,
        0.333, 1.000, 0.500,
        0.667, 0.000, 0.500,
        0.667, 0.333, 0.500,
        0.667, 0.667, 0.500,
        0.667, 1.000, 0.500,
        1.000, 0.000, 0.500,
        1.000, 0.333, 0.500,
        1.000, 0.667, 0.500,
        1.000, 1.000, 0.500,
        0.000, 0.333, 1.000,
        0.000, 0.667, 1.000,
        0.000, 1.000, 1.000,
        0.333, 0.000, 1.000,
        0.333, 0.333, 1.000,
        0.333, 0.667, 1.000,
        0.333, 1.000, 1.000,
        0.667, 0.000, 1.000,
        0.667, 0.333, 1.000,
        0.667, 0.667, 1.000,
        0.667, 1.000, 1.000,
        1.000, 0.000, 1.000,
        1.000, 0.333, 1.000,
        1.000, 0.667, 1.000,
        0.333, 0.000, 0.000,
        0.500, 0.000, 0.000,
        0.667, 0.000, 0.000,
        0.833, 0.000, 0.000,
        1.000, 0.000, 0.000,
        0.000, 0.167, 0.000,
        0.000, 0.333, 0.000,
        0.000, 0.500, 0.000,
        0.000, 0.667, 0.000,
        0.000, 0.833, 0.000,
        0.000, 1.000, 0.000,
        0.000, 0.000, 0.167,
        0.000, 0.000, 0.333,
        0.000, 0.000, 0.500,
        0.000, 0.000, 0.667,
        0.000, 0.000, 0.833,
        0.000, 0.000, 1.000,
        0.000, 0.000, 0.000,
        0.143, 0.143, 0.143,
        0.286, 0.286, 0.286,
        0.429, 0.429, 0.429,
        0.571, 0.571, 0.571,
        0.714, 0.714, 0.714,
        0.857, 0.857, 0.857,
        0.000, 0.447, 0.741,
        0.314, 0.717, 0.741,
        0.50, 0.5, 0
    ]
).astype(np.float32).reshape(-1, 3)

shm_name = "/my_shared_memory"
sem_name = "/sem_npu"

def preprocess(img, input_size, swap=(2, 0, 1)):
    if len(img.shape) == 3:
        padded_img = np.ones((input_size[0], input_size[1], 3), dtype=np.uint8) * 114
    else:
        padded_img = np.ones(input_size, dtype=np.uint8) * 114

    r = min(input_size[0] / img.shape[0], input_size[1] / img.shape[1])

    padded_img = img.transpose(swap)
    padded_img = np.ascontiguousarray(padded_img, dtype=np.float32)
    return padded_img, r

def nms(boxes, scores, nms_thr):
    """Single class NMS implemented in Numpy."""
    x1 = boxes[:, 0]
    y1 = boxes[:, 1]
    x2 = boxes[:, 2]
    y2 = boxes[:, 3]

    areas = (x2 - x1 + 1) * (y2 - y1 + 1)
    order = scores.argsort()[::-1]

    keep = []
    while order.size > 0:
        i = order[0]
        keep.append(i)
        xx1 = np.maximum(x1[i], x1[order[1:]])
        yy1 = np.maximum(y1[i], y1[order[1:]])
        xx2 = np.minimum(x2[i], x2[order[1:]])
        yy2 = np.minimum(y2[i], y2[order[1:]])

        w = np.maximum(0.0, xx2 - xx1 + 1)
        h = np.maximum(0.0, yy2 - yy1 + 1)
        inter = w * h
        ovr = inter / (areas[i] + areas[order[1:]] - inter)

        inds = np.where(ovr <= nms_thr)[0]
        order = order[inds + 1]

    return keep

def demo_postprocess(outputs, img_size, p6=False):
    grids = []
    expanded_strides = []
    strides = [8, 16, 32] if not p6 else [8, 16, 32, 64]

    hsizes = [img_size[0] // stride for stride in strides]
    wsizes = [img_size[1] // stride for stride in strides]

    for hsize, wsize, stride in zip(hsizes, wsizes, strides):
        xv, yv = np.meshgrid(np.arange(wsize), np.arange(hsize))
        grid = np.stack((xv, yv), 2).reshape(1, -1, 2)
        grids.append(grid)
        shape = grid.shape[:2]
        expanded_strides.append(np.full((*shape, 1), stride))

    grids = np.concatenate(grids, 1)
    expanded_strides = np.concatenate(expanded_strides, 1)
    outputs[..., :2] = (outputs[..., :2] + grids) * expanded_strides
    outputs[..., 2:4] = np.exp(outputs[..., 2:4]) * expanded_strides

    return outputs

def demo_postprocess_stage_0(img_size, p6=False):
    grids = []
    expanded_strides = []
    strides = [8, 16, 32] if not p6 else [8, 16, 32, 64]

    hsizes = [img_size[0] // stride for stride in strides]
    wsizes = [img_size[1] // stride for stride in strides]

    for hsize, wsize, stride in zip(hsizes, wsizes, strides):
        xv, yv = np.meshgrid(np.arange(wsize), np.arange(hsize))
        grid = np.stack((xv, yv), 2).reshape(1, -1, 2)
        grids.append(grid)
        shape = grid.shape[:2]
        expanded_strides.append(np.full((*shape, 1), stride))

    grids = np.concatenate(grids, 1)
    expanded_strides = np.concatenate(expanded_strides, 1)

    return grids, expanded_strides

def demo_postprocess_stage_1(outputs, grids, expanded_strides):
    outputs[..., :2] = (outputs[..., :2] + grids) * expanded_strides
    outputs[..., 2:4] = np.exp(outputs[..., 2:4]) * expanded_strides
    return outputs

def multiclass_nms_class_aware(boxes, scores, nms_thr, score_thr):
    """Multiclass NMS implemented in Numpy. Class-aware version."""
    final_dets = []
    num_classes = scores.shape[1]
    for cls_ind in range(num_classes):
        cls_scores = scores[:, cls_ind]
        valid_score_mask = cls_scores > score_thr
        if valid_score_mask.sum() == 0:
            continue
        else:
            valid_scores = cls_scores[valid_score_mask]
            valid_boxes = boxes[valid_score_mask]
            keep = nms(valid_boxes, valid_scores, nms_thr)
            if len(keep) > 0:
                cls_inds = np.ones((len(keep), 1)) * cls_ind
                dets = np.concatenate(
                    [valid_boxes[keep], valid_scores[keep, None], cls_inds], 1
                )
                final_dets.append(dets)
    if len(final_dets) == 0:
        print("no dets")
        return None
    return np.concatenate(final_dets, 0)

def vis(img, boxes, scores, cls_ids, conf=0.5, class_names=None):

    for i in range(len(boxes)):
        box = boxes[i]
        cls_id = int(cls_ids[i])
        score = scores[i]
        # if score < conf:
        #     continue
        x0 = int(box[0])
        y0 = int(box[1])
        x1 = int(box[2])
        y1 = int(box[3])
        # print(cls_id, ': ', x0, ',', y0, ', ', x1, ', ', y1, ', ', score)

        color = (_COLORS[cls_id] * 255).astype(np.uint8).tolist()
        text = '{}:{:.1f}%'.format(class_names[cls_id], score * 100)
        txt_color = (0, 0, 0) if np.mean(_COLORS[cls_id]) > 0.5 else (255, 255, 255)
        font = cv2.FONT_HERSHEY_SIMPLEX

        txt_size = cv2.getTextSize(text, font, 0.4, 1)[0]
        cv2.rectangle(img, (x0, y0), (x1, y1), color, 2)

        txt_bk_color = (_COLORS[cls_id] * 255 * 0.7).astype(np.uint8).tolist()
        cv2.rectangle(
            img,
            (x0, y0 + 1),
            (x0 + txt_size[0] + 1, y0 + int(1.5*txt_size[1])),
            txt_bk_color,
            -1
        )
        cv2.putText(img, text, (x0, y0 + txt_size[1]), font, 0.4, txt_color, thickness=1)

    return img

if __name__ == '__main__':
    window_name = 'video'
    cv2.namedWindow('video', cv2.WINDOW_AUTOSIZE)
    input_shape = (480, 640)

    scale = 1
    input_type = np.uint8
    dtype_min = 0
    dtype_max = 255
    model_path = "./aipu.bin"
    fm_idxes = []
    wt_idxes = []
    job_cfg = {
        "partition_id": 0,
        "dbg_dispatch": 0,
        "dbg_core_id": 0,
        "qos_level": 1
    }
    global_cfg = {
        "simulator":"",
        "log_file_path":"",
        "x2_arch_desc":"X2_1204MP3",
        "log_level":"0",
        "verbose":"0",
        "enable_avx":"0",
        "enable_calloc":"0",
        "en_eval":"1"
    }
    global_cfg["simulator"] = "~/NPU/simulator/bin/aipu_simulator_x2"
    mem_dump_config= {
        "dump_dir" : "./"
    }
    npu = NPU()
    ret = npu.aipu_init_context()
    ret = npu.aipu_config_global(AIPU_CONFIG_TYPE_SIMULATION, global_cfg)
    retmap = npu.aipu_load_graph(model_path)
    if retmap["ret"] == 0:
        graph_id = retmap["data"]
    else:
        print("npu: aipu_load_graph fail, graph: ")
        exit(-1)
    retmap = npu.aipu_get_tensor_count(graph_id, AIPU_TENSOR_TYPE_INPUT)
    if retmap["ret"] == 0:
        input_cnt = retmap["data"]
    else:
        print("npu: aipu_get_tensor_count fail")
        exit(-1)
    print("input tensor count is " + str(input_cnt))
    # retmap = npu.aipu_create_job(graph_id, job_cfg, fm_idxes, wt_idxes)
    # job_id = retmap["data"]
    # ret = npu.aipu_config_job(job_id, AIPU_CONFIG_TYPE_SIMULATION, global_cfg)
    save_count = 0
    last_time = time.time()
    first_time = last_time

    # replace data source
    shm = posix_ipc.SharedMemory(shm_name)
    shared_mem = mmap.mmap(shm.fd, shm.size)

    sem = posix_ipc.Semaphore(sem_name)

    while True:
        # Calculate size based on RGB888 format (3 bytes per pixel)
        rgb_size = input_shape[0] * input_shape[1] * 3

        # acquire sem
        sem.acquire()

        # Read data from shared memory
        rgb_data = shared_mem[:rgb_size]

        # release sem
        sem.release()

        # Convert flat bytes to numpy arrays
        data = np.frombuffer(rgb_data, dtype=np.uint8)
        if data is None:
            break

        data = data.reshape((480, 640, 3)) 
        data = cv2.resize(data, (640, 480))
        data[:, :, [0, 2]] = data[:, :, [2, 0]]
        backup_image=data
        input_data, ratio = preprocess(data, input_shape)

        retmap = npu.aipu_create_job(graph_id, job_cfg, fm_idxes, wt_idxes)
        job_id = retmap["data"]

        ret = npu.aipu_load_tensor(job_id, 0, input_data, D_UINT8)
        ret = npu.aipu_finish_job(job_id, -1)

        # retmap = npu.aipu_get_tensor_count(graph_id, AIPU_TENSOR_TYPE_OUTPUT)
        # output_cnt = retmap["data"]
        # print(f"output cnt = {output_cnt}")
        output_tensor = npu.aipu_get_tensor(job_id, AIPU_TENSOR_TYPE_OUTPUT, 0, D_INT8)
        output = np.array(output_tensor["data"]).astype(np.int8)

        output_scale = 36.53845977783203
        output_zp = 0
        output_type = np.int8
        output_dtype_min = -128
        output_dtype_max = 127
        output = (output.astype(np.float32) + output_zp) / output_scale
        output = np.reshape(output, (1, 6300, 85))

        npu.aipu_clean_job(job_id)

        output = demo_postprocess(output, input_shape)[0]
        boxes = output[:, :4]
        scores = output[:, 4:5] * output[:, 5:]
        boxes_xyxy = np.ones_like(boxes)
        boxes_xyxy[:, 0] = boxes[:, 0] - boxes[:, 2] / 2.
        boxes_xyxy[:, 1] = boxes[:, 1] - boxes[:, 3] / 2.
        boxes_xyxy[:, 2] = boxes[:, 0] + boxes[:, 2] / 2.
        boxes_xyxy[:, 3] = boxes[:, 1] + boxes[:, 3] / 2.
        boxes_xyxy /= ratio

        dets = multiclass_nms_class_aware(boxes_xyxy, scores, nms_thr=0.45, score_thr=0.3)
        if dets is not None:
            final_boxes, final_scores, final_cls_inds = dets[:, :4], dets[:, 4], dets[:, 5]
            origin_img = vis(data, final_boxes, final_scores, final_cls_inds,
                            conf=0.3, class_names=COCO_CLASSES)
        else:
            origin_img = data

        origin_img_uint8 = origin_img.astype(np.uint8)
        cv2.imshow(window_name, origin_img_uint8)

        save_count += 1
        current_time = time.time()
        if current_time - last_time > 10:
            fps = save_count / (current_time - first_time)
            print(f"fps = {fps}, process_cnt = {save_count}")
            last_time = current_time

        cv2.waitKey(1)
    npu.aipu_unload_graph(graph_id)
    npu.aipu_deinit_context()


