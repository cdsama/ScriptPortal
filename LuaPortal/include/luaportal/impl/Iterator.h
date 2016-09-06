/** Allows table iteration.
 */
class Iterator
{
private:
    lua_State* m_L;
    LuaRef m_table;
    LuaRef m_key;
    LuaRef m_value;
    
    void Next()
    {
        m_table.Push();
        m_key.Push();
        if(lua_next(m_L, -2))
        {
            m_value.Pop();
            m_key.Pop();
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
    : m_L(table.GetState())
    , m_table(table)
    , m_key(table.GetState()) // m_key is nil
    , m_value(table.GetState()) // m_value is nil
    {
        if(!table.IsNil()) {
            Next(); // Get the first(key, value) pair from table
        }
    }
    
    lua_State* GetState() const
    {
        return m_L;
    }
    
    LuaRef operator*() const
    {
        return m_value;
    }
    
    LuaRef operator->() const
    {
        return m_value;
    }
    
    Iterator& operator++()
    {
        if(IsNil())
        {
            // if the iterator reaches the end, do nothing
            return *this;
        }
        else
        {
            Next();
            return *this;
        }
    }
    
    inline bool IsNil() const
    {
        return m_key.IsNil();
    }
    
    inline LuaRef Key() const
    {
        return m_key;
    }
    
    inline LuaRef Value() const
    {
        return m_value;
    }
    
private:
    // Don't use postfix increment, it is less efficient
    Iterator operator++(int);
};

