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

//#include "vkLogicalDevice.h"
class vkLogicalDevice;

struct vkCtx
{
  vkCtx(std::vector<std::string>* pLayers, bool d = false);
  ~vkCtx();
  VkResult     init();
  bool         createLogicalDevice(uint32_t dev = 0);           // create logical device
  uint32_t     findSuitableDevice(uint64_t);                            // ranks devices and populates m_suitableDevices

private:
  bool                            m_debug;
  VkInstance                      m_instance;
  vkLogicalDevice*                m_pLogicalDevice;
  char*                           m_validationLayers;
  std::vector<VkLayerProperties>  m_layers;
  std::vector<VkPhysicalDevice>   m_physicalDevices;

  void         enumerateLayers();
  void         printPhyDeviceInfo(uint32_t);
 
};


#endif
