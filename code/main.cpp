#include <WaveSabrePlayerLib.h>
using namespace WaveSabrePlayerLib;
#include "music.h"

#include <vulkan/vulkan.h>
#include "windows.h"

#define GL_GLEXT_PROTOTYPES
#include <GL/glext.h>

#include "defines.h"

#include "rchit.h"
#include "rgen.h"
#include "rint.h"

uint32_t align_up(uint32_t x, uint32_t a)
{
	return (x + (a - 1)) & ~(a - 1);
}

void* sysGetProcAddress(const char* name)
{
	return (void*)wglGetProcAddress(name);
}

typedef struct
{
	float time;
	unsigned int frameId;
	float cameraPosition[3];
	float cameraTarget[3];
} Globals;

VkPhysicalDeviceMemoryProperties physicalMemoryProperties;

uint32_t GetMemoryType(VkPhysicalDeviceMemoryProperties memoryProperties, uint32_t typeBits, VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if (((typeBits & (1 << i)) > 0) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	return ~0u;
}

void CreateBuffer(VkBuffer* buffer, VkDeviceMemory* deviceMemory, VkDevice* device, VkCommandBuffer* cmdBuf, void* data, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags)
{
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.usage = usage | ((memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? 0 : VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	bufferInfo.size = size;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	vkCreateBuffer(*device, &bufferInfo, NULL, buffer);
	
	// Find memory requirements
	VkBufferMemoryRequirementsInfo2 bufferReqs = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2, NULL, *buffer };
	VkMemoryDedicatedRequirements dedicatedRegs = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
	VkMemoryRequirements2 memReqs = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, &dedicatedRegs };

	vkGetBufferMemoryRequirements2(*device, &bufferReqs, &memReqs);

	VkMemoryAllocateInfo memAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memAllocInfo.allocationSize = memReqs.memoryRequirements.size;
	memAllocInfo.memoryTypeIndex = GetMemoryType(physicalMemoryProperties, memReqs.memoryRequirements.memoryTypeBits, memoryFlags);

	*deviceMemory = VK_NULL_HANDLE;
	vkAllocateMemory(*device, &memAllocInfo, NULL, deviceMemory);
	vkBindBufferMemory(*device, *buffer, *deviceMemory, 0);
	
	if (data != NULL)
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(&stagingBuffer, &stagingBufferMemory, device, cmdBuf, NULL, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* tmpData;
		vkMapMemory(*device, stagingBufferMemory, 0, size, 0, &tmpData);
		memcpy(tmpData, data, size);
		vkUnmapMemory(*device, stagingBufferMemory);

		VkBufferCopy cpy;
		cpy.size = size;
		cpy.srcOffset = 0;
		cpy.dstOffset = 0;

		vkCmdCopyBuffer(*cmdBuf, stagingBuffer, *buffer, 1, &cpy);
	}
}

void CreateTexture(VkImage* texture, VkImageView* vkTextureView, VkDevice vkDevice, uint32_t x, uint32_t y, uint32_t z, VkImageType type, VkImageViewType viewType, VkFormat format, VkImageUsageFlags flags)
{
	VkImageCreateInfo icInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	icInfo.imageType = type;
	icInfo.format = format;
	icInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	icInfo.mipLevels = 1;
	icInfo.arrayLayers = 1;
	icInfo.extent.width = x;
	icInfo.extent.height = y;
	icInfo.extent.depth = z;
	icInfo.usage = flags;
	icInfo.initialLayout = VK_IMAGE_LAYOUT_GENERAL;

	vkCreateImage(vkDevice, &icInfo, NULL, texture);
	
	VkMemoryRequirements2 memReqs = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
	VkMemoryDedicatedRequirements dedicatedRegs = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
	VkImageMemoryRequirementsInfo2 imageReqs = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2 };
	imageReqs.image = *texture;
	memReqs.pNext = &dedicatedRegs;
	vkGetImageMemoryRequirements2(vkDevice, &imageReqs, &memReqs);

	VkMemoryAllocateInfo vkMemInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	vkMemInfo.allocationSize = memReqs.memoryRequirements.size;
	vkMemInfo.memoryTypeIndex = 1;
	VkDeviceMemory vkDeviceMemory;
	vkAllocateMemory(vkDevice, &vkMemInfo, NULL, &vkDeviceMemory);
	vkBindImageMemory(vkDevice, *texture, vkDeviceMemory, 0);
	
	VkImageViewCreateInfo vkImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	vkImageViewInfo.image = *texture;
	vkImageViewInfo.viewType = viewType;
	vkImageViewInfo.format = icInfo.format;
	vkImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	vkImageViewInfo.subresourceRange.baseMipLevel = 0;
	vkImageViewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	vkImageViewInfo.subresourceRange.baseArrayLayer = 0;
	vkImageViewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	vkCreateImageView(vkDevice, &vkImageViewInfo, NULL, vkTextureView);
}

int main()
{
	//RECT rect;
	//GetWindowRect(GetDesktopWindow(), &rect);
	//HWND g_hWnd = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_TOPMOST, "EDIT", NULL, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_POPUP, 0, 0, rect.right, rect.bottom, NULL, NULL, NULL, NULL);
	HWND g_hWnd = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_TOPMOST, "EDIT", NULL, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_POPUP, 0, 0, RENDER_WIDTH, RENDER_HEIGHT, NULL, NULL, NULL, NULL);
	ShowCursor(FALSE);

	PIXELFORMATDESCRIPTOR pfd = { 0, 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, 0, 24 };
	HDC g_hDC = GetDC(g_hWnd);
	int format = ChoosePixelFormat(g_hDC, &pfd);
	SetPixelFormat(g_hDC, format, &pfd);
	HGLRC context = wglCreateContext(g_hDC);
	wglMakeCurrent(g_hDC, context);
	
	GLuint font = glGenLists(256);

	HFONT hfont = CreateFont(32 * RENDER_HEIGHT / 1080.0f,		// Height Of Font
		0,                              // Width Of Font
		0,                              // Angle Of Escapement
		0,                              // Orientation Angle
		FW_DONTCARE,					// Font Weight
		FALSE,                          // Italic
		FALSE,                          // Underline
		FALSE,                          // Strikeout
		CHINESEBIG5_CHARSET,            // Character Set Identifier
		OUT_DEFAULT_PRECIS,             // Output Precision
		CLIP_DEFAULT_PRECIS,            // Clipping Precision
		DEFAULT_QUALITY,				// Output Quality
		FF_DONTCARE,					// Family And Pitch
		"Sylfaen");						// Font Name

	SelectObject(g_hDC, hfont);
	wglUseFontBitmaps(g_hDC, 0x2d00, 256, font);
	
	GLuint font2 = glGenLists(256);

	HFONT hfont2 = CreateFont(44 * RENDER_HEIGHT / 1080.0f,		// Height Of Font
		0,                              // Width Of Font
		0,                              // Angle Of Escapement
		0,                              // Orientation Angle
		FW_DONTCARE,					// Font Weight
		FALSE,                          // Italic
		FALSE,                          // Underline
		FALSE,                          // Strikeout
		ANSI_CHARSET,             // Character Set Identifier
		OUT_DEFAULT_PRECIS,             // Output Precision
		CLIP_DEFAULT_PRECIS,            // Clipping Precision
		DEFAULT_QUALITY,				// Output Quality
		FF_DONTCARE,					// Family And Pitch
		"Sylfaen");						// Font Name

	SelectObject(g_hDC, hfont2);
	wglUseFontBitmaps(g_hDC, 32, 96, font2);

	VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "";
	applicationInfo.pEngineName = "";
	applicationInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

	const char* vkApplicationExtensions[] = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
		VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
		VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME };

	VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledExtensionCount = 4;
	instanceCreateInfo.ppEnabledExtensionNames = vkApplicationExtensions;

	VkInstance vkInstance;
	vkCreateInstance(&instanceCreateInfo, NULL, &vkInstance);

	uint32_t deviceCount = 0;
	VkPhysicalDevice devices[12];
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, NULL);
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices);

	VkPhysicalDevice physicalDevice = devices[0];
	const uint32_t graphicsFamily = 0;

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalMemoryProperties);

	VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueCreateInfo.queueFamilyIndex = graphicsFamily;
	queueCreateInfo.queueCount = 1;

	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	const char* vkDeviceExtensions[] = {
		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
		VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
		VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
		VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,
		VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
		VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
		VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		VK_KHR_SPIRV_1_4_EXTENSION_NAME,
		VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
	};

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };

	VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.enabledExtensionCount = 16;
	createInfo.ppEnabledExtensionNames = vkDeviceExtensions;

	VkDevice vkDevice;
	vkCreateDevice(physicalDevice, &createInfo, NULL, &vkDevice);
	
	VkQueue vkQueue;
	vkGetDeviceQueue(vkDevice, graphicsFamily, 0, &vkQueue);

	load_GL(sysGetProcAddress);
	
	// Grab some funcs

	PFN_vkCreateAccelerationStructureKHR pfnvkCreateAccelerationStructureKHR;
	pfnvkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(vkDevice, "vkCreateAccelerationStructureKHR");

	PFN_vkCmdBuildAccelerationStructuresKHR pfnvkCmdBuildAccelerationStructuresKHR;
	pfnvkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(vkDevice, "vkCmdBuildAccelerationStructuresKHR");

	PFN_vkGetAccelerationStructureBuildSizesKHR pfnvkGetAccelerationStructureBuildSizesKHR;
	pfnvkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(vkDevice, "vkGetAccelerationStructureBuildSizesKHR");

	PFN_vkGetAccelerationStructureDeviceAddressKHR pfnvkGetAccelerationStructureDeviceAddressKHR;
	pfnvkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(vkDevice, "vkGetAccelerationStructureDeviceAddressKHR");

	PFN_vkCreateRayTracingPipelinesKHR pfnvkCreateRayTracingPipelinesKHR;
	pfnvkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(vkDevice, "vkCreateRayTracingPipelinesKHR");

	PFN_vkCmdTraceRaysKHR pfnvkCmdTraceRaysKHR;
	pfnvkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(vkDevice, "vkCmdTraceRaysKHR");

	PFN_vkGetRayTracingShaderGroupHandlesKHR pfnvkGetRayTracingShaderGroupHandlesKHR;
	pfnvkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(vkDevice, "vkGetRayTracingShaderGroupHandlesKHR");

	typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC) (int interval);
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");;
	wglSwapIntervalEXT(1);

	vkQueueWaitIdle(vkQueue);
	vkDeviceWaitIdle(vkDevice);

	// Create framebuffers
	VkImage colorRT[4];
	VkImageView colorRTView[4];
	for (int i = 0; i < 4; ++i)
	{
		CreateTexture(&colorRT[i], &colorRTView[i], vkDevice, RENDER_WIDTH, RENDER_HEIGHT, 1, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			| VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
	}

	// Create 3D textures
	VkImage texture[4];
	VkImageView textureView[4];
	for (int i = 0; i < 4; ++i)
	{
		CreateTexture(&texture[i], &textureView[i], vkDevice, MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE, VK_IMAGE_TYPE_3D, VK_IMAGE_VIEW_TYPE_3D, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
	}

	VkSamplerCreateInfo vkSamplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	vkSamplerInfo.magFilter = vkSamplerInfo.minFilter = VK_FILTER_LINEAR;
	vkSamplerInfo.addressModeU = vkSamplerInfo.addressModeV = vkSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VkSampler sampler;
	vkCreateSampler(vkDevice, &vkSamplerInfo, NULL, &sampler);
	
	VkSamplerCreateInfo vkSampler3dInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	vkSamplerInfo.magFilter = vkSamplerInfo.minFilter = VK_FILTER_CUBIC_EXT;
	vkSamplerInfo.addressModeU = vkSamplerInfo.addressModeV = vkSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VkSampler sampler3d;
	vkCreateSampler(vkDevice, &vkSampler3dInfo, NULL, &sampler3d);

	VkCommandPoolCreateInfo cmdPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	cmdPoolCreateInfo.queueFamilyIndex = graphicsFamily;
	VkCommandPool vkCommandPool;
	vkCreateCommandPool(vkDevice, &cmdPoolCreateInfo, NULL, &vkCommandPool);
	
	// Create command buffer

	VkCommandBufferAllocateInfo cmdBufAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cmdBufAllocInfo.commandPool = vkCommandPool;
	cmdBufAllocInfo.commandBufferCount = 1;
	cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	VkCommandBuffer vkCommandBuffer;
	vkAllocateCommandBuffers(vkDevice, &cmdBufAllocInfo, &vkCommandBuffer);
	
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCommandBuffer;
	
	// Create dots buffer for double buffering

	VkBuffer dotsBuffer;
	VkDeviceMemory dotsBufferMemory;
	CreateBuffer(&dotsBuffer, &dotsBufferMemory, &vkDevice, NULL, NULL, sizeof(uint32_t) * 3 * DOTS_COUNT * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	// Create acceleration structure

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };

	VkPhysicalDeviceProperties2 prop2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	prop2.pNext = &m_rtProperties;
	vkGetPhysicalDeviceProperties2(physicalDevice, &prop2);

	VkBuffer aabbsBuffer;
	VkDeviceAddress dataAddress;
	uint32_t aabbsBufferSize = sizeof(float) * 6 * AABBS_COUNT;
	VkDeviceMemory aabbsBufferMemory;
	CreateBuffer(&aabbsBuffer, &aabbsBufferMemory, &vkDevice, NULL, NULL, aabbsBufferSize * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); //VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR

	VkBufferDeviceAddressInfo info = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	info.buffer = aabbsBuffer;
	dataAddress = vkGetBufferDeviceAddress(vkDevice, &info);

	VkAccelerationStructureGeometryAabbsDataKHR aabbs = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR };
	aabbs.data.deviceAddress = dataAddress;
	aabbs.stride = sizeof(float) * 6;

	VkAccelerationStructureGeometryKHR asGeom = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	asGeom.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
	asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	asGeom.geometry.aabbs = aabbs;

	VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo[2];
	buildRangeInfo[0].firstVertex = 0;
	buildRangeInfo[0].primitiveCount = (uint32_t)AABBS_COUNT;
	buildRangeInfo[0].primitiveOffset = 0;
	buildRangeInfo[0].transformOffset = 0;

	buildRangeInfo[1].firstVertex = 0;
	buildRangeInfo[1].primitiveCount = (uint32_t)AABBS_COUNT;
	buildRangeInfo[1].primitiveOffset = aabbsBufferSize;
	buildRangeInfo[1].transformOffset = 0;

	VkAccelerationStructureBuildGeometryInfoKHR buildInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	buildInfo.geometryCount = (uint32_t)1;
	buildInfo.pGeometries = &asGeom;

	// Finding sizes to create acceleration structures and scratch
	VkAccelerationStructureBuildSizesInfoKHR sizeInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	(*pfnvkGetAccelerationStructureBuildSizesKHR)(vkDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &buildRangeInfo[0].primitiveCount, &sizeInfo);

	// Create scratch buffer
	VkBuffer scratchBuffer;
	VkDeviceMemory scratchBufferMemory;
	CreateBuffer(&scratchBuffer, &scratchBufferMemory, &vkDevice, NULL, NULL, sizeInfo.buildScratchSize * 4, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkBufferDeviceAddressInfo scratchInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	scratchInfo.buffer = scratchBuffer;
	VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(vkDevice, &scratchInfo);

	// Actual allocation of buffer and acceleration structure.
	VkAccelerationStructureCreateInfoKHR asCreateInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	asCreateInfo.size = sizeInfo.accelerationStructureSize;  // Will be used to allocate memory.

	// Create asBuffer
	VkBuffer asBuffer;
	VkDeviceMemory asBufferMemory;
	CreateBuffer(&asBuffer, &asBufferMemory, &vkDevice, NULL, NULL, asCreateInfo.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	asCreateInfo.buffer = asBuffer;

	// Create the acceleration structure
	VkAccelerationStructureKHR accelerationStructure;
	(*pfnvkCreateAccelerationStructureKHR)(vkDevice, &asCreateInfo, NULL, &accelerationStructure);
	
	// BuildInfo #2 part
	buildInfo.dstAccelerationStructure = accelerationStructure;  // Setting where the build lands
	buildInfo.scratchData.deviceAddress = scratchAddress;  // All build are using the same scratch buffer

	// Clear the aabbs
	vkBeginCommandBuffer(vkCommandBuffer, &beginInfo);
	vkCmdFillBuffer(vkCommandBuffer, aabbsBuffer, 0, aabbsBufferSize*2, 0);
	vkEndCommandBuffer(vkCommandBuffer);
	vkQueueSubmit(vkQueue, 1, &submitInfo, NULL);
	vkQueueWaitIdle(vkQueue);
	vkBeginCommandBuffer(vkCommandBuffer, &beginInfo);

	// Build range information
	const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo[2] = { &buildRangeInfo[0], &buildRangeInfo[1] };

	// Building the bottom-level-acceleration-structure
	(*pfnvkCmdBuildAccelerationStructuresKHR)(vkCommandBuffer, 1, &buildInfo, &rangeInfo[0]);

	// Since the scratch buffer is reused across builds, we need a barrier to ensure one build
	// is finished before starting the next one.
	VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	vkCmdPipelineBarrier(vkCommandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, NULL, 0, NULL);

	vkEndCommandBuffer(vkCommandBuffer);
	vkQueueSubmit(vkQueue, 1, &submitInfo, NULL);
	vkQueueWaitIdle(vkQueue);
	vkBeginCommandBuffer(vkCommandBuffer, &beginInfo);

	// Create the toplevel acceleration structure

	VkAccelerationStructureDeviceAddressInfoKHR addressInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
	addressInfo.accelerationStructure = accelerationStructure;
	VkDeviceAddress address = (*pfnvkGetAccelerationStructureDeviceAddressKHR)(vkDevice, &addressInfo);

	VkTransformMatrixKHR identity = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f
	};

	VkAccelerationStructureInstanceKHR rayInst;
	rayInst.transform = identity;
	rayInst.instanceCustomIndex = 0;
	rayInst.accelerationStructureReference = address;
	rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR | VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
	rayInst.mask = 0xFF;       //  Only be hit if rayMask & instance.mask != 0
	rayInst.instanceShaderBindingTableRecordOffset = 0;  // ???

	VkBuffer instancesBuffer;  // Buffer of instances containing the matrices and BLAS ids
	VkDeviceMemory instancesBufferMemory;
	CreateBuffer(&instancesBuffer, &instancesBufferMemory, &vkDevice, &vkCommandBuffer, &rayInst, sizeof(VkAccelerationStructureInstanceKHR), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkBufferDeviceAddressInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, NULL, instancesBuffer };
	VkDeviceAddress instBufferAddr = vkGetBufferDeviceAddress(vkDevice, &bufferInfo);

	VkMemoryBarrier barrier2 = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier2.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	vkCmdPipelineBarrier(vkCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier2, 0, NULL, 0, NULL);

	vkEndCommandBuffer(vkCommandBuffer);
	vkQueueSubmit(vkQueue, 1, &submitInfo, NULL);
	vkQueueWaitIdle(vkQueue);
	vkBeginCommandBuffer(vkCommandBuffer, &beginInfo);

	// Creating the TLAS
	// Wraps a device pointer to the above uploaded instances.
	VkAccelerationStructureGeometryInstancesDataKHR instancesVk = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
	instancesVk.data.deviceAddress = instBufferAddr;

	// Put the above into a VkAccelerationStructureGeometryKHR. We need to put the instances struct in a union and label it as instance data.
	VkAccelerationStructureGeometryKHR topASGeometry = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	topASGeometry.geometry.instances = instancesVk;

	// Find sizes
	VkAccelerationStructureBuildGeometryInfoKHR tlBuildInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	tlBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	tlBuildInfo.geometryCount = 1;
	tlBuildInfo.pGeometries = &topASGeometry;
	tlBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	tlBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	tlBuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

	uint32_t countInstance = 1;

	VkAccelerationStructureBuildSizesInfoKHR tlSizeInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	(*pfnvkGetAccelerationStructureBuildSizesKHR)(vkDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tlBuildInfo, &countInstance, &tlSizeInfo);

	// Create TLAS
	VkAccelerationStructureCreateInfoKHR aIcreateInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	aIcreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	aIcreateInfo.size = tlSizeInfo.accelerationStructureSize;

	VkBuffer aIBuffer;
	VkDeviceMemory aIBufferMemory;
	CreateBuffer(&aIBuffer, &aIBufferMemory, &vkDevice, NULL, NULL, aIcreateInfo.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	aIcreateInfo.buffer = aIBuffer;

	(*pfnvkCreateAccelerationStructureKHR)(vkDevice, &aIcreateInfo, NULL, &accelerationStructure);
	
	// Allocate the scratch memory
	VkBuffer scratchBuffer2;
	VkDeviceMemory scratchBufferMemory2;
	CreateBuffer(&scratchBuffer2, &scratchBufferMemory2, &vkDevice, NULL, NULL, tlSizeInfo.buildScratchSize * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkBufferDeviceAddressInfo bufferInfo2 = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, NULL, scratchBuffer2 };
	VkDeviceAddress scratchAddress2 = vkGetBufferDeviceAddress(vkDevice, &bufferInfo2);

	// Update build information
	tlBuildInfo.dstAccelerationStructure = accelerationStructure;
	tlBuildInfo.scratchData.deviceAddress = scratchAddress2;

	// Build Offsets info: n instances
	VkAccelerationStructureBuildRangeInfoKHR        buildOffsetInfo = { countInstance, 0, 0, 0 };
	const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

	// Build the TLAS
	(*pfnvkCmdBuildAccelerationStructuresKHR)(vkCommandBuffer, 1, &tlBuildInfo, &pBuildOffsetInfo);

	vkEndCommandBuffer(vkCommandBuffer);
	vkQueueSubmit(vkQueue, 1, &submitInfo, NULL);
	vkQueueWaitIdle(vkQueue);
	
	// Create descriptor set

	VkDescriptorSetLayoutBinding descriptorLayouts[8] = {
		{0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR},
		{1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4, VK_SHADER_STAGE_RAYGEN_BIT_KHR},
		{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, VK_SHADER_STAGE_RAYGEN_BIT_KHR},
		{3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR},
		{4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR},
		{5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR},
		{6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR},
		{7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR}
	};

	VkDescriptorPoolSize descriptorPoolSize[8] = {
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
	};

	VkDescriptorPool descriptorPool;
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = NULL;
	descriptorPoolInfo.maxSets = 1;
	descriptorPoolInfo.poolSizeCount = 8;
	descriptorPoolInfo.pPoolSizes = descriptorPoolSize;
	descriptorPoolInfo.flags = 0;

	vkCreateDescriptorPool(vkDevice, &descriptorPoolInfo, NULL, &descriptorPool);
	
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT }; // VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO
	descriptorSetLayoutCreateInfo.bindingCount = 8;
	descriptorSetLayoutCreateInfo.pBindings = descriptorLayouts;
	descriptorSetLayoutCreateInfo.flags = 0;
	descriptorSetLayoutCreateInfo.pNext = NULL;

	VkDescriptorSetLayout descriptorSetLayout;
	vkCreateDescriptorSetLayout(vkDevice, &descriptorSetLayoutCreateInfo, NULL, &descriptorSetLayout);
	
	VkDescriptorSet descriptorSet;
	VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocateInfo.descriptorPool = descriptorPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &descriptorSetLayout;
	vkAllocateDescriptorSets(vkDevice, &allocateInfo, &descriptorSet);
	
	VkWriteDescriptorSet writeDescriptorSets[8] = {};

	VkWriteDescriptorSetAccelerationStructureKHR descASInfo = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
	descASInfo.accelerationStructureCount = 1;
	descASInfo.pAccelerationStructures = &accelerationStructure;
	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	writeDescriptorSets[0].dstBinding = 0;
	writeDescriptorSets[0].dstSet = descriptorSet;
	writeDescriptorSets[0].dstArrayElement = 0;
	writeDescriptorSets[0].pNext = &descASInfo;

	VkDescriptorImageInfo imageInfo[4] = { { 0, colorRTView[0], VK_IMAGE_LAYOUT_GENERAL }, { 0, colorRTView[1], VK_IMAGE_LAYOUT_GENERAL }, { 0, colorRTView[2], VK_IMAGE_LAYOUT_GENERAL }, { 0, colorRTView[3], VK_IMAGE_LAYOUT_GENERAL } };
	writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[1].descriptorCount = 4;
	writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writeDescriptorSets[1].dstBinding = 1;
	writeDescriptorSets[1].dstSet = descriptorSet;
	writeDescriptorSets[1].dstArrayElement = 0;
	writeDescriptorSets[1].pImageInfo = imageInfo;

	VkDescriptorImageInfo outputSamplerInfo[4] = { { sampler, colorRTView[0], VK_IMAGE_LAYOUT_GENERAL }, { sampler, colorRTView[1], VK_IMAGE_LAYOUT_GENERAL }, { sampler, colorRTView[2], VK_IMAGE_LAYOUT_GENERAL }, { sampler, colorRTView[3], VK_IMAGE_LAYOUT_GENERAL } };
	writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[2].descriptorCount = 4;
	writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[2].dstBinding = 2;
	writeDescriptorSets[2].dstSet = descriptorSet;
	writeDescriptorSets[2].dstArrayElement = 0;
	writeDescriptorSets[2].pImageInfo = outputSamplerInfo;

	VkBuffer globalsBuffer;
	VkDeviceMemory globalsBufferMemory;
	VkBufferUsageFlags globalsBufferFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	CreateBuffer(&globalsBuffer, &globalsBufferMemory, &vkDevice, NULL, NULL, sizeof(Globals), globalsBufferFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkDescriptorBufferInfo globalsBufferInfo = { globalsBuffer, 0, VK_WHOLE_SIZE };
	writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[3].descriptorCount = 1;
	writeDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSets[3].dstBinding = 3;
	writeDescriptorSets[3].dstSet = descriptorSet;
	writeDescriptorSets[3].dstArrayElement = 0;
	writeDescriptorSets[3].pBufferInfo = &globalsBufferInfo;

	VkDescriptorBufferInfo aabbsBufferInfo = { aabbsBuffer, 0, VK_WHOLE_SIZE };
	writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[4].descriptorCount = 1;
	writeDescriptorSets[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writeDescriptorSets[4].dstBinding = 4;
	writeDescriptorSets[4].dstSet = descriptorSet;
	writeDescriptorSets[4].dstArrayElement = 0;
	writeDescriptorSets[4].pBufferInfo = &aabbsBufferInfo;

	VkDescriptorImageInfo textureInfo[4] = { { 0, textureView[0], VK_IMAGE_LAYOUT_GENERAL }, { 0, textureView[1], VK_IMAGE_LAYOUT_GENERAL }, { 0, textureView[2], VK_IMAGE_LAYOUT_GENERAL }, { 0, textureView[3], VK_IMAGE_LAYOUT_GENERAL } };
	writeDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[5].descriptorCount = 4;
	writeDescriptorSets[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writeDescriptorSets[5].dstBinding = 5;
	writeDescriptorSets[5].dstSet = descriptorSet;
	writeDescriptorSets[5].dstArrayElement = 0;
	writeDescriptorSets[5].pImageInfo = textureInfo;

	VkDescriptorImageInfo samplerInfo[4] = { { sampler3d, textureView[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, { sampler3d, textureView[1], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, { sampler3d, textureView[2], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, { sampler3d, textureView[3], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };
	writeDescriptorSets[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[6].descriptorCount = 4;
	writeDescriptorSets[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[6].dstBinding = 6;
	writeDescriptorSets[6].dstSet = descriptorSet;
	writeDescriptorSets[6].dstArrayElement = 0;
	writeDescriptorSets[6].pImageInfo = samplerInfo;

	VkDescriptorBufferInfo dotsBufferInfo = { dotsBuffer, 0, VK_WHOLE_SIZE };
	writeDescriptorSets[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[7].descriptorCount = 1;
	writeDescriptorSets[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writeDescriptorSets[7].dstBinding = 7;
	writeDescriptorSets[7].dstSet = descriptorSet;
	writeDescriptorSets[7].dstArrayElement = 0;
	writeDescriptorSets[7].pBufferInfo = &dotsBufferInfo;

	vkUpdateDescriptorSets(vkDevice, 8, writeDescriptorSets, 0, NULL);

	VkShaderModuleCreateInfo shaderCreateInfo = {};
	shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	VkShaderModule shaderModuleRGEN = VK_NULL_HANDLE;
	shaderCreateInfo.codeSize = sizeof(rgenData);
	shaderCreateInfo.pCode = (const uint32_t*)rgenData;
	vkCreateShaderModule(vkDevice, &shaderCreateInfo, NULL, &shaderModuleRGEN);
	
	VkShaderModule shaderModuleRINT = VK_NULL_HANDLE;
	shaderCreateInfo.codeSize = sizeof(rintData);
	shaderCreateInfo.pCode = (const uint32_t*)rintData;
	vkCreateShaderModule(vkDevice, &shaderCreateInfo, NULL, &shaderModuleRINT);
	
	VkShaderModule shaderModuleRCHIT = VK_NULL_HANDLE;
	shaderCreateInfo.codeSize = sizeof(rchitData);
	shaderCreateInfo.pCode = (const uint32_t*)rchitData;
	vkCreateShaderModule(vkDevice, &shaderCreateInfo, NULL, &shaderModuleRCHIT);
	
	VkPipelineShaderStageCreateInfo shaderStages[3] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

	shaderStages[0].pName = "main";
	shaderStages[0].module = shaderModuleRGEN;
	shaderStages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	shaderStages[1].pName = "main";
	shaderStages[1].module = shaderModuleRCHIT;
	shaderStages[1].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	shaderStages[2].pName = "main";
	shaderStages[2].module = shaderModuleRINT;
	shaderStages[2].stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;

	VkRayTracingShaderGroupCreateInfoKHR shaderGroups[2] = { { VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR }, { VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR } };

	shaderGroups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	shaderGroups[0].generalShader = 0;
	shaderGroups[0].intersectionShader = VK_SHADER_UNUSED_KHR;
	shaderGroups[0].anyHitShader = VK_SHADER_UNUSED_KHR;
	shaderGroups[0].closestHitShader = VK_SHADER_UNUSED_KHR;

	shaderGroups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
	shaderGroups[1].generalShader = VK_SHADER_UNUSED_KHR;
	shaderGroups[1].intersectionShader = 2;
	shaderGroups[1].anyHitShader = VK_SHADER_UNUSED_KHR;
	shaderGroups[1].closestHitShader = 1;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = NULL;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

	VkPipelineLayout pipelineLayout;
	vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, NULL, &pipelineLayout);
	
	VkRayTracingPipelineCreateInfoKHR rayPipelineInfo = { VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
	rayPipelineInfo.stageCount = 3;
	rayPipelineInfo.pStages = shaderStages;
	rayPipelineInfo.groupCount = 2;
	rayPipelineInfo.pGroups = shaderGroups;
	rayPipelineInfo.maxPipelineRayRecursionDepth = 8;
	rayPipelineInfo.layout = pipelineLayout;

	VkPipeline pipeline;
	(*pfnvkCreateRayTracingPipelinesKHR)(vkDevice, 0, 0, 1, &rayPipelineInfo, NULL, &pipeline);
	
	// Create shader binding table

	int handleCount = 2;
	uint32_t handleSize = m_rtProperties.shaderGroupHandleSize;
	uint32_t handleSizeAligned = align_up(handleSize, m_rtProperties.shaderGroupHandleAlignment);
	uint32_t groupSizeAligned = align_up(handleSizeAligned, m_rtProperties.shaderGroupBaseAlignment);

	VkStridedDeviceAddressRegionKHR m_rgenRegion = { 0 };
	VkStridedDeviceAddressRegionKHR m_missRegion = { 0 };
	VkStridedDeviceAddressRegionKHR m_hitRegion = { 0 };
	VkStridedDeviceAddressRegionKHR m_callRegion = { 0 };

	m_rgenRegion.stride = m_rgenRegion.size = groupSizeAligned;
	m_hitRegion.stride = handleSizeAligned;
	m_hitRegion.size = groupSizeAligned;

	uint32_t dataSize = handleCount * handleSize;
	uint8_t* handles = (uint8_t*)malloc(dataSize);
	(*pfnvkGetRayTracingShaderGroupHandlesKHR)(vkDevice, pipeline, 0, handleCount, dataSize, handles);
	
	// Allocate a buffer for storing the SBT.
	VkDeviceSize sbtSize = m_rgenRegion.size + m_missRegion.size + m_hitRegion.size + m_callRegion.size;
	VkBuffer m_rtSBTBuffer;
	VkDeviceMemory m_rtSBTBufferMemory;
	CreateBuffer(&m_rtSBTBuffer, &m_rtSBTBufferMemory, &vkDevice, NULL, NULL, sbtSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Find the SBT addresses of each group
	VkBufferDeviceAddressInfo sbtInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, NULL, m_rtSBTBuffer };
	VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(vkDevice, &sbtInfo);
	m_rgenRegion.deviceAddress = sbtAddress;
	m_hitRegion.deviceAddress = sbtAddress + m_rgenRegion.size;

	uint8_t* pData;
	vkMapMemory(vkDevice, m_rtSBTBufferMemory, 0, VK_WHOLE_SIZE, 0, (void**)&pData);
	
	memcpy(pData, handles, handleSize);
	memcpy(pData + m_rgenRegion.size, handles + handleSize, handleSize);
	
	vkUnmapMemory(vkDevice, m_rtSBTBufferMemory);

	VkMemoryBarrier barrier3 = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	barrier3.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier3.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	IPlayer* player = new RealtimePlayer(&Song, 4);
	player->Play();

	Globals globals;
	globals.time = 0.0f;
	globals.frameId = 0;
	
	while (!GetAsyncKeyState(0x1B))
	{
		double songPos = player->GetSongPos();
		if (songPos >= player->GetLength())
		{
			break;
		}
		
		int minutes = (int)songPos / 60;
		int seconds = (int)songPos % 60;
		int hundredths = (int)(songPos * 100.0) % 100;

		globals.time = songPos + 0.006f;

		// Animate cameras
		// NOTE: it was in the shader, but reached maximum something so needed to put it here
		const int cameraId = int((globals.time - 0.255) * 100.0 / 360.0) % 3;
		float t = (globals.time - 0.255) * 100.0 / 360.0 - cameraId * 3;
		float speed = globals.time * 0.5f;

		if (globals.time < 67.2 || globals.time > 107.6f)
		{
			globals.cameraTarget[0] = -40.0f;
			globals.cameraTarget[1] = 10.0f;
			globals.cameraTarget[2] = -8.0f;
			
			if (cameraId == 0)
			{
				globals.cameraPosition[0] = 300.0f + cos(speed) * 300.0f;
				globals.cameraPosition[1] = -40.0f;
				globals.cameraPosition[2] = 300.0f + sin(speed) * 300.0f;
			}
			else if (cameraId == 1)
			{
				globals.cameraPosition[0] = 150.0f + sin(speed) * 150.0f;
				globals.cameraPosition[1] = -40.0f;
				globals.cameraPosition[0] = 400.0f - t * 100.0f;
			}
			else
			{
				globals.cameraPosition[0] = 250.0f + cos(speed) * 150.0f;
				globals.cameraPosition[1] = sin(speed * 0.5f) * 40.0f;
				globals.cameraPosition[2] = 150.0f + sin(speed) * 50.0f;
			}
		}
		else
		{
			globals.cameraTarget[0] = 50.0f;
			globals.cameraTarget[1] = -10.0f;
			globals.cameraTarget[2] = 20.0f;

			if (cameraId == 0)
			{
				globals.cameraPosition[0] = 300.0f;
				globals.cameraPosition[1] = 300.0f;
				globals.cameraPosition[2] = 300.0f;
			}
			else if (cameraId == 1)
			{
				globals.cameraPosition[0] = 0.0f;
				globals.cameraPosition[1] = 0.0f;
				globals.cameraPosition[2] = 300.0f;
			}
			else
			{
				globals.cameraPosition[0] = 100.0f;
				globals.cameraPosition[1] = 100.0f;
				globals.cameraPosition[2] = 200.0f;
			}
		}

		// RTX on
		vkBeginCommandBuffer(vkCommandBuffer, &beginInfo);
		(*pfnvkCmdBuildAccelerationStructuresKHR)(vkCommandBuffer, 1, &buildInfo, &rangeInfo[(globals.frameId + 1) % 2]);
		vkCmdPipelineBarrier(vkCommandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, NULL, 0, NULL);
		vkCmdPipelineBarrier(vkCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier2, 0, NULL, 0, NULL);
		(*pfnvkCmdBuildAccelerationStructuresKHR)(vkCommandBuffer, 1, &tlBuildInfo, &pBuildOffsetInfo);
		vkCmdUpdateBuffer(vkCommandBuffer, globalsBuffer, 0, sizeof(globals), &globals);
		vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
		vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
		vkCmdPipelineBarrier(vkCommandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 1, &barrier3, 0, 0, 0, 0);
		(*pfnvkCmdTraceRaysKHR)(vkCommandBuffer, &m_rgenRegion, &m_missRegion, &m_hitRegion, &m_callRegion, RENDER_WIDTH, RENDER_HEIGHT, 1);
		vkEndCommandBuffer(vkCommandBuffer);
		vkQueueSubmit(vkQueue, 1, &submitInfo, 0);
		vkQueueWaitIdle(vkQueue);
	
		glDrawVkImageNV((GLuint64)colorRT[globals.frameId % 2], 0, 0, 0, (float)RENDER_WIDTH, (float)RENDER_HEIGHT, 0, 0, 1, 1, 0);
		
		// Texts animation
		float textFade = 0.1f;
		if (globals.time < 9.6f)
		{
			textFade = 0.35f * (float)pow(globals.time / 9.6f, 2.0f);
		}

		glEnable(GL_BLEND);
		glPushAttrib(GL_LIST_BIT);
		glListBase(font - 32);
		for (float i = -0.98f; i <= 0.98f; i += 0.02f)
		{
			float pE = (float)-fmod((i+1.0) * 200.0 + globals.time *(1.0+fmod((i+1.98) * 10.0, 2.1)) + fmod(i, 0.1) * 4.0 + sin(i), 8.0f) + 3.0f;
			float pS = pE + 2.5f;
			
			for (float j = -0.98f; j <= 0.98f; j += 0.02f)
			{
				if (j>pE && j<pS)
				{
					float a = 1.0f-(j-pE)/2.5f;
					
					if (a > 0.95f)
					{
						glColor4f(1.0f, 1.0f, 1.0f, a * textFade);
					}
					else
					{
						glColor4f(0.2f, 1.0f, 0.4f, a * textFade);
					}

					glRasterPos2f(i, j);
					char c = rand();
					glCallLists(1, GL_UNSIGNED_BYTE, &c);
				}
			}
		}
		
		glListBase(font2 - 32);
		glColor4f(0.95f, 1.0f, 0.98f, 0.6f);
		
		if (globals.time <= 7.2f && globals.time >= 4.8f)
		{
			glRasterPos2f(-0.168f, -0.02f);
			glCallLists(16, GL_UNSIGNED_BYTE, "INVOT TEIH MARTX");
		}

		if (globals.time <= 180.0f && globals.time >= 174.0f)
		{
			glRasterPos2f(-0.162f, -0.02f);
			glCallLists(15, GL_UNSIGNED_BYTE, "SKARAB TEO PASY");
		}

		if (globals.time <= 185.0f && globals.time >= 182.5f)
		{
			glRasterPos2f(-0.27f, -0.02f);
			glCallLists(29, GL_UNSIGNED_BYTE, "TEIH SI ON SPOON, REBELS 2022");
		}

		glPopAttrib();
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glDisable(GL_BLEND);
		
		SwapBuffers(g_hDC);
		++globals.frameId;
	}
	exit(0);
	return 0;
}
