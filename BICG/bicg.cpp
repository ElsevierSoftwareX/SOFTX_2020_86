/**
 * bicg.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
 * Vulkan version
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <sys/time.h>

#include "../vkcomp/VulkanCompute.h"
#include "../vkcomp/stdafx.h"
#include "../polybenchUtilFuncts.h"

/*
Data types, sizes and other constants are defined in the following header file.
Such defines are common for host and device code.
*/
#include "HDcommon.h"

//Error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.5

/* Thread block dimensions */
#define DIM_THREAD_BLOCK_X 256
#define DIM_THREAD_BLOCK_Y 1

#ifndef M_PI
#define M_PI 3.14159
#endif

#define VERBOSE_COMPARE_NUM -1

void init_array(DATA_TYPE *A, DATA_TYPE *p, DATA_TYPE *r)
{
	int i, j;

  	for (i = 0; i < NX; i++)
	{
    		r[i] = i * M_PI;

    		for (j = 0; j < NY; j++)
		{
      			A[i*NY + j] = ((DATA_TYPE) i*j) / NX;
		}
 	}
	
	for (i = 0; i < NY; i++)
	{
    		p[i] = i * M_PI;
	}
}


void compareResults(DATA_TYPE* s, DATA_TYPE* s_outputFromGpu, DATA_TYPE* q, DATA_TYPE* q_outputFromGpu)
{
	int i,fail;
	fail = 0;
    int count = 0;

	// Compare s with s_vulkan
	for (i=0; i<NX; i++)
	{
        if((VERBOSE_COMPARE_NUM>=0) && (count%VERBOSE_COMPARE_NUM==0)) 
			std::cout << "CCHECK [" << count << "] " <<  q[i] << "/" << q_outputFromGpu[i] << std::endl;
        count++;

		if (percentDiff(q[i], q_outputFromGpu[i]) > PERCENT_DIFF_ERROR_THRESHOLD)
		{
			fail++;
		}
	}

    count = 0;

	for (i=0; i<NY; i++)
	{
        if((VERBOSE_COMPARE_NUM>=0) && (count%VERBOSE_COMPARE_NUM==0))
			std::cout << "CCHECK [" << count << "] " <<  s[i] << "/" << s_outputFromGpu[i] << std::endl;
        
		count++;
		if (percentDiff(s[i], s_outputFromGpu[i]) > PERCENT_DIFF_ERROR_THRESHOLD)
		{
			fail++;
		}		
	}
	
	// print results
	printf("Non-Matching CPU-GPU Outputs Beyond Error Threshold of %4.2f Percent: %d\n", PERCENT_DIFF_ERROR_THRESHOLD, fail);
}


void GPU_argv_init(VulkanCompute *vk)
{
	vk->createContext();
	vk->printContextInformation();
}

void bicg_cpu(DATA_TYPE* A, DATA_TYPE* r, DATA_TYPE* s, DATA_TYPE* p, DATA_TYPE* q)
{
	int i,j;
	
  	for (i = 0; i < NY; i++)
	{
		s[i] = 0.0;
	}

    for (i = 0; i < NX; i++)
    {
		q[i] = 0.0;
		for (j = 0; j < NY; j++)
	  	{
	    		s[j] = s[j] + r[i] * A[i*NY + j];
	    		q[i] = q[i] + A[i*NY + j] * p[j];
	  	}
	}
}


void bicgVulkan(VulkanCompute *vk, DATA_TYPE* A, DATA_TYPE* r, DATA_TYPE* s, DATA_TYPE* p, DATA_TYPE* q,
			DATA_TYPE* s_outputFromGpu, DATA_TYPE* q_outputFromGpu)
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"bicgkernel1.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "bicgkernel1") == -1)
	{
		std::cout << "Error in compiling shader bicgkernel1.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"bicgkernel2.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "bicgkernel2") == -1)
	{
		std::cout << "Error in compiling shader bicgkernel2.comp" << std::endl;
		exit(-1);
	}

	DATA_TYPE *A_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NX * NY, BufferUsage::BUF_INOUT);
	DATA_TYPE *q_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NX, BufferUsage::BUF_INOUT);
	DATA_TYPE *p_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NY, BufferUsage::BUF_INOUT);
	DATA_TYPE *r_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NX, BufferUsage::BUF_INOUT);
	DATA_TYPE *s_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NY, BufferUsage::BUF_INOUT);

    memcpy(A_gpu, A, sizeof(DATA_TYPE) * NX * NY);
	memcpy(r_gpu, r, sizeof(DATA_TYPE) * NX);
	memcpy(s_gpu, s, sizeof(DATA_TYPE) * NY);
	memcpy(p_gpu, p, sizeof(DATA_TYPE) * NY);
	memcpy(q_gpu, q, sizeof(DATA_TYPE) * NX);

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(A_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(r_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(s_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(p_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(q_gpu),HOST_TO_DEVICE);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

    ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	ComputeWorkDistribution_t grid1((size_t)(ceil( ((float)NY) / ((float)block.x) )), 1);
    ComputeWorkDistribution_t grid2((size_t)(ceil( ((float)NX) / ((float)block.x) )), 1);

    vk->startCreatePipeline("bicgkernel1");
		vk->setArg(PPTR(A_gpu),"bicgkernel1",4);
		vk->setArg(PPTR(r_gpu),"bicgkernel1",5);
        vk->setArg(PPTR(s_gpu),"bicgkernel1",6);
		vk->setLaunchConfiguration(grid1,block);
	PIPELINE_HANDLE hPipeline1 = vk->finalizePipeline();

    vk->startCreatePipeline("bicgkernel2");
		vk->setArg(PPTR(A_gpu),"bicgkernel2",4);
		vk->setArg(PPTR(p_gpu),"bicgkernel2",5);
        vk->setArg(PPTR(q_gpu),"bicgkernel2",6);
		vk->setLaunchConfiguration(grid2,block);
	PIPELINE_HANDLE hPipeline2 = vk->finalizePipeline();

     vk->startCreateCommandList();
		vk->selectPipeline(hPipeline1);
		vk->launchComputation("bicgkernel1");
        vk->selectPipeline(hPipeline2);
        vk->launchComputation("bicgkernel2");
	vk->finalizeCommandList();

    vk->deviceSynch();

	t_start = rtclock();
    vk->submitWork();
	vk->deviceSynch();
	t_end = rtclock();
	fprintf(stdout, "GPU Runtime: %0.6lfs\n", t_end - t_start);

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(s_gpu),DEVICE_TO_HOST);
        vk->synchBuffer(PPTR(q_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

    memcpy(s_outputFromGpu, s_gpu, sizeof(DATA_TYPE) * NY);
	memcpy(q_outputFromGpu, q_gpu, sizeof(DATA_TYPE) * NX);

}


int main(int argc, char** argv)
{
	double t_start, t_end;

	DATA_TYPE* A;
	DATA_TYPE* r;
	DATA_TYPE* s;
	DATA_TYPE* p;
	DATA_TYPE* q;
	DATA_TYPE* s_outputFromGpu;
	DATA_TYPE* q_outputFromGpu;
 	
	A = (DATA_TYPE*)malloc(NX*NY*sizeof(DATA_TYPE));
	r = (DATA_TYPE*)malloc(NX*sizeof(DATA_TYPE));
	s = (DATA_TYPE*)malloc(NY*sizeof(DATA_TYPE));
	p = (DATA_TYPE*)malloc(NY*sizeof(DATA_TYPE));
	q = (DATA_TYPE*)malloc(NX*sizeof(DATA_TYPE));
	s_outputFromGpu = (DATA_TYPE*)malloc(NY*sizeof(DATA_TYPE));
	q_outputFromGpu = (DATA_TYPE*)malloc(NX*sizeof(DATA_TYPE));

	init_array(A, p, r);

    VulkanCompute vk("",parseDeviceSelectionFromArgs(argc,argv),GIGA);
	GPU_argv_init(&vk);

	bicgVulkan(&vk, A, r, s, p, q, s_outputFromGpu, q_outputFromGpu);

	t_start = rtclock();
	bicg_cpu(A, r, s, p, q);
	t_end = rtclock();

	fprintf(stdout, "CPU Runtime: %0.6lfs\n", t_end - t_start);

	compareResults(s, s_outputFromGpu, q, q_outputFromGpu);

	free(A);
	free(r);
	free(s);
	free(p);
	free(q);
	free(s_outputFromGpu);
	free(q_outputFromGpu);
    	vk.freeResources();

  	return EXIT_SUCCESS;
}

