#ifndef _vkLogicalDevice_h_
#define _vkLogicalDevice_h_

#include <vulkan/vulkan.hpp>
#include <optional>


struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;

  bool isComplete() { return graphicsFamily.has_value(); }
};

class vkLogicalDevice
{
public:
  vkLogicalDevice(VkPhysicalDevice);
  ~vkLogicalDevice();
  vkLogicalDevice() = delete;

private:
  VkPhysicalDevice   m_physDevice;
  VkDevice           m_logicalDevice;

  QueueFamilyIndices findQueueFamilies();
};





#endif
