#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
#include "vulkan_memory_allocator.h"
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    VmaAllocator g_vma_allocator;
    
    void vulkan_init_memory_allocator(VkDevice device, VkPhysicalDevice physical_device, VkInstance instance) {
        VmaAllocatorCreateInfo allocator_info = {};
        allocator_info.vulkanApiVersion = VK_API_VERSION_1_2;
        allocator_info.physicalDevice = physical_device;
        allocator_info.device = device;
        allocator_info.instance = instance;
        
        VkResult result = vmaCreateAllocator(&allocator_info, &g_vma_allocator);
        assert(result == VK_SUCCESS);
    }
    
    VkBuffer alloc_vulkan_buffer(size_t size, void *data, Vulkan_allocation_usage allocation_usage) {
#if 1
        VkBufferUsageFlagBits buffer_usage;
        VmaMemoryUsage memory_usage;
        VmaAllocationCreateFlags create_flags;
        bool use_flags = false;
        
        switch (allocation_usage) {
            case usage_staging_buffer: {
                buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                memory_usage = VMA_MEMORY_USAGE_CPU_ONLY;
                create_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
                use_flags = true;
                break;
            }
            case usage_vertex_buffer: {
                buffer_usage = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
                memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
                break;
            }
            case usage_index_buffer: {
                buffer_usage = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
                memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
                break;
            }
            default:
            assert(0);
        }
        
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = buffer_usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VmaAllocationCreateInfo alloc_create_info = {};
        alloc_create_info.usage = memory_usage;
        if (use_flags) {
            alloc_create_info.flags = create_flags;
        }
        
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo alloc_info;
        vmaCreateBuffer(g_vma_allocator, &buffer_info, &alloc_create_info, &buffer, &allocation, &alloc_info);
        if (allocation_usage == usage_staging_buffer) {
            memcpy(alloc_info.pMappedData, data, size);
        }
        
        return buffer;
#else
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = 65536;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        VkBuffer buffer;
        VmaAllocation allocation;
        vmaCreateBuffer(g_vma_allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
        return buffer;
#endif
    }
#if 0
    VkImage alloc_vulkan_image(VkExtent3D extent, int mip_levels, VkFormat format, Vulkan_allocation_usage allocation_usage) {
        VkImageUsageFlags image_usage;
        VmaMemoryUsage memory_usage;
        VmaAllocationCreateFlags create_flags;
        bool use_flags = false;
        
        switch (allocation_usage) {
            case staging_dst_allocation_usage: {
                image_usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
                break;
            }
        }
        
        VkImageCreateInfo image_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent = extent;
        image_info.mipLevels = mip_levels;
        image_info.arrayLayers = 1;
        image_info.format = format;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = image_usage;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VmaAllocationCreateInfo alloc_create_info = {};
        alloc_create_info.usage = memory_usage;
        if (use_flags) {
            alloc_create_info.flags = create_flags;
        }
        
        VkImage image;
        VmaAllocation allocation;
        VmaAllocationInfo alloc_info;
        vmaCreateImage(g_vma_allocator, &image_info, &alloc_create_info, &image, &allocation, &alloc_info);
        
        return image;
    }
    
#endif
#ifdef __cplusplus
}
#endif