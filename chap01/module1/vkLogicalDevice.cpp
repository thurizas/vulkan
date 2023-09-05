#include <iostream>

#include "vkCtx.h"
#include "vkLogicalDevice.h"
#include "vkException.h"
#include <vulkan/vulkan.hpp>

/************************************************************************************************************************
 * function :
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   : Aug 2023 (GKHuber)
************************************************************************************************************************/
vkLogicalDevice::vkLogicalDevice(vkCtx* pCtx, VkPhysicalDevice d, bool dbg) : m_pCtx(pCtx), m_debug(dbg), m_physDevice(d), m_logicalDevice(VK_NULL_HANDLE)
{
  VkResult res = VK_SUCCESS;
  float queuePriority = 1.0f;
  
  std::vector<const char*> layers = m_pCtx->getValidationLayers();

  // specify queues device must have
  // QueueFamilyIndices   indices = findQueueFamilies();
  // TODO : findQueueFamilies fails to populate vector....thus we fail....

  VkDeviceQueueCreateInfo  queueCreateInfo{};
  memset((void*)&queueCreateInfo, 0, sizeof(queueCreateInfo));
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = 0; // TODO : need to determine this programmatically. indices.graphicsFamily.value();
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  // specify features device must have
  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo   createInfo{};
  memset((void*)&createInfo, 0, sizeof(createInfo));
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = 0;

  if (m_debug)
  {
    createInfo.enabledLayerCount = m_pCtx->getLayerCnt();
    createInfo.ppEnabledLayerNames = layers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

 res = vkCreateDevice(d, &createInfo, nullptr, &m_logicalDevice);
 if (res == VK_SUCCESS)
 {
   std::cout << "[+] logical device created successfully" << std::endl;
   vkGetDeviceQueue(m_logicalDevice, 0 /*indices.graphicsFamily.value()*/, 0, &m_graphicsQueue);
 }
 else
 {
   vkException exc(res, "vkLogicalDevice::vkLogicalDevice", "");
   throw exc;
 }

}



/************************************************************************************************************************
 * function :
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   : Aug 2023 (GKHuber)
************************************************************************************************************************/
vkLogicalDevice::~vkLogicalDevice()
{
  vkDestroyDevice(m_logicalDevice, nullptr);  
}



///////////////////////////////////////////////////////////////////////////////////////////////////////
// private functions....
/************************************************************************************************************************
 * function :
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   : Aug 2023 (GKHuber)
************************************************************************************************************************/
QueueFamilyIndices vkLogicalDevice::findQueueFamilies()
{
  VkResult              res = VK_SUCCESS;
  QueueFamilyIndices    indices;
  uint32_t              cntQueueFamilies = 0;
  std::vector<VkQueueFamilyProperties>  vecQueueFamilies;

  vkGetPhysicalDeviceQueueFamilyProperties(m_physDevice, &cntQueueFamilies, nullptr);
  if (0 != cntQueueFamilies)
  {
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
  }
  else
  {
    std::cout << "[-] failed to find any queue families" << std::endl;
  }

  // logic to find queue we are looking for
  return indices;
}