#ifdef __cplusplus
extern "C" {
#endif
    
    typedef enum Vulkan_allocation_usage {
        usage_staging_buffer,
        usage_vertex_buffer,
        usage_index_buffer,
    } Vulkan_allocation_usage;
    
    void vulkan_init_memory_allocator(VkDevice device, VkPhysicalDevice physical_device, VkInstance instance);
    VkBuffer alloc_vulkan_buffer(size_t size, void *data, Vulkan_allocation_usage allocation_usage);
    VkImage alloc_vulkan_image(VkExtent3D extent, int mip_levels, VkFormat format, Vulkan_allocation_usage allocation_usage);
    
#ifdef __cplusplus
}
#endif