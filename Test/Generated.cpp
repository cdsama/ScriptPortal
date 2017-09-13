#include <lua.hpp>
#include <luaportal/luaportal.h>
#include "Object.h"
#include "Player.h"
#include "PlayerManager.h"

void RegisterAPIs(luaportal::LuaState& LOL) 
{
	LOL.GlobalContext()
	.BeginNamespace("ot")
	.BeginEnum<testnamespace::ObjectType>("OT")
	.AddEnumValue("Player", testnamespace::ObjectType::Player)
	.AddEnumValue("Npc", testnamespace::ObjectType::Npc)
	.AddEnumValue("Item", testnamespace::ObjectType::Item)
	.EndEnum()
	.EndNamespace()
	.BeginClass<Object>("Object")
	.Def(luaportal::Constructor<const std::string&, const int, const std::function<void(bool), int>&, const int&&, const int* const, int**, const int**, const int&, const int&>())
	.AddFunction("GetNameFunc", &Object::GetName)
	.AddStaticData("OnObjEnd", &Object::OnEnd, true)
	.EndClass()
	.DeriveClass<Player,Object>("LuaPlayer")
	.AddFunction("GetDiscribe", &Player::GetDiscribe)
	.AddData("ID", &Player::ID, false)
	.AddData("IP", &Player::IP, true)
	.AddProperty("strName", &Player::GetStrName)
	.AddProperty("Name2", &Player::GetStrName2, &Player::SetStrName2)
	.EndClass()
	.BeginClass<PlayerManager>("PlayerManager")
	.AddStaticFunction("GetPlayerManagerVersion", &PlayerManager::GetPlayerManagerVersion)
	.AddCFunction("TestCFunction1", &PlayerManager::TestCFunction1)
	.AddStaticCFunction("TestCFunction2", &PlayerManager::TestCFunction2)
	.AddFunction("GetPlayer", &PlayerManager::GetPlayer)
	.EndClass()
	.AddFunction("GetCurrentTime", &GetCurrentTime)
	.AddFunction("GetCurrentTimeTest", &Test::GetCurrentTimeTest)
	.AddFunction("GetPlayerManager", &PlayerManager::GetPlayerManager)
	.AddCFunction("TestCFunction3", &PlayerManager::TestCFunction3)
	;
}

void UnregisterStaticLuaProperties() 
{
	Object::OnEnd = nullptr;
}
