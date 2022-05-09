#include <Components/AudioGeometryComponent.h>

#include "Core/MeshScene.h"

using namespace NoxEngine;


AudioGeometryComponent::AudioGeometryComponent() {

	id = ComponentType::AudioGeometryType;

	has_texture = 0;
	has_normal = 0;
	use_indices = 1;

	geometryId = -1;
	render = false;
	registered = false;

	shape = Shape::Undefined;
	glRenderType = GL_LINES;

	v1 = vec3(0);
	v2 = vec3(0);
	v3 = vec3(0);
	v4 = vec3(0);
}

void AudioGeometryComponent::loadMesh(const MeshScene* meshScene) {

	// Copy all vertices, faces, indices over
	for (u32 i = 0; i < meshScene->meshes.size(); i++) {
		vertices.insert(vertices.end(), meshScene->meshes[i]->getVertices().begin(), meshScene->meshes[i]->getVertices().end());
		faces.insert(faces.end(), meshScene->meshes[i]->getFaces().begin(), meshScene->meshes[i]->getFaces().end());
		indices.insert(indices.end(), meshScene->meshes[i]->getIndices().begin(), meshScene->meshes[i]->getIndices().end());
	}

	// Indices are never present using our FBX loader as of (2022-May-09), so we convert face indices to line indices here
	if (indices.size() == 0) {
		generateIndicesFromTriangles(faces);
	}

	// Even though we don't use texcoords / normals, we still need to fill in dummy data?
	// (Pattern as per GridObject)
	texCoords.resize(vertices.size());
	normals.resize(vertices.size());
}

void AudioGeometryComponent::generateBoundingBox(const Array<vec3>& verts) {

	// no vertices - cannot generate bounding box
	if (verts.size() == 0) return;

	// find the min/max x,y,z respectively (more efficient to find both min and max in 1 pass)
	vec3 l = verts[0], u = verts[0];
	for (const vec3 v : verts) {
		if (l.x > v.x) l.x = v.x;
		if (l.y > v.y) l.y = v.y;
		if (l.z > v.z) l.z = v.z;
		if (u.x > v.x) u.x = v.x;
		if (u.y > v.y) u.y = v.y;
		if (u.z > v.z) u.z = v.z;
	}

	// Given min/max vertices, generate the 6 other corners and generate faces / indices for lines as follows:
	//      6_______ 5
	//      /|     /| 
	//     / |    / |
	//   7/__|__4/__|1
	//    |  /2  |  /
	//    | /    | /
	//    |/_____|/
	//   3       0

	vertices = {
		{ u.x, l.y, l.z },
		{ u.x, l.y, u.z },
		{ l.x, l.y, u.z },
		l,					// min
		{ u.x, u.y, l.z },
		u,					// max
		{ l.x, u.y, u.z },
		{ l.x, u.y, l.z }
	};

	faces = {
		{1,2,3},
		{7,6,5},
		{4,5,1},
		{5,6,2},
		{2,6,7},
		{0,3,7},
		{0,1,3},
		{4,7,5},
		{0,4,1},
		{1,5,2},
		{3,2,7},
		{4,0,7}
	};

	generateIndicesFromTriangles(faces);
}

// Assume the 4 vertices is in CCW, and is planar
// TODO: planar test
// TODO: cross product to ensure CCW
void AudioGeometryComponent::generatePlane(vec3 _v1, vec3 _v2, vec3 _v3, vec3 _v4) {
	
	vertices = { _v1, _v2, _v3, _v4 };
	faces = {
		{0, 1, 2},
		{0, 2, 3}
	};

	generateIndicesFromTriangles(faces);
}



void AudioGeometryComponent::generateIndicesFromTriangles(const Array<ivec3>& _faces) {

	u32 nExistingIndices = indices.size();
	indices.resize(indices.size() + _faces.size() * 6);

	for (u32 f = 0; f < _faces.size(); f++) {
		ivec3 face = _faces[f];
		u32 start = nExistingIndices + f * 6;

		// indices form 3 lines: (0,1), (1,2), (2,0)
		indices[start + 0] = indices[start + 5] = face[0];
		indices[start + 1] = indices[start + 2] = face[1];
		indices[start + 3] = indices[start + 4] = face[2];
	}
}