#ifndef WINDOW_TYPES_H
#define WINDOW_TYPES_H

#include <string>
#include <map>

/* SQLite related new types
*/

typedef std::basic_string <unsigned char> ustring;

/* Window state: main, select, popup, etc...
*/

struct WindowState {
	std::string state = "main";

	bool operator ==(const char *st){
		if(state == std::string(st)){
			return true;
		} else {
			return false;
		}
	}

	bool operator ==(std::string st){
		if(state == st){
			return true;
		} else {
			return false;
		}
	}

	struct WindowState& operator=(const char *st){
		state = std::string(st);
		return *this;
	}

	struct WindowState& operator=(std::string st){
		state = st;
		return *this;
	}
};

struct RenderRegistry {
	std::map<std::string, bool> registry;
	
	void add(std::string window){
		registry[window] = false;
	}
	
	void show(std::string window){
		registry[window] = true;
	}
	
	void hide(std::string window){
		registry[window] = false;
	}

	bool get(std::string window){
		return registry[window];
	}
};

#endif