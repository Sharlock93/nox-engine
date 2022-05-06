#pragma once

#include <assimp/scene.h>
#include <Utils/Utils.h>

#include "MeshNode.h"
#include "Mesh.h"
#include "Types.h"

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

		void updateCeilAndFloor();

		void flipUV();
		void setAnimationIndex(u32 num);

		// Reset the play time
		void resetAnimation();

		u32 getNumOfAnimations();
		bool hasAnimations();
		
		void updateNumTicks(u32 animationIndex, u32 num);
		void insertFrame(u32 animationIndex, u32 selectedFrame);
		void setFrameIndex(u32 index);

		// This is the root of the node nodeHierarchy
		// It contains all the child nodes
		MeshNode nodeHierarchy;
		// A reference to all the nodes
		Array<MeshNode*> allNodes;
		// List of mesh objects
		Array<Mesh*> meshes;

		u32 frameIndex;
		u32 animationIndex;

		time_type accumulator;
		time_type timeStep;

		i32 whichTickFloor;
		i32 whichTickCeil;

		// Animation data
		Array<aiAnimation*> animations;

		// Animation clip -> numTicks
		Array<i32> numTicks;
		// Animation clip -> duration
		Array<time_type> animationDuration;

		bool playAnimation = false;

	private:
		void extractGeometricInfo(const aiScene* scene);
		void extractTextureInfo(const aiScene* scene, Mesh *mesh);
		void extractAnimationInfo(const aiScene* scene);

		void createNodeHierarchy(aiNode* aiRootnode, MeshNode* rootNode);
		void traverseTreeStructure(aiNode* node, MeshNode* targetParent);
		void searchThroughTree(MeshNode* targetNode);

		// Just some function for printing
		void loopAllNodes(MeshNode node, Array<MeshNode>& list);
	};
}
