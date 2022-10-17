# CMakeList.txt : CMake project for FVR, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

#compile Definitions.
if (WIN32)
set(PLATFORM_NAME "_WIN")
endif ()

find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
message(FATAL_ERROR "Vulkan not found")
else()
message(STATUS "Vulkan found")
message(STATUS ${Vulkan_INCLUDE_DIRS})
endif()

#Add source to this project's executable.
add_library (FVR
"Header/Handles.h" 
"Header/vk_DescriptorBuilder.h" 
"Header/vk_engine.h" 
"Header/vk_Helpers.h" 
"Header/vk_initializers.h" 
"Header/vk_mesh.h" 
"Header/vk_textures.h"
"Header/vk_types.h"
"Header/VulkanFrontEnd.h"

"Scr/vk_DescriptorBuilder.cpp" 
"Scr/vk_engine.cpp" 
"Scr/vk_Helpers.cpp" 
"Scr/vk_initializers.cpp" 
"Scr/vk_mesh.cpp" 
"Scr/vk_textures.cpp"
"Scr/VulkanFrontEnd.cpp"
)

#Include library
target_include_directories(FVR PUBLIC
${Vulkan_INCLUDE_DIRS}
"..\\External\glm"
"..\\External\glfw-3.3.7.bin.WIN64\include"
"..\\External\stb-master"
"..\\External\tinyobjloader-master"
"..\\External\vk-bootstrap-master\src"
"..\\External\VulkanMemoryAllocator-master\include"
"Header"
"Src"
"..\\External\imgui-docking"
)


#link libraries
target_link_libraries(FVR Vulkan::Vulkan ${VULKAN_LIB_LIST})