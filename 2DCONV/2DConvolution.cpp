/**
 * 2DConvolution.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
 * Vulkan version
 */

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "../vkcomp/VulkanCompute.h"
#include "../vkcomp/stdafx.h"
#include "../polybenchUtilFuncts.h"

/*
Data types, sizes and other constants are defined in the following header file.
Such defines are common for host and device code.
*/
#include "HDcommon.h"

//define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

/* Thread block dimensions */
#define DIM_THREAD_BLOCK_X 32
#define DIM_THREAD_BLOCK_Y 8

#define VERBOSE_COMPARE_NUM -1

void conv2D(DATA_TYPE* A, DATA_TYPE* B)
{
	int i, j;
	DATA_TYPE c11, c12, c13, c21, c22, c23, c31, c32, c33;

	c11 = +0.2;  c21 = +0.5;  c31 = -0.8;
	c12 = -0.3;  c22 = +0.6;  c32 = -0.9;
	c13 = +0.4;  c23 = +0.7;  c33 = +0.10;


	for (i = 1; i < NI - 1; ++i) // 0
	{
		for (j = 1; j < NJ - 1; ++j) // 1
		{
			B[i*NJ + j] = c11 * A[(i - 1)*NJ + (j - 1)]  +  c12 * A[(i + 0)*NJ + (j - 1)]  +  c13 * A[(i + 1)*NJ + (j - 1)]
				+ c21 * A[(i - 1)*NJ + (j + 0)]  +  c22 * A[(i + 0)*NJ + (j + 0)]  +  c23 * A[(i + 1)*NJ + (j + 0)]
				+ c31 * A[(i - 1)*NJ + (j + 1)]  +  c32 * A[(i + 0)*NJ + (j + 1)]  +  c33 * A[(i + 1)*NJ + (j + 1)];
		}
	}
}



void init(DATA_TYPE* A)
{
	int i, j;

	for (i = 0; i < NI; ++i)
    	{
		for (j = 0; j < NJ; ++j)
		{
			A[i*NJ + j] = (float)rand()/RAND_MAX;
        	}
    	}
}


void compareResults(DATA_TYPE* B, DATA_TYPE* B_outputFromGpu)
{
	int i, j, fail;
	fail = 0;
	int count = 0;

	// Compare a and b
	for (i=1; i < (NI-1); i++)
	{
		for (j=1; j < (NJ-1); j++)
		{
			if((VERBOSE_COMPARE_NUM>=0) && (count%VERBOSE_COMPARE_NUM==0))
				std::cout << "CCHECK [" << count << "] " << B[i*NJ+j] << "/" << B_outputFromGpu[i*NJ+j] << std::endl;
			count++;
			if (percentDiff(B[i*NJ + j], B_outputFromGpu[i*NJ + j]) > PERCENT_DIFF_ERROR_THRESHOLD)
			{
				fail++;
			}
		}
	}
	// Print results
	printf("Non-Matching CPU-GPU Outputs Beyond Error Threshold of %4.2f Percent: %d\n", PERCENT_DIFF_ERROR_THRESHOLD, fail);
}


void GPU_argv_init(VulkanCompute *vk)
{
	vk->createContext();
	vk->printContextInformation();
}


void convolution2DVulkan(VulkanCompute *vk, DATA_TYPE *A, DATA_TYPE* B_outputFromGpu)
{
	double t_start, t_end;

	char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"conv2DKernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "conv2DKernel") == -1)
	{
		std::cout << "Error in compiling shader conv2DKernel.comp" << std::endl;
		exit(-1);
	}

	//4,5
	DATA_TYPE *A_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE)*NI*NJ, BufferUsage::BUF_INOUT);
    DATA_TYPE *B_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE)*NI*NJ, BufferUsage::BUF_INOUT);

	memcpy(A_gpu,A,sizeof(DATA_TYPE)*NI*NJ);// "copy" back the data for main function

	vk->startCreateCommandList();
		vk->synchBuffer(PPTR(A_gpu),HOST_TO_DEVICE);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	/*dim3 block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	dim3 grid((size_t)ceil( ((float)NI) / ((float)block.x) ), (size_t)ceil( ((float)NJ) / ((float)block.y)) );*/
	ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	ComputeWorkDistribution_t grid((size_t)ceil( ((float)NI) / ((float)block.x) ), (size_t)ceil( ((float)NJ) / ((float)block.y)) );

	vk->startCreatePipeline("conv2DKernel");
		vk->setArg(PPTR(A_gpu),"conv2DKernel",4);
		vk->setArg(PPTR(B_gpu),"conv2DKernel",5);
		vk->setLaunchConfiguration(grid,block);
	PIPELINE_HANDLE hPipeline = vk->finalizePipeline();

	vk->startCreateCommandList();
		vk->selectPipeline(hPipeline);
		vk->launchComputation("conv2DKernel");
	vk->finalizeCommandList();

	vk->deviceSynch();

	t_start = rtclock();

	/*Convolution2D_kernel<<<grid,block>>>(A_gpu,B_gpu);
	cudaThreadSynchronize();*/
	vk->submitWork();
	vk->deviceSynch();

	t_end = rtclock();

	fprintf(stdout, "GPU Runtime: %0.6lfs\n", t_end - t_start);

	//cudaMemcpy(B_outputFromGpu, B_gpu, sizeof(DATA_TYPE) * NI * NJ, cudaMemcpyDeviceToHost);
	vk->startCreateCommandList();
		vk->synchBuffer(PPTR(B_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	memcpy(B_outputFromGpu,B_gpu,sizeof(DATA_TYPE)*NI*NJ);
}


int main(int argc, char *argv[])
{
	double t_start, t_end;

	DATA_TYPE* A = (DATA_TYPE*)malloc(NI*NJ*sizeof(DATA_TYPE));
	DATA_TYPE* B = (DATA_TYPE*)malloc(NI*NJ*sizeof(DATA_TYPE));
	DATA_TYPE* B_outputFromGpu = (DATA_TYPE*)malloc(NI*NJ*sizeof(DATA_TYPE));
	init(A);

	VulkanCompute vk("",parseDeviceSelectionFromArgs(argc,argv),GIGA);
	GPU_argv_init(&vk);

	convolution2DVulkan(&vk, A, B_outputFromGpu);

	t_start = rtclock();
	conv2D(A, B);
	t_end = rtclock();
	fprintf(stdout, "CPU Runtime: %0.6lfs\n", t_end - t_start);

	compareResults(B, B_outputFromGpu);

	free(A);
	free(B);
	free(B_outputFromGpu);

	vk.freeResources();

	return EXIT_SUCCESS;
}
