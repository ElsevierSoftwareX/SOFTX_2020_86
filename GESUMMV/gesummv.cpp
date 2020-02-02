/**
 * gesummv.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
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
#define DIM_THREAD_BLOCK_X 256
#define DIM_THREAD_BLOCK_Y 1

#define VERBOSE_COMPARE_NUM -1

void gesummv(DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *x, DATA_TYPE *y, DATA_TYPE *tmp)
{
	int i, j;
	
	for (i = 0; i < N; i++)
	{
		tmp[i] = 0;
		y[i] = 0;
		for (j = 0; j < N; j++)
		{
			tmp[i] = A[i*N + j] * x[j] + tmp[i];
			y[i] = B[i*N + j] * x[j] + y[i];
		}
		
		y[i] = ALPHA * tmp[i] + BETA * y[i];
	}
}


void init(DATA_TYPE* A, DATA_TYPE* x)
{
  	int i, j;

 	for (i = 0; i < N; i++)
    {
    	x[i] = ((DATA_TYPE) i) / N;
      	
		for (j = 0; j < N; j++) 
		{
			A[i*N + j] = ((DATA_TYPE) i*j) / N;
		}
    }
}


void compareResults(DATA_TYPE* y, DATA_TYPE* y_outputFromGpu)
{
	int i, fail;
	fail = 0;
    int count = 0;
	
	for (i=0; i<(N); i++) 
	{

        if((VERBOSE_COMPARE_NUM>=0) && (count%VERBOSE_COMPARE_NUM==0)) 
			std::cout << "CCHECK [" << count << "] " << y[i] << "/" << y_outputFromGpu[i] << std::endl;
        
		count++;
		if (percentDiff(y[i], y_outputFromGpu[i]) > PERCENT_DIFF_ERROR_THRESHOLD) 
		{
			fail++;
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


void gesummvVulkan(VulkanCompute *vk, DATA_TYPE* A, DATA_TYPE* B, DATA_TYPE* x, DATA_TYPE* y, DATA_TYPE* tmp, DATA_TYPE* y_outputFromGpu)
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"gesummvkernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "gesummvkernel") == -1)
	{
		std::cout << "Error in compiling shader gesummvkernel.comp" << std::endl;
		exit(-1);
	}		

	DATA_TYPE *A_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N * N, BufferUsage::BUF_INOUT);
	DATA_TYPE *B_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N * N, BufferUsage::BUF_INOUT);
	DATA_TYPE *x_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N, BufferUsage::BUF_INOUT);
	DATA_TYPE *y_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N, BufferUsage::BUF_INOUT);
	DATA_TYPE *tmp_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N, BufferUsage::BUF_INOUT);

	memcpy(A_gpu, A, sizeof(DATA_TYPE) * N * N);
	memcpy(B_gpu, B, sizeof(DATA_TYPE) * N * N);
	memcpy(x_gpu, x, sizeof(DATA_TYPE) * N);
	memcpy(y_gpu, y, sizeof(DATA_TYPE) * N);
	memcpy(tmp_gpu, tmp, sizeof(DATA_TYPE) * N);

     vk->startCreateCommandList();
		vk->synchBuffer(PPTR(A_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(B_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(x_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(y_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(tmp_gpu),HOST_TO_DEVICE);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

    /*
	dim3 block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	dim3 grid((unsigned int)ceil( ((float)N) / ((float)block.x) ), 1);*/
    ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	ComputeWorkDistribution_t grid((unsigned int)ceil( ((float)N) / ((float)block.x) ), 1);

    vk->startCreatePipeline("gesummvkernel");
		vk->setArg(PPTR(A_gpu),"gesummvkernel",4);
		vk->setArg(PPTR(B_gpu),"gesummvkernel",5);
        vk->setArg(PPTR(x_gpu),"gesummvkernel",6);
        vk->setArg(PPTR(y_gpu),"gesummvkernel",7);
        vk->setArg(PPTR(tmp_gpu),"gesummvkernel",8);
		vk->setLaunchConfiguration(grid,block);
	PIPELINE_HANDLE hPipeline = vk->finalizePipeline();

    vk->startCreateCommandList();
	    vk->selectPipeline(hPipeline);
	    vk->launchComputation("gesummvkernel");
	vk->finalizeCommandList();

    vk->deviceSynch();

	t_start = rtclock();
	/*gesummv_kernel<<< grid, block>>>(A_gpu,B_gpu,x_gpu, y_gpu, tmp_gpu);
	cudaThreadSynchronize();*/
    vk->submitWork();
    vk->deviceSynch();
	t_end = rtclock();

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(y_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	memcpy(y_outputFromGpu, y_gpu, sizeof(DATA_TYPE) * N);

	fprintf(stdout, "GPU Runtime: %0.6lfs\n", t_end - t_start);
}


int main(int argc, char *argv[])
{
	double t_start, t_end;

	DATA_TYPE* A;
	DATA_TYPE* B;  
	DATA_TYPE* x;  
	DATA_TYPE* y;
	DATA_TYPE* y_outputFromGpu;
	DATA_TYPE* tmp;
	
	A = (DATA_TYPE*)malloc(N*N*sizeof(DATA_TYPE));
	B = (DATA_TYPE*)malloc(N*N*sizeof(DATA_TYPE));
	x = (DATA_TYPE*)malloc(N*sizeof(DATA_TYPE)); 
	y = (DATA_TYPE*)malloc(N*sizeof(DATA_TYPE));
	y_outputFromGpu = (DATA_TYPE*)malloc(N*sizeof(DATA_TYPE));
	tmp = (DATA_TYPE*)malloc(N*sizeof(DATA_TYPE));

	init(A, x);
	
    VulkanCompute vk;
	GPU_argv_init(&vk);

	gesummvVulkan(&vk, A, B, x, y, tmp, y_outputFromGpu);
	
	t_start = rtclock();
	gesummv(A, B, x, y, tmp);
	t_end = rtclock();
	fprintf(stdout, "CPU Runtime: %0.6lfs\n", t_end - t_start);
	
	compareResults(y, y_outputFromGpu);

	free(A);
	free(B);  
	free(x);  
	free(y);
	free(y_outputFromGpu);
	free(tmp);
    vk.freeResources();

	return EXIT_SUCCESS;
}

