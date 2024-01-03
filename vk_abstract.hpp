#ifndef VK_ABSTRACT_HPP
#define VK_ABSTRACT_HPP

#include "common.hpp"
#include <vector>

struct vk_buffer_t {
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize size;
};

struct vk_mesh_t {
	vk_buffer_t buffer;
	uint32_t mesh_vertex_count;
	uint32_t mesh_index_count;
};

struct vk_image_t {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};

struct vk_pipeline_create_t {
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;
	VkPrimitiveTopology topology;
	VkPolygonMode polygon;
	VkCullModeFlags cull;
	VkFrontFace front;
	VkBool32 depth_test;
	VkCompareOp depth_comparison;
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
};

struct vk_pipeline_t {
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_layout;
};

struct transform_t {
	vec3 position;
	vec3 rotation;
	vec3 scale;
};

struct mesh_t {
	vk_mesh_t mesh;
	vk_pipeline_t * pipeline;
	transform_t transform;
};

#endif