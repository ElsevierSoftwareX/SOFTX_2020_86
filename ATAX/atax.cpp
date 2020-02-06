/**
 * atax.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
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
#define PERCENT_DIFF_ERROR_THRESHOLD 0.5

/* Thread block dimensions */
#define DIM_THREAD_BLOCK_X 256
#define DIM_THREAD_BLOCK_Y 1

#ifndef M_PI
#define M_PI 3.14159
#endif

#define VERBOSE_COMPARE_NUM -1

#define WARM_UP_RUN

void init_array(DATA_TYPE *x, DATA_TYPE *A)
{
	int i, j;

	for (i = 0; i < NX; i++)
	{
		x[i] = i * M_PI;
		for (j = 0; j < NY; j++)
		{
			A[i*NY + j] = ((DATA_TYPE) i*(j)) / NX;
		}
	}
}


void compareResults(DATA_TYPE *z, DATA_TYPE *z_outputFromGpu)
{
	int i, fail;
	fail = 0;
    int count = 0;

	for (i=0; i<NY; i++)
	{
        if((VERBOSE_COMPARE_NUM>=0) && (count%VERBOSE_COMPARE_NUM==0))
			std::cout << "CCHECK [" << count << "] " << z[i] << "/" << z_outputFromGpu[i] << std::endl;

        count++;
		if (percentDiff(z[i], z_outputFromGpu[i]) > PERCENT_DIFF_ERROR_THRESHOLD)
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

void atax_cpu(DATA_TYPE* A, DATA_TYPE* x, DATA_TYPE* y, DATA_TYPE* tmp)
{
	int i,j;
	
	for (i= 0; i < NY; i++)
	{
    	y[i] = 0;
	}
  
	for (i = 0; i < NX; i++)
 	{
      	tmp[i] = 0;

      	for (j = 0; j < NY; j++)
		{
			tmp[i] = tmp[i] + A[i*NY + j] * x[j];
		}
		
      	for (j = 0; j < NY; j++)
		{
			y[j] = y[j] + A[i*NY + j] * tmp[i];
		}
    }
}


void ataxGpu(VulkanCompute *vk, DATA_TYPE* A, DATA_TYPE* x, DATA_TYPE* y, DATA_TYPE* tmp, DATA_TYPE* y_outputFromGpu)
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"ataxkernel1.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "ataxkernel1") == -1)
	{
		std::cout << "Error in compiling shader ataxkernel1.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"ataxkernel2.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "ataxkernel2") == -1)
	{
		std::cout << "Error in compiling shader ataxkernel2.comp" << std::endl;
		exit(-1);
	}

	DATA_TYPE *A_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NX * NY, BufferUsage::BUF_INOUT);
	DATA_TYPE *x_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NY, BufferUsage::BUF_INOUT);
	DATA_TYPE *y_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NY, BufferUsage::BUF_INOUT);
	DATA_TYPE *tmp_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NX, BufferUsage::BUF_INOUT);

	ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	ComputeWorkDistribution_t grid1((size_t)(ceil( ((float)NX) / ((float)block.x) )), 1);
    ComputeWorkDistribution_t grid2((size_t)(ceil( ((float)NY) / ((float)block.x) )), 1);

	vk->startCreatePipeline("ataxkernel1");
		vk->setArg(PPTR(A_gpu),"ataxkernel1",4);
		vk->setArg(PPTR(x_gpu),"ataxkernel1",5);
        vk->setArg(PPTR(tmp_gpu),"ataxkernel1",6);
		vk->setLaunchConfiguration(grid1,block);
	PIPELINE_HANDLE hPipeline1 = vk->finalizePipeline();

    vk->startCreatePipeline("ataxkernel2");
		vk->setArg(PPTR(A_gpu),"ataxkernel2",4);
		vk->setArg(PPTR(y_gpu),"ataxkernel2",5);
        vk->setArg(PPTR(tmp_gpu),"ataxkernel2",6);
		vk->setLaunchConfiguration(grid2,block);
	PIPELINE_HANDLE hPipeline2 = vk->finalizePipeline();

#ifdef WARM_UP_RUN
	const uint8_t iterations = 2;
#else
	const uint8_t iterations = 1;
#endif 

	for(uint8_t iter=0; iter<iterations; iter++){

		memcpy(A_gpu,A,sizeof(DATA_TYPE) * NX * NY);
		memcpy(x_gpu,x,sizeof(DATA_TYPE) * NY);
		memcpy(y_gpu,y,sizeof(DATA_TYPE) * NY);
		memcpy(tmp_gpu,tmp,sizeof(DATA_TYPE) * NX);

		vk->startCreateCommandList();
			vk->synchBuffer(PPTR(A_gpu),HOST_TO_DEVICE);
			vk->synchBuffer(PPTR(x_gpu),HOST_TO_DEVICE);
			vk->synchBuffer(PPTR(y_gpu),HOST_TO_DEVICE);
			vk->synchBuffer(PPTR(tmp_gpu),HOST_TO_DEVICE);
		vk->finalizeCommandList();
		vk->submitWork();
		vk->deviceSynch();
	
		vk->startCreateCommandList();
			vk->selectPipeline(hPipeline1);
			vk->launchComputation("ataxkernel1");
			vk->selectPipeline(hPipeline2);
			vk->launchComputation("ataxkernel2");
		vk->finalizeCommandList();

		vk->deviceSynch();

		t_start = rtclock();

		vk->submitWork();
		vk->deviceSynch();
		/*atax_kernel1<<< grid1, block >>>(A_gpu,x_gpu,tmp_gpu);
		cudaThreadSynchronize();
		atax_kernel2<<< grid2, block >>>(A_gpu,y_gpu,tmp_gpu);
		cudaThreadSynchronize();*/
		t_end = rtclock();

		if(iterations>1&&iter==0)
			fprintf(stdout, "GPU (Warmup) Runtime: %0.6lfs\n", t_end - t_start);
		else fprintf(stdout, "GPU Runtime: %0.6lfs\n", t_end - t_start);
	}

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(y_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();
	
	//cudaMemcpy(y_outputFromGpu, y_gpu, sizeof(DATA_TYPE) * NX, cudaMemcpyDeviceToHost);

    memcpy(y_outputFromGpu,y_gpu,sizeof(DATA_TYPE) * NX);

}


int main(int argc, char** argv)
{
	double t_start, t_end;

	DATA_TYPE* A;
	DATA_TYPE* x;
	DATA_TYPE* y;
	DATA_TYPE* y_outputFromGpu;
	DATA_TYPE* tmp;

	A = (DATA_TYPE*)malloc(NX*NY*sizeof(DATA_TYPE));
	x = (DATA_TYPE*)malloc(NY*sizeof(DATA_TYPE));
	y = (DATA_TYPE*)malloc(NY*sizeof(DATA_TYPE));
	y_outputFromGpu = (DATA_TYPE*)malloc(NY*sizeof(DATA_TYPE));
	tmp = (DATA_TYPE*)malloc(NX*sizeof(DATA_TYPE));

	init_array(x, A);

    VulkanCompute vk("",parseDeviceSelectionFromArgs(argc,argv),GIGA);
	GPU_argv_init(&vk);
	ataxGpu(&vk, A, x, y, tmp, y_outputFromGpu);
	
	t_start = rtclock();
	atax_cpu(A, x, y, tmp);
	t_end = rtclock();
	fprintf(stdout, "CPU Runtime: %0.6lfs\n", t_end - t_start);

	compareResults(y, y_outputFromGpu);

	free(A);
	free(x);
	free(y);
	free(y_outputFromGpu);
	free(tmp);
    vk.freeResources();

  	return EXIT_SUCCESS;
}

