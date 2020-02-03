/**
 * correlation.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
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

//define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 1.05

/* Thread block dimensions for kernel 1*/
#define DIM_THREAD_BLOCK_KERNEL_1_X 256
#define DIM_THREAD_BLOCK_KERNEL_1_Y 1

/* Thread block dimensions for kernel 2*/
#define DIM_THREAD_BLOCK_KERNEL_2_X 256
#define DIM_THREAD_BLOCK_KERNEL_2_Y 1

/* Thread block dimensions for kernel 3*/
#define DIM_THREAD_BLOCK_KERNEL_3_X 32
#define DIM_THREAD_BLOCK_KERNEL_3_Y 8

/* Thread block dimensions for kernel 4*/
#define DIM_THREAD_BLOCK_KERNEL_4_X 256
#define DIM_THREAD_BLOCK_KERNEL_4_Y 1

#define sqrt_of_array_cell(x,j) sqrt(x[j])

#define VERBOSE_COMPARE_NUM -1

void init_arrays(DATA_TYPE* data)
{
	int i, j;
	
	for (i=0; i < (M+1); i++) 
	{
    		for (j=0; j< (N+1); j++) 
		{
       			data[i*(N+1) + j] = ((DATA_TYPE) i*j)/ (M+1);	
       		}
    	}
}


void correlation(DATA_TYPE* data, DATA_TYPE* mean, DATA_TYPE* stddev, DATA_TYPE* symmat)
{
	int i, j, j1, j2;	
	
	// Determine mean of column vectors of input data matrix 
  	for (j = 1; j < (M+1); j++)
   	{
  		mean[j] = 0.0;

   		for (i = 1; i < (N+1); i++)
		{
			mean[j] += data[i*(M+1) + j];
   		}
		
		mean[j] /= (DATA_TYPE)FLOAT_N;
   	}

	// Determine standard deviations of column vectors of data matrix. 
  	for (j = 1; j < (M+1); j++)
   	{
   		stddev[j] = 0.0;
      
		for (i = 1; i < (N+1); i++)
		{
			stddev[j] += (data[i*(M+1) + j] - mean[j]) * (data[i*(M+1) + j] - mean[j]);
		}
		
		stddev[j] /= FLOAT_N;
		stddev[j] = sqrt_of_array_cell(stddev, j);
		stddev[j] = stddev[j] <= EPS ? 1.0 : stddev[j];
	}

 	// Center and reduce the column vectors. 
  	for (i = 1; i < (N+1); i++)
	{
		for (j = 1; j < (M+1); j++)
		{
			data[i*(M+1) + j] -= mean[j];
			data[i*(M+1) + j] /= (sqrt(FLOAT_N)*stddev[j]) ;
		}
	}

	// Calculate the m * m correlation matrix. 
  	for (j1 = 1; j1 < M; j1++)
	{	
		symmat[j1*(M+1) + j1] = 1.0;
    
		for (j2 = j1+1; j2 < (M+1); j2++)
		{
	  		symmat[j1*(M+1) + j2] = 0.0;

	  		for (i = 1; i < (N+1); i++)
			{
	   			symmat[j1*(M+1) + j2] += (data[i*(M+1) + j1] * data[i*(M+1) + j2]);
			}

	  		symmat[j2*(M+1) + j1] = symmat[j1*(M+1) + j2];
		}
	}
 
	symmat[M*(M+1) + M] = 1.0;
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
				std::cout << "CCHECK [" << count << "] " <<  symmat[i*(N+1) + j] << "/" << symmat_outputFromGpu[i*(N+1) + j] << std::endl;
           
		    count++;
			if (percentDiff(symmat[i*(N+1) + j], symmat_outputFromGpu[i*(N+1) + j]) > PERCENT_DIFF_ERROR_THRESHOLD)
			{
				fail++;
				printf("i: %d j: %d\n1: %f 2: %f\n", i, j, symmat[i*N + j], symmat_outputFromGpu[i*N + j]);
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

void correlationVulkan(VulkanCompute *vk, DATA_TYPE* data, DATA_TYPE* mean, DATA_TYPE* stddev, DATA_TYPE* symmat,
			DATA_TYPE* symmat_outputFromGpu)
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

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"stdkernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "stdkernel") == -1)
	{
		std::cout << "Error in compiling shader stdkernel.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"reducekernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "reducekernel") == -1)
	{
		std::cout << "Error in compiling shader reducekernel.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"corrkernel.comp");

    if (vk->loadAndCompileShader(crossFileAdapter, "corrkernel") == -1)
	{
		std::cout << "Error in compiling shader corrkernel.comp" << std::endl;
		exit(-1);
	}

	DATA_TYPE *data_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * (M+1) * (N+1), BufferUsage::BUF_INOUT);
    DATA_TYPE *symmat_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * (M+1) * (N+1), BufferUsage::BUF_INOUT);
	DATA_TYPE *stddev_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * (M+1), BufferUsage::BUF_INOUT);
	DATA_TYPE *mean_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * (M+1), BufferUsage::BUF_INOUT);
	
	memcpy(data_gpu, data, sizeof(DATA_TYPE) * (M+1) * (N+1));
	memcpy(symmat_gpu, symmat, sizeof(DATA_TYPE) * (M+1) * (N+1));
	memcpy(stddev_gpu, stddev, sizeof(DATA_TYPE) * (M+1));
	memcpy(mean_gpu, mean, sizeof(DATA_TYPE) * (M+1));

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(data_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(symmat_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(stddev_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(mean_gpu),HOST_TO_DEVICE);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

    /*  
	dim3 block1(DIM_THREAD_BLOCK_KERNEL_1_X, DIM_THREAD_BLOCK_KERNEL_1_Y);
	dim3 grid1((size_t)(ceil((float)(M)) / ((float)DIM_THREAD_BLOCK_KERNEL_1_X)), 1);*/
    ComputeWorkDistribution_t block1(DIM_THREAD_BLOCK_KERNEL_1_X, DIM_THREAD_BLOCK_KERNEL_1_Y);
	ComputeWorkDistribution_t grid1((size_t)(ceil((float)(M)) / ((float)DIM_THREAD_BLOCK_KERNEL_1_X)), 1);
  
	/*dim3 block2(DIM_THREAD_BLOCK_KERNEL_2_X, DIM_THREAD_BLOCK_KERNEL_2_Y);
	dim3 grid2((size_t)(ceil((float)(M)) / ((float)DIM_THREAD_BLOCK_KERNEL_2_X)), 1);*/
    ComputeWorkDistribution_t block2(DIM_THREAD_BLOCK_KERNEL_2_X, DIM_THREAD_BLOCK_KERNEL_2_Y);
	ComputeWorkDistribution_t grid2((size_t)(ceil((float)(M)) / ((float)DIM_THREAD_BLOCK_KERNEL_2_X)), 1);
	
	/*dim3 block3(DIM_THREAD_BLOCK_KERNEL_3_X, DIM_THREAD_BLOCK_KERNEL_3_Y);
	dim3 grid3((size_t)(ceil((float)(M)) / ((float)DIM_THREAD_BLOCK_KERNEL_3_X)), (size_t)(ceil((float)(N)) / ((float)DIM_THREAD_BLOCK_KERNEL_3_Y)));*/
    ComputeWorkDistribution_t block3(DIM_THREAD_BLOCK_KERNEL_3_X, DIM_THREAD_BLOCK_KERNEL_3_Y);
	ComputeWorkDistribution_t grid3((size_t)(ceil((float)(M)) / ((float)DIM_THREAD_BLOCK_KERNEL_3_X)), (size_t)(ceil((float)(N)) / ((float)DIM_THREAD_BLOCK_KERNEL_3_Y)));

	/*dim3 block4(DIM_THREAD_BLOCK_KERNEL_4_X, DIM_THREAD_BLOCK_KERNEL_4_Y);
	dim3 grid4((size_t)(ceil((float)(M)) / ((float)DIM_THREAD_BLOCK_KERNEL_4_X)), 1);*/
    ComputeWorkDistribution_t block4(DIM_THREAD_BLOCK_KERNEL_4_X, DIM_THREAD_BLOCK_KERNEL_4_Y);
	ComputeWorkDistribution_t grid4((size_t)(ceil((float)(M)) / ((float)DIM_THREAD_BLOCK_KERNEL_4_X)), 1);

    vk->startCreatePipeline("meankernel");
		vk->setArg(PPTR(mean_gpu),"meankernel",4);
		vk->setArg(PPTR(data_gpu),"meankernel",5);
		vk->setLaunchConfiguration(grid1,block1);
	PIPELINE_HANDLE hPipeline1 = vk->finalizePipeline();

    vk->startCreatePipeline("stdkernel");
		vk->setArg(PPTR(mean_gpu),"stdkernel",4);
		vk->setArg(PPTR(stddev_gpu),"stdkernel",5);
        vk->setArg(PPTR(data_gpu),"stdkernel",6);
		vk->setLaunchConfiguration(grid2,block2);
	PIPELINE_HANDLE hPipeline2 = vk->finalizePipeline();

    vk->startCreatePipeline("reducekernel");
		vk->setArg(PPTR(mean_gpu),"reducekernel",4);
		vk->setArg(PPTR(stddev_gpu),"reducekernel",5);
        vk->setArg(PPTR(data_gpu),"reducekernel",6);
		vk->setLaunchConfiguration(grid3,block3);
	PIPELINE_HANDLE hPipeline3 = vk->finalizePipeline();

	vk->startCreatePipeline("corrkernel");
		vk->setArg(PPTR(symmat_gpu),"corrkernel",4);
		vk->setArg(PPTR(data_gpu),"corrkernel",5);
		vk->setLaunchConfiguration(grid4,block4);
	PIPELINE_HANDLE hPipeline4 = vk->finalizePipeline();

    vk->startCreateCommandList();
		vk->selectPipeline(hPipeline1);
		vk->launchComputation("meankernel");
        vk->selectPipeline(hPipeline2);
        vk->launchComputation("stdkernel");
        vk->selectPipeline(hPipeline3);
        vk->launchComputation("reducekernel");
        vk->selectPipeline(hPipeline4);
        vk->launchComputation("corrkernel");
	vk->finalizeCommandList();

    vk->deviceSynch();

	t_start = rtclock();
    vk->submitWork();
	vk->deviceSynch();
	/*mean_kernel<<< grid1, block1 >>>(mean_gpu,data_gpu);
	cudaThreadSynchronize();
	std_kernel<<< grid2, block2 >>>(mean_gpu,stddev_gpu,data_gpu);
	cudaThreadSynchronize();
	reduce_kernel<<< grid3, block3 >>>(mean_gpu,stddev_gpu,data_gpu);
	cudaThreadSynchronize();
	corr_kernel<<< grid4, block4 >>>(symmat_gpu,data_gpu);
	cudaThreadSynchronize();*/
	t_end = rtclock();
	fprintf(stdout, "GPU Runtime: %0.6lfs\n", t_end - t_start);	

	/*DATA_TYPE valueAtSymmatIndexMTimesMPlus1PlusMPoint = 1.0;
	cudaMemcpy(&(symmat_gpu[(M)*(M+1) + (M)]), &valueAtSymmatIndexMTimesMPlus1PlusMPoint, sizeof(DATA_TYPE), cudaMemcpyHostToDevice);
    	WTF? Looks like he's changing a single value of the output buffer using a H2D memcpy. Ridiculous.
	I am using my brain properly and I'll put this instruction into the last kernel (to be executed by thread with global id 0).
	*/

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(symmat_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	//cudaMemcpy(symmat_outputFromGpu, symmat_gpu, sizeof(DATA_TYPE) * (M+1) * (N+1), cudaMemcpyDeviceToHost);
    memcpy(symmat_outputFromGpu, symmat_gpu, sizeof(DATA_TYPE) * (M+1) * (N+1));

}


int main(int argc, char *argv[])
{
	double t_start, t_end;

	DATA_TYPE* data;
	DATA_TYPE* mean;
	DATA_TYPE* stddev;
	DATA_TYPE* symmat;
	DATA_TYPE* symmat_outputFromGpu;

	data = (DATA_TYPE*)malloc((M+1)*(N+1)*sizeof(DATA_TYPE));
	mean = (DATA_TYPE*)malloc((M+1)*sizeof(DATA_TYPE));
	stddev = (DATA_TYPE*)malloc((M+1)*sizeof(DATA_TYPE));
	symmat = (DATA_TYPE*)malloc((M+1)*(N+1)*sizeof(DATA_TYPE));
	symmat_outputFromGpu = (DATA_TYPE*)malloc((M+1)*(N+1)*sizeof(DATA_TYPE));

	init_arrays(data);
    
    VulkanCompute vk("",parseDeviceSelectionFromArgs(argc,argv),GIGA);
	GPU_argv_init(&vk);

	correlationVulkan(&vk, data, mean, stddev, symmat, symmat_outputFromGpu);

	t_start = rtclock();
	correlation(data, mean, stddev, symmat);
	t_end = rtclock();

	fprintf(stdout, "CPU Runtime: %0.6lfs\n", t_end - t_start);
    
	compareResults(symmat, symmat_outputFromGpu);

	free(data);
	free(mean);
	free(stddev);
	free(symmat);
	free(symmat_outputFromGpu);
    vk.freeResources();

  	return EXIT_SUCCESS;
}

