#include "mesh.h"

#include <iostream>

mesh::mesh() { }

mesh::mesh(VkPhysicalDevice phyDevice, VkDevice logDevice, 
	         VkQueue xferQueue, VkCommandPool xferCmdPool, 
	         std::vector<vertex>* vertices, std::vector<uint32_t>* indices)
{
  m_vertexCount = (int)vertices->size();
	m_indexCount = (int)indices->size();

  m_physical = phyDevice;
  m_device = logDevice;

  createVertexBuffer(xferQueue, xferCmdPool, vertices);
	createIndexBuffer(xferQueue, xferCmdPool, indices);
}

mesh::~mesh()
{

}

int mesh::getVertexCount()
{
  return m_vertexCount;
}

int mesh::getIndexCount()
{
	return m_indexCount;
}

VkBuffer mesh::getVertexBuffer()
{
  return m_vertexBuffer;
}

VkBuffer mesh::getIndexBuffer()
{
	return m_indexBuffer;
}


void mesh::destroyBuffers()
{
  vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
  vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

	vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
	vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
}

/************************************************************************************************************************
 * function  : createVertexBuffer 
 *
 * abstract  : this function creates and populates a vertex on the GPU that contains the vertex data for this mesh.  In
 *             this case we use a staging buffer to hold the data and then used a cmd** function to move the data.  This
 *             means that the data will be stored on the GPU in an optimal format because the CPU does not have visibility
 *             into the buffer
 *
 * parameters: xferQueue -- [in] a Vulkan object that represents the transfer queue to use for the operation (by standard
 *                          the graphics queue must support a transfer operation).
 *             xferCmdPool -- [in] a command pool to use to created the command executed on the xferQueue
 *             vertices -- [in] pointer to a std::vector containing the vertex data for each point 
 *
 * returns   : void, modifies m_vertexBuffer.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
void mesh::createVertexBuffer(VkQueue xferQueue, VkCommandPool xferCmdPool, std::vector<vertex>* vertices)
{
	// Get size of buffer needed for vertices
	VkDeviceSize bufferSize = sizeof(vertex) * vertices->size();

	// Temporary buffer to "stage" vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create Staging Buffer and Allocate Memory to it
	createBuffer(m_physical, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	// MAP MEMORY TO VERTEX BUFFER
	void* data;																// 1. Create pointer to a point in normal memory
	vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);			// 2. "Map" the vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferSize);							// 3. Copy memory from vertices vector to the point
	vkUnmapMemory(m_device, stagingBufferMemory);									// 4. Unmap the vertex buffer memory

	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
	// Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it and not CPU (host)
	createBuffer(m_physical, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_vertexBuffer, &m_vertexBufferMemory);

	// Copy staging buffer to vertex buffer on GPU
	copyBuffer(m_device, xferQueue, xferCmdPool, stagingBuffer, m_vertexBuffer, bufferSize);

	// Clean up staging buffer parts
	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void mesh::createIndexBuffer(VkQueue xferQueue, VkCommandPool xferCmdPool, std::vector<uint32_t>* indices)
{
	// Get size of buffer needed for indices
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	// Temporary buffer to "stage" index data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(m_physical, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	// MAP MEMORY TO INDEX BUFFER
	void* data;
	vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices->data(), (size_t)bufferSize);
	vkUnmapMemory(m_device, stagingBufferMemory);

	// Create buffer for INDEX data on GPU access only area
	createBuffer(m_physical, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_indexBuffer, &m_indexBufferMemory);

	// Copy from staging buffer to GPU access buffer
	copyBuffer(m_device, xferQueue, xferCmdPool, stagingBuffer, m_indexBuffer, bufferSize);

	// Destroy + Release Staging Buffer resources
	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}












