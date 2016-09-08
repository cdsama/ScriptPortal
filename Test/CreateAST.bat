@echo off

hp.exe  -c LUA_CLASS -e LUA_ENUM -f LUA_FUNCTION -p LUA_PROPERTY -o ast.json -- Player.h Object.h PlayerManager.h
lab.exe -o Generated.cpp -- ast.json

pause