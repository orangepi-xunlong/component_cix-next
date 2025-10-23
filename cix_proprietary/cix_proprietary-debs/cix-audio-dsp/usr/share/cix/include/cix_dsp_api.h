/* Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _CIX_DSP_API_H_
#define _CIX_DSP_API_H_

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif

typedef enum {
	DSPCOMP_ERR_NO_ERROR		=  0,			/* operation success */
	DSPCOMP_ERR_PARAM_INVALID	= -1,			/* invalid parameter */
	DSPCOMP_ERR_MEM_ALLOCATE	= -2,			/* memory allocate failed */
	DSPCOMP_ERR_SYSIO_HANDLE	= -3,			/* sysio handle failed */
	DSPCOMP_ERR_CODEC_UNAVAILABLE	= -4,			/* not install codec library */
	DSPCOMP_ERR_UNKNOWN		= -1000,		/* unknown error */
} dspcomp_errcode;

typedef enum {
	/* dsp component state */
	DSPCOMP_STATE_OUTPUT_NONE,			/* no output data generated */
	DSPCOMP_STATE_OUTPUT_INITED,			/* output initialized to generate */
	DSPCOMP_STATE_OUTPUT_READY,			/* output data ready to take away */
	DSPCOMP_STATE_OUTPUT_DONE,			/* output data done, all output data take away */
} dspcomp_state;

typedef enum {
	/* dsp component operation code */
	DSPCOMP_OPCODE_DECODER = 0,
	DSPCOMP_OPCODE_ENCODER,
	DSPCOMP_OPCODE_MIXER,
	DSPCOMP_OPCODE_PCM_GAIN,
	DSPCOMP_OPCODE_MAX,
} dspcomp_opcode;

typedef enum {
	/* dsp component audio format */
	DSPCOMP_FMT_MP3 = 1,
	DSPCOMP_FMT_XHEAAC,
	DSPCOMP_FMT_HEAACV2,
	DSPCOMP_FMT_FLAC,
	DSPCOMP_FMT_VORBIS,
	DSPCOMP_FMT_OPUS,
	DSPCOMP_FMT_AACELDV2,
	/* dsp component mixer format */
	DSPCOMP_FMT_MIXER = 100,
	/* dsp component pcm gain format */
	DSPCOMP_FMT_PCM_GAIN = 110,
	DSPCOMP_FMT_MAX,
} dspcomp_format;

typedef enum {
	/* component parameter set/get scope */
	DSPCOMP_PARAM_SAMPLERATE = 0,
	DSPCOMP_PARAM_CHANNEL,
	DSPCOMP_PARAM_WIDTH,
	DSPCOMP_PARAM_GAIN_FACTOR,
	DSPCOMP_PARAM_AUDOBJTYPE,
	DSPCOMP_PARAM_BITRATE,
	DSPCOMP_PARAM_BITRATE_MODE,
	DSPCOMP_PARAM_BSFORMAT,
	DSPCOMP_PARAM_ASC_DATA,
	DSPCOMP_PARAM_ASC_LENGTH,
	DSPCOMP_PARAM_FRAME_LENGTH,
	DSPCOMP_PARAM_SBR_ENABLE,
	DSPCOMP_PARAM_SBR_RATIO,
	DSPCOMP_PARAM_CHANNEL_MODE,

	/* conponent parameter get scope */
	DSPCOMP_PARAM_PRODUCED = 100,

	/* wrapper private parameter get scope */
	DSPCOMP_PARAM_INBUF_SIZE = 200,
	DSPCOMP_PARAM_OUTBUF_SIZE,

	DSPCOMP_PARAM_TYPE_MAX,
} dspcomp_paramtype;

struct dspcomp_config {
	dspcomp_opcode opcode;
	dspcomp_format format;
	int num_input_buf;
	int num_output_buf;
};

struct dspcomp_param {
	dspcomp_paramtype type;
	union {
		int value;
		void *addr;
	};
};

/* API Function ID */
enum {
	API_DSPCOMP_GET_VERSION = 0,
	API_DSPCOMP_GET_CODEC_CYCLES,
	API_DSPCOMP_GET_ACTIVE_CYCLES,
	API_DSPCOMP_CREATE,
	API_DSPCOMP_DELETE,
	API_DSPCOMP_FLUSH,
	API_DSPCOMP_GET_PARAM,
	API_DSPCOMP_SET_PARAM,
	API_DSPCOMP_CONNECT,
	API_DSPCOMP_DISCONNECT,
	API_DSPCOMP_PROCESS_FRAME_OUT_DISCONNECT_SYNC,
	API_DSPCOMP_PROCESS_FRAME_OUT_CONNECT_SYNC,
	API_DSPCOMP_PROCESS_FRAME_IN_CONNECT_SYNC,
};

typedef void * dspcomp_handle;

/*
 * Entry function of the wrapper library, every wrapper library should implement
 * this function and tell a specific API function pointer.
 */
EXTERN dspcomp_errcode dsp_comp_query_interface(unsigned int id, void **func);

/* API Funtion Prototypes List */
typedef dspcomp_errcode (*dspcomp_query_interface)(unsigned int id, void **func);

typedef const char * (*dspcomp_get_version)();

typedef dspcomp_errcode (*dspcomp_get_codec_cycles)(dspcomp_handle comp_handle,
						    void *codec_cycles);
typedef dspcomp_errcode (*dspcomp_get_active_cycles)(dspcomp_handle comp_handle,
						     void *active_cycles);

typedef dspcomp_errcode (*dspcomp_create)(dspcomp_handle *comp_handle,
					  struct dspcomp_config *comp_config);
typedef dspcomp_errcode (*dspcomp_delete)(dspcomp_handle comp_handle);

typedef dspcomp_errcode (*dspcomp_flush)(dspcomp_handle comp_handle);

typedef dspcomp_errcode (*dspcomp_get_param)(dspcomp_handle comp_handle,
					     struct dspcomp_param *comp_param);
typedef dspcomp_errcode (*dspcomp_set_param)(dspcomp_handle comp_handle,
					     unsigned int num_param,
					     struct dspcomp_param comp_param[]);

typedef dspcomp_errcode (*dspcomp_connect)(dspcomp_handle comp_handle_src,
					   unsigned int src_out_port,
					   dspcomp_handle comp_handle_dest,
					   unsigned int dest_in_port,
					   unsigned int num_buf);
typedef dspcomp_errcode (*dspcomp_disconnect)(dspcomp_handle comp_handle_src,
					      unsigned int src_out_port,
					      dspcomp_handle comp_handle_dest,
					      unsigned int dest_in_port);

typedef dspcomp_errcode (*dspcomp_process_frame_out_disconnect_sync)(dspcomp_handle comp_handle,
								     unsigned char *inputbuf,
								     unsigned int inputsize,
								     unsigned int *input_consumed,
								     unsigned char **outputbuf,
								     unsigned int *outputsize,
								     dspcomp_state *state);

typedef dspcomp_errcode (*dspcomp_process_frame_out_connect_sync)(dspcomp_handle comp_handle,
								  unsigned char *inputbuf,
								  unsigned int inputsize,
								  unsigned int *input_consumed,
								  dspcomp_state *state);

typedef dspcomp_errcode (*dspcomp_process_frame_in_connect_sync)(dspcomp_handle comp_handle,
								 unsigned char **outputbuf,
								 unsigned int *outputsize,
								 dspcomp_state *state);

#endif
