#pragma once

#include <map>
#include <Core/Types.h>
#include <typeindex>

namespace NoxEngine {

	// Note (Vincent): This implies the developer cannot create new component types
	enum ComponentType : i32 {
		AbstractType = 0,
		TransformType,
		RenderableType,
		AnimationType,
		LightSourceType,
		AudioSourceType,
		AudioListenerType,
		AudioGeometryType,
		//CollisionType,	// If we want to implement physics, collision can be implement similarlly to audio obstruction
		ScriptType,
		CameraType,
		ComponentTypeCount
	};

	enum ComponentTypeFlag : HasCompBitMask {
		AbstractFlag			= 0,
		TransformFlag			= 1 << (TransformType			- 1),
		RenderableFlag			= 1 << (RenderableType			- 1),
		AnimationFlag			= 1 << (AnimationType			- 1),
		LightSourceFlag			= 1 << (LightSourceType			- 1),
		AudioSourceFlag			= 1 << (AudioSourceType			- 1),
		AudioListenerFlag		= 1 << (AudioListenerType		- 1),
		AudioGeometryFlag		= 1 << (AudioGeometryType		- 1),
		ScriptFlag				= 1 << (ScriptType				- 1),
		CameraFlag				= 1 << (CameraType				- 1),
	};

	extern Map<std::type_index, ComponentType> kComponentTypeMap;
	extern Map<ComponentType, String> kComponentTypeNames;


	// Fill maps
	void initComponentTypes();
}
