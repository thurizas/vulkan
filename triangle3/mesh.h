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
  mesh(VkPhysicalDevice, VkDevice, 
       VkQueue xferQueue, VkCommandPool xferCmdPool, 
       std::vector<vertex>*, std::vector<uint32_t>*);
  ~mesh();

  int getVertexCount();
  int getIndexCount();

  VkBuffer getVertexBuffer();
  VkBuffer getIndexBuffer();
  
  void destroyBuffers();

private:
  int              m_vertexCount;
  VkBuffer         m_vertexBuffer;
  VkDeviceMemory   m_vertexBufferMemory;

  int              m_indexCount;
  VkBuffer         m_indexBuffer;
  VkDeviceMemory   m_indexBufferMemory;

  VkPhysicalDevice m_physical;
  VkDevice         m_device;

  void      createVertexBuffer(VkQueue xferQueue, VkCommandPool xferCmdPool, std::vector<vertex>* vertices);
  void      createIndexBuffer(VkQueue xferQueue, VkCommandPool xferCmdPool, std::vector<uint32_t>* indices);

};

#endif

