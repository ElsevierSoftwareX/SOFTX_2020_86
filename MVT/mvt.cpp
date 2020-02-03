/**
 * mvt.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
 * Vulkan version
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

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

void init_array(DATA_TYPE* A, DATA_TYPE* x1, DATA_TYPE* x2, DATA_TYPE* y1, DATA_TYPE* y2)
{
	int i, j;

	for (i = 0; i < N; i++)
	{
		x1[i] = ((DATA_TYPE) i) / N;
		x2[i] = ((DATA_TYPE) i + 1) / N;
		y1[i] = ((DATA_TYPE) i + 3) / N;
		y2[i] = ((DATA_TYPE) i + 4) / N;
		for (j = 0; j < N; j++)
		{
			A[i*N + j] = ((DATA_TYPE) i*j) / N;
		}
	}
}



void runMvt(DATA_TYPE* a, DATA_TYPE* x1, DATA_TYPE* x2, DATA_TYPE* y1, DATA_TYPE* y2)
{
	int i, j;
	
	for (i=0; i<N; i++) 
	{
		for (j=0; j<N; j++) 
		{
       			x1[i] = x1[i] + a[i*N + j] * y1[j];
        	}
    	}
	
	for (i=0; i<N; i++) 
	{
		for (j=0; j<N; j++) 
		{
 		       	x2[i] = x2[i] + a[j*N + i] * y2[j];
      		}
    	}
}


void compareResults(DATA_TYPE* x1, DATA_TYPE* x1_outputFromGpu, DATA_TYPE* x2, DATA_TYPE* x2_outputFromGpu)
{
	int i, fail;
	fail = 0;
    int count = 0;
	
	for (i=0; i<N; i++) 
	{

        if((VERBOSE_COMPARE_NUM>=0) && (count%VERBOSE_COMPARE_NUM==0))
            std::cout << "CCHECK [" << count << "] " << x1[i] << "/" << x1_outputFromGpu[i] << " " << x2[i] << "/" << x2_outputFromGpu[i] << std::endl;
       
		count++;
		if (percentDiff(x1[i], x1_outputFromGpu[i]) > PERCENT_DIFF_ERROR_THRESHOLD)
		{
			fail++;
		}

		if (percentDiff(x2[i], x2_outputFromGpu[i]) > PERCENT_DIFF_ERROR_THRESHOLD)
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

void mvtVulkan(VulkanCompute *vk, DATA_TYPE* a, DATA_TYPE* x1, DATA_TYPE* x2, DATA_TYPE* y_1, DATA_TYPE* y_2, 
			DATA_TYPE* x1_outputFromGpu, DATA_TYPE* x2_outputFromGpu)
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"mvtkernel1.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "mvtkernel1") == -1)
	{
		std::cout << "Error in compiling shader mvtkernel1.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"mvtkernel2.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "mvtkernel2") == -1)
	{
		std::cout << "Error in compiling shader mvtkernel2.comp" << std::endl;
		exit(-1);
	}

	DATA_TYPE* a_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N * N, BufferUsage::BUF_INOUT);
	DATA_TYPE* x1_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N, BufferUsage::BUF_INOUT);
	DATA_TYPE* x2_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N, BufferUsage::BUF_INOUT);
	DATA_TYPE* y_1_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N, BufferUsage::BUF_INOUT);
	DATA_TYPE* y_2_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N, BufferUsage::BUF_INOUT);

	memcpy(a_gpu, a, sizeof(DATA_TYPE) * N * N);
	memcpy(x1_gpu, x1, sizeof(DATA_TYPE) * N);
	memcpy(x2_gpu, x2, sizeof(DATA_TYPE) * N);
	memcpy(y_1_gpu, y_1, sizeof(DATA_TYPE) * N);
	memcpy(y_2_gpu, y_2, sizeof(DATA_TYPE) * N);

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(a_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(x1_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(x2_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(y_1_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(y_2_gpu),HOST_TO_DEVICE);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();
	
	/*dim3 block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	dim3 grid((size_t)ceil((float)N/ ((float)DIM_THREAD_BLOCK_X)), 1);*/
    ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
    ComputeWorkDistribution_t grid((size_t)ceil((float)N/ ((float)DIM_THREAD_BLOCK_X)), 1);

    vk->startCreatePipeline("mvtkernel1");
		vk->setArg(PPTR(a_gpu),"mvtkernel1",4);
		vk->setArg(PPTR(x1_gpu),"mvtkernel1",5);
        vk->setArg(PPTR(y_1_gpu),"mvtkernel1",6);
		vk->setLaunchConfiguration(grid,block);
	PIPELINE_HANDLE hPipeline1 = vk->finalizePipeline();

    vk->startCreatePipeline("mvtkernel2");
		vk->setArg(PPTR(a_gpu),"mvtkernel2",4);
        vk->setArg(PPTR(x2_gpu),"mvtkernel2",5);
        vk->setArg(PPTR(y_2_gpu),"mvtkernel2",6);
		vk->setLaunchConfiguration(grid,block);
	PIPELINE_HANDLE hPipeline2 = vk->finalizePipeline();

    vk->startCreateCommandList();
            vk->selectPipeline(hPipeline1);
            vk->launchComputation("mvtkernel1");
            vk->selectPipeline(hPipeline2);
            vk->launchComputation("mvtkernel2");
    vk->finalizeCommandList();
	vk->deviceSynch();
	
	t_start = rtclock();
	/*mvt_kernel1<<<grid,block>>>(a_gpu,x1_gpu,y_1_gpu);
	mvt_kernel2<<<grid,block>>>(a_gpu,x2_gpu,y_2_gpu);
	cudaThreadSynchronize();*/
    vk->submitWork();
    vk->deviceSynch();
	t_end = rtclock();
	fprintf(stdout, "GPU Runtime: %0.6lfs\n", t_end - t_start);

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(x1_gpu),DEVICE_TO_HOST);
        vk->synchBuffer(PPTR(x2_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	memcpy(x1_outputFromGpu, x1_gpu, sizeof(DATA_TYPE) * N);
	memcpy(x2_outputFromGpu, x2_gpu, sizeof(DATA_TYPE) * N);    
}


int main(int argc, char *argv[])
{
	double t_start, t_end;

	DATA_TYPE* a;
	DATA_TYPE* x1;
	DATA_TYPE* x2;
	DATA_TYPE* x1_outputFromGpu;
	DATA_TYPE* x2_outputFromGpu;
	DATA_TYPE* y_1;
	DATA_TYPE* y_2;

	a = (DATA_TYPE*)malloc(N*N*sizeof(DATA_TYPE));
	x1 = (DATA_TYPE*)malloc(N*sizeof(DATA_TYPE));
	x2 = (DATA_TYPE*)malloc(N*sizeof(DATA_TYPE));
	x1_outputFromGpu = (DATA_TYPE*)malloc(N*sizeof(DATA_TYPE));
	x2_outputFromGpu = (DATA_TYPE*)malloc(N*sizeof(DATA_TYPE));
	y_1 = (DATA_TYPE*)malloc(N*sizeof(DATA_TYPE));
	y_2 = (DATA_TYPE*)malloc(N*sizeof(DATA_TYPE));

	init_array(a, x1, x2, y_1, y_2);
	
    VulkanCompute vk("",parseDeviceSelectionFromArgs(argc,argv),GIGA);
	GPU_argv_init(&vk);

	mvtVulkan(&vk, a, x1, x2, y_1, y_2, x1_outputFromGpu, x2_outputFromGpu);
	
	t_start = rtclock();

	//run the algorithm on the CPU
	runMvt(a, x1, x2, y_1, y_2);

	t_end = rtclock();
	fprintf(stdout, "CPU Runtime: %0.6lfs\n", t_end - t_start);
	
	compareResults(x1, x1_outputFromGpu, x2, x2_outputFromGpu);

	free(a);
	free(x1);
	free(x2);
	free(x1_outputFromGpu);
	free(x2_outputFromGpu);
	free(y_1);
	free(y_2);
    vk.freeResources();

  	return EXIT_SUCCESS;
}

