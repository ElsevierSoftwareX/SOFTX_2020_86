/**
 * datatypes.h: This file is part of the vkpolybench test suite,
 * See LICENSE.md for vkpolybench and other 3rd party licenses. 
 */

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <stdint.h>

enum BufferUsage
{
	BUF_IN,
	BUF_OUT,
	BUF_INOUT
};

struct Buffer_t {
	BufferUsage bufferUsage;
	uint64_t size;
};

struct BufferDelimiter_t {
	uint32_t buffer_handle;
	uint64_t size;
	BufferUsage buffer_usage;
};


#ifdef _WIN32

	struct DXBufferBundle_t {

		BufferUsage buffer_usage;
		uint64_t size;
		intptr_t pBuffer;
		intptr_t pView;

	};

#endif

//TODO: Do class
struct ComputeWorkDistribution_t {
public:
	uint32_t x;
	uint32_t y;
	uint32_t z;

	ComputeWorkDistribution_t()
	{
		x = 128;
		y = 1;
		z = 1;
	}

	ComputeWorkDistribution_t(uint32_t _x, uint32_t _y=1, uint32_t _z=1)
	{
		x = _x;
		y = _y;
		z = _z;
	}

};


struct LaunchConfiguration_t {

	ComputeWorkDistribution_t groups_or_blocks;
	ComputeWorkDistribution_t threads_or_workitems;

	LaunchConfiguration_t()
	{
		groups_or_blocks = ComputeWorkDistribution_t(128,1,1);
		threads_or_workitems = ComputeWorkDistribution_t(128,1,1);
	}

	LaunchConfiguration_t(ComputeWorkDistribution_t gr, ComputeWorkDistribution_t threads)
	{
		groups_or_blocks = gr;
		threads_or_workitems = threads;
	}
	
};

#endif
