#ifndef _vkCtk_h_
#define _vtCtk_h_

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
  //std::vector<const char*>        m_validationLayers;
  std::string                     m_validationLayers;
  std::vector<VkLayerProperties>  m_layers;
  std::vector<VkPhysicalDevice>   m_physicalDevices;
  std::vector<std::pair<uint32_t, uint32_t>> m_suitableDevices;

  void         enumerateLayers();
  void         printPhyDeviceInfo(uint32_t);
  bool         findSuitableDevice();                    // ranks devices and populates m_suitableDevices

};


#endif
