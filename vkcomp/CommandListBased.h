#ifndef COMMAND_LIST_BASED_H
#define COMMAND_LIST_BASED_H

#include "macrodefs.h"
#include <vector>
#include <map>

#define CMD_LIST_IS_CREATED 0
#define CMD_LIST_IN_CREATION 1
#define CMD_LIST_IS_RESET 2

#define PIPELINE_IS_RESTING 0
#define PIPELINE_IN_CREATION 1

#define PIPELINE_HANDLE uint32_t

/*!
@author Nicola Capodieci
@date December, 2016
@brief CommandListBased member function definitions

This class is meant to be extended by wrappers of new generations APIs such as Vulkan and Direct Compute 12.
The purpose of this class is to create a specific interface able to deal with the different paradigm of new GPU oriente APIs, as
the ComputeInterface alone cannot expose such functionalities to older APIs.

*/
class CommandListBased
{
public:
	/*! \fn void CommandListBased()
	\brief Constructor. Has to be called by child classes
	*/
	CommandListBased();

	/*! \fn void startCreatePipeline(std::string program_id)
	\brief signal the beginning of a creation of a pipeline.
	CommandListBased APIs will have to create a pipeline for every GPU program and program launch configuration. Inside a pipeline creation block, 
	(a block identified by the startCreatePipeline and finalizePipeline function calls) the user can define input output layouts, program and its launch configuration.
	This method has to be called by overriding methods in child classes, as it evaluates illegal operations such as nested pipeline creation or attempting to create a command
	list before finalizing the current pipeline. 
	The API wrapper implementer should also check if a specific member function calls fits within a pipeline creation block.
	\param program_id: the string identifier of a previosly loaded and compiled GPU program.
	*/
	void startCreatePipeline(std::string shader_id);

	/*! \fn void setSymbol(const uint32_t location, uint32_t byte_width)
	\brief sets a symbol as layout component of a GPU program
	This function can only be called inside a pipeline creation block.
	Like any other operation within the pipeline creation block, no actual CPU GPU data passing is performed.
	\param location: layout location of the specified symbol
	\param byte_width: size in bytes of the constant to be passed to the GPU program.
	*/
	virtual void setSymbol(const uint32_t location, uint32_t byte_width) = 0;

	/*! \fn uint32_t finalizePipeline()
	\brief finalize the pipeline that was currently created. Might produce a binary blob.
	This method can only be called if a pipeline creation was previously started.
	\return a PIPELINE_HANDLE instance, which is basically an integer value to be used during pipeline selection when building the command list.
	*/
	uint32_t finalizePipeline();

	/*! \fn void startCreateCommandList()
	\brief signal the beginning of a creation of a command list.
	CommandListBased APIs will have to create a command list in which every buffers are transferred, pipeline are selected and symbols are pushed to the GPU. 
	Inside a command list creation block, (a block identified by the startCreateCommandList and finalizeCommandList function calls) the user can record all the commands to
	be later submitted to the GPU.
	This method has to be called by overriding methods in child classes, as it evaluates illegal operations such as nested command list creation or attempting to create a pipeline
	list before finalizing the command list.
	The API wrapper implementer should also check if a specific member function calls fits within a command list creation creation block.
	*/
	void startCreateCommandList();
	
	/*! \fn uint32_t finalizeCommandList()
	\brief finalize the command list that was currently created.
	This method can only be called if a command list creation was previously started.
	*/
	void finalizeCommandList();

	/*! \fn void submitWork
	\brief submits a previously created command list.
	*/
	virtual void submitWork()=0;

	/*! \fn void selectPipeline(const uint32_t pipeline_index)
	\brief selects a previously created pipeline
	Can only be called inside a create command list block.
	*/
	void selectPipeline(const uint32_t pipeline_index);

	/*!\fn void updatePointer(void **h_pptr)
	\brief update the specified pointer address. It might be mandatory after erasing a (sub)resource.
	Since APIs that implement the CommandListBased interface rely on a persistent staging buffer, subresource allocation and pointer tracking
	are implementer responsability. Segmentation handling operations have to be explicitely coded one way or another when a host device buffer couple is erased.
	This might imply updating addresses accordingly.
	\param elem: pointer to pointer of a host side buffer created with deviceSideAllocation
	*/
	void updatePointer(void** elem);

	/*! \fn void updatePointers(std::vector<void**> v)
	\brief updates the specified pointer addresses. It might be mandatory after erasing a (sub)resource.
	Same consideration as the updatePointer member function.
	\param v: vector of pointers to pointers.
	*/
	void updatePointers(std::vector<void**> v);

	~CommandListBased();

protected:
	/*! \fn bool verifiyCmdListState(const uint8_t expectation)
	\brief verifies if the command list is in the expected state
	\param expectation: one of CMD_LIST_IS_CREATED, CMD_LIST_IN_CREATION, CMD_LIST_IS_RESET
	\return a boolean that indicates if the state expectation is met.
	*/
	bool verifyCmdListState(const uint8_t expectation);

	/*! \fn bool verifyPipelineCreationState(const uint8_t expectation)
	\brief verifies if a pipeline creation stage is in the expected state
	\param expectation: one of PIPELINE_IS_RESTING, PIPELINE_IN_CREATION
	\return a boolean that indicates if the state expectation is met.
	*/
	bool verifyPipelineCreationState(const uint8_t expectation);

	/*! A comodity data structure that holds a reference to the pointers to update.
	*/
	std::map<intptr_t, intptr_t> set_to_update;

private:
	uint8_t cmd_list_creation_state;
	uint8_t pipeline_creation_state;
};

#endif
