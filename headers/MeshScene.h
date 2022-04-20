#pragma once
#include <vector>
#include <glm/glm.hpp>

#include <MeshNode2.h>
#include <Mesh2.h>

// Compiler is my biggest enemy
#define NOMINMAX
#include <assimp/scene.h>
#undef NOMINMAX

#include <Types.h>
#include <Utils.h>

namespace NoxEngine {

	class MeshScene {
	public:

		MeshScene(const aiScene* mesh);
		MeshScene();
		~MeshScene();

		void printAllNodes();
		void printAllMeshNodes();

		void update();
		void update(time_type dt);

		void flipUV();

		u32 getNumOfAnimations();

		// This is the root of the node nodeHierarchy
		// It contains all the child nodes
		MeshNode2 nodeHierarchy;
		// A reference to all the nodes
		std::vector<MeshNode2*> allNodes;
		// List of mesh objects
		std::vector<Mesh2*> meshes;

		u32 frameIndex;
		u32 animationIndex;

		time_type accumulator;
		time_type timeStep;

		i32 whichTickFloor;
		i32 whichTickCeil;

		// Animation data
		std::vector<aiAnimation*> animations;

		// Animation clip -> numTicks
		std::vector<unsigned int> numTicks;
		// Animation clip -> duration
		std::vector<time_type> animationDuration;

	private:
		void extractGeometricInfo(const aiScene* scene);
		void extractTextureInfo(const aiScene* scene, Mesh2 *mesh);
		void extractAnimationInfo(const aiScene* scene);

		void createNodeHierarchy(aiNode* aiRootnode, MeshNode2* rootNode);
		void traverseTreeStructure(aiNode* node, MeshNode2* targetParent);
		void searchThroughTree(MeshNode2* targetNode);

		// Just some function for printing
		void loopAllNodes(MeshNode2 node, std::vector<MeshNode2>& list);
	};
}