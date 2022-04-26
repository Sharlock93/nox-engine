#pragma once
#include <vector>
#include <glm/glm.hpp>

#include <MeshNode.h>
#include <RenderableComponent.h>

// Compiler is my biggest enemy
#define NOMINMAX
#include <assimp/scene.h>
#undef NOMINMAX

#include <Types.h>
#include <Utils.h>


namespace NoxEngine {

	class Mesh2 : public RenderableComponent {
	public:
		Mesh2();
		~Mesh2();


		std::string name;
		bool hasBones;

		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<u32> faceIndices;
		std::vector<glm::vec2> texCoords;

		void setTexture(const char* filename);

		// IRenderable funcs
		inline i32 getNumOfVertices() { return (i32)vertices.size(); }
		inline i32 getNumOfTexCoord() { return (i32)texCoords.size(); }
		inline i32 getNumOfNormals() { return (i32)normals.size(); }
		inline i32 getNumOfElements() { return (i32)faceIndices.size() / 3; }

		void getArrayOfVertices(std::vector<f32>* v);
		void getArrayOfTexCoord(std::vector<f32>* tC);
		void getArrayOfNormals(std::vector<f32>* n);
		void getArrayOfElements(std::vector<i32>* el);

		const char* getNormalTexture() { return normTexName; }
		const char* getAmbientTexture() { return ambTexName; }

		//const char* ambTexName = "S:/Masters/COMP5530M Group Project/Work/code/NoxEngine/assets/meshes/textures/Terracotta_Tiles_002_Base_Color.jpg";
		const char* ambTexName = "C:/Users/Steven/Gitlab/noxengine/assets/meshes/textures/Terracotta_Tiles_002_Base_Color.jpg";
		const char* normTexName = "S:/Masters/COMP5530M Group Project/Work/code/NoxEngine/assets/meshes/textures/Terracotta_Tiles_002_Normal.jpg";
		
		void* CastType(const i32 castID);
		void prepTheData();
	
	};
}