/*
VulkanCompute.h: This file is part of the vkpolybench test suite,
See LICENSE.md for vkpolybench and other 3rd party licenses. 
*/

#ifndef VULKAN_COMPUTE_H
#define VULKAN_COMPUTE_H

#include "stdafx.h"
#include "ComputeInterface.h"
#include "CommandListBased.h"
#include "macrodefs.h"

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	#include <android/native_activity.h>
	#include <android/asset_manager.h>
	#include <android_native_app_glue.h>
	#include <sys/system_properties.h>
	#include "VulkanAndroid.h"
#endif

#include "vulkan.h"
#include <string.h>
#include <vector>
#include <unordered_map>
#include <map>

#define VK_CRITICAL_CALL(x) last_operation_error = x

#define GIGA 1024*1024*1024

/*checks if an updated SPV file is present for a given glsl src
issue: changes in header files are not tracked!*/
#define GLSL_TO_SPV_UPDATE_COMPILATION_CHECK

//Will remove generated spv binary after a successfull glsl->spv compilation
//#define REMOVE_SPV_AFTER_COMPILATION

//#define PRE_ALLOCATED_BUFFER_FOR_PUSH_CONSTANTS_SIZE 100*1024

#define VK_WFI \
	    if(device!=NULL) \
		   vkDeviceWaitIdle(device); \

//#define DEBUG_VK_ENABLED 

typedef std::unordered_map<std::string, VkShaderModule>::iterator iter_programs;

struct VulkanBufferStruct { 
	VkDeviceMemory memory;
	VkBuffer buffer;
	VkDescriptorBufferInfo buffer_info;
	//std::string id;
};

struct WIP_pipeline {
	VkComputePipelineCreateInfo wip_pipeline;
	VkPipelineLayoutCreateInfo wip_layout;
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings;
	std::vector<VkWriteDescriptorSet> write_descriptor_sets;
	std::string shader_id;
	uint32_t size_of_push_constants;
};

struct ProducedVKPipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkDescriptorPool descr_pool;
	VkDescriptorSet descriptor_set;
	std::string shader_id;
	LaunchConfiguration_t launch_configuration;
};

class VulkanCompute : public ComputeInterface, public CommandListBased
{
public:
	VulkanCompute(std::string glslc_path="", uint32_t preferred_vendor_=NO_VENDOR_PREFERRED, uint64_t device_memory_requirement = GIGA);
	void createContext();
	void errorCheck();
	int32_t loadAndCompileShader(const std::string, const std::string);
	int32_t loadAndCompileShader(CrossFileAdapter, const std::string);
	void printContextInformation();
	void *deviceSideAllocation(const uint64_t size, const BufferUsage buffer_usage, const uint32_t stride = 0);

#ifdef __ANDROID__
	void setAndroidAppCtx(android_app *app_ctx);
	android_app *getAndroidAppCtx();
#endif

	void startCreatePipeline(std::string shader_id);
	void selectPipeline(const uint32_t selected_pipeline);
	uint32_t finalizePipeline();
	void startCreateCommandList();
	void finalizeCommandList();
	void submitWork();
	
	void synchBuffer(void **data, const uint8_t direction);
	void setArg(void**, const std::string shader_id, const uint32_t);
	void setSymbol(const uint32_t location, uint32_t byte_width = 4);
	inline void synchLaunch();
	void copySymbolInt(int value, const  std::string shader, const uint32_t location);
	void copySymbolDouble(double value, const  std::string shader, const uint32_t location);
	void copySymbolFloat(float value, const std::string shader, const uint32_t location);
	void setLaunchConfiguration(const ComputeWorkDistribution_t blocks, const ComputeWorkDistribution_t threads = ComputeWorkDistribution_t{0,0,0});
	void launchComputation(const std::string computation_identifier);
	void deviceSynch();
	void freeResource(void* resource);
	void freeResources();
	//void updatePointer(void** ptr);
	//void updatePointers(std::vector<void**> v) {};
	~VulkanCompute();

private:

	typedef std::unordered_map<std::string, VkShaderModule>::iterator program_iter;
	typedef std::map<uint32_t, ProducedVKPipeline>::iterator pipeline_iter;
	//typedef std::unordered_map<uint32_t, intptr_t>::iterator pc_map_iterator;
	typedef std::/*unordered_*/map<intptr_t, VulkanBufferStruct>::iterator buffer_iterator;
	typedef std::map<intptr_t, uint64_t>::iterator iter_PSB;

	//uint32_t buffer_counter;

	VkResult last_operation_error;
	VkInstance instance;
	VkPhysicalDevice phys_device;
	VkPhysicalDeviceProperties phys_device_props;
	VkDevice device;
	VkQueue queue; //likely to become a vector
	VkSubmitInfo submit_info;
	VkPhysicalDeviceMemoryProperties mem_props;
	VkCommandPool cmd_pool;
	VkCommandBuffer cmd_bufs; //likely to become a vector
	std::string last_loaded_shader;
	VkShaderModule last_loaded_shader_module;
	WIP_pipeline wip_pipeline_creation;
	VkSpecializationInfo wip_spec_info;
	std::vector<VkSpecializationMapEntry> wip_specs_map;

	VulkanBufferStruct PSB; //stands for Persistent Staging Buffer
	uint8_t* PSB_data;
	uint32_t  pre_allocated_spec_constants[3];

	uint32_t queue_family;
	std::string glslc_folder;
	uint32_t preferred_vendor;
	uint64_t device_memory_requirements;
	std::vector<const char*> instance_extensions;

	std::unordered_map<std::string, VkShaderModule> program_map;
	std::unordered_map<std::string, std::unordered_map<uint32_t,uint32_t>> pc_offset_map;

	std::map<intptr_t, uint64_t> PSB_allocation_table;
	std::/*unordered_*/map<intptr_t, VulkanBufferStruct> device_side_allocation_table;
	std::map<uint32_t, ProducedVKPipeline> pipeline_table;

	int32_t currently_selected_pipeline_index;

#ifdef DEBUG_VK_ENABLED
	std::vector<const char*> dbg_layer_strings;
	std::vector<const char*> dbg_extension_strings;
	VkDebugReportCallbackEXT debug_callback_handle;
	void setupDebug();
	void destroyDebug();
#endif

#ifdef __ANDROID__
	android_app *androidapp;
#endif

	int32_t getMemoryType(uint32_t typeBits, VkFlags properties, const VkPhysicalDeviceMemoryProperties *const mem_props);
	const char *fromVKDeviceTypeToString(const VkPhysicalDeviceType dev_type);
	template <typename T> void setUpPushConstant(T value, const uint32_t location);
	bool checkGLSLSPVtimestampDifference(const std::string &glsl_src, const std::string &spv_bin);
	
};
#endif
