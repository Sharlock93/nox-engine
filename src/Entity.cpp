// System/std includes
#include <iostream>
#include <cassert>

#include <Entity.h>
#include <GameState.h>
#include <ComponentType.h>
#include <IComponent.h>
#include <PositionComponent.h>
#include <RenderableComponent.h>

using namespace NoxEngine;
using NoxEngineUtils::Logger;


Entity::Entity(i32 _id, char* _name)
	: 
	id(_id), 
	hasComp(0) {

	if (_name != nullptr) name = _name;
	else name = (char *)calloc(ENTITY_NAME_MAX_LEN, sizeof(char));
}

Entity::Entity(Scene* scene, char* _name)
	:
	hasComp(0) {

	assert(scene != nullptr);

	// Assign values to fields
	id = scene->nEntitiesAdded; 
	
	if (_name != nullptr) name = _name;
	else {
		// placeholder name
		name = (char*)calloc(ENTITY_NAME_MAX_LEN, sizeof(char));
		snprintf(name, ENTITY_NAME_MAX_LEN, "Game Object %i", scene->nEntitiesAdded + 1);
	}
}

Entity::Entity(Scene* scene, const char* _name)
	:
	hasComp(0) {

	assert(scene != nullptr);

	// Assign values to fields
	id = scene->nEntitiesAdded;

	name = (char*)calloc(ENTITY_NAME_MAX_LEN, sizeof(char));
	memcpy(name, _name, ENTITY_NAME_MAX_LEN);
}

Entity::Entity(Entity&& other) : id(other.id), hasComp(other.hasComp), components(other.components), name(other.name) {
}

Entity::~Entity() {

	free(name);

	// Gotta be careful. When comp are destroyed the subsystem have to know
}


bool Entity::containsComps(HasCompBitMask mask) {
	return (hasComp & mask) == mask;
}


template <typename T>
void Entity::addComp(T *comp) {

	assert(comp->id != ComponentType::AbstractType);

	// Invalid type, don't add
	if (kComponentTypeMap.find(typeid(T)) == kComponentTypeMap.end()) {
		Logger::debug("Attempted to add invalid component type (%s), aborted", typeid(T).name());
		return;
	}

	if (components.find(typeid(T)) != components.end()) {
		Logger::debug("Component (ID: %d) already exists in Entity ", comp->id);
		return;
	}

	components[typeid(T)] = comp;
	hasComp |= ( 1 << (comp->id-1) );
}


template <typename T>
T *Entity::getComp() {

	// Invalid type, return nullptr
	if (kComponentTypeMap.find(typeid(T)) == kComponentTypeMap.end()) {
		Logger::debug("Invalid component type requested (typeid = %s)", typeid(T).name());
		return nullptr;
	}

	// Found component. Cast it to type T
	if (auto res = components.find(typeid(T)); res != components.end()) {
		return res->second->CastType<T>();
	}

	// Component does not exist in this entity
	return nullptr;
}


// explicit template instantiation
template void Entity::addComp<PositionComponent>(PositionComponent*);
template void Entity::addComp<RenderableComponent>(RenderableComponent*);

template PositionComponent* Entity::getComp<PositionComponent>();
template RenderableComponent* Entity::getComp<RenderableComponent>();
