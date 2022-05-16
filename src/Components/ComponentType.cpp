#include "Components/ComponentType.h"

// Component classes
#include <Components/IComponent.h>
#include <Components/TransformComponent.h>
#include <Components/RenderableComponent.h>
#include <Components/AnimationComponent.h>
#include <Components/AudioSourceComponent.h>
#include <Components/AudioGeometryComponent.h>
#include <Components/ScriptComponent.h>
#include <Components/CameraComponent.h>
#include <Components/EmissionComponent.h>


namespace NoxEngine {

	// Give the extern maps/lists a definition in one compile unit
	std::map<std::type_index, ComponentType> kComponentTypeMap;
	std::map<ComponentType, String> kComponentTypeNames;
}


void NoxEngine::initComponentTypes() {
	
	kComponentTypeMap = {
		{ typeid(IComponent),				AbstractType },
		{ typeid(TransformComponent),		TransformType },
		{ typeid(RenderableComponent),		RenderableType },
		{ typeid(AnimationComponent),		AnimationType},
		{ typeid(EmissionComponent),		LightSourceType },
		{ typeid(AudioSourceComponent),		AudioSourceType },
		//{ typeid(AudioListenerComponent),	AudioListenerType },
		{ typeid(AudioGeometryComponent),	AudioGeometryType },
		{ typeid(ScriptComponent),			ScriptType },
		{ typeid(CameraComponent),			CameraType }
	};

	kComponentTypeNames = {
		{ AbstractType,			"Abstract Component" },
		{ TransformType,		"Transform Component" },
		{ RenderableType,		"Renderable Component" },
		{ AnimationType,		"Animation Component" },
		{ LightSourceType,		"Light Source Component" },
		{ AudioSourceType,		"Audio Source Component" },
		{ AudioListenerType,	"Audio Listener Component" },	// TODO: Implement
		{ AudioGeometryType,	"Audio Geometry Component" },
		{ ScriptType,			"Script Component" },
		{ CameraType,			"Camera Component" }
	};

	// assert every type has a name
	for (int i = 0; i < ComponentType::ComponentTypeCount; i++) {
		assert(kComponentTypeNames.find(static_cast<ComponentType>(i)) != kComponentTypeNames.end() && "A component type must have a name as a string - define it!");
	}
}
