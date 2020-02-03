/**
 * fdtd2d.cpp: This file is part of the PolyBench/GPU 1.0 test suite,
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
#define PERCENT_DIFF_ERROR_THRESHOLD 10.05

/* Thread block dimensions */
#define DIM_THREAD_BLOCK_X 32
#define DIM_THREAD_BLOCK_Y 8

#define VERBOSE_COMPARE_NUM -1

void init_arrays(DATA_TYPE* _fict_, DATA_TYPE* ex, DATA_TYPE* ey, DATA_TYPE* hz)
{
	int i, j;

  	for (i = 0; i < tmax; i++)
	{
		_fict_[i] = (DATA_TYPE) i;
	}
	
	for (i = 0; i < NX; i++)
	{
		for (j = 0; j < NY; j++)
		{
			ex[i*NY + j] = ((DATA_TYPE) i*(j+1) + 1) / NX;
			ey[i*NY + j] = ((DATA_TYPE) (i-1)*(j+2) + 2) / NX;
			hz[i*NY + j] = ((DATA_TYPE) (i-9)*(j+4) + 3) / NX;
		}
	}
}


void runFdtd(DATA_TYPE* _fict_, DATA_TYPE* ex, DATA_TYPE* ey, DATA_TYPE* hz)
{
	int t, i, j;
	
	for (t=0; t < tmax; t++)  
	{
		for (j=0; j < NY; j++)
		{
			ey[0*NY + j] = _fict_[t];
		}
	
		for (i = 1; i < NX; i++)
		{
       		for (j = 0; j < NY; j++)
			{
       			ey[i*NY + j] = ey[i*NY + j] - 0.5*(hz[i*NY + j] - hz[(i-1)*NY + j]);
        		}
		}

		for (i = 0; i < NX; i++)
		{
       		for (j = 1; j < NY; j++)
			{
				ex[i*(NY+1) + j] = ex[i*(NY+1) + j] - 0.5*(hz[i*NY + j] - hz[i*NY + (j-1)]);
			}
		}

		for (i = 0; i < NX; i++)
		{
			for (j = 0; j < NY; j++)
			{
				hz[i*NY + j] = hz[i*NY + j] - 0.7*(ex[i*(NY+1) + (j+1)] - ex[i*(NY+1) + j] + ey[(i+1)*NY + j] - ey[i*NY + j]);
			}
		}
	}
}


void compareResults(DATA_TYPE* hz1, DATA_TYPE* hz2)
{
	int i, j, fail;
	fail = 0;
    int count = 0;
	
	for (i=0; i < NX; i++) 
	{
		for (j=0; j < NY; j++) 
		{
            if((VERBOSE_COMPARE_NUM>=0) && (count%VERBOSE_COMPARE_NUM==0))
				std::cout << "CCHECK [" << count << "] " << hz1[i*NY + j] << "/" << hz2[i*NY + j] << std::endl;
            
			count++;
			if (percentDiff(hz1[i*NY + j], hz2[i*NY + j]) > PERCENT_DIFF_ERROR_THRESHOLD) 
			{
				std::cout << "Discrepancy in " << hz1[i*NY+j] <<  "/" << hz2[i*NY+j] << std::endl;
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

void fdtdVulkan(VulkanCompute *vk, DATA_TYPE* _fict_, DATA_TYPE* ex, DATA_TYPE* ey, DATA_TYPE* hz, DATA_TYPE* hz_outputFromGpu)
{
	double t_start, t_end;

    char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	CrossFileAdapter crossFileAdapter;
	std::string cwd(buff);

	crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"fdtdstep1kernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "fdtdstep1kernel") == -1)
	{
		std::cout << "Error in compiling shader fdtdstep1kernel.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"fdtdstep2kernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "fdtdstep2kernel") == -1)
	{
		std::cout << "Error in compiling shader fdtdstep2kernel.comp" << std::endl;
		exit(-1);
	}

    crossFileAdapter.setAbsolutePath(cwd+FILE_SEPARATOR+"fdtdstep3kernel.comp");

	if (vk->loadAndCompileShader(crossFileAdapter, "fdtdstep3kernel") == -1)
	{
		std::cout << "Error in compiling shader fdtdstep3kernel.comp" << std::endl;
		exit(-1);
	}

	DATA_TYPE *_fict_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * tmax, BufferUsage::BUF_INOUT);
	DATA_TYPE *ex_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NX * (NY + 1), BufferUsage::BUF_INOUT);
	DATA_TYPE *ey_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * (NX + 1) * NY, BufferUsage::BUF_INOUT);
	DATA_TYPE *hz_gpu = (DATA_TYPE*) vk->deviceSideAllocation(sizeof(DATA_TYPE) * NX * NY, BufferUsage::BUF_INOUT);

	memcpy(_fict_gpu, _fict_, sizeof(DATA_TYPE) * tmax);
	memcpy(ex_gpu, ex, sizeof(DATA_TYPE) * NX * (NY + 1));
	memcpy(ey_gpu, ey, sizeof(DATA_TYPE) * (NX + 1) * NY);
	memcpy(hz_gpu, hz, sizeof(DATA_TYPE) * NX * NY);

    vk->startCreateCommandList();
		vk->synchBuffer(PPTR(_fict_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(ex_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(ey_gpu),HOST_TO_DEVICE);
        vk->synchBuffer(PPTR(hz_gpu),HOST_TO_DEVICE);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	/*dim3 block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	dim3 grid( (size_t)ceil(((float)NY) / ((float)block.x)), (size_t)ceil(((float)NX) / ((float)block.y)));*/

    ComputeWorkDistribution_t block(DIM_THREAD_BLOCK_X, DIM_THREAD_BLOCK_Y);
	ComputeWorkDistribution_t grid((size_t)ceil(((float)NY) / ((float)block.x)), (size_t)ceil(((float)NX) / ((float)block.y)));

    vk->startCreatePipeline("fdtdstep1kernel");
		vk->setArg(PPTR(_fict_gpu),"fdtdstep1kernel",4);
		vk->setArg(PPTR(ex_gpu),"fdtdstep1kernel",5);
        vk->setArg(PPTR(ey_gpu),"fdtdstep1kernel",6);
        vk->setArg(PPTR(hz_gpu),"fdtdstep1kernel",7);
        vk->setSymbol(0, sizeof(int));
		vk->setLaunchConfiguration(grid,block);
	PIPELINE_HANDLE hPipeline1 = vk->finalizePipeline();

    vk->startCreatePipeline("fdtdstep2kernel");
		vk->setArg(PPTR(ex_gpu),"fdtdstep2kernel",4);
        vk->setArg(PPTR(ey_gpu),"fdtdstep2kernel",5);
        vk->setArg(PPTR(hz_gpu),"fdtdstep2kernel",6);
        vk->setSymbol(0, sizeof(int));
		vk->setLaunchConfiguration(grid,block);
	PIPELINE_HANDLE hPipeline2 = vk->finalizePipeline();

    vk->startCreatePipeline("fdtdstep3kernel");
		vk->setArg(PPTR(ex_gpu),"fdtdstep3kernel",4);
        vk->setArg(PPTR(ey_gpu),"fdtdstep3kernel",5);
        vk->setArg(PPTR(hz_gpu),"fdtdstep3kernel",6);
        vk->setSymbol(0, sizeof(int));
		vk->setLaunchConfiguration(grid,block);
	PIPELINE_HANDLE hPipeline3 = vk->finalizePipeline();

    vk->startCreateCommandList();
        for(int t = 0; t< tmax; t++){
             vk->selectPipeline(hPipeline1);
             vk->copySymbolInt(t,"fdtdstep1kernel",0);
             vk->launchComputation("fdtdstep1kernel");
             vk->selectPipeline(hPipeline2);
             vk->copySymbolInt(t,"fdtdstep2kernel",0);
             vk->launchComputation("fdtdstep2kernel");
             vk->selectPipeline(hPipeline3);
             vk->copySymbolInt(t,"fdtdstep3kernel",0);
             vk->launchComputation("fdtdstep3kernel");
        }
    vk->finalizeCommandList();
	vk->deviceSynch();


	t_start = rtclock();

    vk->submitWork();
	vk->deviceSynch();

	/*for(int t = 0; t< tmax; t++)
	{
		fdtd_step1_kernel<<<grid,block>>>(_fict_gpu, ex_gpu, ey_gpu, hz_gpu, t);
		cudaThreadSynchronize();
		fdtd_step2_kernel<<<grid,block>>>(ex_gpu, ey_gpu, hz_gpu, t);
		cudaThreadSynchronize();
		fdtd_step3_kernel<<<grid,block>>>(ex_gpu, ey_gpu, hz_gpu, t);
		cudaThreadSynchronize();
	}*/
	
	t_end = rtclock();
    fprintf(stdout, "GPU Runtime: %0.6lfs\n", t_end - t_start);

     vk->startCreateCommandList();
		vk->synchBuffer(PPTR(hz_gpu),DEVICE_TO_HOST);
	vk->finalizeCommandList();
	vk->submitWork();
	vk->deviceSynch();

	memcpy(hz_outputFromGpu, hz_gpu, sizeof(DATA_TYPE) * NX * NY);	
		
}


int main(int argc, char *argv[])
{
	double t_start, t_end;

	DATA_TYPE* _fict_;
	DATA_TYPE* ex;
	DATA_TYPE* ey;
	DATA_TYPE* hz;
	DATA_TYPE* hz_outputFromGpu;

	_fict_ = (DATA_TYPE*)malloc(tmax*sizeof(DATA_TYPE));
	ex = (DATA_TYPE*)malloc(NX*(NY+1)*sizeof(DATA_TYPE));
	ey = (DATA_TYPE*)malloc((NX+1)*NY*sizeof(DATA_TYPE));
	hz = (DATA_TYPE*)malloc(NX*NY*sizeof(DATA_TYPE));
	hz_outputFromGpu = (DATA_TYPE*)malloc(NX*NY*sizeof(DATA_TYPE));

	init_arrays(_fict_, ex, ey, hz);

    VulkanCompute vk("",parseDeviceSelectionFromArgs(argc,argv),GIGA);
	GPU_argv_init(&vk);

	fdtdVulkan(&vk, _fict_, ex, ey, hz, hz_outputFromGpu);

	t_start = rtclock();
	runFdtd(_fict_, ex, ey, hz);
	t_end = rtclock();
	
	fprintf(stdout, "CPU Runtime: %0.6lfs\n", t_end - t_start);
	
	compareResults(hz, hz_outputFromGpu);

	free(_fict_);
	free(ex);
	free(ey);
	free(hz);
	free(hz_outputFromGpu);
    vk.freeResources();

	return EXIT_SUCCESS;
}

