/**/


struct Nil {};

/*
 * Always use LuaRef as a local variable.
 * NEVER use LuaRef as a member/global/static variable.
 */
class LuaRef {
    
    class Proxy;
    friend class Proxy;
    friend class Iterator;
    friend struct Stack<LuaRef> ;
    friend struct Stack<Proxy> ;
    friend std::ostream& operator<<(std::ostream&, LuaRef::Proxy const&);
    
private:
    
    /*
     * A proxy for assigning table element(operator [] of LuaRef).
     * NEVER use Proxy alone.
     */
    class Proxy {
        
        friend class LuaRef;
        friend struct Stack< Proxy > ;
        
    private:
        /*
         * The key is on the top of stack,
         * and the table is just below the top.
         * Stack:
         *   [TOP] key
         *   [ 2 ] table
         * The key and the table is popped off the stack
         */
        Proxy(lua_State* L)
        : m_L(L) {
            m_keyRef = luaL_ref(L, LUA_REGISTRYINDEX);
            m_tableRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        
        
        /*
         * This function may trigger a metamethod for the "index" event.
         */
        int CreateRef() const {
            Push();
            return luaL_ref(m_L, LUA_REGISTRYINDEX);
        }
        
        /*
         * Push the value onto the Lua stack.
         * May invoke metamethods.
         */
        void Push() const {
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_tableRef);
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_keyRef);
            lua_gettable(m_L, -2); // This may trigger the "index" event.
            lua_remove(m_L, -2);
        }
        
    private:
        lua_State* m_L;
        int m_tableRef;
        int m_keyRef;
        
    public:
        ~Proxy() {
            luaL_unref(m_L, LUA_REGISTRYINDEX, m_tableRef);
            luaL_unref(m_L, LUA_REGISTRYINDEX, m_keyRef);
        }
        
        /*
         * Assign a value to this table key.
         * May invoke metamethods.
         */
        template<typename T>
        Proxy& operator=(T v) {
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_tableRef);
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_keyRef);
            Stack<T>::Push(m_L, v);
            lua_settable(m_L, -3); // This may trgger the "newindex" event
            lua_pop(m_L, 1);
            return *this;
        }
        
        Proxy& operator=(Proxy const& other) {
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_tableRef);
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_keyRef);
            other.Push();
            lua_settable(m_L, -3); // This may trgger the "newindex" event
            lua_pop(m_L, 1);
            return *this;
        }
        
        /*
         * Assign a value to this table key.
         * Will not invoke metamethods.
         */
        template<typename T>
        Proxy& rawSet(T v) {
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_tableRef);
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_keyRef);
            Stack<T>::Push(m_L, v);
            lua_rawset(m_L, -3); // This may trgger the "newindex" event
            lua_pop(m_L, 1);
            return *this;
        }
        
        Proxy& rawSet(Proxy const& other) {
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_tableRef);
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_keyRef);
            other.Push();
            lua_rawset(m_L, -3); // This may trgger the "newindex" event
            lua_pop(m_L, 1);
            return *this;
        }
        
        lua_State* GetState() const { return m_L; }
        
        int GetType() const {
            Push();
            int type = lua_type(m_L, -1);
            lua_pop(m_L, 1);
            return type;
        }
        
        const char * typeName(){
            return lua_typename(m_L, GetType());
        }
        
        bool IsNil() const { return GetType() == LUA_TNIL; }
        bool IsNumber() const { return GetType() == LUA_TNUMBER; }
        bool IsString() const { return GetType() == LUA_TSTRING; }
        bool IsTable() const { return GetType() == LUA_TTABLE; }
        bool IsFunction() const { return GetType() == LUA_TFUNCTION; }
        bool IsUserdata() const { return GetType() == LUA_TUSERDATA; }
        bool IsThread() const { return GetType() == LUA_TTHREAD; }
        bool IsLightUserdata() const { return GetType() == LUA_TLIGHTUSERDATA; }
        
        /*
         * Explicit conversion.
         * May invoke metamethods.
         */
        template<typename T>
        T Cast() const {
            Push();
            T t = Stack<T>::Get(m_L, lua_gettop(m_L));
            lua_pop(m_L, 1);
            return t;
        }
        
        /*
         * Implicit conversion
         * May invoke metamethods.
         */
        template<typename T>
        operator T() const {
            return Cast<T>();
        }
        
        /*
         * Access a table value using a key.
         * May invoke metamethods.
         */
        template<typename T>
        Proxy operator[](T key) const {
            Push();
            Stack<T>::Push(m_L, key);
            return Proxy(m_L);
        }
        
        LuaRef operator()() const {
            Push();
            lua_pcall(m_L, 0, 1, 0);
            return popLuaRef(m_L);
        }
        
        template<typename... Args>
        LuaRef operator()(Args... args) const {
            Push();
            int nargs = pushArgs(m_L, args...);
            lua_pcall(m_L, nargs, 1, 0);
            return popLuaRef(m_L);
        }
        
        int length() const {
            Push(); // push 1
            lua_len(m_L, -1); // push 1
            int len =(int)luaL_checkinteger(m_L, -1);
            lua_pop(m_L, 2); // pop 2
            return len;
        }
        
        std::string tostring() const {
            lua_getglobal(m_L, "tostring");
            Push();
            lua_call(m_L, 1, 1);
            std::string str = lua_tostring(m_L, 1);
            lua_pop(m_L, 1);
            return str;
        }
        
    private:
        /*
         * Copy is NOT allowed.
         */
        Proxy(const Proxy& other);
        
    };
    
private:
    lua_State* m_L;
    int m_ref;
    
    template<typename T>
    static int pushArgs(lua_State* L, T t) {
        Stack<T>::Push(L, t);
        return 1;
    }
    
    template<typename Head, typename... Args>
    static int pushArgs(lua_State* L, Head h, Args... args) {
        Stack<Head>::Push(L, h);
        int nargs = 1 + pushArgs(L, args...);
        return nargs;
    }
    
    static LuaRef popLuaRef(lua_State* L) {
        LuaRef v(L);
        v.Pop();
        return v;
    }
    
    int CreateRef() const {
        if(m_ref != LUA_REFNIL) {
            Push();
            return luaL_ref(m_L, LUA_REGISTRYINDEX);
        }
        else {
            return LUA_REFNIL;
        }
    }
    
    /*
     * Push the object onto the Lua stack
     */
    void Push() const {
        lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_ref);
    }
    
    /*
     * Pop the top of Lua stack and assign the ref to this->m_ref
     */
    void Pop() {
        luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
        m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
    }
    
public:
    explicit LuaRef(lua_State* L)
    : m_L(L),
    m_ref(LUA_REFNIL) {
    }
    
    template<typename T>
    LuaRef(lua_State* L, T v)
    : m_L(L) {
        Stack<T>::Push(m_L, v);
        m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
    }
    
    LuaRef(LuaRef const& other)
    : m_L(other.m_L),
    m_ref(other.CreateRef()) {
    }
    
    /*
     * Create a LuaRef from Proxy.
     * May invoke metamethods.
     */
    LuaRef(Proxy const& proxy)
    : m_L(proxy.GetState()),
    m_ref(proxy.CreateRef()) {
    }
    
    ~LuaRef() {
        luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
    }
    
    template<typename T>
    LuaRef& operator=(T rhs) {
        luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
        Stack<T>::Push(m_L, rhs);
        m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
        return *this;
    }
    
    LuaRef& operator=(LuaRef const& rhs) {
        luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
        m_L = rhs.GetState();
        rhs.Push();
        m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
        return *this;
    }
    
    /*
     * May invoke metamethod.
     */
    LuaRef& operator=(Proxy const& rhs) {
        luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
        m_L = rhs.GetState();
        m_ref = rhs.CreateRef();
        return *this;
    }
    
    std::string tostring() const {
        lua_getglobal(m_L, "tostring");
        Push();
        lua_call(m_L, 1, 1);
        std::string str = lua_tostring(m_L, 1);
        lua_pop(m_L, 1);
        return str;
    }
    
    lua_State* GetState() const {
        return m_L;
    }
    
    /*
     * Return lua_type
     */
    int GetType() const {
        int type;
        if(m_ref != LUA_REFNIL) {
            Push();
            type = lua_type(m_L, -1);
            lua_pop(m_L, 1);
        }
        else {
            type = LUA_TNIL;
        }
        return type;
    }
    
    const char * typeName() {
        return lua_typename(m_L, GetType());
    }
    
    bool IsNil() const { return GetType() == LUA_TNIL; }
    bool IsNumber() const { return GetType() == LUA_TNUMBER; }
    bool IsString() const { return GetType() == LUA_TSTRING; }
    bool IsTable() const { return GetType() == LUA_TTABLE; }
    bool IsFunction() const { return GetType() == LUA_TFUNCTION; }
    bool IsUserdata() const { return GetType() == LUA_TUSERDATA; }
    bool IsThread() const { return GetType() == LUA_TTHREAD; }
    bool IsLightUserdata() const { return GetType() == LUA_TLIGHTUSERDATA; }
    
    /*
     * Explicit conversion
     */
    template<typename T>
    T Cast() const {
        Push();
        T t = Stack<T>::Get(m_L, lua_gettop(m_L));
        lua_pop(m_L, 1);
        return t;
    }
    
    template<typename T>
    operator T() const {
        return Cast<T>();
    }
    
    int length() const {
        Push(); // push 1
        lua_len(m_L, -1); // push 1
        int len =(int)luaL_checkinteger(m_L, -1);
        lua_pop(m_L, 2); // pop 2
        return len;
    }
    
    /*
     * [] for access to element table[key].
     * May invoke metamethods.
     * NOT USED
     template<typename T>
     LuaRef operator[](T key) const {
     Push();
     Stack<T>::Push(m_L, key);
     lua_gettable(m_L, -2);
     LuaRef v = LuaRef::popLuaRef(m_L);
     lua_pop(m_L, 1);
     return v;
     }*/
    
    /*
     * Access to element table[key].
     */
    template<typename T>
    Proxy operator[](T key) const {
        Push();
        Stack<T>::Push(m_L, key);
        return Proxy(m_L);
    }
    
    LuaRef operator()() const {
        Push();
        lua_pcall(m_L, 0, 1, 0);
        return popLuaRef(m_L);
    }
    
    template<typename... Args>
    LuaRef operator()(Args... args) const {
        Push();
        int nargs = pushArgs(m_L, args...);
        lua_pcall(m_L, nargs, 1, 0);
        return popLuaRef(m_L);
    }
    
    static LuaRef GetGlobal(lua_State* L, char const* name) {
        lua_getglobal(L, name);
        return LuaRef::popLuaRef(L);
    }
    
    static LuaRef getindex(lua_State* L, int index) {
        lua_pushvalue(L, index);
        return LuaRef::popLuaRef(L);
    }
    
};


inline LuaRef GetGlobal(lua_State* L, char const* name) {
    return LuaRef::GetGlobal(L, name);
}

inline LuaRef getIndex(lua_State* L, int index) {
    return LuaRef::getindex(L, index);
}

template<> struct Stack< Nil > {
    static void Push(lua_State* L, Nil) {
        lua_pushnil(L);
    }
};


template<> struct Stack< LuaRef::Proxy > {
    static void Push(lua_State* L, LuaRef::Proxy const& v) {
        assert(equalstates(L, v.GetState()));
        v.Push();
    }
};


template<> struct Stack< LuaRef > {
    static void Push(lua_State* L, LuaRef const& v) {
        assert(equalstates(L, v.GetState()));
        v.Push();
    }
    static LuaRef Get(lua_State* L, int index) {
        lua_pushvalue(L, index);
        return LuaRef::popLuaRef(L);
    }
};



inline void printLuaRef(std::ostream& os, LuaRef const& v) {
    int type = v.GetType();
    switch(type) {
        case LUA_TNIL:
            os<< "nil";
            break;
        case LUA_TNUMBER:
            os<< v.Cast<lua_Number>();
            break;
        case LUA_TBOOLEAN:
            os<<(v.Cast<bool>() ? "true" : "false");
            break;
        case LUA_TSTRING:
            os<< '"'<< v.Cast<std::string>()<< '"';
            break;
        case LUA_TTABLE:
        case LUA_TFUNCTION:
        case LUA_TUSERDATA:
        case LUA_TTHREAD:
        case LUA_TLIGHTUSERDATA:
            os<< v.tostring();
            break;
        default:
            os<< "unknown";
            break;
    }
}

inline std::ostream& operator<<(std::ostream& os, LuaRef const& v) {
    printLuaRef(os, v);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, LuaRef::Proxy const& v) {
    printLuaRef(os, LuaRef(v));
    return os;
}

