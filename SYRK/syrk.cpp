/**
 * syrk.cpp: This file is part of the vkpolybench test suite,
 * Vulkan version.
 * CPU reference implementation is derived from PolyBench/GPU 1.0.
 * See LICENSE.md for vkpolybench and other 3rd party licenses. 
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
#define DIM_THREAD_BLOCK_X 32
#define DIM_THREAD_BLOCK_Y 8

#define VERBOSE_COMPARE_NUM -1

#define WARM_UP_RUN 

void init_arrays(DATA_TYPE* A, DATA_TYPE* C)
{
	int i, j;
	
	for (i = 0; i < N; i++)
    	{
		for (j = 0; j < M; j++)
		{
			A[i*M + j] = ((DATA_TYPE) i*j) / N;
		}
		
		for (j = 0; j < N; j++)
		{
			C[i*M + j] = ((DATA_TYPE) i*j + 2) / N;
		}
	}
}


void syrk(DATA_TYPE* A, DATA_TYPE* C)
{
	int i, j, k;
	
	/*  C := alpha*A*A' + beta*C */
	for (i = 0; i < N; i++)
	{
		for (j = 0; j < N; j++)
		{
			C[i*M + j] *= beta;
		}
	}
	
	for (i = 0; i < N; i++)
	{
		for (j = 0; j < N; j++)
		{
			for (k = 0; k < M; k++)
			{
				C[i*N + j] += alpha * A[i*M + k] * A[j*M + k];
			}
		}
	}
}


void compareResults(DATA_TYPE* C, DATA_TYPE* C_outputFromGpu)
{
	int i,j,fail;
	fail = 0;
    int count = 0;
    
	// Compare C with D
	for (i=0; i<N; i++)
	{
		for (j=0; j<M; j++)
		{

            if((VERBOSE_COMPARE_NUM>=0) && (count%VERBOSE_COMPARE_NUM==0)) 
                std::cout << "CCHECK [" << count << "] " << C[i*M + j] << "/" << C_outputFromGpu[i*M + j] << std::endl;
            
			count++;
			if (percentDiff(C[i*M + j], C_outputFromGpu[i*M + j]) > PERCENT_DIFF_ERROR_THRESHOLD)
			{
				fail++;
			}
		}
	}
	
	// print results
	PRINT_SANITY("Non-Matching CPU-GPU Outputs Beyond Error Threshold of %4.2f Percent: %d\n", PERCENT_DIFF_ERROR_THRESHOLD, fail);
}

ANDROID_MAIN("SYRK")

void GPU_argv_init(VulkanCompute *vk)
{
	vk->createContext();
	vk->printContextInformation();
#ifdef __ANDROID__
	vk->setAndroidAppCtx(androidapp);
	PRINT_SANITY("INFO: This VK benchmark has been compiled for Android. Problem size is reduced to N %d and M %d", N, M);
#endif
}

void syrkVulkan(VulkanCompute *vk, DATA_TYPE* A, DATA_TYPE* C, DATA_TYPE* C_outputFromGpu)
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"syrkkernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "syrkkernel") == -1)
	{
		std::cout << "Error in compiling shader syrkkernel.comp" << std::endl;
		exit(-1);
	}

	DATA_TYPE* A_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N * M, BufferUsage::BUF_INOUT);
	DATA_TYPE* C_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N * N, BufferUsage::BUF_INOUT);

    ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
    ComputeWorkDistribution_t grid((size_t)(ceil(((float)N) / ((float)DIM_THREAD_BLOCK_X))), (size_t)ceil(((float)N) / ((float)DIM_THREAD_BLOCK_Y)));

    vk->startCreatePipeline("syrkkernel");
		vk->setArg(PPTR(A_gpu),"syrkkernel",4);
		vk->setArg(PPTR(C_gpu),"syrkkernel",5);
		vk->setLaunchConfiguration(grid,block);
	PIPELINE_HANDLE hPipeline = vk->finalizePipeline();


#ifdef WARM_UP_RUN
	const uint8_t iterations = 2;
#else
	const uint8_t iterations = 1;
#endif 

	for(uint8_t iter=0; iter<iterations; iter++){

		memcpy(A_gpu, A, sizeof(DATA_TYPE) * N * M);
		memcpy(C_gpu, C, sizeof(DATA_TYPE) * N * N);

		vk->startCreateCommandList();
			vk->synchBuffer(PPTR(A_gpu),HOST_TO_DEVICE);
			vk->synchBuffer(PPTR(C_gpu),HOST_TO_DEVICE);
		vk->finalizeCommandList();
		vk->submitWork();
		vk->deviceSynch();

		vk->startCreateCommandList();
				vk->selectPipeline(hPipeline);
				vk->launchComputation("syrkkernel");
		vk->finalizeCommandList();
		vk->deviceSynch();

		t_start = rtclock();
		vk->submitWork();
		vk->deviceSynch();
		t_end = rtclock();

		if(iterations>1&&iter==0)
			PRINT_RESULT("GPU (Warmup) Runtime: %0.6lfs\n", t_end - t_start);
		else PRINT_RESULT("GPU Runtime: %0.6lfs\n", t_end - t_start);
	
	}

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(C_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	memcpy(C_outputFromGpu, C_gpu, sizeof(DATA_TYPE) * N * N);
}


int main(int argc, char *argv[])
{
	double t_start, t_end;

	DATA_TYPE* A;
	DATA_TYPE* C;
	DATA_TYPE* C_outputFromGpu;

	A = (DATA_TYPE*)malloc(N*M*sizeof(DATA_TYPE));
	C = (DATA_TYPE*)malloc(N*M*sizeof(DATA_TYPE));
	C_outputFromGpu = (DATA_TYPE*)malloc(N*M*sizeof(DATA_TYPE));

	init_arrays(A, C);

    VulkanCompute vk("",parseDeviceSelectionFromArgs(argc,argv),GIGA);
	GPU_argv_init(&vk);

	syrkVulkan(&vk, A, C, C_outputFromGpu);

	t_start = rtclock();
	syrk(A, C);
	t_end = rtclock();
	PRINT_RESULT("CPU Runtime: %0.6lfs\n", t_end - t_start);

	compareResults(C, C_outputFromGpu);

	free(A);
	free(C);
	free(C_outputFromGpu);
    vk.freeResources();

	return EXIT_SUCCESS;
}

