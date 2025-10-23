/*
 * -----------------------------------------------------------------------------
 * The proprietary software and information contained in this file is
 * confidential and may only be used by an authorized person under a valid
 * licensing agreement from Arm Limited or its affiliates.
 *
 * Copyright (C) 2013-2022. Arm Limited or its affiliates. All rights reserved.
 *
 * This entire notice must be reproduced on all copies of this file and
 * copies of this file may only be made by an authorized person under a valid
 * licensing agreement from Arm Limited or its affiliates.
 * -----------------------------------------------------------------------------
 */

/* Kernel function to sum two vectors, result is stored in a. */
__kernel void add( __global float *a, __global float *b, uint length)
{
	/* Obtain the ID for this work item - we will use this to locate the corresponding */
	/* position in the vectors. */
	size_t id = get_global_id(0);

	/* The kernel may be invoked on data points which are outside the vector (in order to honour
	   global and local work space alignments for a particular device). Simply return in these cases. */
	if( id < length )
	{
		a[id] += b[id];
	}
}

/* Kernel function to multiply two vectors, result is stored in a. */
__kernel void mul( __global float *a, __global float *b, uint length)
{
	/* Obtain the ID for this work item - we will use this to locate the corresponding */
	/* position in the vectors. */
	size_t id = get_global_id(0);

	/* The kernel may be invoked on data points which are outside the vector (in order to honour
	   global and local work space alignments for a particular device). Simply return in these cases. */
	if( id < length )
	{
		a[id] *= b[id];
	}
}
