#include "EventManager.h"
#include <assert.h>
#include <Types.h>
#include <Utils.h>
#include <cstdarg>

using namespace NoxEngine;
using NoxEngineUtils::Logger;

void EventManager::addListener(String eventName, ListenFunc func) {

	if(_event_subs.contains(eventName)) {

		EventEntryIt entry = _event_subs.find(eventName);
		entry->second.push_back(func);

	} else {
		//TODO(sharo): fix this atrocity 
		_event_subs.insert(std::pair(eventName, std::vector<ListenFunc>{func}));
	}

}

void EventManager::signal(String eventName, ...) {

	bool exists = _event_subs.contains(eventName);

	assert(exists);

	if(exists) {
		auto it = _event_subs[eventName];
		for(i32 i = 0; i < it.size(); i++) {
			va_list var_arg;
			va_start(var_arg, eventName);
			it[i](var_arg);
			va_end(var_arg);
		}
	} else {
		LOG_DEBUG("%s event doesn't exist, can't signal.", eventName.c_str());
	}
}

