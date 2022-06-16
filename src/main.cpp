#include <vulkan/vulkan.hpp>
#include <iostream>
#include <fstream>


int main(int argc, char *argv[]) {

////////////////////////////////////////////////////////////////////////
//                          VULKAN INSTANCE                           //
////////////////////////////////////////////////////////////////////////
	vk::ApplicationInfo AppInfo{
		"VulkanCompute",      // Application Name
		1,                    // Application Version
		nullptr,              // Engine Name or nullptr
		0,                    // Engine Version
		VK_API_VERSION_1_1    // Vulkan API version
	};

	const std::vector<const char*> Layers = { "VK_LAYER_KHRONOS_validation" };
	vk::InstanceCreateInfo InstanceCreateInfo(
			vk::InstanceCreateFlags(), // Flags
			&AppInfo,                  // Application Info
			Layers.size(),             // Layers count
			Layers.data()              // Layers
			);
	vk::Instance Instance = vk::createInstance(InstanceCreateInfo);


////////////////////////////////////////////////////////////////////////
//                          PHYSICAL DEVICE                           //
////////////////////////////////////////////////////////////////////////
	vk::PhysicalDevice PhysicalDevice = Instance.enumeratePhysicalDevices().front();
	vk::PhysicalDeviceProperties DeviceProps = PhysicalDevice.getProperties();
	std::cout << "Device Name    : " << DeviceProps.deviceName << std::endl;
	const uint32_t ApiVersion = DeviceProps.apiVersion;
	std::cout << "Vulkan Version : " << VK_VERSION_MAJOR(ApiVersion) << "." << VK_VERSION_MINOR(ApiVersion) << "." << VK_VERSION_PATCH(ApiVersion) << std::endl;
	// vk::PhysicalDeviceLimits DeviceLimits = DeviceProps.limits;
	// std::cout << "Max Compute Shared Memory Size: " << DeviceLimits.maxComputeSharedMemorySize / 1024 << " KB" << std::endl;


////////////////////////////////////////////////////////////////////////
//                            QUEUE FAMILY                            //
////////////////////////////////////////////////////////////////////////
	std::vector<vk::QueueFamilyProperties> QueueFamilyProps = PhysicalDevice.getQueueFamilyProperties();
	auto PropIt = std::find_if(QueueFamilyProps.begin(), QueueFamilyProps.end(), [](const vk::QueueFamilyProperties& Prop) {
		return Prop.queueFlags & vk::QueueFlagBits::eCompute;
	});
	const uint32_t ComputeQueueFamilyIndex = std::distance(QueueFamilyProps.begin(), PropIt);
	std::cout << "Compute Queue Family Index: " << ComputeQueueFamilyIndex << std::endl;


////////////////////////////////////////////////////////////////////////
//                               DEVICE                               //
////////////////////////////////////////////////////////////////////////
	float queuePriorities = 1.0f;
	vk::DeviceQueueCreateInfo DeviceQueueCreateInfo(
			vk::DeviceQueueCreateFlags(),   // Flags
			ComputeQueueFamilyIndex,        // Queue Family Index
			1,                              // Number of Queues
			&queuePriorities
			);
	vk::DeviceCreateInfo DeviceCreateInfo(
			vk::DeviceCreateFlags(),   // Flags
			1,
			&DeviceQueueCreateInfo      // Device Queue Create Info struct
			);
	vk::Device Device = PhysicalDevice.createDevice(DeviceCreateInfo);

////////////////////////////////////////////////////////////////////////
//                         Allocating Memory                          //
////////////////////////////////////////////////////////////////////////
	// Create the required buffers for the application
    // Allocate the memory to back the buffers
    // Bind the buffers to the memory
	
	// Create buffers
	const uint32_t NumElements = 10;
	const uint32_t BufferSize = NumElements * sizeof(int32_t);

	vk::BufferCreateInfo BufferCreateInfo{
		vk::BufferCreateFlags(),                    // Flags
		BufferSize,                                 // Size
		vk::BufferUsageFlagBits::eStorageBuffer,    // Usage
		vk::SharingMode::eExclusive,                // Sharing mode
		1,                                          // Number of queue family indices
		&ComputeQueueFamilyIndex                    // List of queue family indices
	};
	vk::Buffer InBuffer = Device.createBuffer(BufferCreateInfo);
	vk::Buffer OutBuffer = Device.createBuffer(BufferCreateInfo);

	// Memory req
	vk::MemoryRequirements InBufferMemoryRequirements = Device.getBufferMemoryRequirements(InBuffer);
	vk::MemoryRequirements OutBufferMemoryRequirements = Device.getBufferMemoryRequirements(OutBuffer);

	// query
	vk::PhysicalDeviceMemoryProperties MemoryProperties = PhysicalDevice.getMemoryProperties();

	uint32_t MemoryTypeIndex = uint32_t(~0);
	vk::DeviceSize MemoryHeapSize = uint32_t(~0);
	for (uint32_t CurrentMemoryTypeIndex = 0; CurrentMemoryTypeIndex < MemoryProperties.memoryTypeCount; ++CurrentMemoryTypeIndex)
	{
		vk::MemoryType MemoryType = MemoryProperties.memoryTypes[CurrentMemoryTypeIndex];
		if ((vk::MemoryPropertyFlagBits::eHostVisible & MemoryType.propertyFlags) &&
			(vk::MemoryPropertyFlagBits::eHostCoherent & MemoryType.propertyFlags))
		{
			MemoryHeapSize = MemoryProperties.memoryHeaps[MemoryType.heapIndex].size;
			MemoryTypeIndex = CurrentMemoryTypeIndex;
			break;
		}
	}

	std::cout << "Memory Type Index: " << MemoryTypeIndex << std::endl;
	std::cout << "Memory Heap Size : " << MemoryHeapSize / 1024 / 1024 / 1024 << " GB" << std::endl;

	// Allocate memory
	vk::MemoryAllocateInfo InBufferMemoryAllocateInfo(InBufferMemoryRequirements.size, MemoryTypeIndex);
	vk::MemoryAllocateInfo OutBufferMemoryAllocateInfo(OutBufferMemoryRequirements.size, MemoryTypeIndex);
	vk::DeviceMemory InBufferMemory = Device.allocateMemory(InBufferMemoryAllocateInfo);
	vk::DeviceMemory OutBufferMemory = Device.allocateMemory(InBufferMemoryAllocateInfo);

	// Map memory and write
	int32_t* InBufferPtr = static_cast<int32_t*>(Device.mapMemory(InBufferMemory, 0, BufferSize));
	for (uint32_t I = 0; I < NumElements; ++I) {
		InBufferPtr[I] = I;
	}
	Device.unmapMemory(InBufferMemory);

	// Bind buffers to memory
	Device.bindBufferMemory(InBuffer, InBufferMemory, 0);
	Device.bindBufferMemory(OutBuffer, OutBufferMemory, 0);

////////////////////////////////////////////////////////////////////////
//                              PIPELINE                              //
////////////////////////////////////////////////////////////////////////

	// Shader module
	std::vector<char> ShaderContents;
	if (std::ifstream ShaderFile{ "shaders/shader.comp.spv", std::ios::binary | std::ios::ate }) {
		const size_t FileSize = ShaderFile.tellg();
		ShaderFile.seekg(0);
		ShaderContents.resize(FileSize, '\0');
		ShaderFile.read(ShaderContents.data(), FileSize);
	}

	vk::ShaderModuleCreateInfo ShaderModuleCreateInfo(
		vk::ShaderModuleCreateFlags(),                                // Flags
		ShaderContents.size(),                                        // Code size
		reinterpret_cast<const uint32_t*>(ShaderContents.data()));    // Code
    vk::ShaderModule ShaderModule = Device.createShaderModule(ShaderModuleCreateInfo);

	// Descriptor Set Layout
	// The layout of data to be passed to pipelin
	const std::vector<vk::DescriptorSetLayoutBinding> DescriptorSetLayoutBinding = {
		{0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
		{1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}
	};
	vk::DescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(
		vk::DescriptorSetLayoutCreateFlags(),
		DescriptorSetLayoutBinding);
	vk::DescriptorSetLayout DescriptorSetLayout = Device.createDescriptorSetLayout(DescriptorSetLayoutCreateInfo);

	// Pipeline Layout
	vk::PipelineLayoutCreateInfo PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), DescriptorSetLayout);
	vk::PipelineLayout PipelineLayout = Device.createPipelineLayout(PipelineLayoutCreateInfo);
	vk::PipelineCache PipelineCache = Device.createPipelineCache(vk::PipelineCacheCreateInfo());

	// Compute Pipeline
	vk::PipelineShaderStageCreateInfo PipelineShaderCreateInfo(
		vk::PipelineShaderStageCreateFlags(),  // Flags
		vk::ShaderStageFlagBits::eCompute,     // Stage
		ShaderModule,                          // Shader Module
		"main"                                 // Shader Entry Point
		);
	vk::ComputePipelineCreateInfo ComputePipelineCreateInfo(
		vk::PipelineCreateFlags(),    // Flags
		PipelineShaderCreateInfo,     // Shader Create Info struct
		PipelineLayout                // Pipeline Layout
		);
	vk::Pipeline ComputePipeline = Device.createComputePipeline(PipelineCache, ComputePipelineCreateInfo).value;

////////////////////////////////////////////////////////////////////////
//                          DESCRIPTOR SETS                           //
////////////////////////////////////////////////////////////////////////
	// Descriptor sets must be allocated in a vk::DescriptorPool, so we need to create one first
	vk::DescriptorPoolSize DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 2);
	vk::DescriptorPoolCreateInfo DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlags(), 1, DescriptorPoolSize);
	vk::DescriptorPool DescriptorPool = Device.createDescriptorPool(DescriptorPoolCreateInfo);

	// Allocate descriptor sets, update them to use buffers:
	vk::DescriptorSetAllocateInfo DescriptorSetAllocInfo(DescriptorPool, 1, &DescriptorSetLayout);
	const std::vector<vk::DescriptorSet> DescriptorSets = Device.allocateDescriptorSets(DescriptorSetAllocInfo);
	vk::DescriptorSet DescriptorSet = DescriptorSets.front();
	vk::DescriptorBufferInfo InBufferInfo(InBuffer, 0, NumElements * sizeof(int32_t));
	vk::DescriptorBufferInfo OutBufferInfo(OutBuffer, 0, NumElements * sizeof(int32_t));

	const std::vector<vk::WriteDescriptorSet> WriteDescriptorSets = {
		{DescriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &InBufferInfo},
		{DescriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &OutBufferInfo},
	};
	Device.updateDescriptorSets(WriteDescriptorSets, {});

////////////////////////////////////////////////////////////////////////
//                         SUBMIT WORK TO GPU                         //
////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
	// Command Pool
	vk::CommandPoolCreateInfo CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), ComputeQueueFamilyIndex);
	vk::CommandPool CommandPool = Device.createCommandPool(CommandPoolCreateInfo);
	// Allocate Command buffer from Pool
	vk::CommandBufferAllocateInfo CommandBufferAllocInfo(
    CommandPool,                         // Command Pool
    vk::CommandBufferLevel::ePrimary,    // Level
    1);                                  // Num Command Buffers
	const std::vector<vk::CommandBuffer> CmdBuffers = Device.allocateCommandBuffers(CommandBufferAllocInfo);
	vk::CommandBuffer CmdBuffer = CmdBuffers.front();

	// Record commands
	vk::CommandBufferBeginInfo CmdBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	CmdBuffer.begin(CmdBufferBeginInfo);
	CmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, ComputePipeline);
	CmdBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eCompute,    // Bind point
			PipelineLayout,                  // Pipeline Layout
			0,                               // First descriptor set
			{ DescriptorSet },               // List of descriptor sets
			{});                             // Dynamic offsets
	CmdBuffer.dispatch(NumElements, 1, 1);
	CmdBuffer.end();

	// Fence and submit
	vk::Queue Queue = Device.getQueue(ComputeQueueFamilyIndex, 0);
	vk::Fence Fence = Device.createFence(vk::FenceCreateInfo());
	vk::SubmitInfo SubmitInfo(
			0,                // Num Wait Semaphores
			nullptr,        // Wait Semaphores
			nullptr,        // Pipeline Stage Flags
			1,              // Num Command Buffers
			&CmdBuffer);    // List of command buffers
	Queue.submit({ SubmitInfo }, Fence);
	(void) Device.waitForFences(
			{ Fence },             // List of fences
			true,               // Wait All
			uint64_t(-1));      // Timeout

	// Map output buffer and read results
	InBufferPtr = static_cast<int32_t*>(Device.mapMemory(InBufferMemory, 0, BufferSize));
	std::cout << std::endl;
	std::cout << "INPUT:  ";
	for (uint32_t I = 0; I < NumElements; ++I) {
		std::cout << InBufferPtr[I] << " ";
	}
	std::cout << std::endl;
	Device.unmapMemory(InBufferMemory);

	int32_t* OutBufferPtr = static_cast<int32_t*>(Device.mapMemory(OutBufferMemory, 0, BufferSize));
	std::cout << "OUTPUT: ";
	for (uint32_t I = 0; I < NumElements; ++I) {
		std::cout << OutBufferPtr[I] << " ";
	}
	std::cout << std::endl;
	Device.unmapMemory(OutBufferMemory);
////////////////////////////////////////////////////////////////////////
//                              CLEANUP                               //
////////////////////////////////////////////////////////////////////////
	Device.resetCommandPool(CommandPool, vk::CommandPoolResetFlags());
	Device.destroyFence(Fence);
	Device.destroyDescriptorSetLayout(DescriptorSetLayout);
	Device.destroyPipelineLayout(PipelineLayout);
	Device.destroyPipelineCache(PipelineCache);
	Device.destroyShaderModule(ShaderModule);
	Device.destroyPipeline(ComputePipeline);
	Device.destroyDescriptorPool(DescriptorPool);
	Device.destroyCommandPool(CommandPool);
	Device.freeMemory(InBufferMemory);
	Device.freeMemory(OutBufferMemory);
	Device.destroyBuffer(InBuffer);
	Device.destroyBuffer(OutBuffer);
	Device.destroy();
	Instance.destroy();
    return 0;
}
