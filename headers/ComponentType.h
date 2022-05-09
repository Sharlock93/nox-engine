#pragma once

#include <map>
#include <typeindex>

#include <Types.h>


namespace NoxEngine {

	// Note (Vincent): This implies the developer cannot create new component types
	enum ComponentType : i32 {
		AbstractType = 0,
		PositionType,		// TODO: change to Transform (pos, rot, scale)
		RenderableType,
		LightSourceType,
		AudioSourceType,
		AudioListenerType,
		AudioGeometryType,
		//CollisionType,	// If we want to implement physics, collision can be implement similarlly to audio obstruction
		ScriptType,
		ComponentTypeCount
	};

	enum ComponentTypeFlag : HasCompBitMask {
		AbstractFlag			= 0,
		PositionFlag			= 1 << (PositionType			- 1),
		RenderableFlag			= 1 << (RenderableType			- 1),
		LightSourceFlag			= 1 << (LightSourceType			- 1),
		AudioSourceFlag			= 1 << (AudioSourceType			- 1),
		AudioListenerFlag		= 1 << (AudioListenerType		- 1),
		AudioGeometryFlag		= 1 << (AudioGeometryType		- 1),
		ScriptFlag				= 1 << (ScriptType				- 1),
	};

	extern std::map<std::type_index, ComponentType> kComponentTypeMap;
	extern std::map<ComponentType, String> kComponentTypeNames;


	// Fill maps
	void initComponentTypes();
}