// System/std includes
#include <iostream>
#include <cassert>

#include <Entity.h>
#include <GameState.h>
#include <IComponent.h>
#include <ComponentType.h>

using namespace NoxEngine;
using NoxEngineUtils::Logger;


Entity::Entity(i32 _id) 
	: 
	id(_id), 
	components(0), 
	hasComp(0), 
	name("") {

}

Entity::Entity(i32 _id, String _name) 
	: 
	id(_id), 
	components(0), 
	hasComp(0), 
	name(_name) {

}

Entity::Entity(Scene* scene, String _name)
	:
	components(0),
	hasComp(0) {

	assert(scene != nullptr);

	// Form the placeholder name
	char entName[32];
	snprintf(entName, 32, "Game Object %i", scene->nEntitiesAdded + 1);

	// Assign values to fields
	id = scene->nEntitiesAdded;
	name = _name == "" ? entName : _name;

}

Entity::Entity(Entity&& other) : id(other.id), components(other.components), hasComp(other.hasComp), name(other.name) {

}

Entity::~Entity() {
	// Gotta be careful. When comp are destroyed the subsystem have to know
}

void Entity::addComp(IComponent *comp)
{
	assert(comp->id != ComponentType::AbstractType);

	for (auto c : components) {

		if (c->id == comp->id) {
			Logger::debug("Component (ID: %d) already exists in Entity ", comp->id);
			return;
		}
	}

	components.push_back(comp);
	hasComp |= ( 1 << (comp->id-1) );
}


IComponent * Entity::getComp(ComponentType compId)
{
	// TODO (vincent): optimize
	for (auto c : components)
	{
		if (c->id == compId)
		{
			return c;
		}
	}

	//Logger::debug("The component (ID: %d) does not exist in Entity (ID: %d)", compId, id);
	return nullptr;
}

