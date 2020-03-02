/**
 * stdafx.cpp: This file is part of the vkpolybench test suite,
 * See LICENSE.md for vkpolybench and other 3rd party licenses. 
 */

#include "stdafx.h"
#include <memory>
#include <stdio.h>

std::string exec(const char* cmd) {
	char buffer[128];
	std::string result = std::string(cmd);
	std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
	if (!pipe) throw std::runtime_error("popen() failed!");
	while (!feof(pipe.get())) {
		if (fgets(buffer, 128, pipe.get()) != NULL)
			result += buffer;
	}
	return result;
}
std::string exec(std::string cmd)
{
	return exec(std::string(cmd+" 2>&1").c_str());
}

#ifdef _WIN32

	#define exp7           10000000i64     //1E+7     //C-file part
	#define exp9         1000000000i64     //1E+9
	#define w2ux 116444736000000000i64     //1.jan1601 to 1.jan1970
	

	void unix_time(struct timespec *spec)
	{
		__int64 wintime; GetSystemTimeAsFileTime((FILETIME*)&wintime);
		wintime -= w2ux;  spec->tv_sec = wintime / exp7;
		spec->tv_nsec = wintime % exp7 * 100;
	}

	int clock_gettime(int, timespec *spec)
	{
		static  struct timespec startspec; static double ticks2nano;
		static __int64 startticks, tps = 0;    __int64 tmp, curticks;
		QueryPerformanceFrequency((LARGE_INTEGER*)&tmp); //some strange system can
		if (tps != tmp) {
			tps = tmp; //init ~~ONCE         //possibly change freq ?
			QueryPerformanceCounter((LARGE_INTEGER*)&startticks);
			unix_time(&startspec); ticks2nano = (double)exp9 / tps;
		}
		QueryPerformanceCounter((LARGE_INTEGER*)&curticks); curticks -= startticks;
		spec->tv_sec = startspec.tv_sec + (curticks / tps);
		spec->tv_nsec = startspec.tv_nsec + (double)(curticks % tps) * ticks2nano;
		if (!(spec->tv_nsec < exp9)) { spec->tv_sec++; spec->tv_nsec -= exp9; }
		return 0;
	}
#endif

float getElapsedTime(const timespec *const tstart, const timespec *const tend)
{
	uint64_t end = (uint64_t)(tend->tv_sec) * 1000000000 + (tend->tv_nsec);
	uint64_t start = (uint64_t)(tstart->tv_sec) * 1000000000 + (tstart->tv_nsec);

	return (float)(end-start);

}

//TODO: do we still need this?
int setFIFO99andCore(const int coreID){

#ifdef __linux__ 
	struct sched_param param;
	param.sched_priority = 99;

	int res = sched_setscheduler(getpid(), SCHED_FIFO, 
		&param);

	if(res!=0) {
		printf("sched_setscheduler returned %d. Are you Root?\n", res);
		return res;
	}

	cpu_set_t my_set;        // Define your cpu_set bit mask. 
	CPU_ZERO(&my_set);       // Initialize it all to 0, i.e. no CPUs selected. 
	CPU_SET(coreID, &my_set);     // set the bit that represents the core passed as argument. 
	res = sched_setaffinity(0, sizeof(cpu_set_t), &my_set); // Set affinity of this process to 
                                                  			// the defined mask
	
	if(res!=0) {
		printf("sched_setaffinity returned %d. Are you Root or did you set a non-existing core?\n", res);
		return res;
	}

	return res;
#else 
	printf("Core and priorities [setFIFO99andCore]: Unimplemented\n");
	return -1;
#endif

}
