#include "ComponentType.h"

// Component classes
#include <IComponent.h>
#include <PositionComponent.h>
#include <RenderableComponent.h>


namespace NoxEngine {

	// Give the extern maps/lists a definition in one compile unit
	std::map<std::type_index, ComponentType> kComponentTypeMap;
	std::map<ComponentType, String> kComponentTypeNames;
}


void NoxEngine::initComponentTypes() {
	
	kComponentTypeMap = {
		{ typeid(IComponent),			AbstractType },
		{ typeid(PositionComponent),	PositionType },
		{ typeid(RenderableComponent),	RenderableType }
		//{ typeid(LightSourceComponent),	LightSourceType },
		//{ typeid(AudioSourceComponent),	AudioSourceType },
		//{ typeid(ScriptComponent),	ScriptType },
	};

	kComponentTypeNames = {
		{ AbstractType,		"Abstract Component" },
		{ PositionType,		"Position Component" },
		{ RenderableType,	"Renderable Component" },
		{ LightSourceType,	"Light Source Component" },
		{ AudioSourceType,	"Audio Source Component" },
		{ ScriptType,		"Script Component" }
	};
}