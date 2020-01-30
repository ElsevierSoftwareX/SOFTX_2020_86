/**
 * syrk.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
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
#define N 1024
#define M 1024

/* Thread block dimensions */
#define DIM_THREAD_BLOCK_X 32
#define DIM_THREAD_BLOCK_Y 8

/* Declared constant values for alpha and beta (same as values in PolyBench 2.0) */
#define alpha 12435
#define beta 4546

/* Can switch DATA_TYPE between float and double */
typedef float DATA_TYPE;

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

            if(count%250==0) 
                std::cout << "CCHECK [" << count << "] " << C[i*M + j] << "/" << C_outputFromGpu[i*M + j] << std::endl;
            count++;

			if (percentDiff(C[i*M + j], C_outputFromGpu[i*M + j]) > PERCENT_DIFF_ERROR_THRESHOLD)
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


/*
Original CUDA implementation. You can see the stupidity here in having unused arguments in the kernel. 
ALPHA and BETA are read from macro definitions and not as pushed from kernel submission. 
Such arguments would translate into two additional push constants in vulkan, all for the sake of nothing.
I am not doing it. The GLSL kernel only has buffers as kernel arguments.
__global__ void syrk_kernel(DATA_TYPE ALPHA, DATA_TYPE BETA, DATA_TYPE *a, DATA_TYPE *c)
{
	//  C := alpha*A*A' + beta*C 
	int j = blockIdx.x * blockDim.x + threadIdx.x;
	int i = blockIdx.y * blockDim.y + threadIdx.y;

	if ((i < N) && (j < N))
	{
		c[i * N + j] *= beta;
		int k;		
		for(k=0; k< M; k++)
		{
			c[i * N + j] += alpha * a[i * M + k] * a[j * M + k];
		}
	}
}
*/

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

    /*Same possible bug as in SYR2K. From original version, different allocation size for host/device
    with respect to the output buffer (C, C_gpu)*/
	DATA_TYPE* A_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N * M, BufferUsage::BUF_INOUT);
	DATA_TYPE* C_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * N * N, BufferUsage::BUF_INOUT);

	memcpy(A_gpu, A, sizeof(DATA_TYPE) * N * M);
	memcpy(C_gpu, C, sizeof(DATA_TYPE) * N * N);

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(A_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(C_gpu),HOST_TO_DEVICE);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	/*dim3 block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	dim3 grid((size_t)(ceil(((float)N) / ((float)DIM_THREAD_BLOCK_X))), (size_t)ceil(((float)N) / ((float)DIM_THREAD_BLOCK_Y)));*/
    ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
    ComputeWorkDistribution_t grid((size_t)(ceil(((float)N) / ((float)DIM_THREAD_BLOCK_X))), (size_t)ceil(((float)N) / ((float)DIM_THREAD_BLOCK_Y)));

    vk->startCreatePipeline("syrkkernel");
		vk->setArg(PPTR(A_gpu),"syrkkernel",4);
		vk->setArg(PPTR(C_gpu),"syrkkernel",5);
		vk->setLaunchConfiguration(grid,block);
	PIPELINE_HANDLE hPipeline = vk->finalizePipeline();

    vk->startCreateCommandList();
            vk->selectPipeline(hPipeline);
            vk->launchComputation("syrkkernel");
    vk->finalizeCommandList();
	vk->deviceSynch();

	t_start = rtclock();
    vk->submitWork();
    vk->deviceSynch();
	/*syrk_kernel<<<grid,block>>>(alpha, beta, A_gpu,C_gpu);
	cudaThreadSynchronize();*/
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
	DATA_TYPE* C;
	DATA_TYPE* C_outputFromGpu;

	A = (DATA_TYPE*)malloc(N*M*sizeof(DATA_TYPE));
	C = (DATA_TYPE*)malloc(N*M*sizeof(DATA_TYPE));
	C_outputFromGpu = (DATA_TYPE*)malloc(N*M*sizeof(DATA_TYPE));

	init_arrays(A, C);

    	VulkanCompute vk;
	GPU_argv_init(&vk);

	syrkVulkan(&vk, A, C, C_outputFromGpu);

	t_start = rtclock();
	syrk(A, C);
	t_end = rtclock();
	fprintf(stdout, "CPU Runtime: %0.6lfs\n", t_end - t_start);

	compareResults(C, C_outputFromGpu);

	free(A);
	free(C);
	free(C_outputFromGpu);
    	vk.freeResources();

	return EXIT_SUCCESS;
}

