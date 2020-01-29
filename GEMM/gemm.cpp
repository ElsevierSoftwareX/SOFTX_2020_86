/**
 * gemm.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
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

//define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

/* Problem size */
#define NI 512
#define NJ 512
#define NK 512

/* Thread block dimensions */
#define DIM_THREAD_BLOCK_X 32
#define DIM_THREAD_BLOCK_Y 8

/* Declared constant values for ALPHA and BETA (same as values in PolyBench 2.0) */
#define ALPHA 32412.0f
#define BETA 2123.0f

/* Can switch DATA_TYPE between float and double */
typedef float DATA_TYPE;

void gemm(DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *C)
{
	int i,j,k;
	
	for (i = 0; i < NI; i++)
	{
    	for (j = 0; j < NJ; j++)
    	{
			C[i*NJ + j] *= BETA;
	
			for (k = 0; k < NK; ++k)
			{
	  			C[i*NJ + j] += ALPHA * A[i*NK + k] * B[k*NJ + j];
			}
      	}
	}
}


void init(DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *C)
{
	int i, j;

  	for (i = 0; i < NI; i++)
	{
    	for (j = 0; j < NK; j++)
		{
      		A[i*NK + j] = ((DATA_TYPE) i*j) / NI;
		}
	}

  	for (i = 0; i < NK; i++)
	{
    	for (j = 0; j < NJ; j++)
		{
      		B[i*NJ + j] = ((DATA_TYPE) i*j + 1) / NJ;
		}
	}

  	for (i = 0; i < NI; i++)
	{
    	for (j = 0; j < NJ; j++)
		{
      		C[i*NJ + j] = ((DATA_TYPE) i*j + 2) / NJ;
		}
	}
}


void compareResults(DATA_TYPE* C, DATA_TYPE* C_outputFromGpu)
{
	int i, j, fail;
	fail = 0;
    int count = 0;
	
	// Compare C1 and C2
	for (i=0; i < NI; i++) 
	{
		for (j=0; j < NJ; j++) 
		{
            if(count%250==0) std::cout << "CCHECK [" << count << "] " << C[i*NJ + j] << "/" << C_outputFromGpu[i*NJ + j] << std::endl;
            count++;

			if (percentDiff(C[i*NJ + j], C_outputFromGpu[i*NJ + j]) > PERCENT_DIFF_ERROR_THRESHOLD) 
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


void gemmVulkan(VulkanCompute *vk, DATA_TYPE* A, DATA_TYPE* B, DATA_TYPE* C, DATA_TYPE* C_outputFromGpu)
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"gemmkernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "gemmkernel") == -1)
	{
		std::cout << "Error in compiling shader gemmkernel.comp" << std::endl;
		exit(-1);
	}

	DATA_TYPE *A_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NI * NK, BufferUsage::BUF_INOUT);
	DATA_TYPE *B_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NK * NJ, BufferUsage::BUF_INOUT);
	DATA_TYPE *C_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NI * NJ, BufferUsage::BUF_INOUT);

	memcpy(A_gpu, A, sizeof(DATA_TYPE) * NI * NK);
	memcpy(B_gpu, B, sizeof(DATA_TYPE) * NK * NJ);
	memcpy(C_gpu, C, sizeof(DATA_TYPE) * NI * NJ);

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(A_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(B_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(C_gpu),HOST_TO_DEVICE);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	/*dim3 block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	dim3 grid((size_t)(ceil( ((float)NI)/ ((float)block.x) )),(size_t)(ceil( ((float)NJ)/ ((float)block.y) )));*/
    ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	ComputeWorkDistribution_t grid((size_t)(ceil( ((float)NI)/ ((float)block.x) )),(size_t)(ceil( ((float)NJ)/ ((float)block.y) )));

     vk->startCreatePipeline("gemmkernel");
		vk->setArg(PPTR(A_gpu),"gemmkernel",4);
		vk->setArg(PPTR(B_gpu),"gemmkernel",5);
        vk->setArg(PPTR(C_gpu),"gemmkernel",6);
		vk->setLaunchConfiguration(grid,block);
	PIPELINE_HANDLE hPipeline = vk->finalizePipeline();

    vk->startCreateCommandList();
	    vk->selectPipeline(hPipeline);
	    vk->launchComputation("gemmkernel");
	vk->finalizeCommandList();

    vk->deviceSynch();

	t_start = rtclock();
    vk->submitWork();
	vk->deviceSynch();
	/*gemm_kernel<<< grid, block >>>(A_gpu, B_gpu, C_gpu);
	cudaThreadSynchronize();*/
	t_end = rtclock();
	fprintf(stdout, "GPU Runtime: %0.6lfs\n", t_end - t_start);

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(C_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	memcpy(C_outputFromGpu, C_gpu, sizeof(DATA_TYPE) * NI * NJ);
}
	

int main(int argc, char *argv[])
{
	double t_start, t_end;

	DATA_TYPE* A;
	DATA_TYPE* B;  
	DATA_TYPE* C;  
	DATA_TYPE* C_outputFromGpu; 

	A = (DATA_TYPE*)malloc(NI*NK*sizeof(DATA_TYPE)); 
	B = (DATA_TYPE*)malloc(NK*NJ*sizeof(DATA_TYPE));   
	C = (DATA_TYPE*)malloc(NI*NJ*sizeof(DATA_TYPE)); 
	C_outputFromGpu = (DATA_TYPE*)malloc(NI*NJ*sizeof(DATA_TYPE)); 

	init(A, B, C);
	
    VulkanCompute vk;
	GPU_argv_init(&vk);
	
	gemmVulkan(&vk, A, B, C, C_outputFromGpu);

	t_start = rtclock();	
	gemm(A, B, C);
	t_end = rtclock();
	fprintf(stdout, "CPU Runtime: %0.6lfs\n", t_end - t_start);
	
	compareResults(C, C_outputFromGpu);

	free(A);
	free(B);  
	free(C);  
	free(C_outputFromGpu); 
    vk.freeResources();

    return EXIT_SUCCESS;
}

