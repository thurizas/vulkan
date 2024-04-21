#ifndef _mesh_h_
#define _mesh_h_

#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>

#include <vector>

#include "utilities.h"

struct Model {
  glm::mat4 model;
};

class mesh
{
public:
  mesh();
  mesh(VkPhysicalDevice, VkDevice, VkQueue xferQueue, VkCommandPool xferCmdPool, std::vector<vertex>*, std::vector<uint32_t>*, int newTexId);
  ~mesh();

  void setModel(glm::mat4 newModel);
  Model getModel();

  int getVertexCount();
  int getIndexCount();
  int getTexId();


  VkBuffer getVertexBuffer();
  VkBuffer getIndexBuffer();
  
  void destroyBuffers();

private:
  Model            m_model;
  int              m_texId;

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

