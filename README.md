# vkpolybench  

vkpolybench is a [Vulkan](https://www.khronos.org/vulkan/) port of the original version of [polybench](https://web.cse.ohio-state.edu/~pouchet.2/software/polybench/GPU/index.html).      
All the host code and device code has been translated into Vulkan compute shaders.    
For the host code, a version of [vkcomp](https://git.hipert.unimore.it/rcavicchioli/cpu_gpu_submission) has been used: vkcomp is a wrapper of the compute pipeline    
of the Vulkan API. vkcomp was previously used for a 'CUDA vs Vulkan compute' comparison with respect to    
CPU activity during CPU to GPU command submission (paper [here](https://drops.dagstuhl.de/opus/volltexte/2019/10759/pdf/LIPIcs-ECRTS-2019-22.pdf)).  
For device (kernel/compute shader) code, GLSL shaders are provided in the respective benchmark-specific subfolders; vkcomp will generate SPIR-V files starting from     
the provided GLSL code (.comp files).  

# depedencies and pre-requisites  
- A Vulkan capable GPU/device.  
- Properly installed GPU/device drivers.  
- LunarG Vulkan SDK (available [here](https://www.lunarg.com/vulkan-sdk/)).
- glslangValidator executable from LunarG SDK must be reachable from command line.

# Status of each benchmarks

| Benchmark                       | CPU-GPU Outputs Beyond Error Threshold   | Notes                              |
|---------------------------------|------------------------------------------|------------------------------------|
| 2DCONV                          |         0                                |                                    |                                                                
| 2MM                             |         0                                |                                    |                                  
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

