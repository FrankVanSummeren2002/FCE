# FCE
Custom toy engine. Which was made by me during the summer vacation.

# Setup
There is quite a bit of a setup needed for getting the project up and running as the project requires a lot of exernal libraries which need to be included:

# Bullet
Get bullet here https://github.com/bulletphysics/bullet3/releases

When you have downloaded bullet locate the src folder move this over into the FCE/External/Bullet folder

# ENTT
Get ENTT here https://github.com/skypjack/entt

Again download it, locate the src folder and move it over into FCE/External/ENTT

# GLFW
Go to this website https://www.glfw.org/download.html then choose the 64-bit Windows Binaries
When downloaded move all of the files into the FCE/External/GLFW-3.3.7.bin.WIN64

# GLM
To download GLM go here https://github.com/g-truc/glm/tree/master/glm
From here move everything inside of the glm folder into the FCE/External/glm

# STB
get STB here https://github.com/nothings/stb
From here move everything again in the FCE/External/stb-master

# TinyOBJloader
Get tinyobjloader here https://github.com/tinyobjloader/tinyobjloader
Again put it in FCE/External/tinyobjloader-master

# Vulkan Memory allocator
get the vulkan memory allocator here https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
and put everything inside of it into FCE/External/VulkanMemoryAllocator-master

# Vulkan
Finally install the vulkan sdk https://vulkan.lunarg.com You probably do not want to copy these files over
So instead open the solution -> go to the properties of all of the projects-> go to the additional Include Directories ->change the $(SolutionDir)External\VulkanSDK\1.3.204.1\Include to your path to the Include -> go to Additional Library Dependencies -> change the $(SolutionDir)External\VulkanSDK\1.3.204.1\Lib to your Lib path
