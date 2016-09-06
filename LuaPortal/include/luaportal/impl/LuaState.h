/**/

/* A wrapper for lua_State */
class LuaState {
private:
    lua_State *m_L;
    
public:
    
    typedef std::function<void(const std::string&)> OnError;
    
    LuaState()
    : m_L(luaS_newstate()) {
    }
    
    ~LuaState() {
        luaS_close(m_L);
    }
    
    lua_State* GetState() const {
        return m_L;
    }
    
    void DoFile(const std::string &path, OnError onError = nullptr) {
        if(luaL_dofile(m_L, path.c_str())) {
            std::string errstr = lua_tostring(m_L, -1);
            lua_pop(m_L, 1);
            if(onError) {
                onError(errstr);
            }
        }        
    }
    
    void DoString(const std::string &content, OnError onError = nullptr) {
        if(luaL_dostring(m_L, content.c_str())) {
            std::string errstr = lua_tostring(m_L, -1);
            lua_pop(m_L, 1);
            if(onError) {
                onError(errstr);
            }
        }
    }
    
    void AddSearcher(lua_CFunction func)
    {
        luaS_addSearcher(m_L, func);
    }
    
    template<typename T>
    void SetGlobal(char const* name, T t)
    {
        Push(m_L, t);
        lua_setglobal(m_L, name);
    }
    
    LuaRef GetGlobal(const char *name) {
        return LuaRef::GetGlobal(m_L, name);
    }
    
    Namespace GlobalContext() {
        return Namespace::GetGlobalNamespace(m_L);
    }
    
    LuaRef NewNil() {
        return LuaRef(m_L);
    }
    
    template<typename T>
    LuaRef NewLuaRef(T v) {
        return LuaRef(m_L, v);
    }
    
private:
    /*
     * Copy is not allowed.
     */
    LuaState(const LuaState &other);
    LuaState& operator=(const LuaState &rhs);
    
};

