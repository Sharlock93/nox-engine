#include "Mesh.h"
#include <iostream>

#include <glm/gtx/string_cast.hpp>


Mesh::Mesh(aiScene *scene) {
	aiMesh** loadedMesh = pScene->mMeshes;

	vertices.resize(pScene->mNumMeshes);
	normals.resize(pScene->mNumMeshes);
	faceIndices.resize(pScene->mNumMeshes);
	texCoord.resize(pScene->mNumMeshes);

	
	// Get vertex, normal and textcoord data
	for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {

		const aiMesh* pMesh = loadedMesh[i];

		vertices[i].resize(pMesh->mNumVertices);
		normals[i].resize(pMesh->mNumVertices);
		texCoord[i].resize(pMesh->mNumVertices);
		faceIndices[i].resize(pMesh->mNumFaces);
		
		const aiVector3D zero3D(0.0f, 0.0f, 0.0f);

		const aiVector3D* pVertex = pMesh->mVertices;
		const aiVector3D* pNormal = &(pMesh->mNormals;
		const aiVector3D* pTexCoord = pMesh->HasTextureCoords(0) ?  &(pMesh->mTextureCoords[0][j]) : &zero3D;

		const bool has_texture = pMesh->HasTextureCoords(0);

		for (u32 j = 0; j < pMesh->mNumVertices; ++j) {
			verteices[i].push_back(glm::vec3{pVertex->x, pVertex->y, pVertex->z});
			normals[i].push_back  (glm::vec3{pNormal->x, pNormal->y, pNormal->z});
			texCoord[i].push_back (glm::vec2{pVertex->x, pVertex->y});

			pVertex++;
			pNormal++;
			if(has_texture) {
				pTexCoord++;
			}
		}

		const aiFace* face = pMesh->mFaces;
		for (unsigned int j = 0; j < pMesh->mNumFaces; ++j)
		{

			if (face->mNumIndices == 3)
			{
				faceIndices[i].push_back(face->mIndices[0]);
				faceIndices[i].push_back(face->mIndices[1]);
				faceIndices[i].push_back(face->mIndices[2]);
			}
			else
			{
				printf("Error: number of face indicies is less than 3");
			}
		}
	}

}

Mesh::Mesh()
{

}

Mesh::~Mesh()
{

}

void Mesh::resizeNumOfMeshes(unsigned int i) {
	vertices.resize(i);
	normals.resize(i);
	faceIndices.resize(i);
	texCoord.resize(i);
}

void Mesh::resizeNumOfAnimations(unsigned int i)
{
	animations.resize(i);
	nodeAnimations.resize(i);
	nodeAnimTransformation.resize(i);
	nodeAnimTranslationMatrices.resize(i);
	nodeAnimRotationMatrices.resize(i);
	nodeAnimScalingMatrices.resize(i);
}

void Mesh::createNodeHierarchy(aiNode* aiRootnode, MeshNode* rootNode)
{
	rootNode->name = aiRootnode->mName.C_Str();
	rootNode->parent = nullptr;

	// =====================================================
	// Convert aiMatrix4x4 to glm::mat4
	// We can convert this to a tool later on
	glm::mat4 transformation;
	transformation[0][0] = aiRootnode->mTransformation.a1;
	transformation[0][1] = aiRootnode->mTransformation.b1;
	transformation[0][2] = aiRootnode->mTransformation.c1;
	transformation[0][3] = aiRootnode->mTransformation.d1;

	transformation[1][0] = aiRootnode->mTransformation.a2;
	transformation[1][1] = aiRootnode->mTransformation.b2;
	transformation[1][2] = aiRootnode->mTransformation.c2;
	transformation[1][3] = aiRootnode->mTransformation.d2;

	transformation[2][0] = aiRootnode->mTransformation.a3;
	transformation[2][1] = aiRootnode->mTransformation.b3;
	transformation[2][2] = aiRootnode->mTransformation.c3;
	transformation[2][3] = aiRootnode->mTransformation.d3;

	transformation[3][0] = aiRootnode->mTransformation.a4;
	transformation[3][1] = aiRootnode->mTransformation.b4;
	transformation[3][2] = aiRootnode->mTransformation.c4;
	transformation[3][3] = aiRootnode->mTransformation.d4;
	// =====================================================

	rootNode->transformation = transformation;

	allNodes.push_back(rootNode);

	copyNodesWithMeshes(aiRootnode, rootNode);
	nodeHierarchy = *rootNode;
}

void Mesh::copyNodesWithMeshes(aiNode* node, MeshNode* targetParent)
{
	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		MeshNode* child = new MeshNode;

		child->name = node->mChildren[i]->mName.C_Str();
		child->parent = targetParent;

		if (node->mChildren[i]->mNumMeshes > 0)
		{
			child->hasMesh = true;
			child->meshIndex = node->mChildren[i]->mMeshes[0];
		}

		// ==============================================
		// Convert aiMatrix4x4 to glm::mat4
		// We can convert this to a tool later on
		glm::mat4 transformation;
		transformation[0][0] = node->mTransformation.a1;
		transformation[0][1] = node->mTransformation.a2;
		transformation[0][2] = node->mTransformation.a3;
		transformation[0][3] = node->mTransformation.a4;

		transformation[1][0] = node->mTransformation.b1;
		transformation[1][1] = node->mTransformation.b2;
		transformation[1][2] = node->mTransformation.b3;
		transformation[1][3] = node->mTransformation.b4;

		transformation[2][0] = node->mTransformation.c1;
		transformation[2][1] = node->mTransformation.c2;
		transformation[2][2] = node->mTransformation.c3;
		transformation[2][3] = node->mTransformation.c4;

		transformation[3][0] = node->mTransformation.d1;
		transformation[3][1] = node->mTransformation.d2;
		transformation[3][2] = node->mTransformation.d3;
		transformation[3][3] = node->mTransformation.d4;
		// ==============================================

		child->transformation = transformation;

		allNodes.push_back(child);

		copyNodesWithMeshes(node->mChildren[i], child);
		targetParent->child.push_back(*child);
	}
}

void Mesh::importAnimationData(aiScene* scene)
{

}

void Mesh::printAllNodes()
{
	std::vector<MeshNode> list;

	loopAllNodes(nodeHierarchy, list);

	for (MeshNode node : list)
	{
		std::cout << node.name << "\n";
	}
}

void Mesh::printAllMeshNodes()
{
	std::vector<MeshNode> list;

	loopAllNodes(nodeHierarchy, list);

	for (MeshNode node : list)
	{
		if (node.hasMesh)
		{
			std::cout << node.name << ": " << "\n";
			std::cout << glm::to_string(node.transformation) << "\n";
			std::cout << "====================================================" << "\n";
		}
	}
}

void Mesh::loopAllNodes(MeshNode node, std::vector<MeshNode>& list)
{
	list.push_back(node);
	for (unsigned int i = 0; i < node.child.size(); ++i)
	{
		list.push_back(node.child[i]);

		loopAllNodes(node.child[i], list);
	}
}


void Mesh::prepForRenderer()
{
	std::vector<std::vector<glm::vec3>> mVertices;
	mVertices.resize(vertices.size());

	// Apply transformation from nodes
	for (unsigned int i = 0; i < allNodes.size(); i++)
	{
		if (!allNodes[i]->hasMesh)
		{
			// Get which mesh to transform
			unsigned int meshIndex = allNodes[i]->meshIndex;

			mVertices[meshIndex].resize(vertices[meshIndex].size());

			// Apply Transformation to all vertices of the mesh
			for (unsigned int j = 0; j < vertices[meshIndex].size(); j++)
			{
				mVertices[meshIndex][j] = glm::vec3(allNodes[i]->transformation *
						glm::vec4(vertices[meshIndex][j], 1.0));
			}

			//for (unsigned int j = 0; j < mVertices[meshIndex].size(); j++)
			//{
			//	std::cout << glm::to_string(mVertices[meshIndex][j]) << "\n";
			//}
		}
	}

	for (unsigned int i = 0; i < vertices[0].size(); i++)
	{
		//verticesPreped.push_back(vertices[0][i].x); verticesPreped.push_back(vertices[0][i].y); verticesPreped.push_back(vertices[0][i].z);
		verticesPreped.push_back(mVertices[0][i].x); verticesPreped.push_back(mVertices[0][i].y); verticesPreped.push_back(mVertices[0][i].z);
	}

	for (unsigned int i = 0; i < normals[0].size(); i++)
	{
		normalsPreped.push_back(normals[0][i].x); normalsPreped.push_back(normals[0][i].y); normalsPreped.push_back(normals[0][i].z);
	}

	for (unsigned int i = 0; i < texCoord[0].size(); i++)
	{
		texCoordPreped.push_back(texCoord[0][i].x); texCoordPreped.push_back(texCoord[0][i].y);
	}


	for (unsigned int i = 0; i < faceIndices[0].size(); i++)
	{
		elements.push_back((int)(faceIndices[0][i]));
	}
}


// ========================   IRenderable ======================

void Mesh::getArrayOfVertices(std::vector<float>* v)
{
	std::vector<std::vector<glm::vec3>> mVertices;
	mVertices.resize(vertices.size());

	// Apply transformation from nodes
	for (unsigned int i = 0; i < allNodes.size(); i++)
	{
		if (!allNodes[i]->hasMesh)
		{
			// Get which mesh to transform
			unsigned int meshIndex = allNodes[i]->meshIndex;

			mVertices[meshIndex].resize(vertices[meshIndex].size());

			// Apply Transformation to all vertices of the mesh
			for (unsigned int j = 0; j < vertices[meshIndex].size(); j++)
			{
				mVertices[meshIndex][j] = glm::vec3(allNodes[i]->transformation *
						glm::vec4(vertices[meshIndex][j], 1.0));
			}

			//for (unsigned int j = 0; j < mVertices[meshIndex].size(); j++)
			//{
			//	std::cout << glm::to_string(mVertices[meshIndex][j]) << "\n";
			//}
		}
	}

	for (unsigned int i = 0; i < vertices[0].size(); i++)
	{
		//verticesPreped.push_back(vertices[0][i].x); verticesPreped.push_back(vertices[0][i].y); verticesPreped.push_back(vertices[0][i].z);
		v->push_back(mVertices[0][i].x); v->push_back(mVertices[0][i].y); v->push_back(mVertices[0][i].z);
	}
}


void Mesh::getArrayOfTexCoord(std::vector<float>* tC)
{
	for (unsigned int i = 0; i < texCoord[0].size(); i++)
	{
		tC->push_back(texCoord[0][i].x); tC->push_back(texCoord[0][i].y);
	}

}


void Mesh::getArrayOfNormals(std::vector<float>* n)
{
	for (unsigned int i = 0; i < normals[0].size(); i++)
	{
		n->push_back(normals[0][i].x); n->push_back(normals[0][i].y); n->push_back(normals[0][i].z);
	}
}


void Mesh::getArrayOfElements(std::vector<int>* el)
{
	for (unsigned int i = 0; i < faceIndices[0].size(); i++)
	{
		el->push_back((int)(faceIndices[0][i]));
	}
}
