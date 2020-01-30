/**
 * gramschmidt.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
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
#define M 2048
#define N 2048

/* Thread block dimensions */
#define DIM_THREAD_BLOCK_X 256
#define DIM_THREAD_BLOCK_Y 1

/* Can switch DATA_TYPE between float and double */
typedef float DATA_TYPE;

#define VERBOSE_COMPARE_NUM -1

void gramschmidt(DATA_TYPE* A, DATA_TYPE* R, DATA_TYPE* Q)
{
	int i,j,k;
	DATA_TYPE nrm;
	for (k = 0; k < N; k++)
	{
		nrm = 0;
		for (i = 0; i < M; i++)
		{
			nrm += A[i*N + k] * A[i*N + k];
		}
		
		R[k*N + k] = sqrt(nrm);
		for (i = 0; i < M; i++)
		{
			Q[i*N + k] = A[i*N + k] / R[k*N + k];
		}
		
		for (j = k + 1; j < N; j++)
		{
			R[k*N + j] = 0;
			for (i = 0; i < M; i++)
			{
				R[k*N + j] += Q[i*N + k] * A[i*N + j];
			}
			for (i = 0; i < M; i++)
			{
				A[i*N + j] = A[i*N + j] - Q[i*N + k] * R[k*N + j];
			}
		}
	}
}


void init_array(DATA_TYPE* A)
{
	int i, j;

	for (i = 0; i < M; i++)
	{
		for (j = 0; j < N; j++)
		{
			A[i*N + j] = ((DATA_TYPE) (i+1)*(j+1)) / (M+1);
		}
	}
}


void compareResults(DATA_TYPE* A, DATA_TYPE* A_outputFromGpu)
{
	int i, j, fail;
	fail = 0;
    int count = 0;

	for (i=0; i < M; i++) 
	{
		for (j=0; j < N; j++) 
		{

            if((VERBOSE_COMPARE_NUM>=0) && (count%VERBOSE_COMPARE_NUM==0))
				std::cout << "CCHECK [" << count << "] " << A[i*N + j] << "/" << A_outputFromGpu[i*N + j] << std::endl;
           
		    count++;
			if (percentDiff(A[i*N + j], A_outputFromGpu[i*N + j]) > PERCENT_DIFF_ERROR_THRESHOLD) 
			{				
				fail++;
				printf("i: %d j: %d \n1: %f\n 2: %f\n", i, j, A[i*N + j], A_outputFromGpu[i*N + j]);
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

void gramschmidtVulkan(VulkanCompute *vk, DATA_TYPE* A, DATA_TYPE* R, DATA_TYPE* Q, DATA_TYPE* A_outputFromGpu)
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"gskernel1.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "gskernel1") == -1)
	{
		std::cout << "Error in compiling shader gskernel1.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"gskernel2.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "gskernel2") == -1)
	{
		std::cout << "Error in compiling shader gskernel2.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"gskernel3.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "gskernel3") == -1)
	{
		std::cout << "Error in compiling shader gskernel3.comp" << std::endl;
		exit(-1);
	}

	DATA_TYPE *A_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * M * N, BufferUsage::BUF_INOUT);
	DATA_TYPE *R_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * M * N, BufferUsage::BUF_INOUT);
	DATA_TYPE *Q_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * M * N, BufferUsage::BUF_INOUT);

    memcpy(A_gpu,A,sizeof(DATA_TYPE) * M * N);

     vk->startCreateCommandList();
		vk->synchBuffer(PPTR(A_gpu),HOST_TO_DEVICE);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();
	

    /*dim3 block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);*/
    ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	/*dim3 gridKernel1(1, 1);*/
    ComputeWorkDistribution_t grid1(1,1);
	/*dim3 gridKernel2((size_t)ceil(((float)N) / ((float)DIM_THREAD_BLOCK_X)), 1);*/
    ComputeWorkDistribution_t grid2((size_t)ceil(((float)N) / ((float)DIM_THREAD_BLOCK_X)), 1);
	/*dim3 gridKernel3((size_t)ceil(((float)N) / ((float)DIM_THREAD_BLOCK_X)), 1);*/
    ComputeWorkDistribution_t grid3((size_t)ceil(((float)N) / ((float)DIM_THREAD_BLOCK_X)), 1);

    vk->startCreatePipeline("gskernel1");
		vk->setArg(PPTR(A_gpu),"gskernel1",4);
		vk->setArg(PPTR(R_gpu),"gskernel1",5);
        vk->setArg(PPTR(Q_gpu),"gskernel1",6);
        vk->setSymbol(0, sizeof(int));
		vk->setLaunchConfiguration(grid1,block);
	PIPELINE_HANDLE hPipeline1 = vk->finalizePipeline();

    vk->startCreatePipeline("gskernel2");
		vk->setArg(PPTR(A_gpu),"gskernel2",4);
        vk->setArg(PPTR(R_gpu),"gskernel2",5);
        vk->setArg(PPTR(Q_gpu),"gskernel2",6);
        vk->setSymbol(0, sizeof(int));
		vk->setLaunchConfiguration(grid2,block);
	PIPELINE_HANDLE hPipeline2 = vk->finalizePipeline();

    vk->startCreatePipeline("gskernel3");
		vk->setArg(PPTR(A_gpu),"gskernel3",4);
        vk->setArg(PPTR(R_gpu),"gskernel3",5);
        vk->setArg(PPTR(Q_gpu),"gskernel3",6);
        vk->setSymbol(0, sizeof(int));
		vk->setLaunchConfiguration(grid3,block);
	PIPELINE_HANDLE hPipeline3 = vk->finalizePipeline();

    vk->startCreateCommandList();
        for(int k = 0; k < N; k++){
             vk->selectPipeline(hPipeline1);
             vk->copySymbolInt(k,"gskernel1",0);
             vk->launchComputation("gskernel1");
             vk->selectPipeline(hPipeline2);
             vk->copySymbolInt(k,"gskernel2",0);
             vk->launchComputation("gskernel2");
             vk->selectPipeline(hPipeline3);
             vk->copySymbolInt(k,"gskernel3",0);
             vk->launchComputation("gskernel3");
        }
    vk->finalizeCommandList();
	vk->deviceSynch();

	t_start = rtclock();
	/*int k;
	for (k = 0; k < N; k++)
	{
		gramschmidt_kernel1<<<gridKernel1,block>>>(A_gpu, R_gpu, Q_gpu, k);
		cudaThreadSynchronize();
		gramschmidt_kernel2<<<gridKernel2,block>>>(A_gpu, R_gpu, Q_gpu, k);
		cudaThreadSynchronize();
		gramschmidt_kernel3<<<gridKernel3,block>>>(A_gpu, R_gpu, Q_gpu, k);
		cudaThreadSynchronize();
	}*/
    vk->submitWork();
	vk->deviceSynch();
	t_end = rtclock();
	fprintf(stdout, "GPU Runtime: %0.6lfs\n", t_end - t_start);

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(A_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();
	
	memcpy(A_outputFromGpu, A_gpu, sizeof(DATA_TYPE) * M * N);    

}


int main(int argc, char *argv[])
{
	double t_start, t_end;

	DATA_TYPE* A;
	DATA_TYPE* A_outputFromGpu;
	DATA_TYPE* R;
	DATA_TYPE* Q;
	
	A = (DATA_TYPE*)malloc(M*N*sizeof(DATA_TYPE));
	A_outputFromGpu = (DATA_TYPE*)malloc(M*N*sizeof(DATA_TYPE));
	R = (DATA_TYPE*)malloc(M*N*sizeof(DATA_TYPE));  
	Q = (DATA_TYPE*)malloc(M*N*sizeof(DATA_TYPE));  
	
	init_array(A);
	
    VulkanCompute vk;
	GPU_argv_init(&vk);

	gramschmidtVulkan(&vk, A, R, Q, A_outputFromGpu);
	
	t_start = rtclock();
	gramschmidt(A, R, Q);
	t_end = rtclock();

	fprintf(stdout, "CPU Runtime: %0.6lfs\n", t_end - t_start);
	
	compareResults(A, A_outputFromGpu);
	
	free(A);
	free(A_outputFromGpu);
	free(R);
	free(Q);  
    vk.freeResources();

    return EXIT_SUCCESS;
}

