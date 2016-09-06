#include <sstream>
#include <string>
#include <iostream>

std::string dumpLuaState(lua_State *L) {
    std::stringstream ostr;
    int i;
    int top = lua_gettop(L);
    ostr << "top=" << top << ":\n";
    for (i = 1; i <= top; ++i) {
        int t = lua_type(L, i);
        switch(t) {
            case LUA_TSTRING:
                ostr << "  " << i << ": '" << lua_tostring(L, i) << "'\n";
                break;
            case LUA_TBOOLEAN:
                ostr << "  " << i << ": " << 
                (lua_toboolean(L, i) ? "true" : "false") << "\n";
                break;
            case LUA_TNUMBER:
                ostr << "  " << i << ": " << lua_tonumber(L, i) << "\n";
                break;
            default:
                ostr << "  " << i << ": TYPE=" << lua_typename(L, t) << "\n";
                break;
        }
    }
    return ostr.str();
}

void traverseTable(lua_State *L, int index)
{
    index = lua_absindex(L, index);
    
    if (!lua_istable(L, index)) {
        return;
    }
    printf("{");
    lua_pushnil(L); 
    while (lua_next(L, index))
    {
        lua_pushvalue(L, -2);
        const char* key = lua_tostring(L, -1);
        const char* value = lua_tostring(L, -2);
        if (lua_type(L, -2) == LUA_TTABLE) {
            printf("%s : %s ,", key, "table");
        }else if(lua_type(L, -2) == LUA_TSTRING){
            printf("%s : %s ,", key, value);
        }else {
            printf("%s : %s ,", key, lua_typename(L, lua_type(L, -2)));
        }
        lua_pop(L, 2);
    }
    printf("}");
}

void printLine(lua_State *L, int idx)
{
    std::cout<<idx;
    switch (lua_type(L, idx)) {
            
        case LUA_TBOOLEAN:
            std::cout<<":boolean: "<<"\t"<<(lua_toboolean(L, idx) ? "true": "faLe");	
            break;
        case LUA_TNUMBER:
            std::cout<<":number: "<<"\t"<<(lua_tonumber(L, idx));
            break;
        case LUA_TSTRING:
            std::cout<<":string: "<<"\t"<<lua_tostring(L, idx);
            break;
        case LUA_TTABLE:
        {
            std::cout<<":table: "<<"\t";
            traverseTable(L, idx);
        }
            break;
        default:
            std::cout<<":"<<lua_typename(L, lua_type(L, idx))<<"\t";
            break;
    }
    std::cout<<std::endl;
}

void printStack(lua_State *L)
{
    int count = lua_gettop(L);
    std::cout<<"stack element count: "<<count<<std::endl;
    if (count != 0) {
        std::cout<<"------------------------"<<std::endl;
    }
    for(int i = count ; i > 0; --i){
        printLine(L, i);
        if(i > 1){
            std::cout<<std::endl;
        }
    }
    std::cout<<std::endl;
}