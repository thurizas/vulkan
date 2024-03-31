#ifndef _mesh_h_
#define _mesh_h_

#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>

#include <vector>

#include "utilities.h"

class mesh
{
public:
  mesh();
  mesh(VkPhysicalDevice, VkDevice, std::vector<vertex>* vertices);
  ~mesh();

  int getVertexCount();
  VkBuffer getVertexBuffer();
  void destroyVertexBuffer();

private:
  int              m_vertexCount;
  VkBuffer         m_vertexBuffer;
  VkDeviceMemory   m_vertexBufferMemory;

  VkPhysicalDevice m_physical;
  VkDevice         m_device;

  void      createVertexBuffer(std::vector<vertex>* vertices);
  uint32_t  findMemoryTypeIndex(uint32_t, VkMemoryPropertyFlags);
};

#endif

