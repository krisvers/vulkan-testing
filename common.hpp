#ifndef COMMON_HPP
#define COMMON_HPP

#include <array>
#include <vulkan/vulkan.h>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct extension_t {
	const char * name;
	bool required;
};

typedef extension_t layer_t;

struct vulkan_t {
	VkInstance instance;
	VkAllocationCallbacks * allocator;
	VkPhysicalDevice physical;
	VkDevice device;
	VkSurfaceKHR surface;
	VkSurfaceCapabilitiesKHR surface_capabilities;

	VkViewport viewport;
	VkRect2D scissor;
	VkFormat swapchain_format;
	VkExtent2D swapchain_extent;
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchain_images;
	std::vector<VkImageView> swapchain_views;
	std::vector<VkFramebuffer> framebuffers;
	uint32_t swapchain_image_count = 0;

	VkCommandPool cmd_pool;
	std::vector<VkCommandBuffer> cmd_buffers;
	std::vector<VkSemaphore> semaphores_img_avail;
	std::vector<VkSemaphore> semaphores_render_finished;
	std::vector<VkFence> fences_flight;

	VkShaderModule vertex_shader;
	VkShaderModule fragment_shader;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	VkRenderPass render_pass;

	VkBuffer mesh_buffer;
	VkDeviceMemory mesh_memory;
	uint32_t mesh_vertex_count;
	uint32_t mesh_index_count;

	VkQueue present_queue;
	VkQueue graphics_queue;

	uint32_t graphics_family;
	uint32_t present_family;

	uint32_t frames_in_flight = 1;
	uint32_t current_frame = 0;
	double target_fps = 144;

	uint32_t window_width;
	uint32_t window_height;
	HWND hwnd;
	HINSTANCE hinstance;

	VkDebugUtilsMessengerEXT debug_messenger;
};

struct vertex_t {
	struct { float x, y, z; } pos;
	struct { float r, g, b; } color;
};

inline VkVertexInputBindingDescription vertex_binding_desc() {
	VkVertexInputBindingDescription binding_desc = {
		.binding = 0,
		.stride = sizeof(vertex_t),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	return binding_desc;
}

inline std::array<VkVertexInputAttributeDescription, 2> vertex_attr_desc() {
	std::array<VkVertexInputAttributeDescription, 2> attr_descs;
	attr_descs[0] = {
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(vertex_t, pos),
	};
	attr_descs[1] = {
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(vertex_t, color),
	};

	return attr_descs;
}

inline uint32_t vk_memory_type(vulkan_t & vulkan, uint32_t type_filter, VkMemoryPropertyFlags props) {
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(vulkan.physical, &mem_props);

	for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
		if (type_filter & (1 << i) && (mem_props.memoryTypes[i].propertyFlags & props) == props) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find Vulkan memory type");
}

void vk_create_instance(vulkan_t & vulkan, VkApplicationInfo app_info, const std::vector<layer_t> & vk_requested_layer_names, const std::vector<extension_t> & vk_requested_extension_names);
void vk_create_physical(vulkan_t & vulkan, std::function<int64_t(const VkPhysicalDevice &)> score_func);
void vk_create_device(vulkan_t & vulkan, std::vector<layer_t> & requested_layers, std::vector<extension_t> & requested_extensions);
void vk_create_surface(vulkan_t & vulkan);
void vk_create_swapchain(vulkan_t & vulkan, std::function<size_t(const std::vector<VkSurfaceFormatKHR> &)> choose_fmt_func, std::function<size_t(const std::vector<VkPresentModeKHR> &)> choose_mode_func, std::function<VkExtent2D(const VkSurfaceCapabilitiesKHR &)> choose_extent_func, std::function<uint32_t(uint32_t, uint32_t)> choose_image_count_func);
void vk_init_pipeline(vulkan_t & vulkan, const std::vector<unsigned char> & vertex_spv, const std::vector<unsigned char> & fragment_spv);
void vk_init_render_pass(vulkan_t & vulkan);
void vk_init_framebuffers(vulkan_t & vulkan);
void vk_create_command_utils(vulkan_t & vulkan);
void vk_create_semaphores(vulkan_t & vulkan);
void vk_create_vbo(vulkan_t & vulkan);
VkBuffer vk_create_buffer(vulkan_t & vulkan, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceMemory & memory);
void vk_copy_buffer(vulkan_t & vulkan, VkBuffer src, VkBuffer dst, VkDeviceSize size);

void vk_recreate_swapchain(vulkan_t & vulkan);

void vk_frames_in_flight(vulkan_t & vulkan, uint32_t new_count);

void vk_begin_cmd(vulkan_t & vulkan, VkCommandBuffer & buffer);
void vk_end_cmd(vulkan_t & vulkan, VkCommandBuffer & buffer);

#endif