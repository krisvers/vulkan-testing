#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <limits>
#include <functional>
#include <vulkan/vulkan.h>
#include "common.hpp"

#define VK_CALL(l) { VkResult vr = l; if (vr != VK_SUCCESS) { if (vr != VK_INCOMPLETE) { std::cerr << "Vulkan call failed: " << vr << "\n"; throw std::runtime_error("Vulkan call failed"); } else { std::cout << "Warning: Vulkan call returned incomplete\n"; } } }
#define VK_UNMAKE_VERSION_MAJOR(v) ((v) >> 22)
#define VK_UNMAKE_VERSION_MINOR(v) (((v) >> 12) & 0x3ff)
#define VK_UNMAKE_VERSION_PATCH(v) ((v) & 0xfff)

#define VK_DEBUG_INFO

struct vulkan_t {
	VkInstance instance;
	VkAllocationCallbacks * allocator;
	VkPhysicalDevice physical;
	VkDevice device;
};

void vk_init(vulkan_t & vulkan);
void vk_deinit(vulkan_t & vulkan);

int main(int argc, char ** argv) {
	vulkan_t vulkan = {
		.instance = VK_NULL_HANDLE,
		.allocator = nullptr,
		.physical = VK_NULL_HANDLE,
		.device = VK_NULL_HANDLE,
	};


	vk_init(vulkan);
	vk_deinit(vulkan);

	return 0;
}

void vk_init(vulkan_t & vulkan) {
	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "krisvers' Vulkan playground",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "none",
		.engineVersion = VK_MAKE_VERSION(1, 3, 0),
	};

	std::vector<layer_t> requested_layers;
#ifdef _DEBUG
	requested_layers.push_back({ "VK_LAYER_KHRONOS_validation", false });
#endif

	std::vector<extension_t> requested_extensions;

	requested_extensions.push_back({ VK_KHR_SURFACE_EXTENSION_NAME, true });
	//requested_extensions.push_back({ "VK_KHR_win32_surface", true });

	vk_create_instance(&vulkan.instance, vulkan.allocator, app_info, requested_layers, requested_extensions);

	vk_create_device(
		vulkan.instance,
		&vulkan.device, &vulkan.physical,
		vulkan.allocator, nullptr,
		requested_layers, requested_extensions,

		[](VkPhysicalDevice & device) {
			int32_t score = 0;

			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(device, &props);

			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				score += 1000;
			}

			return score;
		}
	);
}

void vk_deinit(vulkan_t & vulkan) {
	vkDestroyInstance(vulkan.instance, vulkan.allocator);
}

void vk_create_instance(VkInstance * instance, VkAllocationCallbacks * allocator, VkApplicationInfo app_info, std::vector<layer_t> & requested_layers, std::vector<extension_t> & requested_extensions) {

	uint32_t layer_prop_count = 0;
	VK_CALL(vkEnumerateInstanceLayerProperties(&layer_prop_count, nullptr));

	std::vector<VkLayerProperties> layer_properties(layer_prop_count);
	VK_CALL(vkEnumerateInstanceLayerProperties(&layer_prop_count, layer_properties.data()));

	std::vector<const char *> layer_names;
#ifdef VK_DEBUG_INFO
	std::cout << "Requested Vulkan instance layers:\n";
	for (layer_t & layer : requested_layers) {
		std::cout << "    " << (layer.required ? "(required) " : "") << layer.name << "\n";
	}
	std::cout << '\n';
#endif

	for (layer_t & layer : requested_layers) {
		bool found = false;
		for (VkLayerProperties & prop : layer_properties) {
			if (strcmp(prop.layerName, layer.name) == 0) {
				found = true;
				layer_names.push_back(layer.name);
				break;
			}
		}

		if (!found) {
			if (layer.required) {
				std::cout << "Failed to find required Vulkan instance layer: " << layer.name << "\n";
				throw std::runtime_error("Failed to find required Vulkan instance layer");
			}
		}
	}

#ifdef VK_DEBUG_INFO
	std::cout << "Vulkan instance layers:\n";
	for (VkLayerProperties & prop : layer_properties) {
		char c = ' ';
		for (const char *& name : layer_names) {
			if (strcmp(prop.layerName, name) == 0) {
				c = 'X';
				break;
			}
		}

		std::cout << '[' << c << ']' << " " << prop.layerName << "\n";
		std::cout << "     " << prop.description << "\n";
		std::cout << "     Specification version: " << VK_UNMAKE_VERSION_MAJOR(prop.specVersion) << "." << VK_UNMAKE_VERSION_MINOR(prop.specVersion) << "." << VK_UNMAKE_VERSION_PATCH(prop.specVersion) << ", implementation version: " << VK_UNMAKE_VERSION_MAJOR(prop.implementationVersion) << "." << VK_UNMAKE_VERSION_MINOR(prop.implementationVersion) << "." << VK_UNMAKE_VERSION_PATCH(prop.implementationVersion) << "\n\n";
	}
#endif

	uint32_t extension_prop_count = 0;
	VK_CALL(vkEnumerateInstanceExtensionProperties(nullptr, &extension_prop_count, nullptr));

	std::vector<VkExtensionProperties> extension_props(extension_prop_count);
	VK_CALL(vkEnumerateInstanceExtensionProperties(nullptr, &extension_prop_count, extension_props.data()));

	std::vector<const char *> extension_names;
#ifdef VK_DEBUG_INFO
	std::cout << "Requested Vulkan instance extension:\n";
	for (extension_t & extension : requested_extensions) {
		std::cout << "    " << (extension.required ? "(required) " : "") << extension.name << "\n";
	}
	std::cout << '\n';
#endif

	for (extension_t & extension : requested_extensions) {
		bool found = false;
		for (VkExtensionProperties & prop : extension_props) {
			if (strcmp(prop.extensionName, extension.name) == 0) {
				found = true;
				extension_names.push_back(extension.name);
				break;
			}
		}


		if (!found) {
			if (extension.required) {
				std::cout << "Failed to find required Vulkan instance extension: " << extension.name << "\n";
				throw std::runtime_error("Failed to find required Vulkan instance extension");
			}
		}
	}

#ifdef VK_DEBUG_INFO
	std::cout << "Vulkan instance extensions:\n";
	for (VkExtensionProperties & prop : extension_props) {
		char c = ' ';
		for (const char *& name : extension_names) {
			if (strcmp(prop.extensionName, name) == 0) {
				c = 'X';
				break;
			}
		}

		std::cout << '[' << c << ']' << " " << prop.extensionName << "\n";
		std::cout << "     Specification version: " << VK_UNMAKE_VERSION_MAJOR(prop.specVersion) << "." << VK_UNMAKE_VERSION_MINOR(prop.specVersion) << "." << VK_UNMAKE_VERSION_PATCH(prop.specVersion) << "\n\n";
	}
#endif

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &app_info,
		.enabledLayerCount = static_cast<uint32_t>(layer_names.size()),
		.ppEnabledLayerNames = layer_names.data(),
		.enabledExtensionCount = static_cast<uint32_t>(extension_names.size()),
		.ppEnabledExtensionNames = extension_names.data(),
	};

	VK_CALL(vkCreateInstance(&create_info, allocator, instance));
}

void vk_create_device(VkInstance instance, VkDevice * device, VkPhysicalDevice * physical_device, VkAllocationCallbacks * allocator, VkSurfaceKHR * surface, std::vector<layer_t> & requested_layers, std::vector<extension_t> & requested_extensions, std::function<int32_t(VkPhysicalDevice &)> score_func) {
	uint32_t physical_count = 0;
	vkEnumeratePhysicalDevices(instance, &physical_count, nullptr);

	if (physical_count == 0) {
		std::cout << "Failed to find any Vulkan physical devices\n";
		throw std::runtime_error("Failed to find any Vulkan physical devices");
	}

	std::vector<VkPhysicalDevice> physical_devices(physical_count);
	vkEnumeratePhysicalDevices(instance, &physical_count, physical_devices.data());

	int32_t best_score = -1;
	size_t best_index = std::numeric_limits<size_t>::max();
	for (size_t i = 0; i < physical_devices.size(); ++i) {
		int32_t score = score_func(physical_devices[i]);
		if (score == -1) {
			continue;
		}
		if (score > best_score) {
			best_score = score;
			best_index = i;
		}
	}

	#ifdef VK_DEBUG_INFO
	std::cout << "Vulkan physical devices:\n";
	for (VkPhysicalDevice & dev : physical_devices) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(dev, &props);

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_family_props(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, queue_family_props.data());

		char c = ' ';
		if (&physical_devices[best_index] == &dev) {
			c = 'X';
		}
		std::cout << "[" << c << "] " << props.deviceName;
		switch (props.deviceType) {
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:
				std::cout << " (Other)\n";
				break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				std::cout << " (Integrated GPU)\n";
				break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				std::cout << " (Discrete GPU)\n";
				break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
				std::cout << " (Virtual GPU)\n";
				break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:
				std::cout << " (CPU)\n";
				break;
			default:
				std::cout << " (Unknown)\n";
				break;
		}

		std::cout << "     Vendor ID: " << props.vendorID << "\n";
		std::cout << "     Device ID: " << props.deviceID << "\n";
		std::cout << "     API version: " << VK_UNMAKE_VERSION_MAJOR(props.apiVersion) << "." << VK_UNMAKE_VERSION_MINOR(props.apiVersion) << "." << VK_UNMAKE_VERSION_PATCH(props.apiVersion) << "\n";
		std::cout << "     Driver version: " << VK_UNMAKE_VERSION_MAJOR(props.driverVersion) << "." << VK_UNMAKE_VERSION_MINOR(props.driverVersion) << "." << VK_UNMAKE_VERSION_PATCH(props.driverVersion) << "\n";
		std::cout << "     Limits:\n";
		std::cout << "       Max image dimension 1D: " << props.limits.maxImageDimension1D << "\n";
		std::cout << "       Max image dimension 2D: " << props.limits.maxImageDimension2D << "\n";
		std::cout << "       Max image dimension 3D: " << props.limits.maxImageDimension3D << "\n\n";
		std::cout << "     Queue families:\n";
		for (size_t i = 0; i < queue_family_props.size(); ++i) {
			VkQueueFamilyProperties & prop = queue_family_props[i];
			std::cout << "       Family " << i << "\n";
			std::cout << "         Count: " << prop.queueCount << "\n";
			char c = ' ';
			if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				c = 'X';
			}
			std::cout << "           [" << c << "] Graphics\n";

			c = ' ';
			if (prop.queueFlags & VK_QUEUE_COMPUTE_BIT) {
				c = 'X';
			}
			std::cout << "           [" << c << "] Compute\n";

			c = ' ';
			if (prop.queueFlags & VK_QUEUE_TRANSFER_BIT) {
				c = 'X';
			}
			std::cout << "           [" << c << "] Transfer\n";

			c = ' ';
			if (prop.queueFlags & VK_QUEUE_PROTECTED_BIT) {
				c = 'X';
			}
			std::cout << "           [" << c << "] Protected\n";

			c = ' ';
			if (prop.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
				c = 'X';
			}
			std::cout << "           [" << c << "] Sparse Binding\n";
		}
	}
	#endif
}