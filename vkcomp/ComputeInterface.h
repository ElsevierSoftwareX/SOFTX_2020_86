#ifndef COMPUTE_INTERFACE_H
#define COMPUTE_INTERFACE_H

#include "datatypes.h"
#include "CrossFileAdapter.h"

#define NVIDIA_PREFERRED 0x10DE
#define AMD_PREFERRED 0x1002
#define INTEL_PREFERRED 0x163C
#define ARM_MALI_PREFERRED 0x13B5
#define QUALCOMM_PREFERRED 0x5143

#define HOST_TO_DEVICE 0
#define DEVICE_TO_HOST 1

/**
@author Nicola Capodieci
@date November, 2016
@brief Compute Interface member function definitions

All the API wrappers extends this class, hence each member function should be implemented.
None of the member functions listed in this class should be called outside of a API Wrapper implementation class.
Do note that some of this member functions are not purely virtual.
Also, new generation APIs pose restrictions on when and how call some of the member function of this class.

*/
	class ComputeInterface
	{

	public:
		/*! \fn void createContext()
		\brief creates a compute context. Every overloaded versions of this method implemented in child classes must also call this version.
		No specific params are needed for creating a context.
		This method ensures that the context is created just once.
		*/
		void createContext();

		/*!\fn void errorCheck()
		\brief generic abstract error checking procedure.
		According to the GPGPU unified semantics, all the APIs signal an error after returning from (failed) critical calls.
		The API Wrapper programmer has the duty to capture such output errors, so that the error code is correctly "translated" (according to the API in use) 
		when calling this member function.
		Do note: this member function can be called outside of a API impl. class. In this way, the API impl. class programmer can easily decide to avoid error checking if needed and leave it to the end user.
		EXAMPLE:
		#\code
		APISpecificErrorCode_t error = APISpecificCall(...);
		//or
		APISpecificCall(...,&error);
		//then, if needed
		errorCheck(); //translates error in something human readable and decides whether to quit the app or apply fallback(s)
		#\endcode
		*/
		virtual void errorCheck()=0;

		/*! \fn std::string fromVendorIDtoString(uint32_t vendorID);
		\brief Translates into human readable String a hex number identifier of specific HW vendor.
		For obtaining vendor IDs: http://pcidatabase.com/vendors.php?sort=id
		\param vendorID the hex id number that is vendor specific. Such number might be obtained by querying the device during specific API context creation related calls.
		\return a human readable string
		*/
		std::string fromVendorIDtoString(uint32_t vendorID);

		/*! \fn int32_t loadAndCompileShader(const std::string program_str_in, const std::string program_id)
		\brief Loads a gpu program written in a language able to be understood by the specific API from a single string.
		The output of such method is usually a blob of some kind, stored in a table implemented in the API specific child class.
		As of now, this method is able to compile a single program at a time.
		\param program_str_in: the input string containing the program code
		\param program_id: the program id to associate to the specific output binary of the input program. It should correspond to the shader entry point.
		\return shall return -1 in case of failed compile/linking phase. Otherwise it returns and integer corresponding to an index of the data structure in which the binary blob and its entry point are saved.
		*/
		virtual int32_t loadAndCompileShader(const std::string, const std::string)=0;
		/*! \fn int32_t loadAndCompileShader(CrossFileAdapter file_in, const std::string program_id);
		\brief Loads a gpu program written in a language able to be understood by the specific API from a file.
		\param file_in: the input file in which a single program is saved. If the API specific code implies using an entry point, it have to correspond to the second argument of this function.
		\param program_id: the program id to associate to the specific output binary of the input program. It should correspond to the shader entry point.
		\return shall return -1 in case of failed compile/linking phase. Otherwise it returns and integer corresponding to an index of the data structure in which the binary blob and its entry point are saved.
		*/
		virtual int32_t loadAndCompileShader(CrossFileAdapter, const std::string)=0;

		/*! \fn void printContextInformation();
		\brief Prints to std output all the information the API Wrapper implementer thought to be usefull to show to the user.
		Usually this method is called just after context creation, so to see if the context is created for the right device/driver/etc...
		It can be usefull to report the results of extension queries, if the specific API uses extensions.
		*/
		virtual void printContextInformation();

		/*! \fn void* deviceSideAllocation(const uint64_t size, const BufferUsafe buffer_usage, const uint32_t stride)
		\brief Allocates memory on the device and returns a pointer in which the user can read or write at host level.
		The way this method is thought to be implemented is by using a "device side allocation table", which is a data structure that matches a host visible pointer to a device visible pointer.
		The idea is to hide from the final user the difficulties in handling different pointers that live in different address spaces. This is obtained by
		giving back to the user only the pointer in which read and or writing operations are permitted.
		The pointer given as output is valid until the user explicitly erases this resource. Attempting to read or write to an erased resource might crash the program.
		\param size: the total size in byte for the device allocation
		\param buffer_usage: specify the use in terms of read, write or both for the newly created (sub)resource.
		\param stride: the distance in bytes from two instances belonging in the same newly created buffer. Except for d3d11, this latter param is usually ignored.
		\return a void pointer to be casted in the appropriate data type. Such pointer can be read and written in host code.
		*/
		virtual void *deviceSideAllocation(const uint64_t size, const BufferUsage buffer_usage, const uint32_t stride=0)=0;

		/*! \fn void setArg(void** h_pptr, std::string program_id, const uint32_t location)
		\brief sets an argument (i.e. a previously allocated device side resource) as input for the GPU program invocation having "program_id" as id.
		If the inputs are unknown, a fault strategy can be implemented by the API wrapper implementer.
		\param h_pptr: a pointer to pointer to a host pointer given as result of a deviceSideAllocation call. The corresponding device ptr is obtained by querying the device side allocation table.
		\param program_id: the program id of a previously loaded and compiled gpu program.
		\param location: the location in terms of resource binding of this specific argument. Can be inferred by looking at the GPU program code.
		*/
		virtual void setArg(void**, const std::string shader_id, const uint32_t)=0;

		/*! \fn void syncLaunch()
		\brief Makes sure to block the host until the previously launched program finishes its execution.
		*/
		virtual inline void synchLaunch()=0;

		/*! \fn void synchBuffer(void** h_pptr, const uint8_t transfer_direction)
		\brief Perform a host to device or device to host data transfer between the cpu pointer given as input and the corresponding device side allocation.
		As of now and by default, device and host pointer have to be explicitely synchronized by setting up host to device (and viceversa) transfers. 
		Calling this method using a host pointer obtained with a call to deviceSideAllocation, initiates a transfer using the GPU copy engine.
		Calling this method is blocking wrt the host.
		Direction of the transfer is specified in the second param.
		\param h_pptr: a pointer to pointer to a host pointer given as result of a deviceSideAllocation call. The corresponding device ptr is obtained by querying the device side allocation table.
		\param transfer_direction: HOST_TO_DEVICE or DEVICE_TO_HOST. Must be coherent with the declared buffer usage specified as the second argument of deviceSideAllocation.
		*/
		virtual void synchBuffer(void **, const uint8_t) = 0;

		//We have to do this as it turns out that C++ templates are not that much better than Java Generics...
		//At the time I was writing the OpenGLCS class, I noticed the fact that being opengl written in pure C, there is no way
		//of templating uniform updates without getting REALLY messy. I.E. having a glUniform<T> is simply too much stuff to add in terms of classes.
		//Hence, the solution of using three un-templated functions instead of a templated one.
		//On the other hand, when I was writing the DX11CS code, I realized how such approach won't scale due to the ridiculous amount of code
		//to handle in order to update a constant buffer (the closest thing to an openGL uniform according to https://msdn.microsoft.com/en-us/windows/uwp/gaming/porting-uniforms-and-attributes)
		//When I will have to do stuff with the APIs derived from the CommandListBased interface, things are going to get even more crazy in terms of lines of code.
		//However, all API wrappers shall be created equal, so fuck it.
		//The best I can do as of now, is using private functions for mitigating the exponential growth of similar lines of code, so to internally parametrizing stuff.
		//(E.G. copySymbolWithKnownByteWidth in DX11CS.h/cpp)
		//I wrote this in case anyone (especially me) would question the design of this.

		/*!\fn void copySymbolInt(int value, std::string program, const uint32_t location)
		\brief copies an integer value constant from host to device
		\param value: the integer value to pass to the GPU program
		\param program: the program_id string identifier of a previously loaded and compiled GPU program.
		\param location: the location within the program layout of this constant
		*/
		virtual void copySymbolInt(  int value, const  std::string shader, const uint32_t location)=0;

		/*!\fn void copySymbolDouble(double value, std::string program, const uint32_t location)
		\brief copies a double value constant from host to device
		\param value: the double value to pass to the GPU program
		\param program: the program_id string identifier of a previously loaded and compiled GPU program.
		\param location: the location within the program layout of this constant
		*/
		virtual void copySymbolDouble( double value, const  std::string shader, const uint32_t location)=0;

		/*!\fn void copySymbolfloat(float value, std::string program_id, const uint32_t location)
		\brief copies a float value constant from host to device
		\param value: the float value to pass to the GPU program
		\param program_id: the program_id string identifier of a previously loaded and compiled GPU program.
		\param location: the location within the program layout of this constant
		*/
		virtual void copySymbolFloat( float value, const std::string shader, const uint32_t location)=0;

		/*! \fn void setLaunchConfiguration(const ComputeWorkDistribution_t blocks, const ComputeWorkDistribution_t threads)
		\brief sets the launch configuration for the next program launch(es)
		This will set an attribute to this class called latest_launch_conf and will be valid until a new call of setLaunchConfiguration.
		\param blocks: the 3D block configuration. In terms of number of work groups per each dimension.
		\param blocks: the 3D threads configurations in terms of threads (or workitems) per block
		*/
		virtual void setLaunchConfiguration(const ComputeWorkDistribution_t blocks, const ComputeWorkDistribution_t threads=NULL)=0;

		/*! \fn void launchComputation(const std::string program_id)
		\brief launch asynchronously a GPU program identified with program_id. It does that with a previously set launch configuration
		\param program_id: the program_id string identifier of a previously loaded and compiled GPU program.
		*/
		virtual void launchComputation(const std::string computation_identifier)=0;

		/*! \fn deviceSynch()
		\brief blocks the host until all computations enqueued in the GPU are finished.
		*/
		virtual inline void deviceSynch()=0;

		/*! \fn void freeResource(void* resource)
		\brief frees the specified resource
		\param resource: most likely is a host pointer of a previously device side allocated buffer. It will free both the host and the device pointer.
		*/
		virtual void freeResource(void* resource)=0;

		/*\fn void freeResources()
		\brief frees and destroys all the resources created in this context, but not the context itself.
		Thought to dispose of all the host and device allocations, program binaries and API specific caches. 
		*/
		virtual void freeResources()=0;

		virtual ~ComputeInterface();


	protected:
		LaunchConfiguration_t latest_launch_conf;

	private:
		bool context_created = false;


	};

#endif
