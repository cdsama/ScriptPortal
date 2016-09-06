// These are for Lua versions prior to 5.2.0.
//
#if LUA_VERSION_NUM< 502
inline int lua_absindex(lua_State* L, int idx)
{
    if(idx > LUA_REGISTRYINDEX && idx< 0)
        return lua_gettop(L) + idx + 1;
    else
        return idx;
}

inline void lua_rawgetp(lua_State* L, int idx, void const* p)
{
    idx = lua_absindex(L, idx);
    lua_pushlightuserdata(L, const_cast<void*>(p));
    lua_rawget(L,idx);
}

inline void lua_rawsetp(lua_State* L, int idx, void const* p)
{
    idx = lua_absindex(L, idx);
    lua_pushlightuserdata(L, const_cast<void*>(p));
    // put key behind value
    lua_insert(L, -2);
    lua_rawset(L, idx);
}

#define LUA_OPEQ 1
#define LUA_OPLT 2
#define LUA_OPLE 3

inline int lua_compare(lua_State* L, int idx1, int idx2, int op)
{
    switch(op)
    {
        case LUA_OPEQ:
            return lua_equal(L, idx1, idx2);
            break;
            
        case LUA_OPLT:
            return lua_lessthan(L, idx1, idx2);
            break;
            
        case LUA_OPLE:
            return lua_equal(L, idx1, idx2) || lua_lessthan(L, idx1, idx2);
            break;
            
        default:
            return 0;
    };
}

inline int get_length(lua_State* L, int idx)
{
    return int(lua_objlen(L, idx));
}

#else
inline int get_length(lua_State* L, int idx)
{
    lua_len(L, idx);
    int len = int(luaL_checknumber(L, -1));
    lua_pop(L, 1);
    return len;
}

#endif

#ifndef LUA_OK
# define LUAPORTAL_LUA_OK 0
#else
# define LUAPORTAL_LUA_OK LUA_OK
#endif

/** Get a table value, bypassing metamethods.
 */  
inline void rawgetfield(lua_State* L, int index, char const* key)
{
    assert(lua_istable(L, index));
    index = lua_absindex(L, index);
    lua_pushstring(L, key);
    lua_rawget(L, index);
}

/** Set a table value, bypassing metamethods.
 */  
inline void rawsetfield(lua_State* L, int index, char const* key)
{
    assert(lua_istable(L, index));
    index = lua_absindex(L, index);
    lua_pushstring(L, key);
    lua_insert(L, -2);
    lua_rawset(L, index);
}

/** Returns true if the value is a full userdata(not light).
 */
inline bool isfulluserdata(lua_State* L, int index)
{
    return lua_isuserdata(L, index) && !lua_islightuserdata(L, index);
}

/** Test lua_State objects for global equality.
 
 This can determine if two different lua_State objects really point
 to the same global state, such as when using coroutines.
 
 @note This is used for assertions.
 */
inline bool equalstates(lua_State* L1, lua_State* L2)
{
    return lua_topointer(L1, LUA_REGISTRYINDEX) ==
    lua_topointer(L2, LUA_REGISTRYINDEX);
}

inline lua_State* luaS_newstate() {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    return L;
}


inline void luaS_close(lua_State* L) {
    lua_close(L);
}

/*
 * Push t[key] onto the stack, where t is the value at the top of the stack.
 * Bypassing metamethods.
 */
inline void luaS_rawget(lua_State* L, char const* key) {
    lua_pushstring(L, key);
    lua_rawget(L, -2);
}


/*
 * t[key] = v, where v is the value at the top of the stack
 * and t is the value just below the top.
 * This function will pop v from the stack.
 * Bypassing metamethods.
 */
inline void luaS_rawset(lua_State* L, char const* key) {
    lua_pushstring(L, key);
    lua_insert(L, -2);
    lua_rawset(L, -3);
}


/*
 * Push t[key] onto the stack, where t is the value at the given index.
 * Bypassing metamethods.
 */
inline void luaS_rawgeti(lua_State* L, int index, char const* key) {
    index = lua_absindex(L, index);
    lua_pushstring(L, key);
    lua_rawget(L, index);
}


/*
 * t[key] = v, where v is the value at the top of the stack
 * and t is the table at the given index.
 * This function will pop v from the stack.
 * Bypassing metamethods.
 */
inline void luaS_rawseti(lua_State* L, int index, char const* key) {
    index = lua_absindex(L, index);
    lua_pushstring(L, key);
    lua_insert(L, -2);
    lua_rawset(L, index);
}

//------------------------------------------------------------------------------
/**
 Push an object onto the Lua stack.
 */
template<typename T>
inline void Push(lua_State* L, T t)
{
    Stack<T>::Push(L, t);
}

//------------------------------------------------------------------------------
/**
 Set a global value in the lua_State.
 
 @note This works on any type specialized by `Stack`, including `LuaRef` and
 its table proxies.
 */
template<typename T>
inline void SetGlobal(lua_State* L, T t, char const* name)
{
    Push(L, t);
    lua_setglobal(L, name);
}

inline void luaS_addSearcher(lua_State* L, lua_CFunction func)
{
    if(!func) return;
    
    // stack content after the invoking of the function
    // Get loader table
    lua_getglobal(L, "package");                                  /* L: package */
    lua_getfield(L, -1, "searchers");                               /* L: package, loaders */
    
    // insert loader into index 2
    lua_pushcfunction(L, func);                                   /* L: package, loaders, func */
    for(int i =(int)(lua_rawlen(L, -2) + 1); i > 2; --i)
    {
        lua_rawgeti(L, -2, i - 1);                                /* L: package, loaders, func, function */
        // we call lua_rawgeti, so the loader table now is at -3
        lua_rawseti(L, -3, i);                                    /* L: package, loaders, func */
    }
    lua_rawseti(L, -2, 2);                                        /* L: package, loaders */
    
    // set loaders into package
    lua_setfield(L, -2, "searchers");                               /* L: package */
    
    lua_pop(L, 1);
}
