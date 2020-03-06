cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)

set(NAME "COVAR")

set(SRC_DIR ../../${NAME})
set(VKCOMP_DIR ../../vkcomp)

#message( ${Vulkan_LIBRARY} )
#message( ${Vulkan_LIBRARIES} )
#message( ${Vulkan_INCLUDE_DIRS} )

#set(BASE_DIR ../../../base)
#set(EXTERNAL_DIR ../../../external)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DVK_USE_PLATFORM_ANDROID_KHR -DVK_NO_PROTOTYPES")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 --std=gnu++11 -DVK_USE_PLATFORM_ANDROID_KHR -DVK_NO_PROTOTYPES")

file(GLOB BENCH_SRC "${SRC_DIR}/*.cpp")

#For some reasons, in Windows system, if (UNIX) returns true...
#include_directories("/usr/include/vulkan")
include_directories("$ENV{VULKAN_SDK}/Include/vulkan")

file(GLOB VKCOMP_SRC "${VKCOMP_DIR}/*.cpp" "${VKCOMP_DIR}/*.h")
file(GLOB VKCOMP_HEADERS "${VKCOMP_DIR}/*.h")

add_library(vkcomp OBJECT ${VKCOMP_SRC})

add_library(native-lib SHARED $<TARGET_OBJECTS:vkcomp> ${BENCH_SRC})

add_library(native-app-glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

#add_subdirectory(${VKCOMP_DIR} ${VKCOMP_DIR})

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

include_directories(${VKCOMP_DIR})
include_directories(${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(
    native-lib
    native-app-glue
    android
    log
    z
)