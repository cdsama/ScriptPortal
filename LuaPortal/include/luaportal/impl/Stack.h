//------------------------------------------------------------------------------
/**
 Receive the lua_State* as an argument.
 */
template<>
struct Stack<lua_State*>
{
    static lua_State* Get(lua_State* L, int)
    {
        return L;
    }
};

//------------------------------------------------------------------------------
/**
 Push a lua_CFunction.
 */
template<>
struct Stack<lua_CFunction>
{
    static void Push(lua_State* L, lua_CFunction f)
    {
        lua_pushcfunction(L, f);
    }
    
    static lua_CFunction Get(lua_State* L, int index)
    {
        return lua_tocfunction(L, index);
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `int`.
 */
template<>
struct Stack<int>
{
    static inline void Push(lua_State* L, int value)
    {
        lua_pushinteger(L, static_cast<lua_Integer>(value));
    }
    
    static inline int Get(lua_State* L, int index)
    {
        return static_cast<int>(luaL_checkinteger(L, index));
    }
};

template<>
struct Stack<int const&>
{
    static inline void Push(lua_State* L, int value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }
    
    static inline int Get(lua_State* L, int index)
    {
        return static_cast<int >(luaL_checknumber(L, index));
    }
};
//------------------------------------------------------------------------------
/**
 Stack specialization for `unsigned int`.
 */
template<>
struct Stack<unsigned int>
{
    static inline void Push(lua_State* L, unsigned int value)
    {
        lua_pushinteger(L, static_cast<lua_Integer>(value));
    }
    
    static inline unsigned int Get(lua_State* L, int index)
    {
        return static_cast<unsigned int>(luaL_checkinteger(L, index));
    }
};

template<>
struct Stack<unsigned int const&>
{
    static inline void Push(lua_State* L, unsigned int value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }
    
    static inline unsigned int Get(lua_State* L, int index)
    {
        return static_cast<unsigned int >(luaL_checknumber(L, index));
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `unsigned char`.
 */
template<>
struct Stack<unsigned char>
{
    static inline void Push(lua_State* L, unsigned char value)
    {
        lua_pushinteger(L, static_cast<lua_Integer>(value));
    }
    
    static inline unsigned char Get(lua_State* L, int index)
    {
        return static_cast<unsigned char>(luaL_checkinteger(L, index));
    }
};

template<>
struct Stack<unsigned char const&>
{
    static inline void Push(lua_State* L, unsigned char value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }
    
    static inline unsigned char Get(lua_State* L, int index)
    {
        return static_cast<unsigned char>(luaL_checknumber(L, index));
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `short`.
 */
template<>
struct Stack<short>
{
    static inline void Push(lua_State* L, short value)
    {
        lua_pushinteger(L, static_cast<lua_Integer>(value));
    }
    
    static inline short Get(lua_State* L, int index)
    {
        return static_cast<short>(luaL_checkinteger(L, index));
    }
};

template<>
struct Stack<short const&>
{
    static inline void Push(lua_State* L, short value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }
    
    static inline short Get(lua_State* L, int index)
    {
        return static_cast<short>(luaL_checknumber(L, index));
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `unsigned short`.
 */
template<>
struct Stack<unsigned short>
{
    static inline void Push(lua_State* L, unsigned short value)
    {
        lua_pushinteger(L, static_cast<lua_Integer>(value));
    }
    
    static inline unsigned short Get(lua_State* L, int index)
    {
        return static_cast<unsigned short>(luaL_checkinteger(L, index));
    }
};

template<>
struct Stack<unsigned short const&>
{
    static inline void Push(lua_State* L, unsigned short value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }
    
    static inline unsigned short Get(lua_State* L, int index)
    {
        return static_cast<unsigned short>(luaL_checknumber(L, index));
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `long`.
 */
template<>
struct Stack<long>
{
    static inline void Push(lua_State* L, long value)
    {
        lua_pushinteger(L, static_cast<lua_Integer>(value));
    }
    
    static inline long Get(lua_State* L, int index)
    {
        return static_cast<long>(luaL_checkinteger(L, index));
    }
};

template<>
struct Stack<long const&>
{
    static inline void Push(lua_State* L, long value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }
    
    static inline long Get(lua_State* L, int index)
    {
        return static_cast<long>(luaL_checknumber(L, index));
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `unsigned long`.
 */
template<>
struct Stack<unsigned long>
{
    static inline void Push(lua_State* L, unsigned long value)
    {
        lua_pushinteger(L, static_cast<lua_Integer>(value));
    }
    
    static inline unsigned long Get(lua_State* L, int index)
    {
        return static_cast<unsigned long>(luaL_checkinteger(L, index));
    }
};

template<>
struct Stack<unsigned long const&>
{
    static inline void Push(lua_State* L, unsigned long value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }
    
    static inline unsigned long Get(lua_State* L, int index)
    {
        return static_cast<unsigned long>(luaL_checknumber(L, index));
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `float`.
 */
template<>
struct Stack<float>
{
    static inline void Push(lua_State* L, float value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }
    
    static inline float Get(lua_State* L, int index)
    {
        return static_cast<float>(luaL_checknumber(L, index));
    }
};

template<>
struct Stack<float const&>
{
    static inline void Push(lua_State* L, float value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }
    
    static inline float Get(lua_State* L, int index)
    {
        return static_cast<float>(luaL_checknumber(L, index));
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `double`.
 */
template<> struct Stack<double>
{
    static inline void Push(lua_State* L, double value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }
    
    static inline double Get(lua_State* L, int index)
    {
        return static_cast<double>(luaL_checknumber(L, index));
    }
};

template<> struct Stack<double const&>
{
    static inline void Push(lua_State* L, double value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }
    
    static inline double Get(lua_State* L, int index)
    {
        return static_cast<double>(luaL_checknumber(L, index));
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `bool`.
 */
template<>
struct Stack<bool> {
    static inline void Push(lua_State* L, bool value)
    {
        lua_pushboolean(L, value ? 1 : 0);
    }
    
    static inline bool Get(lua_State* L, int index)
    {
        return lua_toboolean(L, index) ? true : false;
    }
};

template<>
struct Stack<bool const&> {
    static inline void Push(lua_State* L, bool value)
    {
        lua_pushboolean(L, value ? 1 : 0);
    }
    
    static inline bool Get(lua_State* L, int index)
    {
        return lua_toboolean(L, index) ? true : false;
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `char`.
 */
template<>
struct Stack<char>
{
    static inline void Push(lua_State* L, char value)
    {
        char str [2] = { value, 0 };
        lua_pushstring(L, str);
    }
    
    static inline char Get(lua_State* L, int index)
    {
        return luaL_checkstring(L, index) [0];
    }
};

template<>
struct Stack<char const&>
{
    static inline void Push(lua_State* L, char value)
    {
        char str [2] = { value, 0 };
        lua_pushstring(L, str);
    }
    
    static inline char Get(lua_State* L, int index)
    {
        return luaL_checkstring(L, index) [0];
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `float`.
 */
template<>
struct Stack<char const*>
{
    static inline void Push(lua_State* L, char const* str)
    {
        if(str != 0)
            lua_pushstring(L, str);
        else
            lua_pushnil(L);
    }
    
    static inline char const* Get(lua_State* L, int index)
    {
        return lua_isnil(L, index) ? 0 : luaL_checkstring(L, index);
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `std::string`.
 */
template<>
struct Stack<std::string>
{
    static inline void Push(lua_State* L, std::string const& str)
    {
        lua_pushlstring(L, str.c_str(), str.size());
    }
    
    static inline std::string Get(lua_State* L, int index)
    {
        size_t len;
        const char *str = luaL_checklstring(L, index, &len);
        return std::string(str, len);
    }
};

//------------------------------------------------------------------------------
/**
 Stack specialization for `std::string const&`.
 */
template<>
struct Stack<std::string const&>
{
    static inline void Push(lua_State* L, std::string const& str)
    {
        lua_pushlstring(L, str.c_str(), str.size());
    }
    
    static inline std::string Get(lua_State* L, int index)
    {
        size_t len;
        const char *str = luaL_checklstring(L, index, &len);
        return std::string(str, len);
    }
};

static int pushArgs(lua_State *L)
{
    return 0;
}
template<typename H,typename... P>
int pushArgs(lua_State *L, H h, P... p) {
    Stack<H>::Push(L, h);
    return 1 + pushArgs(L, p...);
}

class FunctionTransfer {
    lua_State *state;
    int ref;
public:
    FunctionTransfer(lua_State *L, int index)
    {
        state = L;
        ref = 0;
        if(state) {
            lua_pushvalue(state, index);
            ref = luaL_ref(state, LUA_REGISTRYINDEX);
        }
    }
    
    template<typename R , typename... P>
    static void create(lua_State *L, int index, std::function<R(P...)> &func)
    {
        auto auf = std::make_shared<FunctionTransfer>(L, index);
        func = [auf](P... p)->R{
            lua_State *L = auf->getState();
            lua_rawgeti(L, LUA_REGISTRYINDEX, auf->getRef());
            int nargs = pushArgs(L, p...);
            lua_pcall(L, nargs, 1, 0);
            return Stack<R>::Get(L, -1);
        };
    }
    
    template<typename R = void, typename... P>
    static void create(lua_State *L, int index, std::function<void(P...)> &func)
    {
        auto auf = std::make_shared<FunctionTransfer>(L, index);
        func = [auf](P... p)->void{
            lua_State *L = auf->getState();
            lua_rawgeti(L, LUA_REGISTRYINDEX, auf->getRef());
            int nargs = pushArgs(L, p...);
            lua_pcall(L, nargs, 0, 0);
        };
    }
    
    template<typename R, typename... P>
    static void create(lua_State *L, int index, std::function<void(void)> &func)
    {
        auto auf = std::make_shared<FunctionTransfer>(L, index);
        func = [auf]()->void{
            lua_State *L = auf->getState();
            lua_rawgeti(L, LUA_REGISTRYINDEX, auf->getRef());
            lua_pcall(L, 0, 0, 0);
        };
    }
    
    template<typename FT>
    static void Get(lua_State *L, int index, std::function<FT> &func)
    {
        if(lua_isfunction(L, index))
        {
            create(L, index, func);
        } 
        else if(lua_isuserdata(L, index))
        {
            func =(decltype(func)(*luaL_checkudata(L, index, typeid(func).name())));
        } else if(lua_isnil(L, index)) {
            func = nullptr;
        } else {
            luaL_checktype(L, index, LUA_TFUNCTION);
        }
    }
    
    int getRef(){return ref;}
    lua_State* getState(){return state;}
    
    ~FunctionTransfer()
    {
        if(state) {
            luaL_unref(state, LUA_REGISTRYINDEX, ref);
        }
    }
};


template<typename FT>
struct Stack<std::function<FT> >
{
    static inline void Push(lua_State* L, std::function<FT> func)
    {
        if(func) {
            new(lua_newuserdata(L, sizeof(func))) std::function<FT>(func);
            luaL_newmetatable(L, typeid(func).name());
            lua_setmetatable(L, -2);            
        } else {
            lua_pushnil(L);
        }
    }
    
    static inline std::function<FT> Get(lua_State* L, int index)
    {
        std::function<FT> func;
        FunctionTransfer::Get(L, index, func);
        return func;
    }
};
