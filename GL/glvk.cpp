#include "glvk.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>
#include <format>

#define GLVK_DEBUG(msg, sev, type) if (state.debug && state.debugfun != nullptr) { state.debugfun(msg, sev, type); }
#define GLVK_DEBUGF(sev, type, fmt, ...) GLVK_DEBUG(std::format(fmt, __VA_ARGS__).c_str(), sev, type)

struct {
	struct {
		VkInstance instance;
		VkAllocationCallbacks * allocator;
		VkPhysicalDevice physical;
		VkDevice device;
		VkSurfaceKHR surface;
		VkSwapchainKHR swapchain;

		struct {
			uint32_t graphics;
			uint32_t present;
		} qfamilies;
	} vulkan;

	VkViewport viewport;
	HWND hwnd;

	GLboolean debug;
	GLVKdebugfun debugfun;
} state = {
	.vulkan = {
		.instance = VK_NULL_HANDLE,
	},

	.viewport = {
		.x = 0,
		.y = 0,
		.width = 0,
		.height = 0,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	},
	.hwnd = nullptr,
	.debug = GL_FALSE,
	.debugfun = nullptr,
};

void glvkDebug() {
	state.debug = !state.debug;
}

void glvkRegisterDebugCallback(GLVKdebugfun debugfun) {
	state.debugfun = debugfun;
}

struct extension_t {
	const char * name;
	bool required;
};

typedef extension_t layer_t;

GLboolean glvkInit(void * window) {
	if (window == nullptr) {
		return GL_FALSE;
	}

	HWND hwnd = reinterpret_cast<HWND>(window);
	if (IsWindow(hwnd) == FALSE) {
		GLVK_DEBUG("WIN32 window handle is invalid", GLVK_SEVERITY_ERROR, GLVK_TYPE_GLVK);
		return GL_FALSE;
	}

	state.hwnd = hwnd;

	{
		VkApplicationInfo app_info = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "glvk",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "none",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_MAKE_VERSION(1, 3, 0),
		};

		std::vector<layer_t> requested_layers;
		std::vector<extension_t> requested_extensions = {
			{ VK_KHR_SURFACE_EXTENSION_NAME, true },
			{ VK_KHR_WIN32_SURFACE_EXTENSION_NAME, true },
		};

		if (state.debug) {
			requested_layers.push_back({ "VK_LAYER_KHRONOS_validation", false });
			requested_extensions.push_back({ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, false });
		}

		uint32_t layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> layer_props(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, layer_props.data());

		std::vector<const char *> layers;
		for (const layer_t & layer : requested_layers) {
			bool found = false;
			for (const VkLayerProperties & prop : layer_props) {
				if (strcmp(prop.layerName, layer.name) == 0) {
					found = true;
					layers.push_back(layer.name);
				}
			}

			if (layer.required && !found) {
				GLVK_DEBUGF(GLVK_SEVERITY_ERROR, GLVK_TYPE_GLVK, "Failed to find required Vulkan instance layer {}", layer.name);
				return GL_FALSE;
			}
			else if (!found) {
				GLVK_DEBUGF(GLVK_SEVERITY_INFO, GLVK_TYPE_GLVK, "Failed to find Vulkan instance layer {}", layer.name);
			}
		}

		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> extension_props(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extension_props.data());

		std::vector<const char *> extensions;
		for (const extension_t & extension : requested_extensions) {
			bool found = false;
			for (const VkExtensionProperties & prop : extension_props) {
				if (strcmp(prop.extensionName, extension.name) == 0) {
					found = true;
					extensions.push_back(extension.name);
				}
			}

			if (extension.required && !found) {
				GLVK_DEBUGF(GLVK_SEVERITY_ERROR, GLVK_TYPE_GLVK, "Failed to find required Vulkan instance extension {}", extension.name);
				return GL_FALSE;
			}
			else if (!found) {
				GLVK_DEBUGF(GLVK_SEVERITY_INFO, GLVK_TYPE_GLVK, "Failed to find Vulkan instance extension {}", extension.name);
			}
		}

		VkInstanceCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pApplicationInfo = &app_info,
			.enabledLayerCount = static_cast<uint32_t>(layers.size()),
			.ppEnabledLayerNames = layers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data(),
		};

		if (vkCreateInstance(&create_info, state.vulkan.allocator, &state.vulkan.instance) != VK_SUCCESS) {
			GLVK_DEBUG("Failed to create Vulkan instance", GLVK_SEVERITY_ERROR, GLVK_TYPE_VULKAN);
			return GL_FALSE;
		}
	}

	{
		VkWin32SurfaceCreateInfoKHR create_info = {
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.hinstance = GetModuleHandle(nullptr),
			.hwnd = state.hwnd,
		};

		if (vkCreateWin32SurfaceKHR(state.vulkan.instance, &create_info, state.vulkan.allocator, &state.vulkan.surface) != VK_SUCCESS) {
			GLVK_DEBUG("Failed to create Vulkan surface", GLVK_SEVERITY_ERROR, GLVK_TYPE_VULKAN);
			return GL_FALSE;
		}
	}

	{
		uint32_t physical_count = 0;
		vkEnumeratePhysicalDevices(state.vulkan.instance, &physical_count, nullptr);

		std::vector<VkPhysicalDevice> physicals(physical_count);
		vkEnumeratePhysicalDevices(state.vulkan.instance, &physical_count, physicals.data());

		VkPhysicalDeviceProperties physical_props;
		VkPhysicalDeviceFeatures physical_features;
		VkPhysicalDeviceMemoryProperties physical_mem_props;

		{
			size_t best = 0;
			size_t best_index = std::numeric_limits<size_t>::max();
			for (size_t i = 0; i < physicals.size(); ++i) {
				vkGetPhysicalDeviceProperties(physicals[i], &physical_props);
				vkGetPhysicalDeviceFeatures(physicals[i], &physical_features);
				vkGetPhysicalDeviceMemoryProperties(physicals[i], &physical_mem_props);

				size_t score = 0;
				if (physical_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					score += 10000;
				}

				score += physical_props.limits.maxImageDimension2D;
				score += static_cast<size_t>(physical_props.limits.maxFramebufferWidth * physical_props.limits.maxFramebufferHeight) * 0.01f;

				if (score > best) {
					best = score;
					best_index = i;
				}
			}

			if (best_index == std::numeric_limits<size_t>::max()) {
				GLVK_DEBUG("Failed to find discrete GPU", GLVK_SEVERITY_ERROR, GLVK_TYPE_GLVK);
				return GL_FALSE;
			}

			state.vulkan.physical = physicals[best_index];
		}

		if (state.vulkan.physical == VK_NULL_HANDLE) {
			GLVK_DEBUG("Failed to find discrete GPU", GLVK_SEVERITY_ERROR, GLVK_TYPE_GLVK);
			return GL_FALSE;
		}
	}

	{
		std::vector<layer_t> requested_layers;
		std::vector<extension_t> requested_extensions = {
			{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, true },
		};

		uint32_t layer_count = 0;
		vkEnumerateDeviceLayerProperties(state.vulkan.physical, &layer_count, nullptr);

		std::vector<VkLayerProperties> layer_props(layer_count);
		vkEnumerateDeviceLayerProperties(state.vulkan.physical, &layer_count, layer_props.data());

		std::vector<const char *> layer_names;
		for (layer_t & layer : requested_layers) {
			bool found = false;
			for (VkLayerProperties & prop : layer_props) {
				GLVK_DEBUG(prop.layerName, GLVK_SEVERITY_INFO, GLVK_TYPE_GLVK);
				if (strcmp(prop.layerName, layer.name) == 0) {
					found = true;
					layer_names.push_back(layer.name);
					break;
				}
			}

			if (!found) {
				if (layer.required) {
					GLVK_DEBUGF(GLVK_SEVERITY_ERROR, GLVK_TYPE_GLVK, "Failed to find required Vulkan device layer {}", layer.name);
					return GL_FALSE;
				} else {
					GLVK_DEBUGF(GLVK_SEVERITY_INFO, GLVK_TYPE_GLVK, "Failed to find Vulkan device layer {}", layer.name);
				}
			}
		}

		uint32_t extension_count = 0;
		vkEnumerateDeviceExtensionProperties(state.vulkan.physical, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> extension_props(extension_count);
		vkEnumerateDeviceExtensionProperties(state.vulkan.physical, nullptr, &extension_count, extension_props.data());

		std::vector<const char *> extension_names;
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
					GLVK_DEBUGF(GLVK_SEVERITY_ERROR, GLVK_TYPE_GLVK, "Failed to find required Vulkan device extension {}", extension.name);
					return GL_FALSE;
				}
				else {
					GLVK_DEBUGF(GLVK_SEVERITY_INFO, GLVK_TYPE_GLVK, "Failed to find Vulkan device extension {}", extension.name);
				}
			}
		}

		/* queue families */
		{
			uint32_t family_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(state.vulkan.physical, &family_count, nullptr);

			std::vector<VkQueueFamilyProperties> family_props(family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(state.vulkan.physical, &family_count, family_props.data());

			uint32_t gfx = std::numeric_limits<uint32_t>::max();
			uint32_t prs = std::numeric_limits<uint32_t>::max();
			for (uint32_t i = 0; i < family_count; ++i) {
				if (family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					gfx = i;
				}

				VkBool32 present = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(state.vulkan.physical, i, state.vulkan.surface, &present);
				if (present == VK_TRUE) {
					prs = i;
				}

				if (gfx != std::numeric_limits<uint32_t>::max() && prs != std::numeric_limits<uint32_t>::max()) {
					break;
				}
			}

			if (gfx == std::numeric_limits<uint32_t>::max() || prs == std::numeric_limits<uint32_t>::max()) {
				GLVK_DEBUG("Failed to find proper queue families", GLVK_SEVERITY_ERROR, GLVK_TYPE_GLVK);
				return GL_FALSE;
			}
		}

		uint32_t qfamilies[2] = {
			state.vulkan.qfamilies.graphics,
			state.vulkan.qfamilies.present,
		};

		float priority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> qcreate_infos;

		for (size_t i = 0; i < 2; ++i) {
			VkDeviceQueueCreateInfo qcreate_info;
			for (size_t j = 0; j < qcreate_infos.size(); ++j) {
				if (qcreate_infos[j].queueFamilyIndex == qfamilies[i]) {
					goto next;
				}
			}

			qcreate_info = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.queueFamilyIndex = qfamilies[i],
				.queueCount = 1,
				.pQueuePriorities = &priority,
			};

			qcreate_infos.push_back(qcreate_info);
		next:;
		}

		VkDeviceCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueCreateInfoCount = static_cast<uint32_t>(qcreate_infos.size()),
			.pQueueCreateInfos = qcreate_infos.data(),
			.enabledLayerCount = static_cast<uint32_t>(layer_names.size()),
			.ppEnabledLayerNames = layer_names.data(),
			.enabledExtensionCount = static_cast<uint32_t>(extension_names.size()),
			.ppEnabledExtensionNames = extension_names.data(),
			.pEnabledFeatures = nullptr,
		};

		if (vkCreateDevice(state.vulkan.physical, &create_info, state.vulkan.allocator, &state.vulkan.device) != VK_SUCCESS) {
			GLVK_DEBUG("Failed to create Vulkan device", GLVK_SEVERITY_ERROR, GLVK_TYPE_VULKAN);
			return GL_FALSE;
		}
	}

	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state.vulkan.physical, state.vulkan.surface, &capabilities);

		uint32_t formats_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(state.vulkan.physical, state.vulkan.surface, &formats_count, nullptr);

		if (formats_count == 0) {
			GLVK_DEBUG("Failed to find any surface formats", GLVK_SEVERITY_ERROR, GLVK_TYPE_VULKAN);
			return GL_FALSE;
		}

		std::vector<VkSurfaceFormatKHR> formats(formats_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(state.vulkan.physical, state.vulkan.surface, &formats_count, formats.data());

		size_t format_index = std::numeric_limits<size_t>::max();
		for (size_t i = 0; i < formats.size(); ++i) {
			if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				format_index = i;
				break;
			}
		}

		if (format_index == std::numeric_limits<size_t>::max()) {
			GLVK_DEBUG("Failed to find proper surface format", GLVK_SEVERITY_ERROR, GLVK_TYPE_VULKAN);
			return GL_FALSE;
		}

		uint32_t modes_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(state.vulkan.physical, state.vulkan.surface, &modes_count, nullptr);

		if (modes_count == 0) {
			GLVK_DEBUG("Failed to find any surface present modes", GLVK_SEVERITY_ERROR, GLVK_TYPE_VULKAN);
			return GL_FALSE;
		}

		std::vector<VkPresentModeKHR> modes(modes_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(state.vulkan.physical, state.vulkan.surface, &modes_count, modes.data());

		size_t fifo_index = std::numeric_limits<size_t>::max();
		size_t mode_index = std::numeric_limits<size_t>::max();

		for (size_t i = 0; i < modes.size(); ++i) {
			if (modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
				fifo_index = i;
			}

			if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
				mode_index = i;
				break;
			}
		}

		if (mode_index == std::numeric_limits<size_t>::max()) {
			mode_index = fifo_index;
			if (mode_index == std::numeric_limits<size_t>::max()) {
				GLVK_DEBUG("Failed to find proper surface present mode", GLVK_SEVERITY_ERROR, GLVK_TYPE_VULKAN);
				return GL_FALSE;
			}
		}
	}

	return GL_TRUE;
}

void glvkDeinit() {
	vkDestroySurfaceKHR(state.vulkan.instance, state.vulkan.surface, state.vulkan.allocator);
	vkDestroyDevice(state.vulkan.device, state.vulkan.allocator);
	vkDestroyInstance(state.vulkan.instance, state.vulkan.allocator);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	state.viewport.width = width;
	state.viewport.height = height;
	state.viewport.x = x;
	state.viewport.y = y;
}