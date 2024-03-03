
#include <iostream>
#include <set>

#include "vkContext.h"
#include "vkValidations.h"



/************************************************************************************************************************
 * function  : ctor
 *
 * abstract  : Constructs an instance of the vulkan context.  We perform the following tasks,
 *               (a) get a list of the extensions required by GLFW, and check that they exist
 *
 * parameters: pWindow -- [in] pointer to the GLFW window we will be rendering to
 *
 * returns   : 
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
vkContext::vkContext(GLFWwindow* pWindow, bool v) : m_pWindow(pWindow), m_useValidation(v), m_device({ nullptr, nullptr })
{
  uint32_t glfwExtensionCnt = 0;
  const char** glfwExtensions;       

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCnt);
  for (size_t ndx = 0; ndx < glfwExtensionCnt; ndx++)
  {
    m_instanceExtensions.push_back(glfwExtensions[ndx]);
    std::cout << "[?] adding " << glfwExtensions[ndx] << " to list of required extenstions" << std::endl;
  }

  // add VK_EXT_debug_utils to our list of required extensions, if debugging is enabled.
  if (m_useValidation)
  {
    m_instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  if (!checkExtensions())
  {
    std::cerr << "[-] VkInstance does not support required extension" << std::endl;
    throw std::runtime_error("[-] VkInstance does not support required extension");
  }

  if (!checkValidationSupport())
  {
    std::wcerr << "[-] VkInstance does not support a requested validation layer" << std::endl;
    //throw std::runtime_error("[-] VKInstance does not support a requested validation layer");
    m_useValidation = false;
  }
}

/************************************************************************************************************************
 * function  : dtor
 *
 * abstract  : Destroys the instance of the vulkan context
 *
 * parameters: void
 *
 * returns   :
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
vkContext::~vkContext()
{

}

/************************************************************************************************************************
 * function  : initContext
 *
 * abstract  : The function constructs the actual instance of the context.  The following steps are performed:
 *               (a) create the vulkan instance
 *               (b) enumerate the physical devices and gets their properties
 *               (c) select the physical device that matches the application requirements
 *               (d) create a logical device
 *
 * parameters: void
 *
 * returns   : void.  Throws a runtime exception on error
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
int vkContext::initContext()
{
  try
  {
    createInstance();
    setupDebugMessenger();
    createSurface();
    getPhysicalDevice();
    createLogicalDevice();
  }
  catch (const std::runtime_error& e)
  {
    std::cout << "[ERROR] " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return 0;
}

void vkContext::cleanupContext()
{
  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  vkDestroyDevice(m_device.logical, nullptr);
  if(m_useValidation) DestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);
  vkDestroyInstance(m_instance, nullptr);
}

void vkContext::createInstance()
{
  VkApplicationInfo appInfo = { };
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Test Vulkan App";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(m_instanceExtensions.size());
  createInfo.ppEnabledExtensionNames = m_instanceExtensions.data();

  if (m_useValidation)
  {
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    populateDebugMessengerCreateInfo(debugCreateInfo);

    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
  }
  else
  {
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
  }

  VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
  if (result == VK_SUCCESS)
  {
    std::cout << "[+] Instance created successfully" << std::endl;
  }
  else
  {
    throw std::runtime_error("Failed to create Vulkan Instance");
  }
}

void vkContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;
}

void vkContext::setupDebugMessenger()
{
  if (!m_useValidation) return;
    
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

  populateDebugMessengerCreateInfo(createInfo);
  VkResult result = CreateDebugMessengerExt(m_instance, &createInfo, nullptr, &m_messenger);
  
  if (result == VK_SUCCESS)
  {
    std::cerr << "[+] sucessfully created debug messenger" << std::endl;
  }
  else
  {
    std::wcerr << "[-] failed to create debug messenger" << std::endl;
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

void vkContext::createSurface()
{
  VkResult result = glfwCreateWindowSurface(m_instance, m_pWindow, nullptr, &m_surface);
  if (result == VK_SUCCESS)
  {
    std::cerr << "[+] surface created successfully" << std::endl;
  }
  else
  {
    throw std::runtime_error("Failed to create a rendering surface");
  }
}

void vkContext::createLogicalDevice()
{
  queueFamilyIndices indices = getQueueFamilies(m_device.physical, nullptr);
  
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<int> qfi = { indices.graphicsFamily, indices.presentationFamily };

  for (int queueFamilyIndex : qfi)
  {
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;

    float priority = 1.0f;
    queueCreateInfo.pQueuePriorities = &priority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());		
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();								
  //deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());	// Number of enabled logical device extensions
  //deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();							          // List of enabled logical device extensions
  VkPhysicalDeviceFeatures deviceFeatures = {};

  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;			

  VkResult result = vkCreateDevice(m_device.physical, &deviceCreateInfo, nullptr, &m_device.logical);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create a Logical Device");
  }
  else
  {
    std::cerr << "[+] logical device created." << std::endl;

    vkGetDeviceQueue(m_device.logical, indices.graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device.logical, indices.presentationFamily, 0, &m_presentationQueue);
  }
}

void vkContext::getPhysicalDevice()
{
  uint32_t deviceCnt = 0;
  vkEnumeratePhysicalDevices(m_instance, &deviceCnt, nullptr);

  if (deviceCnt == 0)
  {
    std::cerr << "[-] Could not find any physical devices" << std::endl;
    throw std::runtime_error("Can't find GPUs that support Vulkan Instance");
  }

  std::vector<VkPhysicalDevice> deviceList(deviceCnt);
  vkEnumeratePhysicalDevices(m_instance, &deviceCnt, deviceList.data());

  for (const auto &device : deviceList)
  {
    if (checkDeviceSuitable(device))
    {
      m_device.physical = device;
      break;
    }
  }

  if (m_device.physical == nullptr)
  {
    std::cout << "[-] no suitable physical device found" << std::endl;
    throw std::runtime_error("No suitalbe physical device found");
  }
}


bool vkContext::checkExtensions()
{
  uint32_t extensionCnt = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCnt, nullptr);

  std::vector<VkExtensionProperties> extensions(extensionCnt);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCnt, extensions.data());

  // check to see if requested extensions are in list of install extensions.
  std::cerr << "[?] found " << extensionCnt << " supported extensions" << std::endl;
  for (auto reqExt : m_instanceExtensions)
  {
    bool hasExtention = false;

    for (const auto &ext : extensions)
    {
      if (strcmp(reqExt, ext.extensionName) == 0)
      {
        hasExtention = true;
        break;
      }
    }

    if (!hasExtention)
    {
      std::cerr << "[-] extention: " << reqExt << " not found." << std::endl;
      return false;
    }
  }

  return true;
}

bool vkContext::checkValidationSupport()
{
  uint32_t validationLayerCnt;
  vkEnumerateInstanceLayerProperties(&validationLayerCnt, nullptr);

  // Check if no validation layers found AND we want at least 1 layer
  if (validationLayerCnt == 0 && validationLayers.size() > 0)
  {
    return false;
  }

  std::vector<VkLayerProperties> availableLayers(validationLayerCnt);
  vkEnumerateInstanceLayerProperties(&validationLayerCnt, availableLayers.data());

  std::cerr << "[?] found " << validationLayerCnt << " supported layers" << std::endl;
  for (const auto& validationLayer : validationLayers)
  {
    bool hasLayer = false;
    for (const auto& availableLayer : availableLayers)
    {;
      if (strcmp(validationLayer, availableLayer.layerName) == 0)
      {
        hasLayer = true;
        break;
      }
    }

    if (!hasLayer)
    {
      std::cerr << "[-] failed to find validation layer " << validationLayer << std::endl;
      return false;
    }
  }

  return true;
}


bool vkContext::checkDeviceSuitable(VkPhysicalDevice dev)
{
  VkQueueFamilyProperties queueProperties;
  
  VkPhysicalDeviceProperties devProperties;
  vkGetPhysicalDeviceProperties(dev, &devProperties);

  //VkPhysicalDeviceFeatures devFeatures;
  //vkGetPhysicalDeviceFeatures(dev, &devFeatures);
  

  queueFamilyIndices indices = getQueueFamilies(dev, &queueProperties);

  if (indices.isValid())
  {
    std::cerr << "[+] found suitable device: " << devProperties.deviceName << std::endl;
    std::cerr << "    queue families (" << queueProperties.queueCount << ")" << std::endl;
    std::cerr << "    capabilites:" << ((queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "graphics," : "");
    std::cerr << ((queueProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) ? "compute," : "");
    std::cerr << ((queueProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) ? "transfer," : "");
    std::cerr << ((queueProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? "sparse," : "");
    std::cerr << ((queueProperties.queueFlags & VK_QUEUE_PROTECTED_BIT) ? "protected," : "");
    std::cerr << ((queueProperties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) ? "video decode," : "");
    std::cerr << std::endl;
  }

  return indices.isValid();
}


queueFamilyIndices vkContext::getQueueFamilies(VkPhysicalDevice dev, VkQueueFamilyProperties* pProp)
{
  queueFamilyIndices indices;

  uint32_t queueFamilyCnt = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCnt, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCnt);
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCnt, queueFamilyList.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilyList)
  {
    // check to see if there are queue's with the appropriate types set.
    if ((queueFamily.queueCount > 0) && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
    {
      indices.graphicsFamily = i;
    }

    // check to see if the queue family supports presentations
    VkBool32 presentationSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_surface, &presentationSupport);
    if ((queueFamily.queueCount > 0) && presentationSupport)
    {
      indices.presentationFamily = i;
    }

    if (indices.isValid())
    {
      if(nullptr != pProp) *pProp = queueFamily;
      break;
    }

    i++;
  }

  return indices;
}




