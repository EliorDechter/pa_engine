
#ifndef VULKAN_RENDERER
#define VULKAN_RENDERER

#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT
#define NUM_VIEWPORTS 1
#define NUM_SCISSORS NUM_VIEWPORTS
#define DEFAULT_FENCE_TIMEOUT 100000000000

#define  FAST_OBJ_IMPLEMENTATION
#include "src/externals/fast_obj.h"

#include <vulkan/vulkan.h>
#include "vulkan_memory_allocator.h"

void vulkan_assert(VkResult result) {
    assert(result == VK_SUCCESS);
}

typedef struct Uniform_buffer_data {
    Transform_data data;
} Uniform_buffer_data;

Vertex create_vertex(v3 pos, v2 uv, v4 color) {
    Vertex v = {
        .pos = pos,
        .textcoords = uv,
        .color = color
    };
    
    return v;
}

typedef struct Vulkan_texture {
    VkImage image;
    VkImageLayout image_layout;
    VkDeviceMemory device_memory;
    VkImageView image_view;
    u32 width, height;
    u32 mip_levels;
} Vulkan_texture;

typedef struct Vulkan_buffer {
    VkBuffer buffer;
    VkDeviceMemory device_memory;
    void *mapped_data;
    VkDeviceSize size;
} Vulkan_buffer;

typedef struct Vulkan_mesh {
    Vulkan_buffer vertex_buffer, index_buffer;
    u32 vertex_count, index_count;
} Vulkan_mesh;

typedef struct Vulkan_object {
    Vulkan_texture texture;
    Vulkan_buffer uniform_buffer;
    VkDescriptorSet descriptor_set;
    Vulkan_mesh vulkan_mesh;
    Transform_data *transform_data;
} Vulkan_object;

typedef struct Vulkan_info {
    //all the stuff that were created in vulkan init
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    VkPhysicalDeviceProperties physical_device_properties;
    VkQueue queue;
    u32 queue_family_index;
    VkSurfaceKHR surface;
    //VkSurfaceCapabilitiesKHR surface_capabilities; amybe later?
    VkFormat color_format;
    VkColorSpaceKHR color_space;
    VkFormat depth_format;
    u32 current_framebuffer;
    VkPipelineStageFlags submit_pipeline_stages; //deleting will result in an error, do not remove this line
    VkSubmitInfo submit_info;
    
    //this too get created in vulkan init and are used to sync with the swapchain
    VkSemaphore present_complete_semaphore;
    VkSemaphore render_complete_semaphore;
    
    //all the stuff that gets created in the create swapchain function
    VkSwapchainKHR swapchain;
    VkImage *swapchain_images;
    VkImageView *swapchain_image_views;
    u32 swapchain_image_count;
    
    //command pool
    VkCommandPool command_pool;
    
    //command buffers
    VkCommandBuffer *draw_command_buffers;
    u32 draw_command_buffers_num;
    
    //depth stencil stuff
    VkImage depth_image;
    VkImageView depth_image_view;
    VkDeviceMemory depth_image_memory;
    
    //render pass
    VkRenderPass render_pass;
    
    //pipeline cache
    VkPipelineCache pipeline_cache;
    
    VkFramebuffer *framebuffers;
    u32 framebuffers_count;
    
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    
    //synchonization primitives
    VkFence *wait_fences;
    
    //prepared? what does that mean?
    bool prepared;
    
    //view updated
    bool view_updated;
    
    bool init;
    
    //uniform buffer
    Vulkan_buffer uniform_buffer;
    size_t uniform_buffer_dynamic_alignment;
    
    //sampler
    VkSampler sampler;
    
} Vulkan_info;

Vulkan_info g_vulkan;

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
                                                           VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                           void* pUserData) {
    const char *prefix = "";
    
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        prefix = "VERBOSE: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        prefix = "INFO: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        prefix = "WARNING: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        prefix = "ERROR: ";
    }
    
    // Display message to default output (console/logcat)
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        fprintf(stdout, "VULKAN_%s[%d][%s] : %s\n", prefix,  pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
    }
    else {
        fprintf(stderr, "%s[%d][%s] : %s\n", prefix,  pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
    }
    fflush(stdout);
    
    return VK_FALSE;
}

bool get_memory_type(u32 type_bits, VkMemoryPropertyFlags properties, u32 *memory_type) {
    VkPhysicalDeviceMemoryProperties memory_properties = g_vulkan.physical_device_memory_properties;
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if ((type_bits & 1) == 1)
        {
            if ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                *memory_type = i;
                return true;
            }
        }
        type_bits >>= 1;
    }
    return false;
}

void map_buffer(Vulkan_buffer *buffer) {
    //TODO: add size and offset parameters
    VkResult result = vkMapMemory(g_vulkan.device, buffer->device_memory, 0, VK_WHOLE_SIZE, 0, &buffer->mapped_data);
    vulkan_assert(result);
}

void unmap_buffer(Vulkan_buffer *buffer) {
    if (buffer->mapped_data) {
        vkUnmapMemory(g_vulkan.device, buffer->device_memory);
        buffer->mapped_data = NULL;
    }
}

void flush_buffer(Vulkan_buffer *buffer) {
    VkMappedMemoryRange mapped_range = {};
    mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mapped_range.memory = buffer->device_memory;
    mapped_range.offset = 0;
    mapped_range.size = VK_WHOLE_SIZE;
    VkResult result = vkFlushMappedMemoryRanges(g_vulkan.device, 1, &mapped_range);
    vulkan_assert(result);
}

void vulkan_update_buffer(Vulkan_buffer *buffer, void *data, size_t size) {
    assert(size <= buffer->size);
    memcpy(buffer->mapped_data, data, size);
}

Vulkan_buffer vulkan_create_buffer(VkDeviceSize buffer_size, void *data,  VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags) {
    
    Vulkan_buffer buffer = {0};
    
    //NOTE: sascha uses two function: the first one uses a given device memory and the second one uses the buffers struct own memory
    VkResult result; 
    VkDevice device = g_vulkan.device;
    
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = NULL;
    buffer_create_info.usage = buffer_usage_flags;
    buffer_create_info.size = buffer_size;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = NULL;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.flags = 0;
    VkBuffer buffer_handle;
    result = vkCreateBuffer(device, &buffer_create_info, NULL, &buffer_handle);
    vulkan_assert(result);
    
    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer_handle, &buffer_memory_requirements);
    
    VkMemoryAllocateInfo buffer_memory_allocate_info = {};
    buffer_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    buffer_memory_allocate_info.pNext = NULL;
    buffer_memory_allocate_info.allocationSize = buffer_memory_requirements.size;
    bool memory_type_result = get_memory_type(buffer_memory_requirements.memoryTypeBits,
                                              memory_property_flags,
                                              &buffer_memory_allocate_info.memoryTypeIndex);
    
    assert(memory_type_result);
    
    result = vkAllocateMemory(device, &buffer_memory_allocate_info, NULL, &buffer.device_memory);
    vulkan_assert(result);
    
    buffer.buffer = buffer_handle;
    
    if (data) {
        map_buffer(&buffer);
        vulkan_update_buffer(&buffer, data, buffer_size);
        if ((memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
            flush_buffer(&buffer);
        }
        //unmap_buffer(vulkan_info, buffer);
    }
    
    result = vkBindBufferMemory(device, buffer.buffer, buffer.device_memory, 0);
    vulkan_assert(result);
    
    buffer.size = buffer_size;
    
    return buffer;
    
}

void copy_buffer(VkBuffer src_buffer, VkBuffer dest_buffer, VkDeviceSize size, VkCommandPool command_pool, VkDevice device, VkQueue graphics_queue) {
    
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.commandBufferCount = 1;
    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer);
    
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &begin_info);
    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0; 
    copy_region.dstOffset = 0; 
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dest_buffer, 1, &copy_region);
    vkEndCommandBuffer(command_buffer);
    
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);
    
    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void begin_command_buffer(VkCommandBufferLevel level,  VkCommandBuffer *command_buffer) {
    VkResult result;
    VkCommandBufferAllocateInfo command_buffer_allocation_info = {};
    command_buffer_allocation_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocation_info.level = level;
    command_buffer_allocation_info.commandPool = g_vulkan.command_pool;
    command_buffer_allocation_info.commandBufferCount = 1;
    
    vkAllocateCommandBuffers(g_vulkan.device, &command_buffer_allocation_info, command_buffer);
    
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    result = vkBeginCommandBuffer(*command_buffer, &begin_info);
    vulkan_assert(result);
}

void end_command_buffer(VkCommandBuffer command_buffer) {
    VkResult result;
    result = vkEndCommandBuffer(command_buffer);
    vulkan_assert(result);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;
    
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0;
    VkFence fence;
    result = vkCreateFence(g_vulkan.device, &fence_info, NULL, &fence);
    
    result = vkQueueSubmit(g_vulkan.queue, 1, &submitInfo, fence);
    vulkan_assert(result);
    /*
    result = vkQueueWaitIdle(graphicsQueue);
    vulkan_assert(result);
    */
    result = vkWaitForFences(g_vulkan.device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
    vulkan_assert(result);
    
    vkDestroyFence(g_vulkan.device, fence, NULL);
    
    vkFreeCommandBuffers(g_vulkan.device, g_vulkan.command_pool, 1, &command_buffer);
}

#if 0
void copy_buffer_to_image(VkImage image, u32 width, u32 height) {
    VkCommandBuffer command_buffer = beginSingleTimeCommands(command_pool, device);
    
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D){ 0, 0, 0 };
    region.imageExtent = (VkExtent3D){ width, height, 1 };
    
    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    endSingleTimeCommands(command_buffer, graphicsQueue, device, command_pool);
}
#endif

void set_image_layout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresource_range, 
                      VkCommandPool commandPool, VkCommandBuffer command_buffer) {
#if 0
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.image = image;
    barrier.subresourceRange = subresource_range;
    switch (old_layout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        barrier.srcAccessMask = 0;
        break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
        default:
        break;
    }
    
    switch (new_layout)
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        if (barrier.srcAccessMask == 0)
        {
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
        default:
        break;
    }
#endif
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags source_stage = 0;
    VkPipelineStageFlags destination_stage = 0;
    
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        assert("unssopred layout transition");
    }
    
    vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

Vulkan_texture vulkan_create_texture(Texture loaded_texture) {
    VkFormat format;
    int num_channels = loaded_texture.channels;
    if (num_channels == 1) {
        format = VK_FORMAT_R8_UNORM;
    }
    else if (num_channels == 4) {
        format = VK_FORMAT_R8G8B8A8_UNORM;
    }
    else {
        assert(0);
    }
    
    VkResult result; 
    
    VkDevice device = g_vulkan.device;
    
    int image_width, image_height, channels;
    
#if 0
    unsigned char *pixels = stbi_load(path, &image_width, &image_height, &channels, STBI_rgb_alpha);
    assert(pixels);
    //assert ((VkDeviceSize)sizeof(pixels) == image_size);
#else
    image_width = loaded_texture.width;
    image_height = loaded_texture.height;
    channels = loaded_texture.channels;
    u8 *pixels = loaded_texture.data;
#endif
    
    VkDeviceSize image_size = image_width * image_height * channels;
    
    VkCommandBuffer copy_command_buffer;
    begin_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, &copy_command_buffer);
    
#if 1
    VkBuffer staging_buffer;
    VkDeviceMemory staging_device_memory;
    
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = image_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    result = vkCreateBuffer(g_vulkan.device, &buffer_create_info, NULL, &staging_buffer);
    vulkan_assert(result);
    
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(g_vulkan.device, staging_buffer, &memory_requirements);
    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.pNext = NULL;
    memory_allocate_info.allocationSize = memory_requirements.size;
    bool memory_type_result = get_memory_type(memory_requirements.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                              &memory_allocate_info.memoryTypeIndex);
    assert(memory_type_result);
    
    result = vkAllocateMemory(g_vulkan.device, &memory_allocate_info, NULL, &staging_device_memory);
    vulkan_assert(result);
    result = vkBindBufferMemory(device, staging_buffer, staging_device_memory, 0);
    vulkan_assert(result);
    
    // Copy texture data into host local staging buffer
    uint8_t *data;
    result = vkMapMemory(device, staging_device_memory, 0, memory_requirements.size, 0, (void **)&data);
    vulkan_assert(result);
    memcpy(data, pixels, (size_t)image_size);
    vkUnmapMemory(device, staging_device_memory);
    
#else
    VkBuffer staging_buffer = alloc_vulkan_buffer((size_t)image_size, pixels, staging_src_allocation_usage);
    
#endif
    
    // Setup buffer copy regions for each mip level
    u32 mip_levels = 1;
    VkBufferImageCopy *buffer_copy_regions = malloc(sizeof(VkBufferImageCopy) * mip_levels);
    
#if 0
    for (u32 i = 0; i < mip_levels; i++) {
        // Calculate offset into staging buffer for the current mip level
        ktx_size_t offset;
        KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
        assert(ret == KTX_SUCCESS);
        // Setup a buffer image copy structure for the current mip level
        VkBufferImageCopy buffer_copy_region = {};
        buffer_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        buffer_copy_region.imageSubresource.mipLevel = i;
        buffer_copy_region.imageSubresource.baseArrayLayer = 0;
        buffer_copy_region.imageSubresource.layerCount = 1;
        buffer_copy_region.imageExtent.width = image_width >> i;
        buffer_copy_region.imageExtent.height = imaeg_height >> i;
        buffer_copy_region.imageExtent.depth = 1;
        buffer_copy_region.bufferOffset = offset;
        buffer_copy_regions[i] = buffer_copy_region;
    }
#else
    //NOTE: runtime mipmapping doesnt work for now
    VkBufferImageCopy buffer_copy_region = {};
    buffer_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_copy_region.imageSubresource.mipLevel = 0;
    buffer_copy_region.imageSubresource.baseArrayLayer = 0;
    buffer_copy_region.imageSubresource.layerCount = 1;
    buffer_copy_region.imageExtent.width = image_width;
    buffer_copy_region.imageExtent.height = image_height;
    buffer_copy_region.imageExtent.depth = 1;
    buffer_copy_region.bufferOffset = 0;
    buffer_copy_regions[0] = buffer_copy_region;
#endif
    
    //TODO: Fix me
    mip_levels = 1;
    
#if 1
    VkImage image;
    VkDeviceMemory image_device_memory;
    
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = image_width;
    image_create_info.extent.height = image_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = mip_levels;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    result = vkCreateImage(g_vulkan.device, &image_create_info, NULL, &image);
    vulkan_assert(result);
    
    vkGetImageMemoryRequirements(device, image, &memory_requirements);
    memory_allocate_info.memoryTypeIndex = 
        memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_type_result = get_memory_type(memory_requirements.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                         &memory_allocate_info.memoryTypeIndex);
    assert(memory_type_result);
    result = vkAllocateMemory(device, &memory_allocate_info, NULL, &image_device_memory);
    vulkan_assert(result);
    
    vkBindImageMemory(device, image, image_device_memory, 0);
#else
    VkExtent3D image_extent = { .width = image_width, .height = image_height, .depth = 1 };
    VkImage image = alloc_vulkan_image(image_extent, mip_levels, format, staging_dst_allocation_usage);
#endif
    
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = mip_levels;
    subresource_range.layerCount = 1;
    
    set_image_layout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range, g_vulkan.command_pool, copy_command_buffer);
    
    //copy_buffer_to_image(staging_buffer, image, (uint32_t)width, (uint32_t)height, command_pool, device, graphics_queue);
    vkCmdCopyBufferToImage(copy_command_buffer, staging_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, buffer_copy_regions);
    
    set_image_layout(image,  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range, g_vulkan.command_pool, copy_command_buffer);
    
    end_command_buffer(copy_command_buffer);
    
    vkFreeMemory(device, staging_device_memory, NULL);
    vkDestroyBuffer(device, staging_buffer, NULL);
    
    //init texture image views
    //TODO: am i sure about the format?
    VkImageView image_view;
    //VkDeviceMemory image_memory;
    
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    result = vkCreateImageView(device, &view_info, NULL, &image_view);
    vulkan_assert(result);
    
    Vulkan_texture texture = {
        .image = image,
        .image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        //.device_memory = image_device_memory,
        .image_view = image_view,
        .width = image_width,
        .height = image_height,
        .mip_levels = mip_levels,
    };
    
    free(buffer_copy_regions);
    
    return texture;
}

void vulkan_create_swapchain(u32 *width, u32 *height, bool vsync) {
    VkResult result;
    
    VkSwapchainKHR old_swapchain = g_vulkan.swapchain;
    
    VkSurfaceCapabilitiesKHR surface_capabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_vulkan.physical_device, g_vulkan.surface, &surface_capabilities);
    vulkan_assert(result);
    
    uint32_t present_mode_count;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(g_vulkan.physical_device, g_vulkan.surface, &present_mode_count, NULL);
    vulkan_assert(result);
    assert(present_mode_count > 0);
    
    VkPresentModeKHR *present_modes = (VkPresentModeKHR *)malloc(present_mode_count * sizeof(VkPresentModeKHR));
    assert(present_modes);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(g_vulkan.physical_device, g_vulkan.surface, &present_mode_count, present_modes);
    vulkan_assert(result);
    
    VkExtent2D swapchain_extent = {};
    
    if (surface_capabilities.currentExtent.width == (uint32_t)-1) {
        swapchain_extent.width = *width;
        swapchain_extent.height = *height;
    }
    else {
        swapchain_extent = surface_capabilities.currentExtent;
        *width = surface_capabilities.currentExtent.width;
        *height = surface_capabilities.currentExtent.height;
    }
    
    uint32_t desired_num_of_swapchain_images = surface_capabilities.minImageCount + 1;
    if ((surface_capabilities.maxImageCount > 0) && (desired_num_of_swapchain_images > surface_capabilities.maxImageCount))
    {
        desired_num_of_swapchain_images = surface_capabilities.maxImageCount;
    }
    
    VkSurfaceTransformFlagsKHR pre_transform;
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        // We prefer a non-rotated transform
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        pre_transform = surface_capabilities.currentTransform;
    }
    
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    swapchain_create_info.surface = g_vulkan.surface;
    swapchain_create_info.minImageCount = desired_num_of_swapchain_images;
    swapchain_create_info.imageFormat = g_vulkan.color_format;
    swapchain_create_info.imageExtent.width = swapchain_extent.width;
    swapchain_create_info.imageExtent.height = swapchain_extent.height;
    swapchain_create_info.preTransform = pre_transform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //NOTE: sashca has for loop if alpha shit can be used
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR; 
    swapchain_create_info.oldSwapchain = old_swapchain;
    swapchain_create_info.clipped = VK_TRUE; 
    swapchain_create_info.imageColorSpace = g_vulkan.color_space;
    //TODO: sascha has add a number of flags here:  VK_IMAGE_USAGE_TRANSFER_SRC_BIT
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //NOTE: if the presentation and graphics queue are different we'll beed to change this varaible
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = NULL;
    
    // Enable transfer source on swap chain images if supported
    if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    
    // Enable transfer destination on swap chain images if supported
    if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    
    VkSwapchainKHR swapchain;
    result = vkCreateSwapchainKHR(g_vulkan.device, &swapchain_create_info, NULL, &swapchain);
    vulkan_assert(result);
    
    if(old_swapchain != VK_NULL_HANDLE) {
        for (u32 i = 0; i < g_vulkan.swapchain_image_count; ++i) {
            vkDestroyImageView(g_vulkan.device, g_vulkan.swapchain_image_views[i], NULL);
        }
        vkDestroySwapchainKHR(g_vulkan.device, old_swapchain, NULL);
    }
    
    //init image views
    result = vkGetSwapchainImagesKHR(g_vulkan.device, swapchain, &g_vulkan.swapchain_image_count, NULL);
    vulkan_assert(result);
    
    VkImage *swapchain_images = (VkImage *)malloc(g_vulkan.swapchain_image_count * sizeof(VkImage));
    assert(swapchain_images);
    result = vkGetSwapchainImagesKHR(g_vulkan.device, swapchain, &g_vulkan.swapchain_image_count, swapchain_images);
    vulkan_assert(result);
    
    VkImageView *swapchain_image_views = (VkImageView *)malloc(sizeof(VkImageView) * g_vulkan.swapchain_image_count);
    for (uint32_t i = 0; i < g_vulkan.swapchain_image_count; i++) {
        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = NULL;
        image_view_create_info.format = g_vulkan.color_format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.flags = 0;
        image_view_create_info.image = swapchain_images[i];
        
        VkImageView image_view;
        result = vkCreateImageView(g_vulkan.device, &image_view_create_info, NULL, &image_view);
        swapchain_image_views[i] = image_view;
        vulkan_assert(result);
    }
    
    //free(swapchain_images); TODO: not so sure about it
    //uint32_t current_buffer = 0;
    
    if (present_modes) {
        free(present_modes);
    }
    
    g_vulkan.swapchain = swapchain;
    g_vulkan.swapchain_images = swapchain_images;
    g_vulkan.swapchain_image_views = swapchain_image_views;
}

void create_depth_stencil() {
    VkResult result;
    
    VkDevice device = g_vulkan.device;
    VkPhysicalDevice physical_device = g_vulkan.physical_device;
    
    VkImageCreateInfo image_create_info = {};
    VkFormatProperties format_properties; //TODO: didnt I do it earlier?
#if 0
    VkFormat depth_buffer_format = VK_FORMAT_D16_UNORM; //TODO: maybe go for a different format 
#else
    VkFormat depth_buffer_format = g_vulkan.depth_format;
#endif
    vkGetPhysicalDeviceFormatProperties(physical_device, depth_buffer_format, &format_properties);
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL; //TODO: better check what type to use
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = g_vulkan.depth_format;
    image_create_info.extent.width = g_screen_width;
    image_create_info.extent.height = g_screen_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImage depth_image;
    result = vkCreateImage(g_vulkan.device, &image_create_info, NULL, &depth_image);
    vulkan_assert(result);
    
    VkMemoryAllocateInfo depth_allocate_info = {};
    depth_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depth_allocate_info.pNext = NULL;
    VkMemoryRequirements depth_memory_requirements;
    vkGetImageMemoryRequirements(device, depth_image, &depth_memory_requirements);
    depth_allocate_info.allocationSize = depth_memory_requirements.size;
    //TODO: take another look at this function
    bool memory_type_result = get_memory_type(depth_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth_allocate_info.memoryTypeIndex);
    assert(memory_type_result);
    VkDeviceMemory depth_memory;
    result = vkAllocateMemory(device, &depth_allocate_info, NULL, &depth_memory);
    vulkan_assert(result);
    result = vkBindImageMemory(device, depth_image, depth_memory, 0);
    vulkan_assert(result);
    
    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = NULL;
    image_view_info.image = VK_NULL_HANDLE;
    image_view_info.format = depth_buffer_format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_R;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_G;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_B;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_A;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.flags = 0;
    image_view_info.image = depth_image;
    //NOTE: de fuck
    if (depth_buffer_format == VK_FORMAT_D16_UNORM_S8_UINT || depth_buffer_format == VK_FORMAT_D24_UNORM_S8_UINT ||
        depth_buffer_format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        image_view_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    VkImageView depth_image_view;
    result = vkCreateImageView(device, &image_view_info, NULL, &depth_image_view);
    vulkan_assert(result);
    
    g_vulkan.depth_image_view = depth_image_view;
    g_vulkan.depth_image = depth_image;
    g_vulkan.depth_image_memory = depth_memory;
}

void setup_framebuffer() {
    VkResult result;
    u32 swapchain_image_count = g_vulkan.swapchain_image_count;
    VkDevice device = g_vulkan.device;
    VkFramebuffer *framebuffers = (VkFramebuffer *)malloc(swapchain_image_count * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < swapchain_image_count; ++i) {
        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = NULL;
        framebuffer_create_info.renderPass = g_vulkan.render_pass;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = (VkImageView[]){g_vulkan.swapchain_image_views[i], g_vulkan.depth_image_view };
        framebuffer_create_info.width = g_screen_width;
        framebuffer_create_info.height = g_screen_height;
        framebuffer_create_info.layers = 1;
        result = vkCreateFramebuffer(device, &framebuffer_create_info, NULL, framebuffers + i);
        vulkan_assert(result);
    }
    
    g_vulkan.framebuffers = framebuffers;
    g_vulkan.framebuffers_count =  swapchain_image_count;
}

typedef enum Vulkan_buffer_type { vertex_buffer_type, index_buffer_type } Vulkan_buffer_type;

Vulkan_buffer create_cpu_buffer(size_t size, void *data, VkBufferUsageFlags usage) {
    Vulkan_buffer v_buffer = {0};
#if 0
    vulkan_create_buffer(&buffer, size, data, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
#else
    VkBuffer buffer = alloc_vulkan_buffer(size, data, usage_staging_buffer);
    v_buffer.buffer = buffer;
    v_buffer.size = size;
#endif
    return v_buffer;
}

Vulkan_buffer create_gpu_buffer(VkDeviceSize size, VkBufferUsageFlags usage) {
    Vulkan_buffer v_buffer = {0};
#if 0
    vulkan_create_buffer(&buffer, size, NULL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
#else
    VkBuffer buffer = alloc_vulkan_buffer(size, null, usage_vertex_buffer);
    v_buffer.buffer = buffer;
    v_buffer.size = size;
#endif
    return v_buffer;
}

void copy_cpu_buffers_to_gpu_buffers(Vulkan_buffer **cpu_buffers, Vulkan_buffer **gpu_buffers, int num) {
    VkCommandBuffer copy_command_buffer;
    begin_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, &copy_command_buffer);
    VkBufferCopy copy_region = {0};
    for (int i = 0; i < num; ++i) {
        copy_region.size = cpu_buffers[i]->size;
        vkCmdCopyBuffer(copy_command_buffer, cpu_buffers[i]->buffer, gpu_buffers[i]->buffer, 1, &copy_region);
    }
    end_command_buffer(copy_command_buffer);
}

Vulkan_mesh vulkan_create_mesh(Mesh mesh) {
    
    Vertex *vertex_data = mesh.vertices;
    int *index_data = mesh.indices;
    int vertex_count = mesh.num_vertices;
    int index_count = mesh.num_indices;
    
    size_t vertex_data_size = sizeof(Vertex) * vertex_count;
    size_t index_data_size = sizeof(int) * index_count;
    
    Vulkan_buffer vertex_staging_buffer = create_cpu_buffer(vertex_data_size, vertex_data, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    Vulkan_buffer index_staging_buffer = create_cpu_buffer(index_data_size, index_data, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    Vulkan_buffer vertex_buffer = create_gpu_buffer(vertex_data_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    Vulkan_buffer index_buffer = create_gpu_buffer(index_data_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    Vulkan_buffer *cpu_buffers[] = { &vertex_staging_buffer, &index_staging_buffer };
    Vulkan_buffer *gpu_buffers[] = { &vertex_buffer, &index_buffer };
    copy_cpu_buffers_to_gpu_buffers(cpu_buffers, gpu_buffers, 2);
    
    vkDestroyBuffer(g_vulkan.device, vertex_staging_buffer.buffer, NULL);
    vkFreeMemory(g_vulkan.device, vertex_staging_buffer.device_memory, NULL);
    
    vkDestroyBuffer(g_vulkan.device, index_staging_buffer.buffer, NULL);
    vkFreeMemory(g_vulkan.device, index_staging_buffer.device_memory, NULL);
    
    Vulkan_mesh vulkan_mesh;
    vulkan_mesh.vertex_buffer = vertex_buffer;
    vulkan_mesh.index_buffer = index_buffer;
    vulkan_mesh.vertex_count = vertex_count;
    vulkan_mesh.index_count = index_count;
    
    return vulkan_mesh;
}

void vulkan_create_sampler() {
    VkResult result;
    //init texture sampler
#if 0
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSampler sampler;
    result = vkCreateSampler(g_vulkan.device, &sampler_info, NULL, &sampler);
    vulkan_assert(result);
#else
    u32 mipLevels = 1;
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.compareOp = VK_COMPARE_OP_NEVER;
    sampler_info.minLod = 0.0f;
    // Max level-of-detail should match mip level count
    sampler_info.maxLod =  (float)mipLevels;
    // Only enable anisotropic filtering if enabled on the devicec
    sampler_info.maxAnisotropy =  g_vulkan.physical_device_properties.limits.maxSamplerAnisotropy ;
    sampler_info.anisotropyEnable = true;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VkSampler sampler;
    result = vkCreateSampler(g_vulkan.device, &sampler_info, NULL, &sampler);
    vulkan_assert(result);
#endif
    
    g_vulkan.sampler = sampler;
}

void vulkan_create_dynamic_uniform_buffer(int num_objects) {
    size_t min_alignment = g_vulkan.physical_device_properties.limits.minUniformBufferOffsetAlignment;
    size_t dynamic_alignment = sizeof(m4) * 3;
    if (min_alignment > 0) {
        dynamic_alignment = (dynamic_alignment + min_alignment - 1) & ~(min_alignment - 1);
    }
    size_t buffer_size = num_objects * dynamic_alignment;
    Vulkan_buffer uniform_buffer = vulkan_create_buffer(buffer_size, null, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    map_buffer(&uniform_buffer);
    
    g_vulkan.uniform_buffer_dynamic_alignment = dynamic_alignment;
    g_vulkan.uniform_buffer = uniform_buffer;
}

typedef struct Push_constant_data {
    int texture_index;
} Push_constant_data;

typedef struct Vulkan_renderer_object {
    VkPipeline pipeline;
    u32 num_vertices;
    Push_constant_data push_constant_data;
} Vulkan_renderer_object;

void vulkan_create_command_buffer(Vulkan_buffer buffer, Vulkan_renderer_object *objects, u32 num_objects, VkPipeline default_pipeline) {
    VkResult result;
    
    VkCommandBufferBeginInfo command_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    
    for (u32 command_buffer_index = 0; command_buffer_index < g_vulkan.draw_command_buffers_num; ++command_buffer_index) {
        VkCommandBuffer command_buffer = g_vulkan.draw_command_buffers[command_buffer_index];
        
        VkClearValue clear_values[2];
        clear_values[0].color.float32[0] = 0.2f;
        clear_values[0].color.float32[1] = 0.2f;
        clear_values[0].color.float32[2] = 0.2f;
        clear_values[0].color.float32[3] = 0.2f;
        clear_values[1].depthStencil.depth = 1.0f;
        clear_values[1].depthStencil.stencil = 0;
        
        VkRenderPassBeginInfo render_pass_begin = {};
        render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin.pNext = NULL;
        render_pass_begin.renderPass = g_vulkan.render_pass;
        render_pass_begin.renderArea.offset.x = 0;
        render_pass_begin.renderArea.offset.y = 0;
        render_pass_begin.renderArea.extent.width = g_screen_width;
        render_pass_begin.renderArea.extent.height = g_screen_height;
        render_pass_begin.clearValueCount = 2;
        render_pass_begin.pClearValues = clear_values;
        render_pass_begin.framebuffer = g_vulkan.framebuffers[command_buffer_index];
        
        result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        vulkan_assert(result);
        
        vkCmdBeginRenderPass(command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
        
        VkViewport viewport;
        viewport.height = (float)g_screen_height; //TODO: flip the y axis to accomidate vulkan's coordinate system
        viewport.width = (float)g_screen_width;
        viewport.minDepth = (float)0.0f;
        viewport.maxDepth = (float)1.0f;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        
        VkRect2D scissor;
        scissor.extent.width = g_screen_width;
        scissor.extent.height = g_screen_height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
        
        VkCommandBuffer draw_command_buffer = command_buffer;
        
        VkDeviceSize offsets[1] = {};
        
        VkPipeline current_pipeline = default_pipeline; //TODO: change
        vkCmdBindPipeline(draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, current_pipeline);
        
        vkCmdBindVertexBuffers(draw_command_buffer, 0, 1, &buffer.buffer, offsets);
        
        u32 total_num_vertices = 0;
        for (int object_index = 0; object_index < num_objects; ++object_index) {
            Vulkan_renderer_object object = objects[object_index];
            if (object.pipeline != current_pipeline) {
                vkCmdBindPipeline(draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.pipeline);
            }
            
            vkCmdPushConstants(draw_command_buffer, g_vulkan.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(Push_constant_data), &object.push_constant_data);
            
            u32 dynamic_offset = object_index * g_vulkan.uniform_buffer_dynamic_alignment;
            assert(dynamic_offset <= g_vulkan.uniform_buffer.size);
            //assert(((char *)(g_vulkan.uniform_buffer.mapped_data))[dynamic_offset] != 0);
            vkCmdBindDescriptorSets(draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vulkan.pipeline_layout, 0, 1, &g_vulkan.descriptor_set, 1, &dynamic_offset);
            
            u32 first_vertex = total_num_vertices;
            u32 num_vertices = object.num_vertices;
            vkCmdDraw(draw_command_buffer, num_vertices, 1, first_vertex, 0);
            total_num_vertices += object.num_vertices;
        }
        
        vkCmdEndRenderPass(command_buffer);
        
        result = vkEndCommandBuffer(command_buffer);
        vulkan_assert(result);
    }
}

void vulkan_allocate_command_buffers() {
    VkResult result;
    //TODO: check the commanbuffercount again
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = NULL;
    command_buffer_allocate_info.commandPool = g_vulkan.command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = g_vulkan.swapchain_image_count;
    
    g_vulkan.draw_command_buffers = (VkCommandBuffer *)malloc(sizeof(VkCommandBuffer) * g_vulkan.swapchain_image_count);
    result = vkAllocateCommandBuffers(g_vulkan.device, &command_buffer_allocate_info, g_vulkan.draw_command_buffers);
    vulkan_assert(result);
    
    g_vulkan.draw_command_buffers_num = g_vulkan.swapchain_image_count;
}

void destroy_command_buffers() {
    vkFreeCommandBuffers(g_vulkan.device, g_vulkan.command_pool, g_vulkan.draw_command_buffers_num, g_vulkan.draw_command_buffers);
}

void view_changed(Vulkan_buffer *uniform_buffer, Transform_data *transform_data) {
    //vulkan_update_uniform_buffer(uniform_buffer, transform_data);
}

void window_resize() {
    if (!g_vulkan.prepared) {
        return;
    }
    g_vulkan.prepared = false;
    
    vkDeviceWaitIdle(g_vulkan.device);
    
    u32 width = g_screen_width, height = g_screen_height;
    vulkan_create_swapchain(&width, &height, true); //== setup swapchain
    
    VkDevice device = g_vulkan.device;
    vkDestroyImageView(device, g_vulkan.depth_image_view, NULL);
    vkDestroyImage(device, g_vulkan.depth_image, NULL);
    vkFreeMemory(device, g_vulkan.depth_image_memory, NULL);
    create_depth_stencil();
    for (u32 i = 0; i < g_vulkan.framebuffers_count; ++i) {
        vkDestroyFramebuffer(device, g_vulkan.framebuffers[i], NULL);
    }
    setup_framebuffer();
    
    destroy_command_buffers();
    
    vulkan_allocate_command_buffers();
#if 0 
    draw_vulkan(&g_vulkan.descriptor_set, &g_vulkan.mesh); 
#endif
    
    vkDeviceWaitIdle(device);
    
    //TODO: update camera aspect ration
    
    //derived class window resized
    
    //view_changed(); //TODO: uncomment this!!!!
}

void vulkan_update_descriptor_set(Vulkan_texture *textures, int num_textures) {
    assert(num_textures > 0);
    const int num_samplers = 1;
    const int num_images = num_textures;
    const int num_dynamic_uniform_buffers = 1;
    const int total_num_descriptors = num_samplers + num_images + num_dynamic_uniform_buffers;
    assert(num_textures <= num_images);
    VkDescriptorImageInfo *textures_image_infos = malloc(num_textures * sizeof(VkDescriptorImageInfo));
    for (int i = 0; i < num_textures; ++i) {
        VkDescriptorImageInfo image_info = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = textures[i].image_view
        };
        textures_image_infos[i] = image_info;
    }
    
    VkDescriptorImageInfo sampler_image_info = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .sampler = g_vulkan.sampler
    };
    
    VkDescriptorBufferInfo uniform_buffer_info = {
        .buffer = g_vulkan.uniform_buffer.buffer,
        .range = g_vulkan.uniform_buffer.size
    };
    
    VkWriteDescriptorSet write_descriptor_set[] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = g_vulkan.descriptor_set,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .pBufferInfo = &uniform_buffer_info,
            .dstArrayElement = 0,
            .dstBinding = 0,
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = g_vulkan.descriptor_set,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo = &sampler_image_info,
            .dstArrayElement = 0,
            .dstBinding = 1,
        },
        {
            
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = g_vulkan.descriptor_set,
            .descriptorCount = num_images,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = textures_image_infos,
            .dstArrayElement = 0,
            .dstBinding = 2,
        }
    };
    
    vkUpdateDescriptorSets(g_vulkan.device, array_count(write_descriptor_set), write_descriptor_set, 0, null);
    
    free(textures_image_infos);
}

VkShaderModule vulkan_create_shader_module(const char *shader_code, size_t shader_length) {
    VkResult result;
    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = shader_length;
    shader_module_create_info.pCode = (const uint32_t *) shader_code;
    VkShaderModule shader_module;
    result = vkCreateShaderModule(g_vulkan.device, &shader_module_create_info, NULL, &shader_module);
    vulkan_assert(result);
    
    return shader_module;
}

typedef enum Pipeline_flags {
    pipeline_option_enable_backface_culling = 1 << 1,
    pipeline_option_enable_depth = 1 << 2,
    pipeline_option_enable_blending = 1 << 3
} Pipeline_flags;

VkPipeline vulkan_create_pipeline(VkShaderModule vertex_shader, VkShaderModule fragment_shader, Pipeline_flags options) {
    VkResult result;
    
    VkVertexInputBindingDescription *vertex_input_binding_description = null;
    set_sbuff_length(vertex_input_binding_description, 1);
    vertex_input_binding_description->binding = 0;
    vertex_input_binding_description->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertex_input_binding_description->stride = sizeof(Vertex);
    
    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[] = {
        {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, pos)
        },
        {
            .binding = 0,
            .location = 1,
            .format = VK_FORMAT_R32G32_SFLOAT, 
            .offset = offsetof(Vertex, textcoords)
        },
        {
            .binding = 0,
            .location = 2,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(Vertex, color)
        }
    };
    
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1, 
        .pVertexBindingDescriptions = vertex_input_binding_description,
        .vertexAttributeDescriptionCount = array_count(vertex_input_attribute_descriptions),
        .pVertexAttributeDescriptions = vertex_input_attribute_descriptions
    };
    
    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = {};
    pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_input_assembly_state_create_info.pNext = NULL;
    pipeline_input_assembly_state_create_info.flags = 0;
    pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
    pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.pNext = NULL;
    rasterization_state_create_info.flags = 0;
    
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    
#if 0
    if (options & pipeline_option_enable_backface_culling) {
        rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT; //TODO: enable backface culling
    }
    else {
        rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
    }
#else
    rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
#endif
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasConstantFactor = 0;
    rasterization_state_create_info.depthBiasClamp = 0;
    rasterization_state_create_info.depthBiasSlopeFactor = 0;
    rasterization_state_create_info.lineWidth = 1.0f;
    
    VkPipelineColorBlendAttachmentState color_blend_attachment  = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = {};
    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
    pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    pipeline_color_blend_state_create_info.attachmentCount = 1;
    pipeline_color_blend_state_create_info.pAttachments = &color_blend_attachment;
    
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {};
    if (options & pipeline_option_enable_blending) {
        pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipeline_depth_stencil_state_create_info.depthTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
        pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    }
    else {
        //TODO: add depth option
        pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipeline_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
        pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
        pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
        pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    }
    
    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {};
    pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_viewport_state_create_info.pNext = NULL;
    pipeline_viewport_state_create_info.flags = 0;
    pipeline_viewport_state_create_info.viewportCount = 1;
    pipeline_viewport_state_create_info.scissorCount = 1;
    pipeline_viewport_state_create_info.pScissors = NULL;
    pipeline_viewport_state_create_info.pViewports = NULL;
    
    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {};
    pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    
    VkDynamicState dynamic_state_enables[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pNext = NULL;
    dynamic_state.pDynamicStates = dynamic_state_enables;
    dynamic_state.dynamicStateCount = array_count(dynamic_state_enables);
    
    
    VkPipelineShaderStageCreateInfo shader_stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader,
            .pName = "main"
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader,
            .pName = "main"
        }
    };
    
    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext = NULL;
    pipeline_create_info.layout = g_vulkan.pipeline_layout;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = 0;
    pipeline_create_info.flags = 0;
    pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
    pipeline_create_info.pTessellationState = NULL;
    pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state;
    pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
    pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.stageCount = array_count(shader_stages);
    pipeline_create_info.renderPass = g_vulkan.render_pass;
    pipeline_create_info.subpass = 0;
    
    VkPipeline pipeline;
    result = vkCreateGraphicsPipelines(g_vulkan.device, g_vulkan.pipeline_cache, 1, &pipeline_create_info, NULL, &pipeline);
    vulkan_assert(result);
    
    vector_free(vertex_input_binding_description);
    
    return pipeline;
}

VkResult queue_present() {
    VkPresentInfoKHR present_info;
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = NULL;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &g_vulkan.swapchain;
    present_info.pImageIndices = &g_vulkan.current_framebuffer;
    present_info.pWaitSemaphores = &g_vulkan.render_complete_semaphore;
    present_info.waitSemaphoreCount = 1;
    present_info.pResults = NULL;
    VkResult result = vkQueuePresentKHR(g_vulkan.queue, &present_info);
    
    return result;
}

void submit_frame() {
    VkResult result = queue_present();
    if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            window_resize();
            return;
        }
        else {
            vulkan_assert(result);
        }
    }
    result = vkQueueWaitIdle(g_vulkan.queue);
    vulkan_assert(result);
}

void prepare_frame() {
    //base
    //NOTE: this is the function acquirenextimage in saschas swapchain class
    VkResult result = vkAcquireNextImageKHR(g_vulkan.device, g_vulkan.swapchain, UINT64_MAX, g_vulkan.present_complete_semaphore, NULL, &g_vulkan.current_framebuffer);
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        window_resize();
    }
    else {
        vulkan_assert(result);
    }
}

void init_vulkan(Platform *platform) {
    VkResult result;
    
    bool validation = true;
    
    //get instance layers
    uint32_t instance_layer_count;
    vkEnumerateInstanceLayerProperties(&instance_layer_count ,NULL);
    VkLayerProperties *instance_layer_properties = (VkLayerProperties *)allocate_frame(sizeof(VkLayerProperties) * instance_layer_count);
    result = vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layer_properties);
    vulkan_assert(result);
    
    //add debug layer
    char **instance_layer_names = (char **)allocate_frame(sizeof(char *) * 1);
    instance_layer_names[0] = "VK_LAYER_KHRONOS_validation";
    //TODO: check for validation layer
    
    //get instance extensions
    //VkExtensionProperties *instance_extensions = NULL;
    for (int i = 0; i < instance_layer_count; ++i) {
        uint32_t instance_extensions_count = 0;
        vkEnumerateInstanceExtensionProperties(instance_layer_properties[i].layerName, &instance_extensions_count, NULL);
        if (instance_extensions_count) {
            //VkExtensionProperties *added_extensions = (VkExtensionProperties *)arraddn(instance_extensions, instance_extensions_count);
            VkExtensionProperties *added_extensions = (VkExtensionProperties *)allocate_perm(sizeof(VkExtensionProperties) * instance_extensions_count);
            
            result = vkEnumerateInstanceExtensionProperties(instance_layer_properties[i].layerName, &instance_extensions_count, added_extensions);
            vulkan_assert(result);
        }
    }
    
    //add surface extension
#if 0
    //TODO: this is messy as fuck, look for nicer way to load into a dynamic array
    uint32_t instance_extensions_count;
    glfwGetRequiredInstanceExtensions(&instance_extensions_count);
    const char **_instance_extensions = NULL;
    _instance_extensions = glfwGetRequiredInstanceExtensions(&instance_extensions_count);
    const char **instance_extensions = NULL;
    for (int i = 0; i < instance_extensions_count; ++i) {
        arrput(instance_extensions, _instance_extensions[i]);
    }
#else
    const char **instance_extensions = null;
    arrput(instance_extensions, VK_KHR_SURFACE_EXTENSION_NAME);
    arrput(instance_extensions, VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
    arrput(instance_extensions, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif
    //add debug extension
    //TODO: check if the extension even exists by comparing it to instance layer properties
    if (validation) {
        //TODO: ok so i got no idea where debug_report name came from...
        arrput(instance_extensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        arrput(instance_extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    //create instance
    char *app_name = "app";
    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = NULL;
    application_info.pApplicationName = app_name;
    application_info.applicationVersion = 1;
    application_info.pEngineName = app_name;
    application_info.engineVersion = 1;
    application_info.apiVersion = VK_API_VERSION_1_2; 
    
    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = NULL;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &application_info;
    instance_info.enabledLayerCount = 1;
    if (validation) {
        const char *arr[1] = {"VK_LAYER_KHRONOS_validation"};
        instance_info.ppEnabledLayerNames = arr;
    }
    instance_info.enabledExtensionCount = arrlen(instance_extensions);
    instance_info.ppEnabledExtensionNames = (const char * const *)instance_extensions;
    
    VkInstance instance;
    result = vkCreateInstance(&instance_info, NULL, &instance);
    vulkan_assert(result);
    
    //init debug callback
    if (validation) {
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        assert(vkCreateDebugUtilsMessengerEXT);
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        assert(vkDestroyDebugUtilsMessengerEXT);
        VkDebugUtilsMessengerEXT debugUtilsMessenger;
        
        VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI = {};
        debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debugUtilsMessengerCI.pfnUserCallback = debugUtilsMessengerCallback;
        result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCI, NULL, &debugUtilsMessenger);
        //vulkan_assert(result);
    }
    
    //get physical devices
    uint32_t physical_devices_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_devices_count, NULL);
    assert(physical_devices_count);
    VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * physical_devices_count);
    result = vkEnumeratePhysicalDevices(instance, &physical_devices_count, physical_devices);
    vulkan_assert(result);
    VkPhysicalDevice physical_device = physical_devices[0];
    
    //get queue families
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
    assert(queue_family_count);
    VkQueueFamilyProperties *queue_family_properties = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties);
    vulkan_assert(result);
    
    //get device extensions
#if 0
    uint32_t device_extensions_count = 0;
    char *device_layer_name = NULL;
    vkEnumerateDeviceExtensionProperties(physical_device, device_layer_name, &device_extensions_count, NULL);
    VkExtensionProperties *device_extensions = (vkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * device_extensions_count);
    vkEnumerateDeviceExtensionProperties(physical_device, layer_name, &device_extensions_count, device_extensions);
    vulkan_assert(result);
#endif
    char **device_extension_names = null;
    vector_add(device_extension_names, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    vector_add(device_extension_names, VK_KHR_MAINTENANCE3_EXTENSION_NAME);
    vector_add(device_extension_names, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    
    //get memory properties
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);
    
    //get physical device properties
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
    
    
    g_vulkan.physical_device_properties = physical_device_properties;
    
    //init device extensions 
    //soon...
    
    //get depth format
    VkFormat depth_formats[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };
    VkFormat depth_format;
    for (int i = 0; i < array_count(depth_formats); ++i) {
        VkFormatProperties format_properties;
        vkGetPhysicalDeviceFormatProperties(physical_device, depth_formats[i], &format_properties);
        if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depth_format = depth_formats[i];
        }
    }
    
    //create window
    //TODO: larger resolution results in bugs
#if 0
    int window_width = 500;
    int window_height = 500;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(window_width, window_height, app_name, 0, 0);
#endif
    
    //create surface
    //NOTE: in saschas this is part of swapchain class init surface
#if 0
    VkSurfaceKHR surface;
    result = glfwCreateWindowSurface(instance, window, NULL, &surface);
    vulkan_assert(result);
#else
    VkSurfaceKHR surface;
    VkXlibSurfaceCreateInfoKHR surface_create_info = {0};
    surface_create_info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    surface_create_info.dpy = platform->display;
    surface_create_info.window = platform->window;
    result = vkCreateXlibSurfaceKHR(instance, &surface_create_info, null, &surface);
    vulkan_assert(result);
#endif
    
    //init glfw
#if 0
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    
    glfwSetWindowUserPointer(window, );
#endif
    
    //check for presentation queue
    //NOTE: validation layer forces me yo use the second version
#if 0
    uint32_t queue_family_index = UINT32_MAX;
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        if (glfwGetPhysicalDevicePresentationSupport(instance, physical_device, i) == GLFW_TRUE) {
            queue_family_index = i;
            break;
        }
    }
    assert(queue_family_index != UINT32_MAX);
#endif
    u32 queue_family_index = UINT32_MAX;
    VkBool32 presentation_support;
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentation_support);
        if (presentation_support) {
            queue_family_index = i;
            break;
        }
    }
    assert(queue_family_index != UINT32_MAX);
    
    //check for color format and space
    uint32_t format_count;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, NULL);
    vulkan_assert(result);
    VkSurfaceFormatKHR *surface_formats = (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surface_formats);
    vulkan_assert(result);
    VkFormat color_format;
    VkColorSpaceKHR color_space;
    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        color_format = VK_FORMAT_B8G8R8A8_UNORM;
        color_space = surface_formats[0].colorSpace;
    }
    else {
        bool found_desired_format = false;
        for (int i = 0; i < format_count; ++i) {
            if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
                color_format = surface_formats[i].format;
                color_space = surface_formats[i].colorSpace;
                found_desired_format = true;
                break;
            }
        }
        if (!found_desired_format) {
            color_format = surface_formats[0].format;
            color_space = surface_formats[0].colorSpace;
        }
    }
    
    //init device (logical)
    VkDeviceQueueCreateInfo queue_info = {};
    float queue_priorities[1] = {0.0};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = NULL;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = queue_priorities;
    queue_info.queueFamilyIndex = queue_family_index;
    
    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_set_indexing_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE
    };
    
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(physical_device, &device_features);
    //NOTE: sascha uses a derived class function to get features
    //also the next line is from tutorial 
    device_features.samplerAnisotropy = VK_TRUE;
    device_features.fillModeNonSolid = VK_TRUE;
    
    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = &descriptor_set_indexing_features;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = array_count(device_extension_names);
    device_info.ppEnabledExtensionNames = (const char * const *)device_extension_names;
    device_info.pEnabledFeatures = &device_features;
    
    VkDevice device;
    result = vkCreateDevice(physical_device, &device_info, NULL, &device);
    vulkan_assert(result);
    
    //init allocator
    vulkan_init_memory_allocator(device, physical_device, instance);
    
    //init queue
    VkQueue queue;
    vkGetDeviceQueue(device, queue_family_index, 0, &queue);
    
    //get pipeline stages flags
    //VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    
    /*
    VkSemaphore imageAcquiredSemaphore;
    VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo;
    imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    imageAcquiredSemaphoreCreateInfo.pNext = NULL;
    imageAcquiredSemaphoreCreateInfo.flags = 0;
    */
    
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(device, &semaphore_create_info, NULL, &g_vulkan.present_complete_semaphore);
    vkCreateSemaphore(device, &semaphore_create_info, NULL, &g_vulkan.render_complete_semaphore);
    
    g_vulkan.submit_pipeline_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //VkPipelineStageFlags submit_pipeline_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pWaitDstStageMask = &g_vulkan.submit_pipeline_stages;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &g_vulkan.present_complete_semaphore;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &g_vulkan.render_complete_semaphore;
    
    g_vulkan.instance = instance;
    g_vulkan.physical_device = physical_device;
    g_vulkan.device = device;
    g_vulkan.physical_device_properties = physical_device_properties;
    g_vulkan.queue = queue;
    g_vulkan.queue_family_index = queue_family_index;
    g_vulkan.color_format = color_format;
    g_vulkan.color_space = color_space;
    g_vulkan.depth_format = depth_format;
    //g_vulkan.present_complete_semaphore = present_complete_semaphore;
    //g_vulkan.render_complete_semaphore = render_complete_semaphore;
    g_vulkan.surface = surface;
    g_vulkan.current_framebuffer = 0;
    g_vulkan.physical_device_memory_properties = physical_device_memory_properties;
    g_vulkan.submit_info = submit_info;
    g_vulkan.view_updated = false;
    g_vulkan.init = true;
}

void create_command_pool() {
    VkResult result;
    VkCommandPoolCreateInfo command_pool_info = {};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext = NULL;
    command_pool_info.queueFamilyIndex = g_vulkan.queue_family_index;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    VkCommandPool command_pool;
    result = vkCreateCommandPool(g_vulkan.device, &command_pool_info, NULL, &command_pool);
    vulkan_assert(result);
    g_vulkan.command_pool = command_pool;
}

void create_pipeline_cache() { 
    VkResult result;
    VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
    pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkPipelineCache pipeline_cache;
    result = vkCreatePipelineCache(g_vulkan.device, &pipeline_cache_create_info, NULL, &pipeline_cache);
    vulkan_assert(result);
    g_vulkan.pipeline_cache = pipeline_cache;
}

void vulkan_create_render_pass() {
    VkResult result;
    
    VkAttachmentDescription attachments[2] = {};
    attachments[0].format = g_vulkan.color_format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;;
    
    attachments[1].format = g_vulkan.depth_format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference color_attachment_reference = {};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depth_attachment_reference = {};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass_description = {};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.flags = 0;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = NULL;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_reference;
    subpass_description.pResolveAttachments = NULL;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = NULL;
    
    //NOTE: why are there two dependencies? requires further investigation
#if 0
    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = 0;
#else
    VkSubpassDependency dependencies[2];
    
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
#endif
    
    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = NULL;
    render_pass_create_info.attachmentCount = array_count(attachments);
    render_pass_create_info.pAttachments = attachments;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = array_count(dependencies);
    render_pass_create_info.pDependencies = dependencies;
    VkRenderPass render_pass;
    result = vkCreateRenderPass(g_vulkan.device, &render_pass_create_info, NULL, &render_pass);
    vulkan_assert(result);
    g_vulkan.render_pass = render_pass;
}

void vulkan_create_descriptor_set() {
    VkResult result;
    
    const int num_samplers = 1;
    const int num_images = 200;
    const int num_dynamic_uniform_buffers = 1;
    const int total_num_descriptors = num_samplers + num_images + num_dynamic_uniform_buffers;
    
    //create descriptor pool
    VkDescriptorPoolSize descriptor_pool_sizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = num_samplers,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = num_images,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = num_dynamic_uniform_buffers,
        }
    };
    
    VkDescriptorPoolCreateInfo decriptor_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = null,
        .maxSets = total_num_descriptors, 
        .poolSizeCount = array_count(descriptor_pool_sizes),
        .pPoolSizes = descriptor_pool_sizes,
    };
    
    VkDescriptorPool descriptor_pool;
    result = vkCreateDescriptorPool(g_vulkan.device, &decriptor_pool_create_info, null, &descriptor_pool);
    vulkan_assert(result);
    
    //create descriptor layout
    VkDescriptorSetLayoutBinding bindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = num_dynamic_uniform_buffers,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = num_samplers,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = num_images,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        }
    };
    
    VkDescriptorBindingFlags flags[3] = {0};
    flags[2] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    
    VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 3,
        .pBindingFlags = flags
    };
    
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &binding_flags,
        .bindingCount = array_count(bindings),
        .pBindings = bindings,
    };
    
    result = vkCreateDescriptorSetLayout(g_vulkan.device, &descriptor_set_layout_create_info, null, &g_vulkan.descriptor_set_layout);
    vulkan_assert(result);
    
    //create push constants
    VkPushConstantRange push_constant_range = {
        .stageFlags = VK_SHADER_STAGE_ALL,
        .size = sizeof(Push_constant_data)
    };
    
    //create pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range,
        .setLayoutCount = 1,
        .pSetLayouts = &g_vulkan.descriptor_set_layout,
    };
    VkPipelineLayout pipeline_layout;
    result = vkCreatePipelineLayout(g_vulkan.device, &pipeline_layout_create_info, null, &pipeline_layout);
    vulkan_assert(result);
    
    //allocate descriptor set
    u32 counts[1];
    counts[0] = 200;
    
    VkDescriptorSetVariableDescriptorCountAllocateInfo set_counts = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
        .descriptorSetCount = 1,
        .pDescriptorCounts = counts
    };
    
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
        .pNext = &set_counts,
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &g_vulkan.descriptor_set_layout,
    };
    
    VkDescriptorSet descriptor_set;
    result = vkAllocateDescriptorSets(g_vulkan.device, &descriptor_set_allocate_info, &descriptor_set);
    vulkan_assert(result);
    
    g_vulkan.descriptor_set = descriptor_set;
    g_vulkan.pipeline_layout = pipeline_layout;
}

void create_synchronization_primitives() {
    VkFenceCreateInfo fence_create_info;
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence *fences = malloc(sizeof(VkFence) * g_vulkan.draw_command_buffers_num);
    for (int i = 0; i < g_vulkan.draw_command_buffers_num; ++i) {
        VkResult result = vkCreateFence(g_vulkan.device, &fence_create_info, NULL, fences + i);
        vulkan_assert(result);
    }
    g_vulkan.wait_fences = fences;
}

void vulkan_next_frame() {
    prepare_frame();
    g_vulkan.submit_info.commandBufferCount = 1;
    g_vulkan.submit_info.pCommandBuffers = &g_vulkan.draw_command_buffers[g_vulkan.current_framebuffer];
    VkResult result = vkQueueSubmit(g_vulkan.queue, 1, &g_vulkan.submit_info, VK_NULL_HANDLE);
    vulkan_assert(result);
    submit_frame();
}

void prepare_vulkan() {
    //saschas base class
    //sashca uses init swapchain but its redundant here because all that code is in init vulkan function
    g_vulkan.swapchain = VK_NULL_HANDLE;
    u32 width = g_screen_width, height = g_screen_height;
    vulkan_create_swapchain(&width, &height, true); //== setup swapchain
    create_command_pool();
    vulkan_allocate_command_buffers();
    create_synchronization_primitives(); //TODO: get the the bottom of who's using the primitives
    create_depth_stencil();
    vulkan_create_render_pass();
    create_pipeline_cache();
    setup_framebuffer();
    
    vulkan_create_descriptor_set();
    vulkan_create_sampler();
    
}

void vulkan_update_textures(Vulkan_texture *textures, int num_textures) {
    vulkan_update_descriptor_set(textures, num_textures);
}

#endif
