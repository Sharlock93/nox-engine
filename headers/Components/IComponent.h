/*
 * An interface: communication with Entity.
 * Allows a class that implements this interface to be used as a component of Entity. 
 * Typically, a class implementing this would also implement an interface comunicating with a subsystem of engine
 * 
 * Each component has an ID allowing to distinguish between them
*/
#pragma once

#include <Components/ComponentType.h>
#include <Core/Entity.h>
#include <Core/Types.h>

namespace NoxEngine {

	// forward declares
	enum ComponentType : i32;


	class IComponent
	{
		public:
			//ComponentType id;		// Need to rely on every component class to have a static id

			Entity* parent;
			// This function is implemented to be able to downcast classes stored as IComponent to their respective actual classes
			// TODO (Vincent): fix this witchcraft
			template <class T> inline T* CastType() { 
				void* ptr = dynamic_cast<T*>(this);
				return reinterpret_cast<T*>(ptr); 
			};
			// What's the representation of this component in the inspector?
			// This function being virtual also allows dynamic_cast to be used in CastType()
			virtual void displayUI() {};
			virtual inline ComponentType id() { return ComponentType::AbstractType; };
			//void set_id(ComponentType value) { id = value; };

			virtual Entity* getParentEntity() { return parent; };
			virtual void attachedToEntity(Entity* ent) { parent = ent; };
	};
}

