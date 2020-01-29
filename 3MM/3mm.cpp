/**
 * 3mm.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
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

/* Problem size. */
# define NI 512
# define NJ 512
# define NK 512
# define NL 512
# define NM 512

/* Thread block dimensions */
#define DIM_THREAD_BLOCK_X 32
#define DIM_THREAD_BLOCK_Y 8

/* Can switch DATA_TYPE between float and double */
typedef float DATA_TYPE;

void init_array(DATA_TYPE* A, DATA_TYPE* B, DATA_TYPE* C, DATA_TYPE* D)
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
			B[i*NJ + j] = ((DATA_TYPE) i*(j+1)) / NJ;
		}
	}
  
	for (i = 0; i < NJ; i++)
	{
		for (j = 0; j < NM; j++)
		{
			C[i*NM + j] = ((DATA_TYPE) i*(j+3)) / NL;
		}
	}
  
	for (i = 0; i < NM; i++)
	{
		for (j = 0; j < NL; j++)
		{
			D[i*NL + j] = ((DATA_TYPE) i*(j+2)) / NK;
		}
	}
}


void compareResults(DATA_TYPE *G, DATA_TYPE *G_outputFromGpu)
{
	int i,j,fail;
	fail = 0;
    int count = 0;

	for (i=0; i < NI; i++)
	{
		for (j=0; j < NL; j++)
		{
            if(count%250==0) std::cout << "CCHECK [" << count << "] " << G[i*NL + j] << "/" << G_outputFromGpu[i*NL + j] << std::endl;
            count++;
			if (percentDiff(G[i*NL + j], G_outputFromGpu[i*NL + j]) > PERCENT_DIFF_ERROR_THRESHOLD)
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

	
void mm3_cpu(DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *C, DATA_TYPE *D, DATA_TYPE *E, DATA_TYPE *F, DATA_TYPE *G)
{
	int i,j,k;
	
	/* E := A*B */
	for (i = 0; i < NI; i++)
	{
		for (j = 0; j < NJ; j++)
		{
			E[i*NJ + j] = 0;
			for (k = 0; k < NK; ++k)
			{
				E[i*NJ + j] += A[i*NK + k] * B[k*NJ + j];
			}
		}
	}
		
	/* F := C*D */
	for (i = 0; i < NJ; i++)
	{
		for (j = 0; j < NL; j++)
		{
			F[i*NL + j] = 0;
			for (k = 0; k < NM; ++k)
			{
				F[i*NL + j] += C[i*NM + k] * D[k*NL + j];
			}
		}
	}

  	/* G := E*F */
	for (i = 0; i < NI; i++)
	{
		for (j = 0; j < NL; j++)
		{
			G[i*NL + j] = 0;
			for (k = 0; k < NJ; ++k)
			{
				G[i*NL + j] += E[i*NJ + k] * F[k*NL + j];
			}
		}
	}
}


void mm3Vulkan(VulkanCompute *vk, DATA_TYPE* A, DATA_TYPE* B, DATA_TYPE* C, DATA_TYPE* D, DATA_TYPE* E, DATA_TYPE* F, 
		DATA_TYPE* G, DATA_TYPE* G_outputFromGpu)
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"mm3kernel1.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "mm3kernel1") == -1)
	{
		std::cout << "Error in compiling shader mm3kernel1.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"mm3kernel2.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "mm3kernel2") == -1)
	{
		std::cout << "Error in compiling shader mm3kernel2.comp" << std::endl;
		exit(-1);
	}

    	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"mm3kernel3.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "mm3kernel3") == -1)
	{
		std::cout << "Error in compiling shader mm3kernel3.comp" << std::endl;
		exit(-1);
	}


	DATA_TYPE *A_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NI * NK, BufferUsage::BUF_INOUT);
	DATA_TYPE *B_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NK * NJ, BufferUsage::BUF_INOUT);
	DATA_TYPE *C_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NJ * NM, BufferUsage::BUF_INOUT);
	DATA_TYPE *D_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NM * NL, BufferUsage::BUF_INOUT);
	DATA_TYPE *E_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NI * NJ, BufferUsage::BUF_INOUT);
	DATA_TYPE *F_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NJ * NL, BufferUsage::BUF_INOUT);
	DATA_TYPE *G_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NI * NL, BufferUsage::BUF_INOUT);

    memcpy(A_gpu,A,sizeof(DATA_TYPE) * NI * NK);
    memcpy(B_gpu,B,sizeof(DATA_TYPE) * NK * NJ);
    memcpy(C_gpu,C,sizeof(DATA_TYPE) * NJ * NM);
    memcpy(D_gpu,D,sizeof(DATA_TYPE) * NM * NL);
    memcpy(E_gpu,E,sizeof(DATA_TYPE) * NI * NJ);
    memcpy(F_gpu,F,sizeof(DATA_TYPE) * NJ * NL);
    memcpy(G_gpu,G,sizeof(DATA_TYPE) * NI * NL);

     vk->startCreateCommandList();
		vk->synchBuffer(PPTR(A_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(B_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(C_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(D_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(E_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(F_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(G_gpu),HOST_TO_DEVICE);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	/*cudaMemcpy(A_gpu, A, sizeof(DATA_TYPE) * NI * NK, cudaMemcpyHostToDevice);
	cudaMemcpy(B_gpu, B, sizeof(DATA_TYPE) * NK * NJ, cudaMemcpyHostToDevice);
	cudaMemcpy(C_gpu, C, sizeof(DATA_TYPE) * NJ * NM, cudaMemcpyHostToDevice);
	cudaMemcpy(D_gpu, D, sizeof(DATA_TYPE) * NM * NL, cudaMemcpyHostToDevice);
	cudaMemcpy(E_gpu, E, sizeof(DATA_TYPE) * NI * NJ, cudaMemcpyHostToDevice);
	cudaMemcpy(F_gpu, F, sizeof(DATA_TYPE) * NJ * NL, cudaMemcpyHostToDevice);
	cudaMemcpy(G_gpu, G, sizeof(DATA_TYPE) * NI * NL, cudaMemcpyHostToDevice);
	
	dim3 block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	dim3 grid1((size_t)(ceil( ((float)NJ) / ((float)DIM_THREAD_BLOCK_X) )),(size_t)(ceil((float)NI/ ((float)DIM_THREAD_BLOCK_Y) )));
	dim3 grid2((size_t)(ceil( ((float)NL) / ((float)DIM_THREAD_BLOCK_X) )),(size_t)(ceil((float)NJ/ ((float)DIM_THREAD_BLOCK_Y) )));
	dim3 grid3((size_t)(ceil( ((float)NL) / ((float)DIM_THREAD_BLOCK_X) )),(size_t)(ceil((float)NI/ ((float)DIM_THREAD_BLOCK_Y) )));*/

    ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	ComputeWorkDistribution_t grid1((size_t)(ceil( ((float)NJ) / ((float)DIM_THREAD_BLOCK_X) )),(size_t)(ceil((float)NI/ ((float)DIM_THREAD_BLOCK_Y) )));
    ComputeWorkDistribution_t grid2((size_t)(ceil( ((float)NL) / ((float)DIM_THREAD_BLOCK_X) )),(size_t)(ceil((float)NJ/ ((float)DIM_THREAD_BLOCK_Y) )));
    ComputeWorkDistribution_t grid3((size_t)(ceil( ((float)NL) / ((float)DIM_THREAD_BLOCK_X) )),(size_t)(ceil((float)NI/ ((float)DIM_THREAD_BLOCK_Y) )));

    vk->startCreatePipeline("mm3kernel1");
		vk->setArg(PPTR(A_gpu),"mm3kernel1",4);
		vk->setArg(PPTR(B_gpu),"mm3kernel1",5);
        vk->setArg(PPTR(E_gpu),"mm3kernel1",6);
		vk->setLaunchConfiguration(grid1,block);
	PIPELINE_HANDLE hPipeline1 = vk->finalizePipeline();

    vk->startCreatePipeline("mm3kernel2");
		vk->setArg(PPTR(C_gpu),"mm3kernel2",4);
		vk->setArg(PPTR(D_gpu),"mm3kernel2",5);
        vk->setArg(PPTR(F_gpu),"mm3kernel2",6);
		vk->setLaunchConfiguration(grid2,block);
	PIPELINE_HANDLE hPipeline2 = vk->finalizePipeline();

    vk->startCreatePipeline("mm3kernel3");
		vk->setArg(PPTR(E_gpu),"mm3kernel3",4);
		vk->setArg(PPTR(F_gpu),"mm3kernel3",5);
        vk->setArg(PPTR(G_gpu),"mm3kernel3",6);
		vk->setLaunchConfiguration(grid3,block);
	PIPELINE_HANDLE hPipeline3 = vk->finalizePipeline();

    vk->startCreateCommandList();
		vk->selectPipeline(hPipeline1);
		vk->launchComputation("mm3kernel1");
        vk->selectPipeline(hPipeline2);
        vk->launchComputation("mm3kernel2");
        vk->selectPipeline(hPipeline3);
        vk->launchComputation("mm3kernel3");
	vk->finalizeCommandList();

	vk->deviceSynch();

    t_start = rtclock();
	vk->submitWork();
	vk->deviceSynch();
	t_end = rtclock();
	fprintf(stdout, "GPU Runtime: %0.6lfs\n", t_end - t_start);
    
     vk->startCreateCommandList();
		vk->synchBuffer(PPTR(G_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

    memcpy(G_outputFromGpu,G_gpu,sizeof(DATA_TYPE) * NI * NL);
}


int main(int argc, char** argv)
{
	double t_start, t_end;

	DATA_TYPE* A;
	DATA_TYPE* B;
	DATA_TYPE* C;
	DATA_TYPE* D;
	DATA_TYPE* E;
	DATA_TYPE* F;
	DATA_TYPE* G;
	DATA_TYPE* G_outputFromGpu;

	A = (DATA_TYPE*)malloc(NI*NK*sizeof(DATA_TYPE));
	B = (DATA_TYPE*)malloc(NK*NJ*sizeof(DATA_TYPE));
	C = (DATA_TYPE*)malloc(NJ*NM*sizeof(DATA_TYPE));
	D = (DATA_TYPE*)malloc(NM*NL*sizeof(DATA_TYPE));
	E = (DATA_TYPE*)malloc(NI*NJ*sizeof(DATA_TYPE));
	F = (DATA_TYPE*)malloc(NJ*NL*sizeof(DATA_TYPE));
	G = (DATA_TYPE*)malloc(NI*NL*sizeof(DATA_TYPE));
	G_outputFromGpu = (DATA_TYPE*)malloc(NI*NL*sizeof(DATA_TYPE));

	init_array(A, B, C, D);

    VulkanCompute vk;
	GPU_argv_init(&vk);

	mm3Vulkan(&vk, A, B, C, D, E, F, G, G_outputFromGpu);

	t_start = rtclock();

	mm3_cpu(A, B, C, D, E, F, G);
	
	t_end = rtclock();

	fprintf(stdout, "CPU Runtime: %0.6lfs\n", t_end - t_start);

	compareResults(G, G_outputFromGpu);

	free(A);
	free(B);
	free(C);
	free(D);
	free(E);
	free(F);
	free(G);
	free(G_outputFromGpu);
    vk.freeResources();

	return EXIT_SUCCESS;
}

