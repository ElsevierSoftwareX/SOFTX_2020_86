#ifndef STDAFX_H
#define STDAFX_H

/**
 * stdafx.h: This file is part of the vkpolybench test suite,
 * See LICENSE.md for vkpolybench and other 3rd party licenses. 
 */

//OS/platform indipendent headers

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>

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
#define stat _stat
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

template<typename T>
std::string anyToString(const T& value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}


#endif
