#pragma once

#include <iostream>
extern "C" {
# include "lua/lua.h"
# include "lua/lauxlib.h"
# include "lua/lualib.h"
}
#include <LuaBridge/LuaBridge.h>

int lua();