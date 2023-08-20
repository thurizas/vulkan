#ifndef _vkLogicalDevice_h_
#define _vkLogicalDevice_h_

#include <vulkan/vulkan.hpp>
#include <optional>

struct vkCtx;

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;

  bool isComplete() { return graphicsFamily.has_value(); }
};

class vkLogicalDevice
{
public:
  vkLogicalDevice(vkCtx*, VkPhysicalDevice, bool debug = false);
  ~vkLogicalDevice();
  vkLogicalDevice() = delete;

private:
  vkCtx*             m_pCtx;
  bool               m_debug;
  VkPhysicalDevice   m_physDevice;
  VkDevice           m_logicalDevice;
  VkQueue            m_graphicsQueue;

  QueueFamilyIndices findQueueFamilies();
};





#endif
