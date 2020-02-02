#include "stdafx.h"
#include "CommandListBased.h"
#include "macrodefs.h"

CommandListBased::CommandListBased()
{
	cmd_list_creation_state = CMD_LIST_IS_RESET;
	pipeline_creation_state = PIPELINE_IS_RESTING;
}

void CommandListBased::startCreatePipeline(std::string shader_id)
{
	if (!verifyPipelineCreationState(PIPELINE_IS_RESTING))
		FATAL_EXIT("It is illegal to create a pipeline without finalizing the previously created one")

	if(verifyCmdListState(CMD_LIST_IN_CREATION))
		FATAL_EXIT("It is illegal to create a pipeline while recording a command list")

		pipeline_creation_state = PIPELINE_IN_CREATION;
}

uint32_t CommandListBased::finalizePipeline()
{
	if (!verifyPipelineCreationState(PIPELINE_IN_CREATION))
		FATAL_EXIT("Attempting to finalize an unknown pipeline")

		pipeline_creation_state = PIPELINE_IS_RESTING;

	return 0;
}

void CommandListBased::startCreateCommandList()
{
	if (verifyCmdListState(CMD_LIST_IN_CREATION))
		FATAL_EXIT("It is illegal to create a command list without finalizing the previously created one")

	if (!verifyPipelineCreationState(PIPELINE_IS_RESTING))
		FATAL_EXIT("It is illegal to start recording a command list while creating a pipeline")

		cmd_list_creation_state = CMD_LIST_IN_CREATION;
}

void CommandListBased::finalizeCommandList()
{
	if(!verifyCmdListState(CMD_LIST_IN_CREATION))
		FATAL_EXIT("Attempting to create a command list without finalizing the previously created one")

		cmd_list_creation_state = CMD_LIST_IS_CREATED;

}

void CommandListBased::selectPipeline(const uint32_t pipeline_index)
{
	if(!verifyCmdListState(CMD_LIST_IN_CREATION))
		FATAL_EXIT("You can select a pipeline only during command buffer recording")
}

void CommandListBased::updatePointer(void **ptr)
{
	if (set_to_update.size() == 0) return;

	auto it = set_to_update.find((intptr_t)(*ptr));

	if (it == set_to_update.end())
		return;

	(*ptr) = (void*)it->second;

	set_to_update.erase(it->first);
}

void CommandListBased::updatePointers(std::vector<void**> pptr_vec)
{
	for (size_t i = 0; i < pptr_vec.size(); i++)
		updatePointer(pptr_vec[i]);

	std::cout << "left to update " << set_to_update.size() << std::endl;
}

CommandListBased::~CommandListBased()
{
}

bool CommandListBased::verifyCmdListState(const uint8_t expectation)
{
	if (expectation != cmd_list_creation_state)
		return false;

	return true;
}

bool CommandListBased::verifyPipelineCreationState(const uint8_t expectation)
{
	if (expectation != pipeline_creation_state)
		return false;

	return true;
}
