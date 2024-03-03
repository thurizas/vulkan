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
  VkDebugUtilsMessengerEXT m_messenger;
  VkInstance               m_instance;
  VkQueue                  m_graphicsQueue;
  VkQueue                  m_presentationQueue;
  VkSurfaceKHR             m_surface;
  std::vector<const char*> m_instanceExtensions = std::vector<const char*>();
  struct {
    VkPhysicalDevice  physical;
    VkDevice          logical;
  } m_device;


  void createInstance();
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT&);
  void setupDebugMessenger();
  void createSurface();
  void createLogicalDevice();

  void getPhysicalDevice();

  bool checkExtensions();
  bool checkValidationSupport();
  bool checkDeviceSuitable(VkPhysicalDevice);
  
  queueFamilyIndices getQueueFamilies(VkPhysicalDevice, VkQueueFamilyProperties*);
};


#endif 
