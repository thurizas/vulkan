#include <iostream>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <vulkan/vulkan.hpp>

#include "vkException.h"
#include "vkCtx.h"
#include "vkLogicalDevice.h"
#include <GLFW/glfw3.h>


#define VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV     0x00000100
#define VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD 0x00000080 
#define VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD 0x00000040 
#define VK_QUEUE_OPTICAL_FLOW_BIT_NV               0x00000100
#define VK_QUEUE_VIDEO_DECODE_BIT_KHR              0x00000020
#define VK_QUEUE_VIDEO_ENCODE_BIT_KHR              0x00000040        // Provided by VK_KHR_video_encode_queue



/************************************************************************************************************************
 * function  : ctor  
 *
 * abstract  : This constructs an instance of the vulkan context.  This function performs the following operations,
 *             (1) enumerates the validation layers that are installed
 *             (2) constructs a validation string (char* m_validationLayers) by confirming that all names passed in from 
 *                 the command line are actually installed on the system
 *             (3) constructs an extension string (char* m_extentions) by confirming that all names passed in from the 
 *                 the command line are actually installed on the system.
 *
 * parameters: pLayers -- [in] std::vector<std::string>*  contains a vector of requested name of the validation layers
 *             pExt    -- [in] std::vector<std::string>* contains a vector of requested names of extentions
 *             pWindow -- [in] pointer to a GLFWwindow structure representing the GLFW window that rendering will be 
 *                        done to.
 *             d -- [in] flag to indicate if this is being run in debug mode or not.  validation layers are only active
 *                       in a debug build.  
 *
 * returns   : throws an exception of type vkException if an error occures.  note, failing to find on of the requested
 *             validation layers is not an error, however it will generate a warning.
 *
 * written   : Aug 2023 (GKHuber)
************************************************************************************************************************/
vkCtx::vkCtx( std::vector<std::string>* pLayers, std::vector<const char*>* pExt, GLFWwindow* pWindow, bool d) : m_debug(d), m_physDeviceIndex(-1), 
              m_pWindow(pWindow), m_instance(VK_NULL_HANDLE), m_pLogicalDevice(nullptr), m_validationLayers(nullptr)
{
  if (m_debug)
  {
    enumerateLayers();
    enumerateExtensions();

    // verify layers passed in are actually available, make a list of available layers (m_validationLayers)
    if (pLayers->size() > 0)
    {
      size_t loc = 0;
      // figure out the length of names of the requested layers....
      size_t strLen = std::accumulate(pLayers->begin(), pLayers->end(), (size_t)0, [](size_t sum, const std::string str) { return sum + str.size(); });
      m_validationLayers = new char[sizeof(char) * (strLen + pLayers->size()+2)];               // leave room for null-terminator
      memset((void*)m_validationLayers, '\0', strLen + pLayers->size()+2);

      for (std::string name : *pLayers)
      {
        bool bFound = false;

        for (const VkLayerProperties& layer : m_layerList)
        {
          if (strcmp(name.c_str(), layer.layerName) == 0)
          {
            bFound = true;
            m_cntLayers++;
            memcpy(&m_validationLayers[loc], name.c_str(), name.length());
            loc += name.length()+1;
            break;
          }
        }

        if (!bFound)
        {
          std::cout << "[-] requested validation layer " << name.c_str() << " is not available" << std::endl;
        }
      }

      for (std::string name : *pExt)
      {
        bool bFound = false;

        for (const VkExtensionProperties& ext : m_extensionList)
        {
          if (strcmp(name.c_str(), ext.extensionName) == 0)
          {
            bFound = true;
            m_cntExts++;
            m_extensions.push_back(ext.extensionName);
            break;
          }
        }
      }
    }
  }
}


vkCtx::~vkCtx()
{
  if (nullptr != m_pLogicalDevice) { delete m_pLogicalDevice; m_pLogicalDevice = nullptr; }

  vkDestroyInstance(m_instance, nullptr);

  if (nullptr != m_validationLayers)  { delete[] m_validationLayers; m_validationLayers = nullptr; }
}



/************************************************************************************************************************
 * function  : init
 *
 * abstract  : This function initializes the vulkan context, it does the following operations;
 *             (1) creates the vulkan instance (stored in m_instance).
 *             (2) enumerates the physical devices. retreiving thier physical properties, 
 *             (3) compare the physical properties with the requested physical properties and set this as default.
 *
 * parameters: properties -- [in] a 64-bit value that determines the requested properites of the physical device.
 *
 * returns   : returns a VkResult
 *
 * written   : Aug 2023 (GKHuber)
************************************************************************************************************************/
VkResult vkCtx::init(vkProperties properties, uint32_t* device)
{
  VkResult res = VK_SUCCESS;

  VkApplicationInfo appInfo = {};
  VkInstanceCreateInfo createInfo = {};

  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "module 1";
  appInfo.applicationVersion = 1;
  appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

  // we are using GLFW so if it needs any extensions, get them and add to Vulkan's extension list...
  //uint32_t glfwExtensionCnt = 0;
  //const char** glfwExtensionList;
  //glfwExtensionList = glfwGetRequiredInstanceExtensions(&glfwExtensionCnt);

  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(m_extensions.size());
  createInfo.ppEnabledExtensionNames = m_extensions.data();
  if(m_debug)
  {
    createInfo.enabledLayerCount = m_cntLayers;
    createInfo.ppEnabledLayerNames = const_cast<const char* const*>(&m_validationLayers);
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  res = vkCreateInstance(&createInfo, nullptr, &m_instance);                            // create the vulkan instance 

  if (res == VK_SUCCESS)
  {
    uint32_t cntPDevices = 0;
    res = vkEnumeratePhysicalDevices(m_instance, &cntPDevices, nullptr);
    m_physicalDevices.resize(cntPDevices);
    res = vkEnumeratePhysicalDevices(m_instance, &cntPDevices, m_physicalDevices.data());

    *device = findSuitableDevice(properties);
    if (-1 == *device)
    {
      std::cout << "[-] failed to find a suitable device" << std::endl;
      res = VK_ERROR_INITIALIZATION_FAILED;
    }
  }
  else
  {
    std::cout << "[-] failed to create a vulkan instance, error code is: " << res << std::endl;
  }

  return res;
}

/*
 * finds suitable device(s) base on application specific requirements
 *       low order three bits - GPU type 000b - other, 001n - integrated GPU, 010b - discrete GPU, 011b - virtual, 100b - CPU
 *       
 */
/************************************************************************************************************************
 * function  : findSuitableDevice
 *
 * abstract  : This function attempts to determine is one of the detected physical devices is usable, and returns it index 
 *             in the vector of physical devices (m_physicalDevices).  vkGetPhysicalDeviceProperties will fill in a 
 *             VkPhysicalDeviceProperties structure;
 *               typedef struct VkPhysicalDeviceProperties {
 *                     uint32_t                            apiVersion;
 *                     uint32_t                            driverVersion;
 *                     uint32_t                            vendorID;
 *                     uint32_t                            deviceID;
 *                     VkPhysicalDeviceType                deviceType;
 *                     char                                deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
 *                     uint8_t                             pipelineCacheUUID[VK_UUID_SIZE];
 *                     VkPhysicalDeviceLimits              limits;
 *                     VkPhysicalDeviceSparseProperties    sparseProperties;
 *               } VkPhysicalDeviceProperties;
 * 
 *             The field, deviceType is an enumeration for the type of the device,
 *                enum {VK_PHYSICAL_DEVICE_OTHER = 0, VK_PHYSICAL_DEVICE_INTEGRATED_GPU=1, VK_PHYSICAL_DEVICE_DESCRETE_GPU=2,
 *                      VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU=3,VK_PHYSICAL_DEVICE_TYPE_CPu=4}
 * 
 *             For now we take the first physical device that satisfies the requirements.
 * 
 *     // check to see if found device feature's satisify requested features...
       // TODO : VK defined options are not exclusive  integrated_gpu | discrete_gpu == virtual_gpu
       // TODO : there are 55 features defined in vk SDK, each as a bit-field
       // TODO : we can map all featrues to a bit-map on a 64-bit integer
 *
 * parameters: properties -- [in] 64-bit value representing the requested properties of the physical device.
 *
 * returns   : If no suitable device is found a -1 is returned.  
 *
 * written   : Aug 2023 (GKHuber)
************************************************************************************************************************/
uint32_t vkCtx::findSuitableDevice(vkProperties properties)
{
  uint32_t  nRet = -1;                                     // device that matchs requirements
  uint32_t devProp = properties.type;
  uint32_t devOps = properties.ops;
  std::vector<uint32_t> canidateDevs;                      // vector of canidate devices
  uint32_t  nDev = 0;                                      // index of device in phy device buffer
  
  //for (const auto& device : m_physicalDevices)           
  for(nDev = 0; nDev < m_physicalDevices.size(); nDev++)   // iterate over the physical devices
  {
    VkPhysicalDeviceProperties   phyDevProp;               // check the physical device properties

    // check to see if found device satisify's requested properties...
    vkGetPhysicalDeviceProperties(m_physicalDevices.at(nDev), &phyDevProp);

    // we ask is physical device of type 'T' _and_ is type 'T' requested 
    if (phyDevProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER) continue;
    if((phyDevProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) && (devProp & VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)) canidateDevs.push_back(nDev);
    if ((phyDevProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && (devProp & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) canidateDevs.push_back(nDev);
    if ((phyDevProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) && (devProp & VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)) canidateDevs.push_back(nDev);
    if ((phyDevProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) && (devProp & VK_PHYSICAL_DEVICE_TYPE_CPU)) canidateDevs.push_back(nDev);
  }

  for(uint32_t dev : canidateDevs)                                       // found a canidate device, look at queues
  {
    uint32_t                              cntQueueFamilyProps = 0;
    std::vector<VkQueueFamilyProperties>  queueProperties;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevices.at(dev), &cntQueueFamilyProps, nullptr);
    queueProperties.resize(cntQueueFamilyProps);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevices.at(dev), &cntQueueFamilyProps, queueProperties.data());

    int cnt = 0;
    
    // we ask for each queue, the the queue bit set _and_ is that bit requested.
    for(VkQueueFamilyProperties queue : queueProperties)
    {
      if ((queue.queueFlags & devOps) == devOps)
      {
        nRet = dev;               // canidate device meets out expectations
        std::cout << "[+] found a suitable device: " << dev << " with a suitable queue" << std::endl;
        break;
      }
    }
  }

  return nRet;
}

// device is index into physical device vector
bool vkCtx::createLogicalDevice(uint32_t device)
{
  bool bRet = false;

  try
  {
    m_pLogicalDevice = new vkLogicalDevice(this, m_physicalDevices[device], m_debug);
    bRet = true;
  }
  catch (std::bad_alloc)
  {
    std::cout << "[-] failed to allocate memory for a new logical device" << std::endl;
  }
  catch (vkException& exc)
  {
    std::cout << "[-] failed to create logical device, " << exc.what() << std::endl;
  }

 
  return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// debugging support
VkResult vkCtx::createDebugMessenger(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback)
{
  VkDebugUtilsMessengerCreateInfoEXT   createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;

  // need to load this dynamically as it comes from an extention.
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr)
    return func(m_instance, &createInfo, nullptr, &m_debugMessenger);
  else
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void vkCtx::destroyDebugMessenger()
{
//  vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);

  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) 
  {
    func(m_instance, m_debugMessenger, nullptr);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// private functions
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
void vkCtx::enumerateLayers()
{
  uint32_t  layerCnt = 0;
  vkEnumerateInstanceLayerProperties(&layerCnt, nullptr);

  m_layerList.resize(layerCnt);
  vkEnumerateInstanceLayerProperties(&layerCnt, m_layerList.data());

  uint32_t ndx = 0;
  std::cout << "[+] found " << layerCnt << " validation layers" << std::endl;
  for (VkLayerProperties layer : m_layerList)
  {
    std::cout << "        layer (" << ndx++ << ") " << layer.layerName << " (" << layer.description << ")" << std::endl;
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
void vkCtx::enumerateExtensions()
{
  uint32_t extCnt = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extCnt, nullptr);
  m_extensions.resize(extCnt);

  vkEnumerateInstanceExtensionProperties(nullptr, &extCnt, m_extensionList.data());

  uint32_t ndx = 0;
  std::cout << "[+] found " << extCnt << " extensions " << std::endl;
  for (VkExtensionProperties ext : m_extensionList)
  {
    std::cout << "        extension (" << ndx++ << ") " << ext.extensionName << " ,version: " << ext.specVersion << std::endl;
  }
}


void vkCtx::printPhyDeviceInfo(uint32_t cntPDevices)
{
  uint32_t ndx = 0;

  for (auto device : m_physicalDevices)
  {
    uint32_t cntQueueFamilyProps = 0;
    std::vector<VkQueueFamilyProperties> queueProperties;
    VkPhysicalDeviceMemoryProperties     phyMemProp;
    VkPhysicalDeviceProperties           phyDevProp;

    vkGetPhysicalDeviceProperties(device, &phyDevProp);
    vkGetPhysicalDeviceMemoryProperties(device, &phyMemProp);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &cntQueueFamilyProps, nullptr);
    queueProperties.resize(cntQueueFamilyProps);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &cntQueueFamilyProps, queueProperties.data());


    std::cout << "[+] device (" << ndx << ") # of queue: (" << cntQueueFamilyProps << ") memory types: ("
      << phyMemProp.memoryTypeCount << ") heap types: (" << phyMemProp.memoryHeapCount << ")" << std::endl;
    std::cout << "    device name  : " << phyDevProp.deviceName << std::endl;
    std::cout << "    apiVersion: " << phyDevProp.apiVersion << std::endl;
    std::cout << "    driverVersion: " << phyDevProp.driverVersion << std::endl;
    std::cout << "    device type  : " << phyDevProp.deviceType;
    std::cout << (phyDevProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER ? " - other": "");
    std::cout << (phyDevProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? " - integrated GPU" : "");
    std::cout << (phyDevProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? " - discrete GPU" : "");
    std::cout << (phyDevProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ? " - virtual GPU" : "");
    std::cout << (phyDevProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ? " - CPU" : "");
    std::cout << std::endl;

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
