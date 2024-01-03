#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <limits>
#include <functional>
#include <format>

#include "GL/glvk.hpp"
#include "ktga.hpp"
#include "kobj.hpp"

#include <cstring>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#define VK_CALL(l) { VkResult vr = l; if (vr != VK_SUCCESS) { if (vr != VK_INCOMPLETE) { std::cerr << __FILE__ << ":" << __LINE__ << " Vulkan call failed: " << vr << "\n"; throw std::runtime_error(std::format("{}:{} Vulkan call failed", __FILE__, __LINE__)); } else { std::cout << "Warning: Vulkan call returned incomplete\n"; } } }
#define VK_UNMAKE_VERSION_MAJOR(v) ((v) >> 22)
#define VK_UNMAKE_VERSION_MINOR(v) (((v) >> 12) & 0x3ff)
#define VK_UNMAKE_VERSION_PATCH(v) ((v) & 0xfff)

#define VK_DEBUG_INFO
#define VK_DEBUG

/*
void vk_init(vulkan_t & vulkan);
void vk_deinit(vulkan_t & vulkan);
static vulkan_t * pvulkan = nullptr;
*/

bool running = true;

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case WM_CLOSE:
			PostQuitMessage(0);
			running = false;
			return 0;
		case WM_SIZE: {
				uint32_t w = LOWORD(lparam);
				uint32_t h = HIWORD(lparam);

				return 0;
			}
		default:
			return DefWindowProcA(hwnd, msg, wparam, lparam);
	}
}

int main(int argc, char ** argv) {
	WNDCLASSEXA window_class = {
		.cbSize = sizeof(window_class),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = (WNDPROC) window_proc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = GetModuleHandleA(nullptr),
		.hIcon = LoadIcon(nullptr, IDC_ARROW),
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = (HBRUSH) GetStockObject(0),
		.lpszMenuName = "menu name",
		.lpszClassName = "Window Class",
		.hIconSm = (HICON) LoadImageA(GetModuleHandleA(nullptr), MAKEINTRESOURCEA(5), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR),
	};

	if (RegisterClassExA(&window_class) == 0) {
		std::cout << "Failed to register window class\n";
		throw std::runtime_error("Failed to register window class");
	}

	HWND hwnd;

	{

		RECT rect = {
			.left = 0,
			.top = 0,
			.right = 800,
			.bottom = 600,
		};

		AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);

		hwnd = CreateWindowExA(0, window_class.lpszClassName, "krisvers' vulkan testing", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, window_class.hInstance, NULL);
		if (hwnd == nullptr) {
			std::cout << "Failed to create window\n";
			return 1;
		}
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	glvkDebug();
	glvkRegisterDebugCallback(
		[](const char * message, GLVKseverity severity, GLVKtype type) {
			std::cout << "GLVK debug message: " << message << "\n";
		}
	);

	if (!glvkInit(hwnd)) {
		std::cout << "Failed to initialize glvk\n";
		return 2;
	}

	while (running) {
		MSG msg;
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

	glvkDeinit();

	/*
	vulkan_t vulkan = {
		.instance = VK_NULL_HANDLE,
		.allocator = nullptr,
		.physical = VK_NULL_HANDLE,
		.device = VK_NULL_HANDLE,
		.surface = VK_NULL_HANDLE,

		.swapchain = VK_NULL_HANDLE,

		.cmd_pool = VK_NULL_HANDLE,

		.frames_in_flight = 2,

		.vertex_shader = VK_NULL_HANDLE,
		.fragment_shader = VK_NULL_HANDLE,
		.pipeline_layout = VK_NULL_HANDLE,
		.pipeline = VK_NULL_HANDLE,
		.render_pass = VK_NULL_HANDLE,

		.present_queue = VK_NULL_HANDLE,
		.graphics_queue = VK_NULL_HANDLE,

		.graphics_family = std::numeric_limits<uint32_t>::max(),
		.present_family = std::numeric_limits<uint32_t>::max(),

		.window_width = 800,
		.window_height = 600,
		.hwnd = nullptr,
		.hinstance = GetModuleHandleA(nullptr),

		.debug_messenger = VK_NULL_HANDLE,
	};

	pvulkan = &vulkan;

	{
		RECT rect = {
			.left = 0,
			.top = 0,
			.right = static_cast<LONG>(vulkan.window_width),
			.bottom = static_cast<LONG>(vulkan.window_height),
		};
		AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);

		vulkan.hwnd = CreateWindowExA(0, window_class.lpszClassName, "krisvers' vulkan testing", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, window_class.hInstance, NULL);
		if (vulkan.hwnd == NULL) {
			std::cout << "Failed to create window\n";
			throw std::runtime_error("Failed to create window");
		}
	}

	vk_init(vulkan);

	LARGE_INTEGER perf_freq;
	QueryPerformanceFrequency(&perf_freq);

	LARGE_INTEGER start;
	LARGE_INTEGER end;

	double tdelta = 1 / vulkan.target_fps;
	double delta = 0;

	while (running) {
		QueryPerformanceCounter(&start);
		vk_draw_frame(vulkan);
		QueryPerformanceCounter(&end);
		#ifdef VK_DEBUG_INFO
		std::cout << "Frame time: " << static_cast<double>(end.QuadPart - start.QuadPart) / static_cast<double>(perf_freq.QuadPart) << "\n";
		#endif

		MSG msg;
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}

		QueryPerformanceCounter(&end);
		delta = static_cast<double>(end.QuadPart - start.QuadPart) / static_cast<double>(perf_freq.QuadPart);
		double remaining = tdelta - delta;

		if (remaining > 0) {
			DWORD ms = static_cast<DWORD>(remaining * 1000);

			if (ms > 0) {
				Sleep(ms - 1);
			}
		}

		//vk_frames_in_flight(vulkan, vulkan.frames_in_flight + 1);
	}

	vk_deinit(vulkan);
	*/

	return 0;
}

/*
float counter = 0;
void vk_draw_frame(vulkan_t & vulkan) {
	++counter;
	{
		RECT rect;
		RECT borders;
		if (GetWindowRect(vulkan.hwnd, &rect) == 0) { return; }
		AdjustWindowRectEx(&borders, WS_OVERLAPPEDWINDOW, FALSE, 0);
		LONG bw = (borders.right - borders.left);
		LONG bh = (borders.bottom - borders.top);
		LONG wwb = rect.right - rect.left;
		LONG hwb = rect.bottom - rect.top;
		LONG width = wwb - bw;
		LONG height = hwb - bh;

		vulkan.window_width = static_cast<uint32_t>(width);
		vulkan.window_height = static_cast<uint32_t>(height);

		if (width <= 0 || height <= 0 || wwb <= 0 || hwb <= 0) {
			return;
		}
	}

	{
		uniform_t * ubo = reinterpret_cast<uniform_t *>(vulkan.unif_mappeds[vulkan.current_frame]);
		mat4x4_identity(ubo->model);
		mat4x4_translate(ubo->model, 0, 0, -1);
		mat4x4_rotate(ubo->model, ubo->model, 0, 1, 0, counter / 15);
		mat4x4_scale_aniso(ubo->model, ubo->model, 0.1f, 0.1f, 0.1f);

		mat4x4_identity(ubo->view);
		ubo->view[1][1] *= -1;
		mat4x4_identity(ubo->proj);
		float aspect = static_cast<float>(vulkan.swapchain_extent.width) / static_cast<float>(vulkan.swapchain_extent.height);
		mat4x4_perspective(ubo->proj, 1, aspect, 0.05f, 100.0f);
		//mat4x4_ortho(ubo->proj, -1, 1, 1 / -aspect, 1 / aspect, -100, 100);
	}

	vkWaitForFences(vulkan.device, 1, &vulkan.fences_flight[vulkan.current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(vulkan.device, 1, &vulkan.fences_flight[vulkan.current_frame]);

	uint32_t image_index;
	vkAcquireNextImageKHR(vulkan.device, vulkan.swapchain, std::numeric_limits<uint64_t>::max(), vulkan.semaphores_img_avail[vulkan.current_frame], VK_NULL_HANDLE, &image_index);

	vkResetCommandBuffer(vulkan.cmd_buffers[vulkan.current_frame], 0);
	vk_begin_cmd(vulkan, vulkan.cmd_buffers[vulkan.current_frame]);

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = NULL,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = vulkan.swapchain_images[image_index],
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
	};

	vkCmdPipelineBarrier(vulkan.cmd_buffers[vulkan.current_frame], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

	VkClearValue clears[2] = {
		{ { { 0.1f, 0.1f, 0.1f, 1.0f } } },
		{ 1.0f, 0.0f },
	};

	vulkan.scissor = {
		.offset = { 0, 0 },
		.extent = vulkan.swapchain_extent,
	};

	vulkan.viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(vulkan.swapchain_extent.width),
		.height = static_cast<float>(vulkan.swapchain_extent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	VkRenderPassBeginInfo rp_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = vulkan.render_pass,
		.framebuffer = vulkan.framebuffers[image_index],
		.renderArea = vulkan.scissor,
		.clearValueCount = 2,
		.pClearValues = clears,
	};

	vkCmdBeginRenderPass(vulkan.cmd_buffers[vulkan.current_frame], &rp_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(vulkan.cmd_buffers[vulkan.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan.pipeline);
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(vulkan.cmd_buffers[vulkan.current_frame], 0, 1, &vulkan.mesh_buffer, &offset);
	vkCmdBindIndexBuffer(vulkan.cmd_buffers[vulkan.current_frame], vulkan.mesh_buffer, sizeof(vertex_t) * vulkan.mesh_vertex_count, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(vulkan.cmd_buffers[vulkan.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan.pipeline_layout, 0, 1, vulkan.desc_sets.data(), 0, nullptr);

	vkCmdSetViewport(vulkan.cmd_buffers[vulkan.current_frame], 0, 1, &vulkan.viewport);
	vkCmdSetScissor(vulkan.cmd_buffers[vulkan.current_frame], 0, 1, &vulkan.scissor);

	vkCmdDrawIndexed(vulkan.cmd_buffers[vulkan.current_frame], vulkan.mesh_index_count, 1, 0, 0, 0);

	vkCmdEndRenderPass(vulkan.cmd_buffers[vulkan.current_frame]);
	vk_end_cmd(vulkan, vulkan.cmd_buffers[vulkan.current_frame]);

	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vulkan.semaphores_img_avail[vulkan.current_frame],
		.pWaitDstStageMask = wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vulkan.cmd_buffers[vulkan.current_frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vulkan.semaphores_render_finished[vulkan.current_frame],
	};

	VK_CALL(vkQueueSubmit(vulkan.graphics_queue, 1, &submit_info, vulkan.fences_flight[vulkan.current_frame]));

	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vulkan.semaphores_render_finished[vulkan.current_frame],
		.swapchainCount = 1,
		.pSwapchains = &vulkan.swapchain,
		.pImageIndices = &image_index,
		.pResults = nullptr,
	};

	VK_CALL(vkQueuePresentKHR(vulkan.present_queue, &present_info));

	++vulkan.current_frame;
	if (vulkan.current_frame >= vulkan.frames_in_flight) {
		vulkan.current_frame = 0;
	}
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

	std::vector<layer_t> requested_instance_layers;
	std::vector<extension_t> requested_instance_extensions;

	#ifdef VK_DEBUG
	requested_instance_layers.push_back({ "VK_LAYER_KHRONOS_validation", false });
	requested_instance_extensions.push_back({ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, false });
	#endif

	requested_instance_extensions.push_back({ VK_KHR_SURFACE_EXTENSION_NAME, true });
	requested_instance_extensions.push_back({ "VK_KHR_win32_surface", true });

	vk_create_instance(vulkan, app_info, requested_instance_layers, requested_instance_extensions);

	std::vector<layer_t> requested_device_layers;
	std::vector<extension_t> requested_device_extensions = { { VK_KHR_SWAPCHAIN_EXTENSION_NAME, true } };

	#ifdef VK_DEBUG
	requested_device_layers.push_back({ "VK_LAYER_KHRONOS_validation", false });
	#endif

	vk_create_surface(vulkan);

	vk_create_physical(
		vulkan,

		[](const VkPhysicalDevice & device) {
			int64_t score = 0;

			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(device, &props);

			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				score += 1000;
			}

			score += props.limits.maxImageDimension2D;
			score += static_cast<int64_t>(props.limits.maxFramebufferWidth * props.limits.maxFramebufferHeight) * 0.01f;

			return score;
		}
	);

	vk_create_device(vulkan, requested_device_layers, requested_device_extensions);

	vk_create_swapchain(
		vulkan,

		[](const std::vector<VkSurfaceFormatKHR> & formats) {
			for (size_t i = 0; i < formats.size(); ++i) {
				if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					return i;
				}
			}

			return 0ULL;
		},

		[](const std::vector<VkPresentModeKHR> & modes) {
			size_t fifo_index = 0ULL;
			for (size_t i = 0; i < modes.size(); ++i) {
				switch (modes[i]) {
				case VK_PRESENT_MODE_FIFO_KHR:
					fifo_index = i;
					break;
				case VK_PRESENT_MODE_MAILBOX_KHR:
					return i;
				default: break;
				}
			}

			return fifo_index;
		},

		[](const VkSurfaceCapabilitiesKHR & capabilities) {
			return VkExtent2D{ capabilities.currentExtent.width, capabilities.currentExtent.height };
		},

		[](uint32_t min, uint32_t max) {
			uint32_t target = 2;
			return std::max(std::min(target, max), min);
		}
	);

	vk_init_render_pass(vulkan);

	std::vector<unsigned char> v_spv;
	std::vector<unsigned char> f_spv;
	{
		std::ifstream file("vert.spv", std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			std::cout << "Failed to open vert.spv\n";
			throw std::runtime_error("Failed to open vert.spv");
		}

		size_t size = file.tellg();
		v_spv.resize(size);
		file.seekg(0);
		file.read(reinterpret_cast<char *>(v_spv.data()), size);
		file.close();

		file.open("frag.spv", std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			std::cout << "Failed to open frag.spv\n";
			throw std::runtime_error("Failed to open frag.spv");
		}

		size = file.tellg();
		f_spv.resize(size);
		file.seekg(0);
		file.read(reinterpret_cast<char *>(f_spv.data()), size);
		file.close();
	}

	vk_init_pipeline(vulkan, v_spv, f_spv);
	vk_create_command_utils(vulkan);
	vk_create_depth(vulkan);
	vk_init_framebuffers(vulkan);
	vk_create_buffers(vulkan);
	vk_create_texture(vulkan);
	vk_create_descriptor_utilities(vulkan);
	vk_create_semaphores(vulkan);
}

void vk_deinit(vulkan_t & vulkan) {
	vkDeviceWaitIdle(vulkan.device);

	for (uint32_t i = 0; i < vulkan.frames_in_flight; ++i) {
		vkDestroyFence(vulkan.device, vulkan.fences_flight[i], vulkan.allocator);
		vkDestroySemaphore(vulkan.device, vulkan.semaphores_render_finished[i], vulkan.allocator);
		vkDestroySemaphore(vulkan.device, vulkan.semaphores_img_avail[i], vulkan.allocator);
	}

	vkDestroyDescriptorPool(vulkan.device, vulkan.desc_pool, vulkan.allocator);

	vkDestroySampler(vulkan.device, vulkan.texture_sampler, vulkan.allocator);
	vkDestroyImageView(vulkan.device, vulkan.texture_view, vulkan.allocator);

	vkDestroyImage(vulkan.device, vulkan.texture, vulkan.allocator);
	vkFreeMemory(vulkan.device, vulkan.texture_memory, vulkan.allocator);

	for (uint32_t i = 0; i < vulkan.frames_in_flight; ++i) {
		vkDestroyBuffer(vulkan.device, vulkan.unif_buffers[i], vulkan.allocator);
		vkFreeMemory(vulkan.device, vulkan.unif_memorys[i], vulkan.allocator);
	}

	vkDestroyBuffer(vulkan.device, vulkan.mesh_buffer, vulkan.allocator);
	vkFreeMemory(vulkan.device, vulkan.mesh_memory, vulkan.allocator);

	vkDestroyImage(vulkan.device, vulkan.depth, vulkan.allocator);
	vkFreeMemory(vulkan.device, vulkan.depth_memory, vulkan.allocator);
	vkDestroyImageView(vulkan.device, vulkan.depth_view, vulkan.allocator);

	vkDestroyCommandPool(vulkan.device, vulkan.cmd_pool, vulkan.allocator);

	for (uint32_t i = 0; i < vulkan.swapchain_image_count; ++i) {
		vkDestroyFramebuffer(vulkan.device, vulkan.framebuffers[i], vulkan.allocator);
	}

	vkDestroyPipeline(vulkan.device, vulkan.pipeline, vulkan.allocator);
	vkDestroyPipelineLayout(vulkan.device, vulkan.pipeline_layout, vulkan.allocator);
	vkDestroyRenderPass(vulkan.device, vulkan.render_pass, vulkan.allocator);
	vkDestroyDescriptorSetLayout(vulkan.device, vulkan.desc_layout, vulkan.allocator);

	vkDestroyShaderModule(vulkan.device, vulkan.fragment_shader, vulkan.allocator);
	vkDestroyShaderModule(vulkan.device, vulkan.vertex_shader, vulkan.allocator);

	for (uint32_t i = 0; i < vulkan.swapchain_image_count; ++i) {
		vkDestroyImageView(vulkan.device, vulkan.swapchain_views[i], vulkan.allocator);
	}

	vkDestroySwapchainKHR(vulkan.device, vulkan.swapchain, vulkan.allocator);
	vkDestroyDevice(vulkan.device, vulkan.allocator);
	vkDestroySurfaceKHR(vulkan.instance, vulkan.surface, vulkan.allocator);

	if (vulkan.debug_messenger != VK_NULL_HANDLE) {
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vulkan.instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (vkDestroyDebugUtilsMessengerEXT == nullptr) {
			std::cout << "Failed to find vkDestroyDebugUtilsMessengerEXT\n";
			throw std::runtime_error("Failed to find vkDestroyDebugUtilsMessengerEXT");
		}

		vkDestroyDebugUtilsMessengerEXT(vulkan.instance, vulkan.debug_messenger, vulkan.allocator);
	}

	vkDestroyInstance(vulkan.instance, vulkan.allocator);
}

void vk_create_instance(vulkan_t & vulkan, VkApplicationInfo app_info, const std::vector<layer_t> & requested_layers, const std::vector<extension_t> & requested_extensions) {
	uint32_t layer_prop_count = 0;
	VK_CALL(vkEnumerateInstanceLayerProperties(&layer_prop_count, nullptr));

	std::vector<VkLayerProperties> layer_properties(layer_prop_count);
	VK_CALL(vkEnumerateInstanceLayerProperties(&layer_prop_count, layer_properties.data()));

	std::vector<const char *> layer_names;
	#ifdef VK_DEBUG_INFO
	std::cout << "Requested Vulkan instance layers:\n";
	for (const layer_t & layer : requested_layers) {
		std::cout << "    " << (layer.required ? "(required) " : "") << layer.name << "\n";
	}
	std::cout << '\n';
	#endif

	for (const layer_t & layer : requested_layers) {
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
	for (const extension_t & extension : requested_extensions) {
		std::cout << "    " << (extension.required ? "(required) " : "") << extension.name << "\n";
	}
	std::cout << '\n';
	#endif

	for (const extension_t & extension : requested_extensions) {
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

	VK_CALL(vkCreateInstance(&create_info, vulkan.allocator, &vulkan.instance));

	if (extension_names.size() > 0) {
		for (const char *& name : extension_names) {
			if (strcmp(name, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
				PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vulkan.instance, "vkCreateDebugUtilsMessengerEXT"));
				if (vkCreateDebugUtilsMessengerEXT == nullptr) {
					std::cout << "Failed to find vkCreateDebugUtilsMessengerEXT\n";
					throw std::runtime_error("Failed to find vkCreateDebugUtilsMessengerEXT");
				}

				VkDebugUtilsMessengerCreateInfoEXT create_info = {
					.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
					.pNext = nullptr,
					.flags = 0,
					.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
					.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

					.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT * callback_data, void * user_data) -> VkBool32 {
						std::cout << "Vulkan debug message: " << callback_data->pMessage << "\n";

						if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
							return VK_TRUE;
						}

						return VK_FALSE;
					},

					.pUserData = nullptr,
				};

				VK_CALL(vkCreateDebugUtilsMessengerEXT(vulkan.instance, &create_info, vulkan.allocator, &vulkan.debug_messenger));
				#ifdef VK_DEBUG_INFO
				std::cout << "Registered Vulkan debug messenger\n";
				#endif
			}
		}
	}
}

void vk_create_physical(vulkan_t & vulkan, std::function<int64_t(const VkPhysicalDevice &)> score_func) {
	uint32_t physical_count = 0;
	vkEnumeratePhysicalDevices(vulkan.instance, &physical_count, nullptr);

	if (physical_count == 0) {
		std::cout << "Failed to find any Vulkan physical devices\n";
		throw std::runtime_error("Failed to find any Vulkan physical devices");
	}

	std::vector<VkPhysicalDevice> physical_devices(physical_count);
	vkEnumeratePhysicalDevices(vulkan.instance, &physical_count, physical_devices.data());

	int64_t best_score = -1;
	size_t best_index = std::numeric_limits<size_t>::max();

	#ifdef VK_DEBUG_INFO
	std::cout << "Vulkan physical devices:\n";
	#endif
	for (size_t i = 0; i < physical_devices.size(); ++i) {
		int64_t score = score_func(physical_devices[i]);

		if (score != -1) {
			if (score > best_score) {
				best_score = score;
				best_index = i;
			}
		}

		#ifdef VK_DEBUG_INFO
		VkPhysicalDevice & dev = physical_devices[i];

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
		std::cout << "    " << props.deviceName << " [" << score << "]";
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
			c = ' ';
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

			VkBool32 present = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, vulkan.surface, &present);

			c = ' ';
			if (present) {
				c = 'X';
			}
			std::cout << "           [" << c << "] Present\n";
		}
		std::cout << '\n';
		#endif
	}

	if (best_index == std::numeric_limits<size_t>::max()) {
		std::cout << "Failed to find any suitable Vulkan physical devices\n";
		throw std::runtime_error("Failed to find any suitable Vulkan physical devices");
	}

	vulkan.physical = physical_devices[best_index];
}

void vk_create_device(vulkan_t & vulkan, std::vector<layer_t> & requested_layers, std::vector<extension_t> & requested_extensions) {
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vulkan.physical, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_family_props(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(vulkan.physical, &queue_family_count, queue_family_props.data());

	vulkan.graphics_family = std::numeric_limits<uint32_t>::max();
	vulkan.present_family = std::numeric_limits<uint32_t>::max();
	for (uint32_t i = 0; i < queue_family_props.size(); ++i) {
		if (queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			vulkan.graphics_family = i;
			#ifdef VK_DEBUG_INFO
			std::cout << "Found Vulkan graphics queue family: " << i << "\n";
			#endif
		}

		VkBool32 present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(vulkan.physical, i, vulkan.surface, &present);

		if (present) {
			vulkan.present_family = i;
			#ifdef VK_DEBUG_INFO
			std::cout << "Found Vulkan present queue family: " << i << "\n";
			#endif
		}

		if (vulkan.graphics_family != std::numeric_limits<uint32_t>::max() && vulkan.present_family != std::numeric_limits<uint32_t>::max()) {
			break;
		}
	}

	if (vulkan.graphics_family == std::numeric_limits<uint32_t>::max()) {
		std::cout << "Failed to find any Vulkan graphics queue family\n";
		throw std::runtime_error("Failed to find any Vulkan graphics queue family");
	}

	if (vulkan.present_family == std::numeric_limits<uint32_t>::max()) {
		std::cout << "Failed to find any Vulkan present queue family\n";
		throw std::runtime_error("Failed to find any Vulkan present queue family");
	}
	
	uint32_t layer_prop_count = 0;
	vkEnumerateDeviceLayerProperties(vulkan.physical, &layer_prop_count, nullptr);

	std::vector<VkLayerProperties> layer_props(layer_prop_count);
	vkEnumerateDeviceLayerProperties(vulkan.physical, &layer_prop_count, layer_props.data());

	std::vector<const char *> layer_names;
	for (layer_t & layer : requested_layers) {
		bool found = false;
		for (VkLayerProperties & prop : layer_props) {
			if (strcmp(prop.layerName, layer.name) == 0) {
				found = true;
				layer_names.push_back(layer.name);
				break;
			}
		}

		if (!found) {
			if (layer.required) {
				std::cout << "Failed to find required Vulkan device layer: " << layer.name << "\n";
				throw std::runtime_error("Failed to find required Vulkan device layer");
			}
		}
	}

	#ifdef VK_DEBUG_INFO
	std::cout << "Vulkan device layer:\n";
	for (VkLayerProperties & prop : layer_props) {
		char c = ' ';
		for (const char *& name : layer_names) {
			if (strcmp(prop.layerName, name) == 0) {
				c = 'X';
				break;
			}
		}

		std::cout << '[' << c << ']' << " " << prop.layerName << "\n";
		std::cout << "     " << prop.description << "\n";
		std::cout << "     Specification version: " << VK_UNMAKE_VERSION_MAJOR(prop.specVersion) << "." << VK_UNMAKE_VERSION_MINOR(prop.specVersion) << "." << VK_UNMAKE_VERSION_PATCH(prop.specVersion) << "\n";
		std::cout << "     Implementation version: " << VK_UNMAKE_VERSION_MAJOR(prop.implementationVersion) << "." << VK_UNMAKE_VERSION_MINOR(prop.implementationVersion) << "." << VK_UNMAKE_VERSION_PATCH(prop.implementationVersion) << "\n\n";
	}
	#endif

	uint32_t extension_prop_count = 0;
	vkEnumerateDeviceExtensionProperties(vulkan.physical, nullptr, &extension_prop_count, nullptr);

	std::vector<VkExtensionProperties> extension_props(extension_prop_count);
	vkEnumerateDeviceExtensionProperties(vulkan.physical, nullptr, &extension_prop_count, extension_props.data());

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
				std::cout << "Failed to find required Vulkan device extension: " << extension.name << "\n";
				throw std::runtime_error("Failed to find required Vulkan device extension");
			}
		}
	}

	#ifdef VK_DEBUG_INFO
	std::cout << "Vulkan device extensions:\n";
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

	std::vector<uint32_t> queue_families = { vulkan.graphics_family, vulkan.present_family };

	float priority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

	for (size_t i = 0; i < queue_families.size(); ++i) {
		if (queue_create_infos.size() > 0) {
			for (VkDeviceQueueCreateInfo & create_info : queue_create_infos) {
				if (create_info.queueFamilyIndex != queue_families[i]) {
					queue_create_infos.push_back({
						.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.queueFamilyIndex = queue_families[i],
						.queueCount = 1,
						.pQueuePriorities = &priority,
					});
					break;
				}
			}
		} else {
			queue_create_infos.push_back({
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.queueFamilyIndex = queue_families[i],
				.queueCount = 1,
				.pQueuePriorities = &priority,
			});
		}
	}

	VkPhysicalDeviceFeatures physical_feats = {
		.samplerAnisotropy = VK_TRUE,
	};

	VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
		.pQueueCreateInfos = queue_create_infos.data(),
		.enabledLayerCount = static_cast<uint32_t>(requested_layers.size()),
		.ppEnabledLayerNames = layer_names.data(),
		.enabledExtensionCount = static_cast<uint32_t>(requested_extensions.size()),
		.ppEnabledExtensionNames = extension_names.data(),
		.pEnabledFeatures = &physical_feats,
	};

	VK_CALL(vkCreateDevice(vulkan.physical, &create_info, vulkan.allocator, &vulkan.device));
	vkGetDeviceQueue(vulkan.device, vulkan.present_family, 0, &vulkan.present_queue);
	vkGetDeviceQueue(vulkan.device, vulkan.graphics_family, 0, &vulkan.graphics_queue);
}

void vk_create_surface(vulkan_t & vulkan) {
	VkWin32SurfaceCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.hinstance = vulkan.hinstance,
		.hwnd = vulkan.hwnd,
	};

	VK_CALL(vkCreateWin32SurfaceKHR(vulkan.instance, &create_info, vulkan.allocator, &vulkan.surface));
}

void vk_create_swapchain(vulkan_t & vulkan, std::function<size_t(const std::vector<VkSurfaceFormatKHR> &)> choose_fmt_func, std::function<size_t(const std::vector<VkPresentModeKHR> &)> choose_mode_func, std::function<VkExtent2D(const VkSurfaceCapabilitiesKHR &)> choose_extent_func, std::function<uint32_t(uint32_t, uint32_t)> choose_image_count_func) {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan.physical, vulkan.surface, &vulkan.surface_capabilities);

	#ifdef VK_DEBUG_INFO
	std::cout << "Vulkan surface capabilities:\n";
	std::cout << "    Min image count: " << vulkan.surface_capabilities.minImageCount << "\n";
	std::cout << "    Max image count: " << vulkan.surface_capabilities.maxImageCount << "\n";
	std::cout << "    Current extent: " << vulkan.surface_capabilities.currentExtent.width << "x" << vulkan.surface_capabilities.currentExtent.height << "\n";
	std::cout << "    Min extent: " << vulkan.surface_capabilities.minImageExtent.width << "x" << vulkan.surface_capabilities.minImageExtent.height << "\n";
	std::cout << "    Max extent: " << vulkan.surface_capabilities.maxImageExtent.width << "x" << vulkan.surface_capabilities.maxImageExtent.height << "\n";
	std::cout << "    Max image array layers: " << vulkan.surface_capabilities.maxImageArrayLayers << "\n";
	std::cout << "    Supported transforms: " << vulkan.surface_capabilities.supportedTransforms << "\n";
	std::cout << "    Current transform: " << vulkan.surface_capabilities.currentTransform << "\n";
	std::cout << "    Supported composite alpha flags: " << vulkan.surface_capabilities.supportedCompositeAlpha << "\n";
	std::cout << "    Supported usage flags: " << vulkan.surface_capabilities.supportedUsageFlags << "\n\n";
	std::cout << '\n';
	#endif

	uint32_t formats_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan.physical, vulkan.surface, &formats_count, nullptr);

	if (formats_count == 0) {
		std::cout << "Failed to find any Vulkan surface formats\n";
		throw std::runtime_error("Failed to find any Vulkan surface formats");
	}

	std::vector<VkSurfaceFormatKHR> surface_formats(formats_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan.physical, vulkan.surface, &formats_count, surface_formats.data());

	size_t format_index = choose_fmt_func(surface_formats);

	#ifdef VK_DEBUG_INFO
	std::cout << "Vulkan surface formats:\n";
	for (size_t i = 0; i < surface_formats.size(); ++i) {
		VkSurfaceFormatKHR & format = surface_formats[i];
		char c = ' ';
		if (i == format_index) {
			c = 'X';
		}

		std::cout << "    [" << c << "] "
			<< [format]() {
				switch (format.format) {
					case VK_FORMAT_R8G8B8A8_UNORM:
						return "R8G8B8A8 unsigned normalized";
					case VK_FORMAT_R8G8B8A8_SNORM:
						return "R8G8B8A8 signed normalized";
					case VK_FORMAT_R8G8B8A8_UINT:
						return "R8G8B8A8 unsigned integer";
					case VK_FORMAT_R8G8B8A8_SINT:
						return "R8G8B8A8 signed integer";
					case VK_FORMAT_R8G8B8A8_SRGB:
						return "R8G8B8A8 sRGB";
					case VK_FORMAT_B8G8R8A8_UINT:
						return "B8G8R8A8 unsigned integer";
					case VK_FORMAT_B8G8R8A8_SINT:
						return "B8G8R8A8 signed integer";
					case VK_FORMAT_B8G8R8A8_SRGB:
						return "B8G8R8A8 sRGB";
					default:
						return "Unknown/Unsupported";
				}
			}() << "\n        "
			<< [format]() {
				switch (format.colorSpace) {
					case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
						return "sRGB nonlinear";
					default:
						return "Unknown/Unsupported";
				}
			}() << "\n\n";
	}
	#endif

	uint32_t modes_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan.physical, vulkan.surface, &modes_count, nullptr);

	if (modes_count == 0) {
		std::cout << "Failed to find any Vulkan present modes\n";
		throw std::runtime_error("Failed to find any Vulkan present modes");
	}

	std::vector<VkPresentModeKHR> present_modes(modes_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan.physical, vulkan.surface, &modes_count, present_modes.data());

	size_t mode_index = choose_mode_func(present_modes);

	#ifdef VK_DEBUG_INFO
	std::cout << "Vulkan present modes:\n";
	for (size_t i = 0; i < present_modes.size(); ++i) {
		VkPresentModeKHR & mode = present_modes[i];
		char c = ' ';
		if (i == mode_index) {
			c = 'X';
		}

		std::cout << "    [" << c << ']' << " "
			<< [mode]() {
				switch (mode) {
					case VK_PRESENT_MODE_IMMEDIATE_KHR:
						return "Immediate";
					case VK_PRESENT_MODE_MAILBOX_KHR:
						return "Mailbox";
					case VK_PRESENT_MODE_FIFO_KHR:
						return "FIFO";
					case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
						return "FIFO relaxed";
					default:
						return "Unknown";
				}
			}() << "\n";
	}
	std::cout << '\n';
	#endif
	vulkan.swapchain_format = surface_formats[format_index].format;
	vulkan.swapchain_extent = choose_extent_func(vulkan.surface_capabilities);
	vulkan.swapchain_image_count = choose_image_count_func(vulkan.surface_capabilities.minImageCount, vulkan.surface_capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = vulkan.surface,
		.minImageCount = vulkan.swapchain_image_count,
		.imageFormat = vulkan.swapchain_format,
		.imageColorSpace = surface_formats[format_index].colorSpace,
		.imageExtent = vulkan.swapchain_extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.preTransform = vulkan.surface_capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = present_modes[mode_index],
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};

	uint32_t indices[2] = { vulkan.graphics_family, vulkan.present_family };

	if (vulkan.graphics_family != vulkan.present_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = indices;
	}

	VK_CALL(vkCreateSwapchainKHR(vulkan.device, &create_info, vulkan.allocator, &vulkan.swapchain));

	vkGetSwapchainImagesKHR(vulkan.device, vulkan.swapchain, &vulkan.swapchain_image_count, nullptr);
	vulkan.swapchain_images.resize(vulkan.swapchain_image_count);
	vkGetSwapchainImagesKHR(vulkan.device, vulkan.swapchain, &vulkan.swapchain_image_count, vulkan.swapchain_images.data());

	vulkan.swapchain_views.resize(vulkan.swapchain_image_count);
	for (uint32_t i = 0; i < vulkan.swapchain_image_count; ++i) {
		VkImageViewCreateInfo view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = vulkan.swapchain_images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = vulkan.swapchain_format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		VK_CALL(vkCreateImageView(vulkan.device, &view_create_info, vulkan.allocator, &vulkan.swapchain_views[i]));
	}
}

void vk_init_pipeline(vulkan_t & vulkan, const std::vector<unsigned char> & vertex_spv, const std::vector<unsigned char> & fragment_spv) {
	VkShaderModuleCreateInfo vertex_create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = vertex_spv.size(),
		.pCode = reinterpret_cast<const uint32_t *>(vertex_spv.data()),
	};

	VK_CALL(vkCreateShaderModule(vulkan.device, &vertex_create_info, vulkan.allocator, &vulkan.vertex_shader));

	VkShaderModuleCreateInfo fragment_create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = fragment_spv.size(),
		.pCode = reinterpret_cast<const uint32_t *>(fragment_spv.data()),
	};

	VK_CALL(vkCreateShaderModule(vulkan.device, &fragment_create_info, vulkan.allocator, &vulkan.fragment_shader));

	VkPipelineShaderStageCreateInfo vertex_stage_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vulkan.vertex_shader,
		.pName = "main",
		.pSpecializationInfo = nullptr,
	};

	VkPipelineShaderStageCreateInfo fragment_stage_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = vulkan.fragment_shader,
		.pName = "main",
		.pSpecializationInfo = nullptr,
	};

	VkPipelineShaderStageCreateInfo stages[] = { vertex_stage_create_info, fragment_stage_create_info };

	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo ds_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
		.pDynamicStates = dynamic_states.data(),
	};

	VkVertexInputBindingDescription binding_desc = vertex_binding_desc();
	auto attr_desc = vertex_attr_desc();

	VkPipelineVertexInputStateCreateInfo vis_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding_desc,
		.vertexAttributeDescriptionCount = attr_desc.size(),
		.pVertexAttributeDescriptions = attr_desc.data(),
	};

	VkPipelineInputAssemblyStateCreateInfo ias_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	vulkan.viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(vulkan.swapchain_extent.width),
		.height = static_cast<float>(vulkan.swapchain_extent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	vulkan.scissor = {
		.offset = { 0, 0 },
		.extent = vulkan.swapchain_extent,
	};

	VkPipelineViewportStateCreateInfo vs_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = &vulkan.viewport,
		.scissorCount = 1,
		.pScissors = &vulkan.scissor,
	};

	VkPipelineRasterizationStateCreateInfo rs_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f,
	};

	VkPipelineMultisampleStateCreateInfo ms_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};

	VkPipelineColorBlendAttachmentState cba_state = {
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo cb_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &cba_state,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
	};

	{
		VkDescriptorSetLayoutBinding bindings[2] = {
			{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
				.pImmutableSamplers = nullptr,
			},
			{
				.binding = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.pImmutableSamplers = nullptr,
			},
		};

		VkDescriptorSetLayoutCreateInfo layout_create_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = 2,
			.pBindings = bindings,
		};

		VK_CALL(vkCreateDescriptorSetLayout(vulkan.device, &layout_create_info, vulkan.allocator, &vulkan.desc_layout));
	}

	VkPipelineLayoutCreateInfo layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &vulkan.desc_layout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	VK_CALL(vkCreatePipelineLayout(vulkan.device, &layout_create_info, vulkan.allocator, &vulkan.pipeline_layout));

	VkPipelineDepthStencilStateCreateInfo dss_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {},
	};

	VkGraphicsPipelineCreateInfo pipeline_create_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = 2,
		.pStages = stages,
		.pVertexInputState = &vis_create_info,
		.pInputAssemblyState = &ias_create_info,
		.pTessellationState = nullptr,
		.pViewportState = &vs_create_info,
		.pRasterizationState = &rs_create_info,
		.pMultisampleState = &ms_create_info,
		.pDepthStencilState = &dss_create_info,
		.pColorBlendState = &cb_create_info,
		.pDynamicState = &ds_create_info,
		.layout = vulkan.pipeline_layout,
		.renderPass = vulkan.render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};

	VK_CALL(vkCreateGraphicsPipelines(vulkan.device, VK_NULL_HANDLE, 1, &pipeline_create_info, vulkan.allocator, &vulkan.pipeline));
}

void vk_init_render_pass(vulkan_t & vulkan) {
	VkAttachmentDescription descs[2] = {
		{
			.flags = 0,
			.format = vulkan.swapchain_format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		},
		{
			.flags = 0,
			.format = VK_FORMAT_D32_SFLOAT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
	};

	VkAttachmentReference color = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depth = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass_desc = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = &depth,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr,
	};

	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0,
	};

	VkRenderPassCreateInfo rp_create_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = 2,
		.pAttachments = descs,
		.subpassCount = 1,
		.pSubpasses = &subpass_desc,
		.dependencyCount = 1,
		.pDependencies = &dependency,
	};

	VK_CALL(vkCreateRenderPass(vulkan.device, &rp_create_info, vulkan.allocator, &vulkan.render_pass));
}

void vk_init_framebuffers(vulkan_t & vulkan) {
	vulkan.framebuffers.resize(vulkan.swapchain_views.size());
	for (size_t i = 0; i < vulkan.swapchain_views.size(); ++i) {
		VkImageView attachments[2] = {
			vulkan.swapchain_views[i],
			vulkan.depth_view,
		};

		VkFramebufferCreateInfo fb_create_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = vulkan.render_pass,
			.attachmentCount = 2,
			.pAttachments = attachments,
			.width = vulkan.swapchain_extent.width,
			.height = vulkan.swapchain_extent.height,
			.layers = 1,
		};

		VK_CALL(vkCreateFramebuffer(vulkan.device, &fb_create_info, vulkan.allocator, &vulkan.framebuffers[i]));
	}
}

void vk_create_command_utils(vulkan_t & vulkan) {
	vulkan.cmd_buffers.resize(vulkan.frames_in_flight);

	VkCommandPoolCreateInfo pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = vulkan.graphics_family,
	};

	VK_CALL(vkCreateCommandPool(vulkan.device, &pool_create_info, vulkan.allocator, &vulkan.cmd_pool));

	VkCommandBufferAllocateInfo cmd_alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vulkan.cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = vulkan.frames_in_flight,
	};

	VK_CALL(vkAllocateCommandBuffers(vulkan.device, &cmd_alloc_info, vulkan.cmd_buffers.data()));
}

void vk_begin_cmd(vulkan_t & vulkan, VkCommandBuffer & buffer) {
	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr,
	};

	VK_CALL(vkBeginCommandBuffer(buffer, &begin_info));
}

void vk_end_cmd(vulkan_t & vulkan, VkCommandBuffer & buffer) {
	VK_CALL(vkEndCommandBuffer(buffer));
}

void vk_create_semaphores(vulkan_t & vulkan) {
	vulkan.semaphores_img_avail.resize(vulkan.frames_in_flight);
	vulkan.semaphores_render_finished.resize(vulkan.frames_in_flight);
	vulkan.fences_flight.resize(vulkan.frames_in_flight);

	VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
	};

	VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	for (uint32_t i = 0; i < vulkan.frames_in_flight; ++i) {
		VK_CALL(vkCreateSemaphore(vulkan.device, &semaphore_create_info, vulkan.allocator, &vulkan.semaphores_img_avail[i]));
		VK_CALL(vkCreateSemaphore(vulkan.device, &semaphore_create_info, vulkan.allocator, &vulkan.semaphores_render_finished[i]));
		VK_CALL(vkCreateFence(vulkan.device, &fence_create_info, vulkan.allocator, &vulkan.fences_flight[i]));
	}
}

void vk_frames_in_flight(vulkan_t & vulkan, uint32_t new_count) {
	if (new_count == 0 || new_count == vulkan.frames_in_flight) {
		return;
	}

	vkDeviceWaitIdle(vulkan.device);

	if (new_count < vulkan.frames_in_flight) {
		for (uint32_t i = new_count; i < vulkan.frames_in_flight; ++i) {
			vkDestroySemaphore(vulkan.device, vulkan.semaphores_img_avail[i], vulkan.allocator);
			vkDestroySemaphore(vulkan.device, vulkan.semaphores_render_finished[i], vulkan.allocator);
			vkDestroyFence(vulkan.device, vulkan.fences_flight[i], vulkan.allocator);

			vkDestroyBuffer(vulkan.device, vulkan.unif_buffers[i], vulkan.allocator);
			vkFreeMemory(vulkan.device, vulkan.unif_memorys[i], vulkan.allocator);
		}
		vkFreeCommandBuffers(vulkan.device, vulkan.cmd_pool, vulkan.frames_in_flight - new_count, &vulkan.cmd_buffers[new_count]);
	}

	vulkan.semaphores_img_avail.resize(new_count);
	vulkan.semaphores_render_finished.resize(new_count);
	vulkan.fences_flight.resize(new_count);
	vulkan.cmd_buffers.resize(new_count);
	vulkan.unif_buffers.resize(new_count);
	vulkan.unif_memorys.resize(new_count);
	vulkan.unif_mappeds.resize(new_count);
	
	if (new_count > vulkan.frames_in_flight) {
		for (uint32_t i = vulkan.frames_in_flight; i < new_count; ++i) {
			VkSemaphoreCreateInfo semaphore_create_info = {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
			};

			VkFenceCreateInfo fence_create_info = {
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.pNext = nullptr,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT,
			};

			VK_CALL(vkCreateSemaphore(vulkan.device, &semaphore_create_info, vulkan.allocator, &vulkan.semaphores_img_avail[i]));
			VK_CALL(vkCreateSemaphore(vulkan.device, &semaphore_create_info, vulkan.allocator, &vulkan.semaphores_render_finished[i]));
			VK_CALL(vkCreateFence(vulkan.device, &fence_create_info, vulkan.allocator, &vulkan.fences_flight[i]));

			vulkan.unif_buffers[i] = vk_create_buffer(vulkan, sizeof(uniform_t), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkan.unif_memorys[i]);
			vkMapMemory(vulkan.device, vulkan.unif_memorys[i], 0, sizeof(uniform_t), 0, &vulkan.unif_mappeds[i]);
		}

		VkCommandBufferAllocateInfo cmd_alloc_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = vulkan.cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = vulkan.frames_in_flight,
		};
		VK_CALL(vkAllocateCommandBuffers(vulkan.device, &cmd_alloc_info, &vulkan.cmd_buffers[new_count - vulkan.frames_in_flight]));
	}

	if (vulkan.current_frame >= new_count) {
		vulkan.current_frame = 0;
	}

	vulkan.frames_in_flight = new_count;

	vkDestroyDescriptorPool(vulkan.device, vulkan.desc_pool, vulkan.allocator);
	vk_create_descriptor_utilities(vulkan);
}

void vk_recreate_swapchain(vulkan_t & vulkan) {
	if (vulkan.window_width == 0 || vulkan.window_height == 0) {
		return;
	}
	vkDeviceWaitIdle(vulkan.device);

	for (uint32_t i = 0; i < vulkan.swapchain_image_count; ++i) {
		vkDestroyFramebuffer(vulkan.device, vulkan.framebuffers[i], vulkan.allocator);
	}

	for (uint32_t i = 0; i < vulkan.swapchain_image_count; ++i) {
		vkDestroyImageView(vulkan.device, vulkan.swapchain_views[i], vulkan.allocator);
	}

	vkDestroyImage(vulkan.device, vulkan.depth, vulkan.allocator);
	vkFreeMemory(vulkan.device, vulkan.depth_memory, vulkan.allocator);
	vkDestroyImageView(vulkan.device, vulkan.depth_view, vulkan.allocator);

	vkDestroySwapchainKHR(vulkan.device, vulkan.swapchain, vulkan.allocator);

	vk_create_swapchain(
		vulkan,

		[](const std::vector<VkSurfaceFormatKHR> & formats) {
			for (size_t i = 0; i < formats.size(); ++i) {
				if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					return i;
				}
			}

			return 0ULL;
		},

		[](const std::vector<VkPresentModeKHR> & modes) {
			size_t fifo_index = 0ULL;
			for (size_t i = 0; i < modes.size(); ++i) {
				switch (modes[i]) {
				case VK_PRESENT_MODE_FIFO_KHR:
					fifo_index = i;
					break;
				case VK_PRESENT_MODE_MAILBOX_KHR:
					return i;
				default: break;
				}
			}

			return fifo_index;
		},

		[](const VkSurfaceCapabilitiesKHR & capabilities) {
			return VkExtent2D{ capabilities.currentExtent.width, capabilities.currentExtent.height };
		},

		[](uint32_t min, uint32_t max) {
			uint32_t target = 2;
			return std::max(std::min(target, max), min);
		}
	);

	vk_create_depth(vulkan);
	vk_init_framebuffers(vulkan);
}

void vk_create_buffers(vulkan_t & vulkan) {
	kobj_t kobj;
	{
		std::ifstream file("test.obj", std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			std::cout << "Failed to open test.obj\n";
			throw std::runtime_error("Failed to open test.obj");
		}

		size_t size = file.tellg();
		std::vector<unsigned char> bytes(size);
		file.seekg(0);
		file.read(reinterpret_cast<char *>(bytes.data()), size);
		file.close();

		int ret = kobj_load(&kobj, (void *) bytes.data(), bytes.size());
		if (ret != 0) {
			std::cout << "Failed to load test.obj\n" << ret;
			throw std::runtime_error("Failed to load test.obj");
		}
	}

	vertex_t * vertices = new vertex_t[kobj.vcount];
	uint32_t * indices = new uint32_t[kobj.fcount * 3];

	for (size_t i = 0; i < kobj.vcount; ++i) {
		vertices[i].pos = {
			kobj.vertices[i * 3 + 0],
			kobj.vertices[i * 3 + 1],
			kobj.vertices[i * 3 + 2],
		};

		vertices[i].color = { 1.0f, 1.0f, 1.0f };
	}

	for (size_t i = 0; i < kobj.fcount; ++i) {
		if (kobj.faces[i].v1 != 0) {
			indices[i * 3 + 0] = kobj.faces[i].v1 - 1;
		}
		if (kobj.faces[i].v2 != 0) {
			indices[i * 3 + 1] = kobj.faces[i].v2 - 1;
		}
		if (kobj.faces[i].v3 != 0) {
			indices[i * 3 + 2] = kobj.faces[i].v3 - 1;
		}

		if (kobj.faces[i].vt1 != 0) {
			vertices[kobj.faces[i].v1 - 1].uv = { kobj.uvs[(kobj.faces[i].vt1 - 1) * 2 + 0], kobj.uvs[(kobj.faces[i].vt1 - 1) * 2 + 1] };
		}
		if (kobj.faces[i].vt2 != 0) {
			vertices[kobj.faces[i].v2 - 1].uv = { kobj.uvs[(kobj.faces[i].vt2 - 1) * 2 + 0], kobj.uvs[(kobj.faces[i].vt2 - 1) * 2 + 1] };
		}
		if (kobj.faces[i].vt3 != 0) {
			vertices[kobj.faces[i].v3 - 1].uv = { kobj.uvs[(kobj.faces[i].vt3 - 1) * 2 + 0], kobj.uvs[(kobj.faces[i].vt3 - 1) * 2 + 1] };
		}
	}

	vulkan.mesh_vertex_count = kobj.vcount;
	vulkan.mesh_index_count = kobj.fcount * 3;

	size_t vsize = kobj.vcount * sizeof(vertex_t);
	size_t isize = kobj.fcount * 3 * sizeof(uint32_t);
	size_t size = vsize + isize;

	VkDeviceMemory upload_memory;
	VkBuffer upload = vk_create_buffer(vulkan, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, upload_memory);

	void * memory;
	vkMapMemory(vulkan.device, upload_memory, 0, size, 0, &memory);
	memcpy(memory, vertices, vsize);
	memcpy(reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(memory) + vsize), indices, isize);
	vkUnmapMemory(vulkan.device, upload_memory);

	vulkan.mesh_buffer = vk_create_buffer(vulkan, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan.mesh_memory);
	vk_copy_buffer(vulkan, upload, vulkan.mesh_buffer, size);

	vkDestroyBuffer(vulkan.device, upload, vulkan.allocator);
	vkFreeMemory(vulkan.device, upload_memory, vulkan.allocator);

	vulkan.unif_buffers.resize(vulkan.frames_in_flight);
	vulkan.unif_memorys.resize(vulkan.frames_in_flight);
	vulkan.unif_mappeds.resize(vulkan.frames_in_flight);
	for (uint32_t i = 0; i < vulkan.frames_in_flight; ++i) {
		vulkan.unif_buffers[i] = vk_create_buffer(vulkan, sizeof(uniform_t), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkan.unif_memorys[i]);
		vkMapMemory(vulkan.device, vulkan.unif_memorys[i], 0, sizeof(uniform_t), 0, &vulkan.unif_mappeds[i]);
	}
}

VkBuffer vk_create_buffer(vulkan_t & vulkan, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceMemory & memory) {
	VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
	};

	VkBuffer buffer;
	VK_CALL(vkCreateBuffer(vulkan.device, &create_info, vulkan.allocator, &buffer));

	VkMemoryRequirements reqs;
	vkGetBufferMemoryRequirements(vulkan.device, buffer, &reqs);

	uint32_t memory_type = vk_memory_type(vulkan, reqs.memoryTypeBits, props);
	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = reqs.size,
		.memoryTypeIndex = memory_type,
	};

	VK_CALL(vkAllocateMemory(vulkan.device, &alloc_info, vulkan.allocator, &memory));
	VK_CALL(vkBindBufferMemory(vulkan.device, buffer, memory, 0));

	return buffer;
}

void vk_copy_buffer(vulkan_t & vulkan, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vulkan.cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer cmd_buffer;
	VK_CALL(vkAllocateCommandBuffers(vulkan.device, &alloc_info, &cmd_buffer));

	vk_begin_cmd(vulkan, cmd_buffer);

	VkBufferCopy copy = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size,
	};

	vkCmdCopyBuffer(cmd_buffer, src, dst, 1, &copy);

	vk_end_cmd(vulkan, cmd_buffer);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd_buffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr,
	};

	vkQueueSubmit(vulkan.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkan.graphics_queue);

	vkFreeCommandBuffers(vulkan.device, vulkan.cmd_pool, 1, &cmd_buffer);
}

void vk_create_descriptor_utilities(vulkan_t & vulkan) {
	VkDescriptorPoolSize pool_sizes[2] = {
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = vulkan.frames_in_flight,
		},
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = vulkan.frames_in_flight,
		},
	};

	VkDescriptorPoolCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = vulkan.frames_in_flight,
		.poolSizeCount = 2,
		.pPoolSizes = pool_sizes,
	};

	VK_CALL(vkCreateDescriptorPool(vulkan.device, &create_info, vulkan.allocator, &vulkan.desc_pool));

	std::vector<VkDescriptorSetLayout> layouts(vulkan.frames_in_flight, vulkan.desc_layout);
	VkDescriptorSetAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = vulkan.desc_pool,
		.descriptorSetCount = vulkan.frames_in_flight,
		.pSetLayouts = layouts.data(),
	};

	vulkan.desc_sets.resize(vulkan.frames_in_flight);
	VK_CALL(vkAllocateDescriptorSets(vulkan.device, &alloc_info, vulkan.desc_sets.data()));

	for (uint32_t i = 0; i < vulkan.frames_in_flight; ++i) {
		VkDescriptorBufferInfo buffer_info = {
			.buffer = vulkan.unif_buffers[i],
			.offset = 0,
			.range = sizeof(uniform_t),
		};

		VkDescriptorImageInfo image_info = {
			.sampler = vulkan.texture_sampler,
			.imageView = vulkan.texture_view,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		VkWriteDescriptorSet writes[2] = {
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = vulkan.desc_sets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pImageInfo = nullptr,
				.pBufferInfo = &buffer_info,
				.pTexelBufferView = nullptr,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = vulkan.desc_sets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &image_info,
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr,
			},
		};

		vkUpdateDescriptorSets(vulkan.device, 2, writes, 0, nullptr);
	}
}

void vk_create_texture(vulkan_t & vulkan) {
	ktga_t ktga {};
	{
		std::ifstream file("test.tga", std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			std::cout << "Failed to open test.tga\n";
			throw std::runtime_error("Failed to open test.tga");
		}

		size_t size = file.tellg();
		std::vector<unsigned char> bytes(size);
		file.seekg(0);
		file.read(reinterpret_cast<char *>(bytes.data()), size);
		file.close();

		int ret = ktga_load(&ktga, (void *) bytes.data(), bytes.size());
		if (ret != 0) {
			std::cout << "Failed to load test.tga\n" << ret;
			throw std::runtime_error("Failed to load test.tga");
		}
	}

	VkDeviceSize size = ktga.header.img_w * ktga.header.img_h * (ktga.header.bpp / 8);
	VkDeviceMemory upload_memory;
	VkBuffer upload = vk_create_buffer(vulkan, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, upload_memory);
	void * memory;
	vkMapMemory(vulkan.device, upload_memory, 0, size, 0, &memory);
	memcpy(memory, ktga.bitmap, size);
	vkUnmapMemory(vulkan.device, upload_memory);

	ktga_destroy(&ktga);

	vulkan.texture = vk_create_image(
		vulkan,
		{
			.width = ktga.header.img_w,
			.height = ktga.header.img_h,
			.depth = 1,
		},
		VK_IMAGE_TYPE_2D, VK_FORMAT_B8G8R8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan.texture_memory
	);

	vk_transition_image(vulkan, vulkan.texture, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vk_copy_buffer_to_image(vulkan, upload, vulkan.texture, { ktga.header.img_w, ktga.header.img_h, 1 });
	vk_transition_image(vulkan, vulkan.texture, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(vulkan.device, upload, vulkan.allocator);
	vkFreeMemory(vulkan.device, upload_memory, vulkan.allocator);

	VkImageViewCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = vulkan.texture,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_B8G8R8A8_SRGB,
		.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VK_CALL(vkCreateImageView(vulkan.device, &create_info, vulkan.allocator, &vulkan.texture_view));

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(vulkan.physical, &props);

	VkPhysicalDeviceFeatures feats;
	vkGetPhysicalDeviceFeatures(vulkan.physical, &feats);

	VkSamplerCreateInfo s_create_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1.0f,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};

	VK_CALL(vkCreateSampler(vulkan.device, &s_create_info, vulkan.allocator, &vulkan.texture_sampler));
}

VkImage vk_create_image(vulkan_t & vulkan, VkExtent3D extent, VkImageType type, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props, VkDeviceMemory & memory) {
	VkImageCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.imageType = type,
		.format = format,
		.extent = extent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VkImage image;
	VK_CALL(vkCreateImage(vulkan.device, &create_info, vulkan.allocator, &image));

	VkMemoryRequirements reqs;
	vkGetImageMemoryRequirements(vulkan.device, image, &reqs);

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = reqs.size,
		.memoryTypeIndex = vk_memory_type(vulkan, reqs.memoryTypeBits, props),
	};

	VK_CALL(vkAllocateMemory(vulkan.device, &alloc_info, vulkan.allocator, &memory));
	vkBindImageMemory(vulkan.device, image, memory, 0);

	return image;
}

void vk_transition_image(vulkan_t & vulkan, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) {
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vulkan.cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer cmd_buffer;
	VK_CALL(vkAllocateCommandBuffers(vulkan.device, &alloc_info, &cmd_buffer));

	vk_begin_cmd(vulkan, cmd_buffer);

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VkPipelineStageFlags src_stage;
	VkPipelineStageFlags dst_stage;

	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		
		if (format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		throw std::invalid_argument("Unsupported layout transition");
	}

	vkCmdPipelineBarrier(cmd_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vk_end_cmd(vulkan, cmd_buffer);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd_buffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr,
	};

	vkQueueSubmit(vulkan.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkan.graphics_queue);

	vkFreeCommandBuffers(vulkan.device, vulkan.cmd_pool, 1, &cmd_buffer);
}

void vk_copy_buffer_to_image(vulkan_t & vulkan, VkBuffer buffer, VkImage image, VkExtent3D extent) {
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vulkan.cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer cmd_buffer;
	VK_CALL(vkAllocateCommandBuffers(vulkan.device, &alloc_info, &cmd_buffer));

	vk_begin_cmd(vulkan, cmd_buffer);

	VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = { 0, 0, 0, },
		.imageExtent = extent,
	};

	vkCmdCopyBufferToImage(cmd_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	vk_end_cmd(vulkan, cmd_buffer);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd_buffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr,
	};

	vkQueueSubmit(vulkan.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkan.graphics_queue);

	vkFreeCommandBuffers(vulkan.device, vulkan.cmd_pool, 1, &cmd_buffer);
}

void vk_create_depth(vulkan_t & vulkan) {
	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(vulkan.physical, VK_FORMAT_D32_SFLOAT, &props);

	VkImageTiling tiling;
	if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		tiling = VK_IMAGE_TILING_LINEAR;
	} else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		tiling = VK_IMAGE_TILING_OPTIMAL;
	} else {
		throw std::runtime_error("Format Unsupported");
	}

	vulkan.depth = vk_create_image(
		vulkan,
		{
			.width = vulkan.swapchain_extent.width,
			.height = vulkan.swapchain_extent.height,
			.depth = 1,
		},
		VK_IMAGE_TYPE_2D, VK_FORMAT_D32_SFLOAT,
		tiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan.depth_memory
	);

	VkImageViewCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = vulkan.depth,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_D32_SFLOAT,
		.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VK_CALL(vkCreateImageView(vulkan.device, &create_info, vulkan.allocator, &vulkan.depth_view));
	vk_transition_image(vulkan, vulkan.depth, VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}
*/