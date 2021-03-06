/**
 * covariance.cpp: This file is part of the vkpolybench test suite,
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
#define PERCENT_DIFF_ERROR_THRESHOLD 1.05

/* Thread block dimensions for kernel 1*/
#define DIM_THREAD_BLOCK_KERNEL_1_X 256
#define DIM_THREAD_BLOCK_KERNEL_1_Y 1

/* Thread block dimensions for kernel 2*/
#define DIM_THREAD_BLOCK_KERNEL_2_X 32
#define DIM_THREAD_BLOCK_KERNEL_2_Y 8

/* Thread block dimensions for kernel 3*/
#define DIM_THREAD_BLOCK_KERNEL_3_X 256
#define DIM_THREAD_BLOCK_KERNEL_3_Y 1

#define sqrt_of_array_cell(x,j) sqrt(x[j])

#define VERBOSE_COMPARE_NUM -1

#define WARM_UP_RUN

void init_arrays(DATA_TYPE* data)
{
	int i, j;

	for (i = 1; i < (M+1); i++)
	{
		for (j = 1; j < (N+1); j++)
		{
			data[i*(N+1) + j] = ((DATA_TYPE) i*j) / M;
		}
	}
}


void covariance(DATA_TYPE* data, DATA_TYPE* symmat, DATA_TYPE* mean)
{
	int i, j, j1,j2;

  	/* Determine mean of column vectors of input data matrix */
	for (j = 1; j < (M+1); j++)
	{
		mean[j] = 0.0;
		for (i = 1; i < (N+1); i++)
		{
        		mean[j] += data[i*(M+1) + j];
		}
		mean[j] /= FLOAT_N;
	}

  	/* Center the column vectors. */
	for (i = 1; i < (N+1); i++)
	{
		for (j = 1; j < (M+1); j++)
		{
			data[i*(M+1) + j] -= mean[j];
		}
	}

  	/* Calculate the m * m covariance matrix. */
	for (j1 = 1; j1 < (M+1); j1++)
	{
		for (j2 = j1; j2 < (M+1); j2++)
     		{
       		symmat[j1*(M+1) + j2] = 0.0;
			for (i = 1; i < N+1; i++)
			{
				symmat[j1*(M+1) + j2] += data[i*(M+1) + j1] * data[i*(M+1) + j2];
			}
        		symmat[j2*(M+1) + j1] = symmat[j1*(M+1) + j2];
      		}
	}
}


void compareResults(DATA_TYPE* symmat, DATA_TYPE* symmat_outputFromGpu)
{
	int i,j,fail;
	fail = 0;
    int count = 0;

	for (i=1; i < (M+1); i++)
	{
		for (j=1; j < (N+1); j++)
		{
            if((VERBOSE_COMPARE_NUM>=0) && (count%VERBOSE_COMPARE_NUM==0))
				std::cout << "CCHECK [" << count << "] " << symmat[i*(N+1)+j] << "/" << symmat_outputFromGpu[i*(N+1)+j] << std::endl;
            
			count++;
			if (percentDiff(symmat[i*(N+1) + j], symmat_outputFromGpu[i*(N+1) + j]) > PERCENT_DIFF_ERROR_THRESHOLD)
			{
				fail++;
			}			
		}
	}
	PRINT_SANITY("Non-Matching CPU-GPU Outputs Beyond Error Threshold of %4.2f Percent: %d\n", PERCENT_DIFF_ERROR_THRESHOLD, fail);
}


ANDROID_MAIN("COVAR")

void GPU_argv_init(VulkanCompute *vk)
{
	vk->createContext();
	vk->printContextInformation();
#ifdef __ANDROID__
	vk->setAndroidAppCtx(androidapp);
	PRINT_SANITY("INFO: This VK benchmark has been compiled for Android. Problem size is reduced to N %d and M %d", N,M);
#endif
}

void covarianceVulkan(VulkanCompute *vk, DATA_TYPE* data, DATA_TYPE* symmat, DATA_TYPE* mean, DATA_TYPE* symmat_outputFromGpu)
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"meankernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "meankernel") == -1)
	{
		std::cout << "Error in compiling shader meankernel.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"reducekernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "reducekernel") == -1)
	{
		std::cout << "Error in compiling shader reducekernel.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"covarkernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "covarkernel") == -1)
	{
		std::cout << "Error in compiling shader covarkernel.comp" << std::endl;
		exit(-1);
	}

	DATA_TYPE *data_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * (M+1) * (N+1), BufferUsage::BUF_INOUT);
    DATA_TYPE *symmat_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * (M+1) * (M+1), BufferUsage::BUF_INOUT);
	DATA_TYPE *mean_gpu =  (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * (M+1), BufferUsage::BUF_INOUT);

    ComputeWorkDistribution_t block1(DIM_THREAD_BLOCK_KERNEL_1_X, DIM_THREAD_BLOCK_KERNEL_1_Y);
	ComputeWorkDistribution_t grid1((size_t)(ceil((float)M) / ((float)DIM_THREAD_BLOCK_KERNEL_1_X)), 1);
  
    ComputeWorkDistribution_t block2(DIM_THREAD_BLOCK_KERNEL_2_X, DIM_THREAD_BLOCK_KERNEL_2_Y);
	ComputeWorkDistribution_t grid2((size_t)(ceil((float)M) / ((float)DIM_THREAD_BLOCK_KERNEL_2_X)), (size_t)(ceil((float)N) / ((float)DIM_THREAD_BLOCK_KERNEL_2_X)));

    ComputeWorkDistribution_t block3(DIM_THREAD_BLOCK_KERNEL_3_X, DIM_THREAD_BLOCK_KERNEL_3_Y);
	ComputeWorkDistribution_t grid3((size_t)(ceil((float)M) / ((float)DIM_THREAD_BLOCK_KERNEL_3_X)), 1);

    vk->startCreatePipeline("meankernel");
		vk->setArg(PPTR(mean_gpu),"meankernel",4);
		vk->setArg(PPTR(data_gpu),"meankernel",5);
		vk->setLaunchConfiguration(grid1,block1);
	PIPELINE_HANDLE hPipeline1 = vk->finalizePipeline();

    vk->startCreatePipeline("reducekernel");
		vk->setArg(PPTR(mean_gpu),"reducekernel",4);
        vk->setArg(PPTR(data_gpu),"reducekernel",5);
		vk->setLaunchConfiguration(grid2,block2);
	PIPELINE_HANDLE hPipeline2 = vk->finalizePipeline();

    vk->startCreatePipeline("covarkernel");
		vk->setArg(PPTR(symmat_gpu),"covarkernel",4);
        vk->setArg(PPTR(data_gpu),"covarkernel",5);
		vk->setLaunchConfiguration(grid3,block3);
	PIPELINE_HANDLE hPipeline3 = vk->finalizePipeline();


#ifdef WARM_UP_RUN
	const uint8_t iterations = 2;
#else
	const uint8_t iterations = 1;
#endif 

	for(uint8_t iter=0; iter<iterations; iter++){

		memcpy(data_gpu, data, sizeof(DATA_TYPE) * (M+1) * (N+1));
		memcpy(symmat_gpu, symmat, sizeof(DATA_TYPE) * (M+1) * (M+1));
		memcpy(mean_gpu, mean, sizeof(DATA_TYPE) * (M+1));

		vk->startCreateCommandList();
			vk->synchBuffer(PPTR(data_gpu),HOST_TO_DEVICE);
			vk->synchBuffer(PPTR(symmat_gpu),HOST_TO_DEVICE);
			vk->synchBuffer(PPTR(mean_gpu),HOST_TO_DEVICE);
		vk->finalizeCommandList();
		vk->submitWork();
		vk->deviceSynch();

		vk->startCreateCommandList();
			vk->selectPipeline(hPipeline1);
			vk->launchComputation("meankernel");
			vk->selectPipeline(hPipeline2);
			vk->launchComputation("reducekernel");
			vk->selectPipeline(hPipeline3);
			vk->launchComputation("covarkernel");
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
		vk->synchBuffer(PPTR(symmat_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	memcpy(symmat_outputFromGpu, symmat_gpu, sizeof(DATA_TYPE) * (M+1) * (N+1));
	
}


int main(int argc, char *argv[])
{
	double t_start, t_end;

	DATA_TYPE* data;
	DATA_TYPE* symmat;
	DATA_TYPE* mean;
	DATA_TYPE* symmat_outputFromGpu;	

	data = (DATA_TYPE*)malloc((M+1)*(N+1)*sizeof(DATA_TYPE));
	symmat = (DATA_TYPE*)malloc((M+1)*(M+1)*sizeof(DATA_TYPE));
	mean = (DATA_TYPE*)malloc((M+1)*sizeof(DATA_TYPE));
	symmat_outputFromGpu = (DATA_TYPE*)malloc((M+1)*(M+1)*sizeof(DATA_TYPE));	

	init_arrays(data);
    
    VulkanCompute vk("",parseDeviceSelectionFromArgs(argc,argv),GIGA);
	GPU_argv_init(&vk);

	covarianceVulkan(&vk, data, symmat, mean, symmat_outputFromGpu);
	
	t_start = rtclock();
	covariance(data, symmat, mean);
	t_end = rtclock();
	PRINT_RESULT("CPU Runtime: %0.6lfs\n", t_end - t_start);

	compareResults(symmat, symmat_outputFromGpu);

	free(data);
	free(symmat);
	free(mean);
	free(symmat_outputFromGpu);	
    vk.freeResources();

  	return EXIT_SUCCESS;
}

