//polybenchUtilFuncts.h
//Scott Grauer-Gray (sgrauerg@gmail.com)
//Functions used across hmpp codes

#ifndef POLYBENCH_UTIL_FUNCTS_H
#define POLYBENCH_UTIL_FUNCTS_H

#include <string>
#include <algorithm>
#include "vkcomp/ComputeInterface.h"
#include <iostream>

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	#include <android/native_activity.h>
	#include <android/asset_manager.h>
	#include <android_native_app_glue.h>
	#include <sys/system_properties.h>
	#include "VulkanAndroid.h"
#endif

//define a small float value
#define SMALL_FLOAT_VAL 0.00000001f

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	#define ANDROID_MAIN(benchmarkname) android_app *androidapp;\
	int main(int argc, char *argv[]);\
	void handleAppCommand(android_app * app, int32_t cmd) {\
		if (cmd == APP_CMD_INIT_WINDOW) {\
			char argv[1][10] = {benchmarkname};\
			main((int)1,(char**)argv);\
			ANativeActivity_finish(app->activity);\
		}\ 
	}\
	void android_main(android_app* state) {\
		androidapp = state;\ 
		androidapp->onAppCmd = handleAppCommand;\ 
		int ident, events;\
		struct android_poll_source* source;\
		while ((ident = ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0) {\
			if (source != NULL)	{\
				source->process(androidapp, source);\
			}\
			if (androidapp->destroyRequested != 0) {\
				break;\
			}\
		}\
	}
#else 
	#define ANDROID_MAIN(benchmarkname) {}
#endif

bool isNumber(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}


int parseDeviceSelectionFromArgs(const int argc, const char *const argv[])
{
	if(argc<=1) 
		return NO_VENDOR_PREFERRED;

	std::string sarg(argv[1]);

	if(sarg.compare("h")==0 ||
	sarg.compare("H")==0 ||
	sarg.compare("--help")==0 ||
	sarg.compare("-help")==0 ||
	sarg.compare("-h")==0 ){
		std::cout << "* Usage " << argv[0] << " [device selection]" << std::endl;
		std::cout << "A GPU Device in a multi GPU system might be selected either  " << std::endl;
		std::cout << "	- by indicating an integer for the device among [1,2,3,4] to select the first, second, third or fourth available GPU device. Or" << std::endl;
		std::cout << "	- by indicating a GPU vendor preference among " << std::endl;
		std::cout << "		NVIDIA" << std::endl;
		std::cout << "		AMD or RADEON" << std::endl;
		std::cout << "		QUALCOMM or ADRENO" << std::endl;
		std::cout << "		POWERVR" << std::endl;
		std::cout << " " << std::endl;
		std::cout << "* For changing data sizes and constants: modify the values in the HDcommon.h file in this directory." << std::endl;
		std::cout << "Do note that changing values in HDcommon.h implies recompiling both host and device code." << std::endl;
		exit(0);
	}

	const bool is_num = isNumber(sarg);
	if(is_num){
		const int device_num = atoi(argv[1]);
		switch(device_num){
			case 0: return NO_VENDOR_PREFERRED;
			case 1: return DEV_SELECT_FIRST;
			case 2: return DEV_SELECT_SECOND;
			case 3: return DEV_SELECT_THIRD;
			case 4: return DEV_SELECT_FOURTH;
			default:
				std::cout << "Device selection beyond the fourth device is currently unsupported. Fallback to 'no preference'" << std::endl;
				return NO_VENDOR_PREFERRED;
		}
	} else {
		std::transform(sarg.begin(), sarg.end(), sarg.begin(),
		[](unsigned char c) -> unsigned char { return std::toupper(c); });
		if(sarg.compare("NVIDIA")==0) return NVIDIA_PREFERRED;
		else if (sarg.compare("AMD")==0 || sarg.compare("RADEON")==0) return AMD_PREFERRED;
		else if (sarg.compare("INTEL")==0) return INTEL_PREFERRED;
		else if (sarg.compare("ARM")==0 || sarg.compare("MALI")==0) return ARM_MALI_PREFERRED;
		else if (sarg.compare("QUALCOMM")==0 || sarg.compare("ADRENO")==0) return QUALCOMM_PREFERRED;
		else if (sarg.compare("POWERVR")==0) return IMG_TECH_PREFERRED;
		else {
			std::cout << "Unrecognized preference vendor. Type <program_name> h, for help. Fallback to 'no preference'" << std::endl;
			return NO_VENDOR_PREFERRED;
		}
	}

}

double rtclock()
{
    struct timezone Tzp;
    struct timeval Tp;
    int stat;
    stat = gettimeofday (&Tp, &Tzp);
    if (stat != 0) printf("Error return from gettimeofday: %d",stat);
    return(Tp.tv_sec + Tp.tv_usec*1.0e-6);
}


float absVal(float a)
{
	if(a < 0)
	{
		return (a * -1);
	}
   	else
	{ 
		return a;
	}
}



float percentDiff(double val1, double val2)
{
	if ((absVal(val1) < 0.01) && (absVal(val2) < 0.01))
	{
		return 0.0f;
	}

	else
	{
    		return 100.0f * (absVal(absVal(val1 - val2) / absVal(val1 + SMALL_FLOAT_VAL)));
	}
} 

#endif //POLYBENCH_UTIL_FUNCTS_H
