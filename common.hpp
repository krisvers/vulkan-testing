#ifndef COMMON_HPP
#define COMMON_HPP

#include <vulkan/vulkan.h>

struct extension_t {
	const char * name;
	bool required;
};

typedef extension_t layer_t;

void vk_create_instance(VkInstance * instance, VkAllocationCallbacks * allocator, VkApplicationInfo app_info, std::vector<layer_t> & vk_requested_layer_names, std::vector<extension_t> & vk_requested_extension_names);
void vk_create_device(VkInstance instance, VkDevice * device, VkPhysicalDevice * physical_device, VkAllocationCallbacks * allocator, VkSurfaceKHR * surface, std::vector<layer_t> & requested_layers, std::vector<extension_t> & requested_extensions, std::function<int32_t(VkPhysicalDevice &)> score_func);

#endif