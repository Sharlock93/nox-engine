/* 
 * An interface: comunication with Renderer component of the engine.
 * Allows a class that implements it to be rendered by Renderer
 * 
 * When storing geometric data most formats store separate index lists for dif attributes.
 * OpenGl does not allow multiple index buffers for diff attributes. 
 * Hence, this class should prep the data for the Renderer implemented with OpenGl
 */
#pragma once

#include <Types.h>
#include <glad/glad.h>

namespace NoxEngine {
	class IRenderable
	{

		public:
			virtual ~IRenderable() {};
			IRenderable() {};

			virtual i32 getNumOfVertices() = 0;
			virtual i32 getNumOfTexCoord() = 0;
			virtual i32 getNumOfNormals()  = 0;
			virtual i32 getNumOfFaces() = 0;

			virtual const Array<vec3>& getVertices () const = 0;
			virtual const Array<vec2>& getTexCoords() const = 0;
			virtual const Array<vec3>& getNormals  () const = 0;
			virtual const Array<ivec3>& getFaces   () const = 0;
			virtual const Array<i32>& getIndices   () const = 0;

			virtual const String getNormalTexture() = 0;
			virtual const String getAmbientTexture() = 0;

			// Stores initial attributes without dublicates
			Array<vec3>   vertices;
			Array<vec2>  texCoords;
			Array<vec3>    normals;
			Array<ivec3>     faces;
			Array<i32>     indices;

			i8 has_texture;
			i8 has_normal;
			i8 use_indices;

			f32 color[3];
			GLenum glRenderType;
	};
}
