#include <iostream>

#include "vkLogicalDevice.h"
#include "vkException.h"
#include <vulkan/vulkan.hpp>

vkLogicalDevice::vkLogicalDevice(VkPhysicalDevice d) : m_physDevice(d), m_logicalDevice(VK_NULL_HANDLE)
{
  VkResult res = VK_SUCCESS;
  float queuePriority = 1.0f;
  

  // specify queues device must have
  QueueFamilyIndices   indices = findQueueFamilies();

  VkDeviceQueueCreateInfo  queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  // specify features device must have
  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo   createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = 0;

 res = vkCreateDevice(d, &createInfo, nullptr, &m_logicalDevice);
 if (res == VK_SUCCESS)
 {
   std::cout << "[+] logical device created successfully" << std::endl;
 }
 else
 {
   vkException exc(res, "vkLogicalDevice::vkLogicalDevice", "");
   throw exc;
 }

}

vkLogicalDevice::~vkLogicalDevice()
{
  //vkDestroyDevice(m_logicalDevice, nullptr);  
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// private functions....
QueueFamilyIndices vkLogicalDevice::findQueueFamilies()
{
  QueueFamilyIndices    indices;
  uint32_t cntQueueFamilies = 0;
  std::vector<VkQueueFamilyProperties>  vecQueueFamilies;

  vkGetPhysicalDeviceQueueFamilyProperties(m_physDevice, &cntQueueFamilies, nullptr);
  vecQueueFamilies.reserve(cntQueueFamilies);
  vkGetPhysicalDeviceQueueFamilyProperties(m_physDevice, &cntQueueFamilies, vecQueueFamilies.data());

  int i = 0;
  for (const auto& queueFamily : vecQueueFamilies)
  {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphicsFamily = i;
    }

    if (indices.isComplete()) break;
    i++;
  }

  // logic to find queue we are looking for
  return indices;
}