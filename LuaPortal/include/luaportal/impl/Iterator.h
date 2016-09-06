/** Allows table iteration.
 */
class Iterator
{
private:
    lua_State* m_L;
    LuaRef m_table;
    LuaRef m_key;
    LuaRef m_value;
    
    void next()
    {
        m_table.push();
        m_key.push();
        if (lua_next(m_L, -2))
        {
            m_value.pop();
            m_key.pop();
        }
        else
        {
            m_key = Nil();
            m_value = Nil();
        }
        lua_pop(m_L, 1);
    }
    
public:
    explicit Iterator(LuaRef table)
    : m_L(table.state())
    , m_table(table)
    , m_key(table.state()) // m_key is nil
    , m_value(table.state()) // m_value is nil
    {
        if (!table.isNil()) {
            next(); // get the first (key, value) pair from table
        }
    }
    
    lua_State* state() const
    {
        return m_L;
    }
    
    LuaRef operator* () const
    {
        return m_value;
    }
    
    LuaRef operator-> () const
    {
        return m_value;
    }
    
    Iterator& operator++ ()
    {
        if (isNil())
        {
            // if the iterator reaches the end, do nothing
            return *this;
        }
        else
        {
            next();
            return *this;
        }
    }
    
    inline bool isNil() const
    {
        return m_key.isNil();
    }
    
    inline LuaRef key() const
    {
        return m_key;
    }
    
    inline LuaRef value() const
    {
        return m_value;
    }
    
private:
    // Don't use postfix increment, it is less efficient
    Iterator operator++ (int);
};

