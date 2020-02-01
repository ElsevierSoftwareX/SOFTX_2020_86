#ifndef STDAFX_H
#define STDAFX_H

//OS/platform indipendent headers

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <time.h>

#if defined(_WIN32)
	#include <direct.h>
	#include <Windows.h>
	#include <SDKDDKVer.h>
	#include <tchar.h>
#endif

#if defined(_WIN32) 
#include <dirent.h>
#define GetCurrentDir _getcwd
#define FILE_SEPARATOR '\\'
#define popen _popen
#define pclose _pclose
//#define FOPEN fopen_s
#else
#include <unistd.h>
#include <sched.h>
#define GetCurrentDir getcwd
#define FILE_SEPARATOR '/'
//#define FOPEN fopen
#endif

std::string exec(const char* cmd);
std::string exec(std::string cmd);
void unix_time(struct timespec *spec);
int clock_gettime(int, timespec *spec);
float getElapsedTime(const timespec *const tstart, const timespec *const tend);
int setFIFO99andCore(const int coreID);

#endif
