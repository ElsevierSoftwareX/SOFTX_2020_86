# vkpolybench  

vkpolybench is a [Vulkan](https://www.khronos.org/vulkan/) port of the original version of [polybench](https://web.cse.ohio-state.edu/~pouchet.2/software/polybench/GPU/index.html).      
All the host code and device code has been translated into Vulkan compute shaders. For the host code, an updated version of [vkcomp](https://git.hipert.unimore.it/rcavicchioli/cpu_gpu_submission) has been used: vkcomp is a wrapper of the compute pipeline of the Vulkan API. vkcomp was previously used for a 'CUDA vs Vulkan compute' comparison with respect to CPU activity during CPU to GPU command submission (paper [here](https://drops.dagstuhl.de/opus/volltexte/2019/10759/pdf/LIPIcs-ECRTS-2019-22.pdf)).  
For device (kernel/compute shader) code, GLSL shaders are provided in the respective benchmark-specific subfolders; vkcomp will generate SPIR-V binary files starting from the provided GLSL code (.comp files).

# Depedencies and pre-requisites 
* A Vulkan capable GPU/device.  
* Properly installed GPU/device driver(s).  
* LunarG Vulkan SDK (available [here](https://www.lunarg.com/vulkan-sdk/)).

## For Linux 
vkpolybech has been tested on Ubuntu 18.04  
* glslc executable from LunarG SDK must be reachable from the command line.  
    * type *glslc --version* in a terminal to test if it is working.
* The *Makefile* script assumes Vulkan headers are located in */usr/include/vulkan*

## For Windows 
vkpolybench has been tested on Windows 10 Pro, 1903  
* Download and install [cygwin](https://www.cygwin.com/). Be sure to install the following packages when asked (they are NOT installed by default):  
    * gcc-g++
    * gcc-core
    * make
* The *Makefile* script assumes cygwin bin folder is appended to the *path* environment variable
* The *Makefile* script assumes the Vulkan SDK path is appended to the *path* environment variable

# Compile and test

Once this repository is cloned, move to the project root folder.  
For **building**  
* In Linux:
    * Open a terminal and type *chmod a+x compileScript.sh*  
    * To build all the benchmarks type: *./compileScript*
* In Windows:
    * Open a powershell terminal and type *compileScript.bat*  
    
For **running** all the benchmarks (after build)
* In Linux:
    * Open a terminal and type *./compileScript test*
* In Windows:
    * Open a powershell terminal and type *compileScript.bat test*  

For **cleaning** (obj files and generated binary spv related to all the benchmarks)
* In Linux:
    * Open a terminal and type *./compileScript clean*
* In Windows:
    * Open a powershell terminal and type *compileScript.bat clean*


# Status of each benchmarks

| Benchmark                       | CPU-GPU Outputs Beyond Error Threshold   | Notes                              |
|---------------------------------|------------------------------------------|------------------------------------|
| 2DCONV                          |         0                                |                                    |                                                       | 2MM                             |         0                                |                                    |                                  
| 3DCONV                          |         0                                |                                    |
| 3MM                             |         0                                |                                    |
| ATAX                            |         0                                |                                    |
| BICG                            |         0                                |                                    |
| CORR                            |         0                                |                                    |
| COVAR                           |         0                                |                                    |
| FDTD-2D                         |         2                                |                                    |
| GEMM                            |         0                                |                                    |
| GESUMMV                         |         0                                |                                    |
| GRAMSCHM                        |         0                                | Weird numbers in output buffer     |
| MVT                             |         0                                |                                    |
| SYR2K                           |         0                                | Possible bug from original version |
| SYRK                            |         0                                | Possible bug from original version |

# Tested GPU devices

* NVIDIA GeForce RTX 2070
* NVIDIA TITAN V
* NVIDIA TITAN RTX
* NVIDIA Xavier SoC 
* INTEL HD Graphics 530

# Known issues

* For each benchmark, data types, sizes and other constants are defined in the *HDcommon.h* header file, that is present in each benchmark folder and relates to both 
device and host code. 
If **GLSL_TO_SPV_UPDATE_COMPILATION_CHECK** in *VulkanCompute.h* is defined, vkcomp will check the last modification date
of the glsl source and compares it with last modification of the generated binary spv file (if present). If no mismatches are present, the spv binary will not be 
generated; hence, the last generated .spv file will be read by the application. This mechanism is unable to track modification to the *HDcommon.h* file, so that
if a modification is performed in the header file, the previously generated .spv file must be manually deleted.  


