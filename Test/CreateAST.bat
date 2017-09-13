@echo off

hp.exe  -c LUA_CLASS -e LUA_ENUM -t LUA_CTOR -f LUA_FUNCTION -f LUA_PROPERTY -f LUA_CFUNCTION -p LUA_DATA -p LUA_CALLBACK -n LUA_NAMESPACE -o ast.json -- Player.h Object.h PlayerManager.h
lab.exe -a LUA_CALLBACK -f LUA_PROPERTY -c LUA_CFUNCTION -o Generated.cpp -- ast.json

pause