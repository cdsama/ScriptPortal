@echo off

hp.exe  -c LUA_CLASS -e LUA_ENUM -f LUA_FUNCTION -f LUA_PROPERTY -p LUA_DATA -p LUA_CALLBACK -n LUA_NAMESPACE -o ast.json -- Player.h Object.h PlayerManager.h
lab.exe -a LUA_CALLBACK -f LUA_PROPERTY  -o Generated.cpp -- ast.json

pause