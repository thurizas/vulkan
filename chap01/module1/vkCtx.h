#ifndef _vkCtk_h_
#define _vtCtk_h_

/*
 * creates and manages a new vulkan context.  A vulkan context manages:
 *  (1) a list of enabled layers
 *  (2) a list of vulkan compatable physical devices
 *  (3) a set of attributes for each physical devices (memory, heap, flags)
 *  (4) a logical device determined by the design requirements and the available 
 *      physical devices
 */



#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <vector>
#include <map>

struct GLFWwindow;
class vkLogicalDevice;

// for use in defining properties and reatures we are interested in having
typedef struct _vkProperties
{
  uint64_t type : 3;      // type of graphics card, see VkPhysicalDeviceProperties
  uint64_t ops : 9;       // operation supported by graphics card, see VkQueueFlagBits
  uint64_t features : 52; // features supported by graphics card
} vkProperties;

struct vkCtx
{
  vkCtx(std::vector<std::string>* pLayers, GLFWwindow* pWindow, bool d = false);
  ~vkCtx();
  VkResult     init(vkProperties, uint32_t* device);
  
  bool         createLogicalDevice(uint32_t dev = 0);           // create logical device

  char*        getValidationLayers() { return m_validationLayers; }
  uint32_t     getLayerCnt() { return m_cntLayers; }

private:
  bool                            m_debug;
  uint32_t                        m_cntLayers;
  uint32_t                        m_physDeviceIndex;
  GLFWwindow*                     m_pWindow;
  VkInstance                      m_instance;
  VkSurfaceKHR                    m_surface;
  vkLogicalDevice*                m_pLogicalDevice;
  char*                           m_validationLayers;
  std::vector<VkLayerProperties>  m_layers;
  std::vector<VkPhysicalDevice>   m_physicalDevices;

  void         enumerateLayers();
  uint32_t     findSuitableDevice(vkProperties);                    // find a suitable device
  void         printPhyDeviceInfo(uint32_t);
 
};


#endif
