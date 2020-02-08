/**
 * 2mm.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
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
#define DIM_THREAD_BLOCK_X 32
#define DIM_THREAD_BLOCK_Y 8

#define VERBOSE_COMPARE_NUM -1

/*[Android] Bug. Sanity check and/or vkQueueSubmit fails for repeated runs.
Occasionally also happens for Win10/TITAN V configuration
*/
#ifndef __ANDROID__
#define WARM_UP_RUN
#endif

void init_array(DATA_TYPE* A, DATA_TYPE* B, DATA_TYPE* C, DATA_TYPE* D)
{
	int i, j;

	for (i = 0; i < NI; i++)
	{
		for (j = 0; j < NK; j++)
		{
			A[i*NI + j] = ((DATA_TYPE) i*j) / NI;
		}
	}

	for (i = 0; i < NK; i++)
	{
		for (j = 0; j < NJ; j++)
		{
			B[i*NK + j] = ((DATA_TYPE) i*(j+1)) / NJ;
		}
	}

	for (i = 0; i < NL; i++)
	{
		for (j = 0; j < NJ; j++)
		{
			C[i*NL + j] = ((DATA_TYPE) i*(j+3)) / NL;
		}
	}

	for (i = 0; i < NI; i++)
	{
		for (j = 0; j < NL; j++)
		{
			D[i*NL + j] = ((DATA_TYPE) i*(j+2)) / NK;
		}
	}
}


void compareResults(DATA_TYPE *E, DATA_TYPE *E_outputFromGpu)
{
	int i,j,fail;
	fail = 0;
	int count = 0;

	for (i=0; i < NL; i++)
	{
		for (j=0; j < NI; j++)
		{
			if((VERBOSE_COMPARE_NUM>=0) && (count%VERBOSE_COMPARE_NUM==0))
				std::cout << "CCHECK [" << count  <<  "] " << E[i*NI+j] << "/" << E_outputFromGpu[i*NI+j] << std::endl;

			count++;
			if (percentDiff(E[i*NI + j], E_outputFromGpu[i*NI + j]) > PERCENT_DIFF_ERROR_THRESHOLD)
			{
				fail++;
			}
		}
	}
	
	// print results
	PRINT_SANITY("Non-Matching CPU-GPU Outputs Beyond Error Threshold of %4.2f Percent: %d\n",PERCENT_DIFF_ERROR_THRESHOLD,fail);
}


ANDROID_MAIN("2MM")

void GPU_argv_init(VulkanCompute *vk)
{
	vk->createContext();
	vk->printContextInformation();
#ifdef __ANDROID__
	vk->setAndroidAppCtx(androidapp);
	PRINT_SANITY("INFO: This VK benchmark has been compiled for Android. Problem size is reduced to NI %d and NJ %d and the others", NI,NJ);
#endif
}

void mm2_cpu(DATA_TYPE* A, DATA_TYPE* B, DATA_TYPE* C, DATA_TYPE* D, DATA_TYPE* E)
{
	int i, j, k;
	
  	for (i = 0; i < NI; i++)
	{
		//if(i%100==0) std::cout << "iteration " << i << "/" << (NI-1) << std::endl;

		for (j = 0; j < NJ; j++)
		{
			C[i*NJ + j] = 0.0;
			for (k = 0; k < NK; ++k)
			{
				C[i*NJ + j] += A[i*NK + k] * B[k*NJ + j];
			}
		}
	}
	
	for (i = 0; i < NI; i++)
	{

		//if(i%100==0) std::cout << "iteration " << i << "/" << (NI-1) << std::endl;

		for (j = 0; j < NL; j++)
		{
			E[i*NL + j] = 0.0;
			for (k = 0; k < NJ; ++k)
			{
				E[i*NL + j] += C[i*NJ + k] * D[k*NL + j];
			}
		}
	}
}


void mm2Vulkan(VulkanCompute *vk, DATA_TYPE* A, DATA_TYPE* B, DATA_TYPE* C, DATA_TYPE* D, DATA_TYPE* E, DATA_TYPE* E_outputFromGpu)
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"mm2kernel1.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "mm2kernel1") == -1)
	{
		std::cout << "Error in compiling shader mm2kernel1.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"mm2kernel2.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "mm2kernel2") == -1)
	{
		std::cout << "Error in compiling shader mm2kernel2.comp" << std::endl;
		exit(-1);
	}


	DATA_TYPE *A_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NI * NK, BufferUsage::BUF_INOUT);
	DATA_TYPE *B_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NK * NJ, BufferUsage::BUF_INOUT);
	DATA_TYPE *C_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NI * NJ, BufferUsage::BUF_INOUT);
	DATA_TYPE *D_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NJ * NL, BufferUsage::BUF_INOUT);
	DATA_TYPE *E_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NI * NL, BufferUsage::BUF_INOUT);

	/*dim3 block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	dim3 grid1((size_t)ceil( ((float)NJ) / ((float)block.x) ), (size_t)ceil( ((float)NI) / ((float)block.y)) );
	dim3 grid2((size_t)ceil( ((float)NL) / ((float)block.x) ), (size_t)ceil( ((float)NI) / ((float)block.y)) );*/

    ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	ComputeWorkDistribution_t grid1((size_t)ceil( ((float)NJ) / ((float)block.x) ), (size_t)ceil( ((float)NI) / ((float)block.y)));
    ComputeWorkDistribution_t grid2((size_t)ceil( ((float)NL) / ((float)block.x) ), (size_t)ceil( ((float)NI) / ((float)block.y)));

    vk->startCreatePipeline("mm2kernel1");
		vk->setArg(PPTR(A_gpu),"mm2kernel1",4);
		vk->setArg(PPTR(B_gpu),"mm2kernel1",5);
        vk->setArg(PPTR(C_gpu),"mm2kernel1",6);
		vk->setLaunchConfiguration(grid1,block);
	PIPELINE_HANDLE hPipeline1 = vk->finalizePipeline();

    vk->startCreatePipeline("mm2kernel2");
		vk->setArg(PPTR(C_gpu),"mm2kernel2",4);
		vk->setArg(PPTR(D_gpu),"mm2kernel2",5);
        vk->setArg(PPTR(E_gpu),"mm2kernel2",6);
		vk->setLaunchConfiguration(grid2,block);
	PIPELINE_HANDLE hPipeline2 = vk->finalizePipeline();

#ifdef WARM_UP_RUN
	const uint8_t iterations = 2;
#else
	const uint8_t iterations = 1;
#endif 

	for(uint8_t iter=0; iter<iterations; iter++){

		memcpy(A_gpu,A,sizeof(DATA_TYPE) * NI * NK);
		memcpy(B_gpu,B,sizeof(DATA_TYPE) * NK * NJ);
		memcpy(C_gpu,C,sizeof(DATA_TYPE) * NI * NJ);
		memcpy(D_gpu,D,sizeof(DATA_TYPE) * NJ * NL);

		vk->startCreateCommandList();
			vk->synchBuffer(PPTR(A_gpu),HOST_TO_DEVICE);
			vk->synchBuffer(PPTR(B_gpu),HOST_TO_DEVICE);
			vk->synchBuffer(PPTR(C_gpu),HOST_TO_DEVICE);
			vk->synchBuffer(PPTR(D_gpu),HOST_TO_DEVICE);
		vk->finalizeCommandList();

		vk->submitWork();
		vk->deviceSynch();
			
		vk->startCreateCommandList();
			vk->selectPipeline(hPipeline1);
			vk->launchComputation("mm2kernel1");
			vk->selectPipeline(hPipeline2);
			vk->launchComputation("mm2kernel2");
		vk->finalizeCommandList();

		vk->deviceSynch();

		t_start = rtclock();
		vk->submitWork();
		vk->deviceSynch();
		t_end = rtclock();

		if(iterations>1&&iter==0)
			PRINT_RESULT("GPU (Warmup) Runtime: %0.6lfs\n", t_end-t_start);
		else PRINT_RESULT("GPU Runtime: %0.6lfs\n", t_end - t_start);
	
	}


    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(E_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

    memcpy(E_outputFromGpu, E_gpu, sizeof(DATA_TYPE) * NI * NL);
}


int main(int argc, char** argv)
{
	double t_start, t_end;
	
	DATA_TYPE* C;
	DATA_TYPE* A;
	DATA_TYPE* B;
	DATA_TYPE* D;
	DATA_TYPE* E;
	DATA_TYPE* E_outputFromGpu;

	C = (DATA_TYPE*)malloc(NI*NJ*sizeof(DATA_TYPE));
	A = (DATA_TYPE*)malloc(NI*NK*sizeof(DATA_TYPE));
	B = (DATA_TYPE*)malloc(NK*NJ*sizeof(DATA_TYPE));
	D = (DATA_TYPE*)malloc(NJ*NL*sizeof(DATA_TYPE));
	E = (DATA_TYPE*)malloc(NI*NL*sizeof(DATA_TYPE));
	E_outputFromGpu = (DATA_TYPE*)malloc(NI*NL*sizeof(DATA_TYPE));
    
  	init_array(A, B, C, D);
	
    VulkanCompute vk("",parseDeviceSelectionFromArgs(argc,argv),GIGA);
    GPU_argv_init(&vk);

	mm2Vulkan(&vk, A, B, C, D, E, E_outputFromGpu);

	t_start = rtclock();
	mm2_cpu(A, B, C, D, E);
	t_end = rtclock();
	PRINT_RESULT("CPU Runtime: %0.6lfs\n", t_end - t_start);

	compareResults(E, E_outputFromGpu);

	free(C);
	free(A);
	free(B);
	free(D);
	free(E);
	free(E_outputFromGpu);
    vk.freeResources();

  	return EXIT_SUCCESS;
}

