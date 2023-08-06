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

struct vkCtx
{
  vkCtx(std::vector<std::string>* pLayers, bool d = false);
  ~vkCtx();
  VkResult     init();
  VkResult     createLDevice(uint32_t dev = 0);           // create logical device



private:
  bool                            m_debug;
  VkInstance                      m_instance;
  VkDevice                        m_suitableDevice;
  VkDevice                        m_logicalDevice;
  char*                           m_validationLayers;
  std::vector<VkLayerProperties>  m_layers;
  std::vector<VkPhysicalDevice>   m_physicalDevices;
  std::vector<std::pair<uint32_t, uint32_t>> m_suitableDevices;

  void         enumerateLayers();
  void         printPhyDeviceInfo(uint32_t);
  bool         findSuitableDevice();                    // ranks devices and populates m_suitableDevices

};


#endif
