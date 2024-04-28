#ifndef _MeshModel_h_
#define _MeshModel_h_

#include <vector>
#include <glm/glm.hpp>

#include <assimp/scene.h>

#include "mesh.h"

class MeshModel
{
public:
  MeshModel();
  MeshModel(std::vector<mesh>);
  ~MeshModel();

  size_t    getMeshCount();

  mesh*     getMesh(size_t ndx);

  glm::mat4 getModel();
  void      setModel(glm::mat4);

  void      destroyMeshModel();

  static std::vector<std::string>  LoadMaterials(const aiScene* scene);
  static std::vector<mesh> loadNode(VkPhysicalDevice, VkDevice, VkQueue, VkCommandPool, aiNode*, const aiScene*, std::vector<int>);
  static mesh loadMesh(VkPhysicalDevice, VkDevice, VkQueue, VkCommandPool,aiMesh*, const aiScene*, std::vector<int>);

private:
  std::vector<mesh>  m_meshList;
  glm::mat4          m_model;

};


#endif 
