# <img src="https://git.hipert.unimore.it/ncapodieci/vkpolybench/raw/master/android/common/res/drawable/icon.png" alt="64px" height="64px"> vkpolybench  

vkpolybench is a [Vulkan](https://www.khronos.org/vulkan/) port of the original version of [PolyBench/GPU](https://web.cse.ohio-state.edu/~pouchet.2/software/polybench/GPU/index.html).      
All the host code and device code has been translated into Vulkan compute shaders. For the host code, an updated version of [vkcomp](https://git.hipert.unimore.it/rcavicchioli/cpu_gpu_submission) has been used: vkcomp is a wrapper of the compute pipeline of the Vulkan API. vkcomp was previously used for a 'CUDA vs Vulkan compute' comparison with respect to CPU activity during CPU to GPU command submission (paper [here](https://drops.dagstuhl.de/opus/volltexte/2019/10759/pdf/LIPIcs-ECRTS-2019-22.pdf)).  
For device (kernel/compute shader) code, GLSL shaders are provided in the respective benchmark-specific subfolders; vkcomp will generate SPIR-V binary files starting from the provided GLSL code (.comp files).  
See license information [here](https://git.hipert.unimore.it/ncapodieci/vkpolybench/blob/master/LICENSE.md)

## Depedencies and pre-requisites 
* A Vulkan capable GPU/device.  
* Properly installed GPU/device driver(s).  
* LunarG Vulkan SDK (available [here](https://www.lunarg.com/vulkan-sdk/)).

### For Linux 
vkpolybech has been tested on Ubuntu 18.04  
* glslc executable from LunarG SDK must be reachable from the command line.  
    * type *glslc --version* in a terminal to test if it is working.
* The *Makefile* script assumes Vulkan headers are located in */usr/include/vulkan*

### For Windows 
vkpolybench has been tested on Windows 10 Pro, 1903  
* Download and install [cygwin](https://www.cygwin.com/). Be sure to install the following packages when asked (they are NOT installed by default):  
    * gcc-g++
    * gcc-core
    * make
* The *Makefile* script assumes cygwin bin folder is appended to the *path* environment variable
* The *Makefile* script assumes the Vulkan SDK path is appended to the *path* environment variable

### For Android
vkpolybench has been tested on a smartphone running Android 9. Host PC was Windows 10 with Android NDK and SDK properly installed and set to path.    
Also, [Gradle](https://gradle.org/) 6.1.1 must be installed and set to path.  
Developer and USB debug mode on your smartphone device must be enabled. 

## Compile and test

Once this repository is cloned, move to the project root folder.  
For **building**  
* In Linux:
    * Open a terminal and type *chmod a+x compileCodes.sh*  
    * To build all the benchmarks type: *./compileCodes*
* In Windows:
    * Open a powershell terminal and type *compileCodes.bat*  
* In Android:
    * Each benchmark has to be invidually compiled. On a shell, navigate through *vkpolybench/android/[BENCHMARK_NAME]*
    * type *gradle assembleDebug*  
    
For **running** all the benchmarks (after build)
* In Linux:
    * Open a terminal and type *./compileCodes test*
* In Windows:
    * Open a powershell terminal and type *compileCodes.bat test*  
      
* In case more than one Vulkan capables device are present in the tested system:  
    * in *compileCodes.sh* or *compileCodes.bat* change *DEVICE_SELECTION* variable from 0 to:   
        * *[deviceId]*, as an integer indicating the device identifier of your preferred GPU, or   
        * *[deviceVendorString]*, as a string identifier of the device vendor (e.g. nvidia or intel)  

* For Android:
    * Once a benchmark app is built and your phone is connected to the host PC, type on a shell:  
    *[path to ANDROID_SDK]\sdk\platform-tools\adb.exe install [path to the repo]\vkpolybench\android\2DCONV\build\outputs\apk\debug\2DCONV-debug.apk*  
    Your app should now be installed in your smartphone. Substitute 2DCONV with any other compiled benchmark.  
    * type: *[path to ANDROID_SDK]\sdk\platform-tools\adb.exe logcat -s "vulkanandroid"*. This will capture the logcat output related to vkpolybench   
    * On your phone tap the newly installed app and check the output on the logcat console   

For **cleaning** (obj files and generated binary spv related to all the benchmarks)
* In Linux:
    * Open a terminal and type *./compileCodes clean*
* In Windows:
    * Open a powershell terminal and type *compileCodes.bat clean*
* In Android:
    * Open a shell on your host PC and type *gradle clean*  

## Tests

![alt text](https://git.hipert.unimore.it/ncapodieci/vkpolybench/raw/vkpolyandroid/vkpolyimgs/ubuntutests.png)  
![alt text](https://git.hipert.unimore.it/ncapodieci/vkpolybench/raw/vkpolyandroid/vkpolyimgs/wintests.png)  
![alt text](https://git.hipert.unimore.it/ncapodieci/vkpolybench/raw/vkpolyandroid/vkpolyimgs/androidtests.png)  

## Tested GPU devices

* NVIDIA GeForce RTX 2070
* NVIDIA TITAN V
* NVIDIA TITAN RTX
* NVIDIA Xavier SoC 
* INTEL HD Graphics 530
* ARM MALI G72

## Known issues

* For each benchmark, data types, sizes and other constants are defined in the *HDcommon.h* header file, that is present in each benchmark folder and relates to both 
device and host code. 
If **GLSL_TO_SPV_UPDATE_COMPILATION_CHECK** in *VulkanCompute.h* is defined, vkcomp will check the last modification date
of the glsl source and compares it with last modification of the generated binary spv file (if present). If no mismatches are present, the spv binary will not be 
generated; hence, the last generated .spv file will be read by the application. This mechanism is unable to track modification to the *HDcommon.h* file, so that
if a modification is performed in the header file, the previously generated .spv file must be manually deleted.  


