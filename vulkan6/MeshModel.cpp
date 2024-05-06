#include "MeshModel.h"

MeshModel::MeshModel()
{

}


MeshModel::MeshModel(std::vector<mesh> newMeshList)
{
  m_meshList = newMeshList;
  m_model = glm::mat4(1.0f);
}

MeshModel::~MeshModel()
{
}

size_t MeshModel::getMeshCount()
{
  return m_meshList.size();
}

mesh* MeshModel::getMesh(size_t ndx)
{
  if (ndx >= m_meshList.size())
  {
    throw std::runtime_error("Attempted to access index out of bounds");
  }

  return &m_meshList[ndx];
}

glm::mat4 MeshModel::getModel()
{
  return m_model;
}

void MeshModel::setModel(glm::mat4 newModel)
{
  m_model = newModel;
}

void MeshModel::destroyMeshModel()
{
  for (auto& m : m_meshList)
  {
    m.destroyBuffers();
  }
}

std::vector <std::string> MeshModel::LoadMaterials(const aiScene* scene)
{	
	// Create 1:1 sized list of textures
  std::vector<std::string> textureList(scene->mNumMaterials);
  
  std::cerr << "[?] found " << scene->mNumMaterials << " texture files" << std::endl;
	
	// Go through each material and copy its texture file name (if it exists)
	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		// Get the material
		aiMaterial* material = scene->mMaterials[i];

		// Initialise the texture to empty string (will be replaced if texture exists)
		textureList[i] = "";

		// Check for a Diffuse Texture (standard detail texture)
		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			// Get the path of the texture file
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
			{
				// Cut off any directory information aleady present
				int idx = std::string(path.data).rfind("\\");
				std::string fileName = std::string(path.data).substr(idx + 1);

				textureList[i] = fileName;
			}
			else
			{
			  std::cerr << "[?-] failed to load texture file " << std::string(path.data) << std::endl;
			}
		}
	}

	return textureList;
}

std::vector<mesh> MeshModel::loadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiNode* _node, const aiScene* scene, std::vector<int> matToTex)
{
	std::vector<mesh> meshList;

	// Go through each mesh at this node and create it, then add it to our meshList
	for (size_t i = 0; i < _node->mNumMeshes; i++)
	{
		meshList.push_back(
			loadMesh(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, scene->mMeshes[_node->mMeshes[i]], scene, matToTex)
		);
	}

	// Go through each node attached to this node and load it, then append their meshes to this node's mesh list
	for (size_t i = 0; i < _node->mNumChildren; i++)
	{
		std::vector<mesh> newList = loadNode(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, _node->mChildren[i], scene, matToTex);
		meshList.insert(meshList.end(), newList.begin(), newList.end());
	}

	return meshList;
}
mesh MeshModel::loadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiMesh* _mesh, const aiScene* scene, std::vector<int> matToTex)
{
	std::vector<vertex> vertices;
	std::vector<uint32_t> indices;

	// Resize vertex list to hold all vertices for mesh
	vertices.resize(_mesh->mNumVertices);

	// Go through each vertex and copy it across to our vertices
	for (size_t i = 0; i < _mesh->mNumVertices; i++)
	{
		// Set position
		vertices[i].pos = { _mesh->mVertices[i].x, _mesh->mVertices[i].y, _mesh->mVertices[i].z };

		// Set tex coords (if they exist)
		if (_mesh->mTextureCoords[0])
		{
			vertices[i].tex = { _mesh->mTextureCoords[0][i].x, _mesh->mTextureCoords[0][i].y };
		}
		else
		{
			vertices[i].tex = { 0.0f, 0.0f };
		}

		// Set colour (just use white for now)
		vertices[i].col = { 1.0f, 1.0f, 1.0f };
	}

	// Iterate over indices through faces and copy across
	for (size_t i = 0; i < _mesh->mNumFaces; i++)
	{
		// Get a face
		aiFace face = _mesh->mFaces[i];

		// Go through face's indices and add to list
		for (size_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// Create new mesh with details and return it
	mesh newMesh = mesh(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, &vertices, &indices, matToTex[_mesh->mMaterialIndex]);

	return newMesh;
}


