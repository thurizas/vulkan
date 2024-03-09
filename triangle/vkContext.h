#ifndef _vkContext_h_
#define _vkContext_h_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include "utilities.h"

class vkContext
{
public:
  vkContext(GLFWwindow*, bool v = true);
  ~vkContext();

  int initContext();
  void cleanupContext();


private:
  GLFWwindow* m_pWindow;
  bool        m_useValidation;

  // Vulkan components
  VkDebugUtilsMessengerEXT    m_messenger;
  VkInstance                  m_instance;
  VkQueue                     m_graphicsQueue;
  VkQueue                     m_presentationQueue;
  VkSurfaceKHR                m_surface;
  VkSwapchainKHR              m_swapchain;
  std::vector<const char*>    m_instanceExtensions = std::vector<const char*>();
  std::vector<swapChainImage> m_swapChainImages;
  struct {
    VkPhysicalDevice  physical;
    VkDevice          logical;
  } m_device;

  // Utility components
  VkFormat   m_swapChainImageFormat;
  VkExtent2D m_swapChainExtent;

  // Vulkan functions - create functions
  void createInstance();
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT&);
  void createDebugMessenger();
  void createLogicalDevice();
  void createSurface();
  void createSwapChain();  
  void createGraphicsPipeline();


  // Vulkan functions - get functions
  void getPhysicalDevice();

  // support functions -- checker functions
  bool checkInstanceExtensionSupport();
  bool checkDeviceExtensionSupport(VkPhysicalDevice);
  bool checkValidationLayerSupport();
  bool checkDeviceSuitable(VkPhysicalDevice);
  
  // support functions -- getter functions
  queueFamilyIndices getQueueFamilies(VkPhysicalDevice, VkQueueFamilyProperties*);
  SwapChainDetails   getSwapChainDetails(VkPhysicalDevice);

  // choose functions
  VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats); 
  VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes); 
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities); 

  // generic create functions
  VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags); 
  VkShaderModule createShaderModule(const std::vector<char>& code);
};


#endif 