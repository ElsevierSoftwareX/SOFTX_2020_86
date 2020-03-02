/**
 * ComputeInterface.cpp: This file is part of the vkpolybench test suite,
 * See LICENSE.md for vkpolybench and other 3rd party licenses. 
 */

#include "stdafx.h"
#include "ComputeInterface.h"
#include "macrodefs.h"


void ComputeInterface::createContext()
{

	if (!context_created) context_created = true;
	else
		FATAL_EXIT("Context already created... ")
}

std::string ComputeInterface::fromVendorIDtoString(uint32_t vendorID)
{
	switch (vendorID)
	{
		case NVIDIA_PREFERRED: return "NVIDIA";
		case AMD_PREFERRED: return "AMD";
		case INTEL_PREFERRED: return "INTEL";
		case ARM_MALI_PREFERRED: return "ARM";
		case QUALCOMM_PREFERRED: return "QUALCOMM";
		case IMG_TECH_PREFERRED: return "IMAGINATION TECHNOLOGY";
		default: return "UNKNOWN";
	}
}

void ComputeInterface::printContextInformation()
{
	if (!context_created)
		FATAL_EXIT("Context not created. Unable to print info...")

}

ComputeInterface::~ComputeInterface()
{
}
