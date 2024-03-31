#include "mesh.h"

#include <iostream>

mesh::mesh() { }

mesh::mesh(VkPhysicalDevice phyDevice, VkDevice logDevice, std::vector<vertex>* vertices)
{
  m_vertexCount = (int)vertices->size();
  m_physical = phyDevice;
  m_device = logDevice;
  createVertexBuffer(vertices);
}

mesh::~mesh()
{

}

int mesh::getVertexCount()
{
  return m_vertexCount;
}

VkBuffer mesh::getVertexBuffer()
{
  return m_vertexBuffer;
}

void mesh::destroyVertexBuffer()
{
  vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
  vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
}

/************************************************************************************************************************
 * function  : createVertexBuffer 
 *
 * abstract  : this function creates and populates a vertex on the GPU that contains the vertex data for this mesh.  The 
 *             steps are;
 *               (a) create the vertex buffer - just a pointer to the buffer, no backing memory yet.
 *               (b) get the memory requirements for the buffer we just created from Vulkan/the devcie 
 *               (c) allocate memory for the buffer
 *                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : CPU and GPU can both see and interact with the buffer
 *                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : writes to buffer happen immediately (no need to call flush)
 *               (d) bind the allocated memory to the buffer
 *               (e) move data from CPU to GPU, four step process
 *                     - create arbitrary pointer in CPU memory
 *                     - memory map the vertex buffer memory to that point
 *                     - memcpy data from the verticies vector to the vertex buffer memory
 *                     - unmap  the mapped memory.
 *
 * parameters: vertices -- [in] pointer to a std::vector containing the vertex data for each point 
 *
 * returns   : VkBuffer opaque pointer to the GPU based buffer.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
void mesh::createVertexBuffer(std::vector<vertex>* vertices)
{
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = sizeof(vertex) * vertices->size();                    // size of buffer
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;                   // intended usage of buffer
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result = vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vertexBuffer);
  if (result == VK_SUCCESS)
  {
    std::cout << "[+] sucessfully created vertex buffer" << std::endl;
  }
  else
  {
    throw std::runtime_error("failed to create vertex buffer");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(m_device, m_vertexBuffer, &memRequirements);

  // get memory requirements 
  VkMemoryAllocateInfo  memoryAllocInfo = {};
  memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocInfo.allocationSize = memRequirements.size;
  memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  result = vkAllocateMemory(m_device, &memoryAllocInfo, nullptr, &m_vertexBufferMemory);
  if (result == VK_SUCCESS)
  {
    std::cout << "[+] sucessfully allocated memory for vertex buffer" << std::endl;
  }
  else
  {
    throw std::runtime_error("failed to allocate memory for vertex buffer");
  }

  // find allocated memory to buffer
  vkBindBufferMemory(m_device, m_vertexBuffer, m_vertexBufferMemory, 0);

  // mam cpu-memory to vertex buffer
  void* data;
  result = vkMapMemory(m_device, m_vertexBufferMemory, 0, bufferInfo.size, 0, &data);
  memcpy(data, vertices->data(), (size_t)bufferInfo.size);
  vkUnmapMemory(m_device, m_vertexBufferMemory);
} 


/************************************************************************************************************************
 * function  : findMemoryTypeIndex 
 *
 * abstract  : This functions queries the physical device to get the memroy types supported by the device.  It searchs 
 *             through the list of available memory types and return the index of the type that we need for the vertex 
 *             buffer.  Index of memory type must match corresponding bit in the allowedTypes.
 *
 * parameters: allowedType -- [in]
 *             properties  -- [in] an or-ed list of properties that we require.
 *
 * returns   : the index of the memory type that we need.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
uint32_t mesh::findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties   memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(m_physical, &memoryProperties);

  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
  {
    if ((allowedTypes & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;  // This memory type is valid, so return its index
    }
  }
}