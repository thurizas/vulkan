#include <iostream>
#include <algorithm>
#include <iterator>
#include <vulkan/vulkan.hpp>

#include "vkCtx.h"
#include <GLFW/glfw3.h>


#define VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV     0x00000100
#define VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD 0x00000080 
#define VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD 0x00000040 
#define VK_QUEUE_OPTICAL_FLOW_BIT_NV               0x00000100
#define VK_QUEUE_VIDEO_DECODE_BIT_KHR              0x00000020
#define VK_QUEUE_VIDEO_ENCODE_BIT_KHR              0x00000040        // Provided by VK_KHR_video_encode_queue



vkCtx::vkCtx( std::vector<std::string>* pLayers, bool d) : m_debug(d), m_instance(VK_NULL_HANDLE), m_suitableDevice(VK_NULL_HANDLE), m_logicalDevice(VK_NULL_HANDLE)
{
  if (m_debug)
  {
    enumerateLayers();

    // verify layers passed in are actually available, make a list of available layers (m_validationLayers)
    if (pLayers->size() > 0)
    {
      for (std::string name : *pLayers)
      {
        bool bFound = false;

        for (VkLayerProperties layer : m_layers)
        {
          if (strcmp(name.c_str(), layer.layerName) == 0)
          {
            bFound = true;
            char* temp = new char[name.size() + 1];
            memset((void*)temp, '\0', name.size() + 1);
            memcpy(temp, name.c_str(), name.size() + 17);
            m_validationLayers.push_back(temp);
            break;
          }
        }

        if (!bFound)
        {
          std::cout << "[-] requested validation layer " << name.c_str() << " is not available" << std::endl;
        }
      }
    }
    else
    {
      std::cout << "[ ] no validation layers provided" << std::endl;
    }
  }
}

vkCtx::~vkCtx()
{
  // TODO : delete strings in m_validationLayers
  vkDestroyInstance(m_instance, nullptr);
}

// This function initialized Vulkan and enumerates the physical devices found
// print parameter controls in the device list is printed to consol.
VkResult vkCtx::init()
{
  VkResult res = VK_SUCCESS;

  VkApplicationInfo appInfo = {};
  VkInstanceCreateInfo createInfo = {};

  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "module 1";
  appInfo.applicationVersion = 1;
  appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

  // we are using GLFW so if it needs any extensions, get them and add to Vulkan's extension list...
  uint32_t glfwExtensionCnt = 0;
  const char** glfwExtensionList;
  glfwExtensionList = glfwGetRequiredInstanceExtensions(&glfwExtensionCnt);

  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = glfwExtensionCnt;
  createInfo.ppEnabledExtensionNames = glfwExtensionList;
  if (m_debug && (m_validationLayers.size() >0 ))
  {
    std::vector<const char*> layers = { "VK_LAYER_NV_optimus","VK_LAYER_NV_nsight","VK_LAYER_NV_nsight-sys" };
    createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
    createInfo.ppEnabledLayerNames = m_validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  res = vkCreateInstance(&createInfo, nullptr, &m_instance);

  if (res == VK_SUCCESS)
  {
    uint32_t cntPDevices = 0;
    res = vkEnumeratePhysicalDevices(m_instance, &cntPDevices, nullptr);
    m_physicalDevices.resize(cntPDevices);
    vkEnumeratePhysicalDevices(m_instance, &cntPDevices, m_physicalDevices.data());

    if (res == VK_SUCCESS)
    {
      std::cout << "[+] found " << cntPDevices << " physical devices" << std::endl;
      if (m_debug) printPhyDeviceInfo(cntPDevices);
    }
    else
    {
      std::cout << "[-] failed to enumerate devices, error code is: " << res << std::endl;
    }
  }
  else
  {
    std::cout << "[-] failed to create a vulkan instance, error code is: " << res << std::endl;
  }

  return res;
}

VkResult vkCtx::createLDevice(uint32_t device)
{
  float queuePriority = 1.0f;
  VkResult res = VK_SUCCESS;


  // specify queues device must have
  //QueueFamilyIndices   indices = findQueueFamilies(m_physicalDevice[0]);

  VkDeviceQueueCreateInfo  queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  //queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  // specify features device must have
  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo   createInfo{};
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = 0;

  // if(enabledValidationLayers)
  // {
  // 	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  // 	createInfo.ppEnabledLayerNames = validationLayers.data();
  // }
  // else
  // {
  // 	createInfo.enabledlayerCount = 0;
  // }

  res = vkCreateDevice(m_physicalDevices[0], &createInfo, nullptr, &m_logicalDevice);
  if (res == VK_SUCCESS)
  {
    std::cout << "[+] logical device created successfully" << std::endl;
  }
  else
  {
    std::cout << "[-] failed to create logical device" << std::endl;
  }

  return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// private functions
void vkCtx::enumerateLayers()
{
  uint32_t  layerCnt = 0;
  vkEnumerateInstanceLayerProperties(&layerCnt, nullptr);

  m_layers.resize(layerCnt);
  vkEnumerateInstanceLayerProperties(&layerCnt, m_layers.data());

  uint32_t ndx = 0;
  std::cout << "[+] found " << layerCnt << " validation layers" << std::endl;
  for (VkLayerProperties layer : m_layers)
  {
    std::cout << "        layer (" << ndx++ << ") " << layer.layerName << " (" << layer.description << ")" << std::endl;
  }
}



/*
 * finds suitable device(s) and populate the m_suitableDevices with pairs of data
 *      first element - device index from m_physicalDevices
 *      second element - device score
 */
bool vkCtx::findSuitableDevice()
{
  bool bRet = false;

  for (const auto& device : m_physicalDevices)
  {


  }

  if (m_suitableDevices.size() == 0)
  {
    m_suitableDevice = VK_NULL_HANDLE;
    std::cout << "[-] Unable to find a suitable device" << std::endl;
  }



  return bRet;
}


void vkCtx::printPhyDeviceInfo(uint32_t cntPDevices)
{
  uint32_t ndx = 0;


  for (auto device : m_physicalDevices)
  {
    uint32_t cntQueueFamilyProps = 0;
    std::vector<VkQueueFamilyProperties> queueProperties;
    VkPhysicalDeviceMemoryProperties     phyMemProp;

    vkGetPhysicalDeviceMemoryProperties(device, &phyMemProp);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &cntQueueFamilyProps, nullptr);
    queueProperties.resize(cntQueueFamilyProps);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &cntQueueFamilyProps, queueProperties.data());

    std::cout << "[+] device (" << ndx << ") # of queue: (" << cntQueueFamilyProps << ") memory types: ("
      << phyMemProp.memoryTypeCount << ") heap types: (" << phyMemProp.memoryHeapCount << ")" << std::endl;

    // print out queues....
    for (uint32_t jdx = 0; jdx < cntQueueFamilyProps; jdx++)
    {
      uint32_t flags = queueProperties[jdx].queueFlags;
      std::cout << "        queue ( " << jdx << ") properites flags: " << queueProperties[jdx].queueFlags << " (";
      if ((VK_QUEUE_GRAPHICS_BIT & flags) == VK_QUEUE_GRAPHICS_BIT) std::cout << "graphics, ";
      if ((VK_QUEUE_COMPUTE_BIT & flags) == VK_QUEUE_COMPUTE_BIT) std::cout << "compute, ";
      if ((VK_QUEUE_TRANSFER_BIT & flags) == VK_QUEUE_TRANSFER_BIT) std::cout << "transfer, ";
      if ((VK_QUEUE_SPARSE_BINDING_BIT & flags) == VK_QUEUE_SPARSE_BINDING_BIT) std::cout << "sparse, ";
      if ((VK_QUEUE_PROTECTED_BIT & flags) == VK_QUEUE_PROTECTED_BIT) std::cout << "protected, ";
      if ((VK_QUEUE_VIDEO_DECODE_BIT_KHR & flags) == VK_QUEUE_VIDEO_DECODE_BIT_KHR) std::cout << "decode, ";
      if ((VK_QUEUE_VIDEO_ENCODE_BIT_KHR & flags) == VK_QUEUE_VIDEO_ENCODE_BIT_KHR) std::cout << "encode, ";
      if ((VK_QUEUE_OPTICAL_FLOW_BIT_NV & flags) == VK_QUEUE_OPTICAL_FLOW_BIT_NV) std::cout << "optical flow, ";
      std::cout << ")" << std::endl;
    }


    // print out memory types...
    for (uint32_t jdx = 0; jdx < phyMemProp.memoryTypeCount; jdx++)
    {
      uint32_t flags = phyMemProp.memoryTypes[jdx].propertyFlags;
      std::cout << "        memory ( " << jdx << ") : property flags: " << phyMemProp.memoryTypes[jdx].propertyFlags;
      std::cout << " (";
      if ((VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & flags) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) std::cout << "dev local, ";
      if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & flags) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) std::cout << "host visible, ";
      if ((VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & flags) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) std::cout << "host coherent, ";
      if ((VK_MEMORY_PROPERTY_HOST_CACHED_BIT & flags) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT) std::cout << "host cached, ";
      if ((VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT & flags) == VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) std::cout << "lazy alloc, ";
      if ((VK_MEMORY_PROPERTY_PROTECTED_BIT & flags) == VK_MEMORY_PROPERTY_PROTECTED_BIT) std::cout << "protected, ";
      if ((VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD & flags) == VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) std::cout << "AMD dev coherent, ";
      if ((VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD & flags) == VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) std::cout << "AMD dev uncached, ";
      if ((VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV & flags) == VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV) std::cout << "rdma, ";
      std::cout << ")" << std::endl;
    }
    // print out heap types...
    for (uint32_t jdx = 0; jdx < phyMemProp.memoryHeapCount; jdx++)
    {
      uint32_t flags = phyMemProp.memoryHeaps[jdx].flags;
      std::cout << "        heap ( " << jdx << ") : property flags: " << phyMemProp.memoryHeaps[jdx].flags;
      std::cout << " (";
      if ((VK_MEMORY_HEAP_DEVICE_LOCAL_BIT & flags) == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) std::cout << "dev local, ";
      if ((VK_MEMORY_HEAP_MULTI_INSTANCE_BIT & flags) == VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) std::cout << "dev local, ";
      std::cout << ")" << std::endl;
    }

    ++ndx;
  }
}
