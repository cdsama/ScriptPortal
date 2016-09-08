#include <lua.hpp>
#include <luaportal/luaportal.h>
#include "Object.h"
#include "Player.h"
#include "PlayerManager.h"

void RegistAPIs(luaportal::LuaState& l) 
{
	l.GlobalContext()
	.BeginEnum<ObjectType>("ObjectType")
	.AddEnumValue("Player", ObjectType::Player)
	.AddEnumValue("Npc", ObjectType::Npc)
	.AddEnumValue("Item", ObjectType::Item)
	.EndEnum()
	.BeginClass<Object>("Object")
	.AddFunction("GetName", &Object::GetName)
	.EndClass()
	.DeriveClass<Player,Object>("Player")
	.AddFunction("GetDiscribe", &Player::GetDiscribe)
	.AddData("ID", &Player::ID, false)
	.AddData("IP", &Player::IP, true)
	.EndClass()
	.BeginClass<PlayerManager>("PlayerManager")
	.AddStaticFunction("GetPlayerManagerVersion", &PlayerManager::GetPlayerManagerVersion)
	.AddFunction("GetPlayer", &PlayerManager::GetPlayer)
	.EndClass()
	.AddFunction("GetCurrentTime", &GetCurrentTime)
	.AddFunction("GetCurrentTimeTest", &Test::GetCurrentTimeTest)
	.AddFunction("GetPlayerManager", &PlayerManager::GetPlayerManager)
	;
}
