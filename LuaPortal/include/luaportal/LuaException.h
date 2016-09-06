
class LuaException : public std::exception 
{
private:
    lua_State* m_L;
    std::string m_what;
    
public:
    //----------------------------------------------------------------------------
    /**
     Construct a LuaException after a lua_pcall().
     */
    LuaException(lua_State* L, int /*code*/)
    : m_L(L)
    {
        whatFromStack();
    }
    
    //----------------------------------------------------------------------------
    
    LuaException(lua_State *L,
                  char const*,
                  char const*,
                  long)
    : m_L(L)
    {
        whatFromStack();
    }
    
    //----------------------------------------------------------------------------
    
    ~LuaException() throw()
    {
    }
    
    //----------------------------------------------------------------------------
    
    char const* what() const throw()
    {
        return m_what.c_str();
    }
    
    //============================================================================
    /**
     Throw an exception.
     
     This centralizes all the exceptions thrown, so that we can set
     breakpoints before the stack is unwound, or otherwise customize the
     behavior.
     */
    template<typename Exception>
    static void Throw(Exception e)
    {
        throw e;
    }
    
    //----------------------------------------------------------------------------
    /**
     Wrapper for lua_pcall that throws.
     */
    static void pcall(lua_State* L, int nargs = 0, int nresults = 0, int msgh = 0)
    {
        int code = lua_pcall(L, nargs, nresults, msgh);
        
        if(code != LUAPORTAL_LUA_OK)
            Throw(LuaException(L, code));
    }
    
    //----------------------------------------------------------------------------
    
protected:
    void whatFromStack()
    {
        if(lua_gettop(m_L) > 0)
        {
            char const* s = lua_tostring(m_L, -1);
            m_what = s ? s : "";
        }
        else
        {
            // stack is empty
            m_what = "missing error";
        }
    }
};
