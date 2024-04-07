#ifndef _vkValidations_h_
#define _vkValidations_h_

#include <vector>

// add the default validation layers to the list of required validation layers
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

// create the messenger to deliever messages to the receipent.  Due to being (potentially third-party) 
// layers we need to lookup the address of the function to manually call it.
VkResult CreateDebugMessengerExt(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
  auto fnc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

  if (nullptr != fnc)
  {
    return fnc(instance, pCreateInfo, pAllocator, pDebugMessenger);
  }
  else
  {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

// kill the messenger.  Due to being (potentially third-party) layers we need to lookup the 
// address of the function to manually call it.
void DestroyDebugUtilsMessengerEXT(VkInstance instance , VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
  auto fnc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (nullptr != fnc)
  {
    fnc(instance, debugMessenger, pAllocator);
  }
}

// code for the messenger.  simplistic version, just print out to console.
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallback, void* pUserData)
{
  std::cerr << "[V] VALIDATION " ;
  if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) std::cerr << "debug ";
  else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) std::cerr << "info ";
  else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) std::cerr << "warning ";
  else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) std::cerr << "error ";
  else std::cerr << "unknow severity ";
  std::cerr << pCallback->pMessage << std::endl;

  return VK_FALSE;
}
#endif
