/**
 * syr2k.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
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

//define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

/* Problem size */
#define N 2048
#define M 2048

/* Thread block dimensions */
#define DIM_THREAD_BLOCK_X 32
#define DIM_THREAD_BLOCK_Y 8

/* Declared constant values for ALPHA and BETA (same as values in PolyBench 2.0) */
#define ALPHA 12435
#define BETA 4546

/* Can switch DATA_TYPE between float and double */
typedef float DATA_TYPE;

void init_arrays(DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *C)
{
	int i, j;
  
	for (i = 0; i < N; i++)
    	{
    		for (j = 0; j < N; j++)
		{
			C[i*N + j] = ((DATA_TYPE) i*j + 2) / N;
		}
      	
		for (j = 0; j < M; j++)
		{
	  		A[i*N + j] = ((DATA_TYPE) i*j) / N;
	  		B[i*N + j] = ((DATA_TYPE) i*j + 1) / N;
		}
    	}
}


void syr2k(DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *C)
{
	int i, j, k;
		
  	for (i = 0; i < N; i++)
	{
   		for (j = 0; j < N; j++)
		{
     			C[i*N + j] *= BETA;
		}
	}

  	for (i = 0; i < N; i++)
	{
   		for (j = 0; j < N; j++)
		{
      			for (k = 0; k < M; k++)
			{
	  			C[i*N + j] += ALPHA * A[i*M + k] * B[j*M + k];
	 		 	C[i*N + j] += ALPHA * B[i*M + k] * A[j*M + k];
			}
		}
	}
}


void compareResults(DATA_TYPE *C, DATA_TYPE *C_outputFromGpu)
{
	int i,j,fail;
	fail = 0;
    int count = 0;

	// Compare C with D
	for (i=0; i<N; i++)
	{
		for (j=0; j<N; j++)
		{

            if(count%250==0) 
                std::cout << "CCHECK [" << count << "] " << C[i*N + j] << "/" << C_outputFromGpu[i*N + j] << std::endl;
            count++;

			if (percentDiff(C[i*N + j], C_outputFromGpu[i*N + j]) > PERCENT_DIFF_ERROR_THRESHOLD)
			{ 
				fail++;
			}
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

void syr2kVulkan(VulkanCompute *vk, DATA_TYPE* A, DATA_TYPE* B, DATA_TYPE* C, DATA_TYPE* C_outputFromGpu) 
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"syr2kkernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "syr2kkernel") == -1)
	{
		std::cout << "Error in compiling shader syr2kkernel.comp" << std::endl;
		exit(-1);
	}

    //From the original CUDA polybench, device-alloc of C is N*N, in main is N*M. Possible bug?
	DATA_TYPE *A_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N * M, BufferUsage::BUF_INOUT);
	DATA_TYPE *B_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N * M, BufferUsage::BUF_INOUT);
	DATA_TYPE *C_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N * N, BufferUsage::BUF_INOUT);

	memcpy(A_gpu, A, sizeof(DATA_TYPE) * N * M);
	memcpy(B_gpu, B, sizeof(DATA_TYPE) * N * M);
	memcpy(C_gpu, C, sizeof(DATA_TYPE) * N * N);

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(A_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(B_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(C_gpu),HOST_TO_DEVICE);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();
	
	/*dim3 block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	dim3 grid((size_t)ceil( ((float)N) / ((float)DIM_THREAD_BLOCK_X) ), (size_t)(ceil( ((float)N) / ((float)DIM_THREAD_BLOCK_Y) )));*/
    ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
    ComputeWorkDistribution_t grid((size_t)ceil( ((float)N) / ((float)DIM_THREAD_BLOCK_X) ), (size_t)(ceil( ((float)N) / ((float)DIM_THREAD_BLOCK_Y) )));

    vk->startCreatePipeline("syr2kkernel");
		vk->setArg(PPTR(A_gpu),"syr2kkernel",4);
		vk->setArg(PPTR(B_gpu),"syr2kkernel",5);
        vk->setArg(PPTR(C_gpu),"syr2kkernel",6);
		vk->setLaunchConfiguration(grid,block);
	PIPELINE_HANDLE hPipeline = vk->finalizePipeline();

    vk->startCreateCommandList();
            vk->selectPipeline(hPipeline);
            vk->launchComputation("syr2kkernel");
    vk->finalizeCommandList();
	vk->deviceSynch();

	t_start = rtclock();
	/*syr2k_kernel<<<grid,block>>>(A_gpu,B_gpu,C_gpu);
	cudaThreadSynchronize();*/
    vk->submitWork();
    vk->deviceSynch();
	t_end = rtclock();
	fprintf(stdout, "GPU Runtime: %0.6lfs\n", t_end - t_start);

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(C_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();
	
	memcpy(C_outputFromGpu, C_gpu, sizeof(DATA_TYPE) * N * N);
}


int main()
{
	double t_start, t_end;

	DATA_TYPE* A;
	DATA_TYPE* B;
	DATA_TYPE* C;
	DATA_TYPE* C_outputFromGpu;

	A = (DATA_TYPE*)malloc(N*M*sizeof(DATA_TYPE));
	B = (DATA_TYPE*)malloc(N*M*sizeof(DATA_TYPE));
	C = (DATA_TYPE*)malloc(N*M*sizeof(DATA_TYPE));
	C_outputFromGpu = (DATA_TYPE*)malloc(N*M*sizeof(DATA_TYPE));

	init_arrays(A, B, C);
    
    VulkanCompute vk;
	GPU_argv_init(&vk);

	syr2kVulkan(&vk, A, B, C, C_outputFromGpu);
	
	t_start = rtclock();
	syr2k(A, B, C);
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

