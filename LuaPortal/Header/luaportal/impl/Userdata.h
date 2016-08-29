//==============================================================================
/**
 Return the identity pointer for our lightuserdata tokens.
 
 LuaPortal metatables are tagged with a security "token." The token is a
 lightuserdata created from the identity pointer, used as a key in the
 metatable. The value is a boolean = true, although any value could have been
 used.
 
 Because of Lua's dynamic typing and our improvised system of imposing C++
 class structure, there is the possibility that executing scripts may
 knowingly or unknowingly cause invalid data to get passed to the C functions
 created by LuaPortal. In particular, our security model addresses the
 following:
 
 Notes:
 1. Scripts cannot create a userdata (ignoring the debug lib).
 2. Scripts cannot create a lightuserdata (ignoring the debug lib).
 3. Scripts cannot set the metatable on a userdata.
 4. Our identity key is a unique pointer in the process.
 5. Our metatables have a lightuserdata identity key / value pair.
 6. Our metatables have "__metatable" set to a boolean = false.
 7. Our lightuserdata is unique.
 */
inline void* GetIdentityKey ()
{
    static char value;
    return &value;
}

/**
 Interface to a class pointer retrievable from a userdata.
 */
class Userdata
{
protected:
    void* m_p; // subclasses must set this
    
    //--------------------------------------------------------------------------
    /**
     Get an untyped pointer to the contained class.
     */
    inline void* const GetPointer ()
    {
        return m_p;
    }
    
private:
    //--------------------------------------------------------------------------
    /**
     Validate and retrieve a Userdata on the stack.
     
     The Userdata must exactly match the corresponding class table or
     const table, or else a Lua error is raised. This is used for the
     __gc metamethod.
     */
    static Userdata* GetExactClass (lua_State* L,
                                    int narg,
                                    void const* classKey)
    {
        Userdata* ud = 0;
        int const index = lua_absindex (L, narg);
        
        bool mismatch = false;
        char const* got = 0;
        
        lua_rawgetp (L, LUA_REGISTRYINDEX, classKey);
        assert (lua_istable (L, -1));
        
        // Make sure we have a userdata.
        if (!lua_isuserdata (L, index))
            mismatch = true;
        
        // Make sure it's metatable is ours.
        if (!mismatch)
        {
            lua_getmetatable (L, index);
            lua_rawgetp (L, -1, GetIdentityKey ());
            if (lua_isboolean (L, -1))
            {
                lua_pop (L, 1);
            }
            else
            {
                lua_pop (L, 2);
                mismatch = true;
            }      
        }
        
        if (!mismatch)
        {
            if (lua_rawequal (L, -1, -2))
            {
                // Matches class table.
                lua_pop (L, 2);
                ud = static_cast <Userdata*> (lua_touserdata (L, index));
            }
            else
            {
                rawgetfield (L, -2, "__const");
                if (lua_rawequal (L, -1, -2))
                {
                    // Matches const table
                    lua_pop (L, 3);
                    ud = static_cast <Userdata*> (lua_touserdata (L, index));
                }
                else
                {
                    // Mismatch, but its one of ours so get a type name.
                    rawgetfield (L, -2, "__type");
                    lua_insert (L, -4);
                    lua_pop (L, 2);
                    got = lua_tostring (L, -2);
                    mismatch = true;
                }
            }
        }
        
        if (mismatch)
        {
            rawgetfield (L, -1, "__type");
            assert (lua_type (L, -1) == LUA_TSTRING);
            char const* const expected = lua_tostring (L, -1);
            
            if (got == 0)
                got = lua_typename (L, lua_type (L, index));
            
            char const* const msg = lua_pushfstring (
                                                     L, "%s expected, got %s", expected, got);
            
            if (narg > 0)
                luaL_argerror (L, narg, msg);
            else
                lua_error (L);
        }
        
        return ud;
    }
    
    //--------------------------------------------------------------------------
    /**
     Validate and retrieve a Userdata on the stack.
     
     The Userdata must be derived from or the same as the given base class,
     identified by the key. If canBeConst is false, generates an error if
     the resulting Userdata represents to a const object. We do the type check
     first so that the error message is informative.
     */
    static Userdata* GetClass (lua_State* L,
                               int index,
                               void const* baseClassKey,
                               bool canBeConst)
    {
        assert (index > 0);
        Userdata* ud = 0;
        
        bool mismatch = false;
        char const* got = 0;
        
        lua_rawgetp (L, LUA_REGISTRYINDEX, baseClassKey);
        assert (lua_istable (L, -1));
        
        // Make sure we have a userdata.
        if (lua_isuserdata (L, index))
        {
            // Make sure it's metatable is ours.
            lua_getmetatable (L, index);
            lua_rawgetp (L, -1, GetIdentityKey ());
            if (lua_isboolean (L, -1))
            {
                lua_pop (L, 1);
                
                // If __const is present, object is NOT const.
                rawgetfield (L, -1, "__const");
                assert (lua_istable (L, -1) || lua_isnil (L, -1));
                bool const isConst = lua_isnil (L, -1);
                lua_pop (L, 1);
                
                // Replace the class table with the const table if needed.
                if (isConst)
                {
                    rawgetfield (L, -2, "__const");
                    assert (lua_istable (L, -1));
                    lua_replace (L, -3);
                }
                
                for (;;)
                {
                    if (lua_rawequal (L, -1, -2))
                    {
                        lua_pop (L, 2);
                        
                        // Match, now check const-ness.
                        if (isConst && !canBeConst)
                        {
                            luaL_argerror (L, index, "cannot be const");
                        }
                        else
                        {
                            ud = static_cast <Userdata*> (lua_touserdata (L, index));
                            break;
                        }
                    }
                    else
                    {
                        // Replace current metatable with it's base class.
                        rawgetfield (L, -1, "__parent");
                        /*
                         ud
                         class metatable
                         ud metatable
                         ud __parent (nil)
                         */
                        
                        if (lua_isnil (L, -1))
                        {
                            lua_remove (L, -1);
                            // Mismatch, but its one of ours so get a type name.
                            rawgetfield (L, -1, "__type");
                            lua_insert (L, -3);
                            lua_pop (L, 1);
                            got = lua_tostring (L, -2);
                            mismatch = true;
                            break;
                        }
                        else
                        {
                            lua_remove (L, -2);
                        }
                    }
                }
            }
            else
            {
                lua_pop (L, 2);
                mismatch = true;
            }      
        }
        else
        {
            mismatch = true;
        }
        
        if (mismatch)
        {
            assert (lua_type (L, -1) == LUA_TTABLE);
            rawgetfield (L, -1, "__type");
            assert (lua_type (L, -1) == LUA_TSTRING);
            char const* const expected = lua_tostring (L, -1);
            
            if (got == 0)
                got = lua_typename (L, lua_type (L, index));
            
            char const* const msg = lua_pushfstring (
                                                     L, "%s expected, got %s", expected, got);
            
            luaL_argerror (L, index, msg);
        }
        
        return ud;
    }
    
public:
    virtual ~Userdata () { }
    
    //--------------------------------------------------------------------------
    /**
     Returns the Userdata* if the class on the Lua stack matches.
     
     If the class does not match, a Lua error is raised.
     */
    template <typename T>
    static inline Userdata* GetExact (lua_State* L, int index)
    {
        return GetExactClass (L, index, ClassInfo <T>::GetClassKey ());
    }
    
    //--------------------------------------------------------------------------
    /**
     Get a pointer to the class from the Lua stack.
     
     If the object is not the class or a subclass, or it violates the
     const-ness, a Lua error is raised.
     */
    template <typename T>
    static inline T* get (lua_State* L, int index, bool canBeConst)
    {
        if (lua_isnil (L, index))
            return 0;
        else
            return static_cast <T*> (GetClass (L, index,
                                               ClassInfo <T>::GetClassKey (), canBeConst)->GetPointer ());
    }
};

//----------------------------------------------------------------------------
/**
 Wraps a class object stored in a Lua userdata.
 
 The lifetime of the object is managed by Lua. The object is constructed
 inside the userdata using placement new.
 */
template <typename T>
class UserdataValue : public Userdata
{
private:
    bool constructed = false;
    UserdataValue <T> (UserdataValue <T> const&);
    UserdataValue <T> operator= (UserdataValue <T> const&);
    
    char m_storage [sizeof (T)];
    
    inline T* GetObject ()
    {
        // If this fails to compile it means you forgot to provide
        // a Container specialization for your container!
        //
        return reinterpret_cast <T*> (&m_storage [0]);
    }
    
private:
    /**
     Used for placement construction.
     */
    UserdataValue ()
    {
        m_p = GetObject ();
    }
    
    ~UserdataValue ()
    {
        if (constructed) {
            GetObject ()->~T ();
        } 
    }
    
public:
    /**
     Push a T via placement new.
     
     The caller is responsible for calling placement new using the
     returned uninitialized storage.
     */
    static UserdataValue <T>* place (lua_State* const L)
    {
        UserdataValue <T>* const ud = new (
                                           lua_newuserdata (L, sizeof (UserdataValue <T>))) UserdataValue <T> ();
        lua_rawgetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::GetClassKey ());
        // If this goes off it means you forgot to register the class!
        assert (lua_istable (L, -1));
        lua_setmetatable (L, -2);
        return ud;
    }
    
    void* GetVoidPointer()
    {
        return GetPointer();
    }
    
    void markConstructed()
    {
        constructed = true;
    }
    
    /**
     Push T via copy construction from U.
     */
    template <typename U>
    static inline void push (lua_State* const L, U const& u)
    {
        auto place = UserdataValue<U>::place (L);
        new (place->GetVoidPointer()) U (u);
        place->markConstructed();
    }
};

//----------------------------------------------------------------------------
/**
 Wraps a pointer to a class object inside a Lua userdata.
 
 The lifetime of the object is managed by C++.
 */
class UserdataPtr : public Userdata
{
private:
    UserdataPtr (UserdataPtr const&);
    UserdataPtr operator= (UserdataPtr const&);
    
private:
    /** Push non-const pointer to object using metatable key.
     */
    static void push (lua_State* L, void* const p, void const* const key)
    {
        if (p)
        {
            new (lua_newuserdata (L, sizeof (UserdataPtr))) UserdataPtr (p);
            lua_rawgetp (L, LUA_REGISTRYINDEX, key);
            // If this goes off it means you forgot to register the class!
            assert (lua_istable (L, -1));
            lua_setmetatable (L, -2);
        }
        else
        {
            lua_pushnil (L);
        }
    }
    
    /** Push const pointer to object using metatable key.
     */
    static void push (lua_State* L, void const* const p, void const* const key)
    {
        if (p)
        {
            new (lua_newuserdata (L, sizeof (UserdataPtr)))
            UserdataPtr (const_cast <void*> (p));
            lua_rawgetp (L, LUA_REGISTRYINDEX, key);
            // If this goes off it means you forgot to register the class!
            assert (lua_istable (L, -1));
            lua_setmetatable (L, -2);
        }
        else
        {
            lua_pushnil (L);
        }
    }
    
    explicit UserdataPtr (void* const p)
    {
        m_p = p;
        
        // Can't construct with a null pointer!
        //
        assert (m_p != 0);
    }
    
public:
    /** Push non-const pointer to object.
     */
    template <typename T>
    static inline void push (lua_State* const L, T* const p)
    {
        if (p)
            push (L, p, ClassInfo <T>::GetClassKey ());
        else
            lua_pushnil (L);
    }
    
    /** Push const pointer to object.
     */
    template <typename T>
    static inline void push (lua_State* const L, T const* const p)
    {
        if (p)
            push (L, p, ClassInfo <T>::GetConstKey ());
        else
            lua_pushnil (L);
    }
};

//============================================================================
/**
 Wraps a container thet references a class object.
 
 The template argument C is the container type, ContainerTraits must be
 specialized on C or else a compile error will result.
 */
template <typename C>
class UserdataShared : public Userdata
{
private:
    UserdataShared (UserdataShared <C> const&);
    UserdataShared <C>& operator= (UserdataShared <C> const&);
    
    typedef typename TypeTraits::removeConst <
    typename ContainerTraits <C>::Type>::Type T;
    
    C m_c;
    
private:
    ~UserdataShared ()
    {
    }
    
public:
    /**
     Construct from a container to the class or a derived class.
     */
    template <typename U>
    explicit UserdataShared (U const& u) : m_c (u)
    {
        m_p = const_cast <void*> (reinterpret_cast <void const*> (
                                                                  (ContainerTraits <C>::get (m_c))));
    }
    
    /**
     Construct from a pointer to the class or a derived class.
     */
    template <typename U>
    explicit UserdataShared (U* u) : m_c (u)
    {
        m_p = const_cast <void*> (reinterpret_cast <void const*> (
                                                                  (ContainerTraits <C>::get (m_c))));
    }
};

//----------------------------------------------------------------------------
//
// SFINAE helpers.
//

// non-const objects
template <typename C, bool makeObjectConst>
struct UserdataSharedHelper
{
    typedef typename TypeTraits::removeConst <
    typename ContainerTraits <C>::Type>::Type T;
    
    static void push (lua_State* L, C const& c)
    {
        if (ContainerTraits <C>::get (c) != 0)
        {
            new (lua_newuserdata (L, sizeof (UserdataShared <C>))) UserdataShared <C> (c);
            lua_rawgetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::GetClassKey ());
            // If this goes off it means the class T is unregistered!
            assert (lua_istable (L, -1));
            lua_setmetatable (L, -2);
        }
        else
        {
            lua_pushnil (L);
        }
    }
    
    static void push (lua_State* L, T* const t)
    {
        if (t)
        {
            new (lua_newuserdata (L, sizeof (UserdataShared <C>))) UserdataShared <C> (t);
            lua_rawgetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::GetClassKey ());
            // If this goes off it means the class T is unregistered!
            assert (lua_istable (L, -1));
            lua_setmetatable (L, -2);
        }
        else
        {
            lua_pushnil (L);
        }
    }
};

// const objects
template <typename C>
struct UserdataSharedHelper <C, true>
{
    typedef typename TypeTraits::removeConst <
    typename ContainerTraits <C>::Type>::Type T;
    
    static void push (lua_State* L, C const& c)
    {
        if (ContainerTraits <C>::get (c) != 0)
        {
            new (lua_newuserdata (L, sizeof (UserdataShared <C>))) UserdataShared <C> (c);
            lua_rawgetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::GetConstKey ());
            // If this goes off it means the class T is unregistered!
            assert (lua_istable (L, -1));
            lua_setmetatable (L, -2);
        }
        else
        {
            lua_pushnil (L);
        }
    }
    
    static void push (lua_State* L, T* const t)
    {
        if (t)
        {
            new (lua_newuserdata (L, sizeof (UserdataShared <C>))) UserdataShared <C> (t);
            lua_rawgetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::GetConstKey ());
            // If this goes off it means the class T is unregistered!
            assert (lua_istable (L, -1));
            lua_setmetatable (L, -2);
        }
        else
        {
            lua_pushnil (L);
        }
    }
};

/**
 Pass by container.
 
 The container controls the object lifetime. Typically this will be a
 lifetime shared by C++ and Lua using a reference count. Because of type
 erasure, containers like std::shared_ptr will not work. Containers must
 either be of the intrusive variety, or in the style of the RefCountedPtr
 type provided by LuaPortal (that uses a global hash table).
 */
template <typename C, bool byContainer>
struct StackHelper
{
    static inline void push (lua_State* L, C const& c)
    {
        UserdataSharedHelper <C,
        TypeTraits::isConst <typename ContainerTraits <C>::Type>::value>::push (L, c);
    }
    
    typedef typename TypeTraits::removeConst <
    typename ContainerTraits <C>::Type>::Type T;
    
    static inline C get (lua_State* L, int index)
    {
        return Userdata::get <T> (L, index, true);
    }
};

/**
 Pass by value.
 
 Lifetime is managed by Lua. A C++ function which accesses a pointer or
 reference to an object outside the activation record in which it was
 retrieved may result in undefined behavior if Lua garbage collected it.
 */
template <typename T>
struct StackHelper <T, false>
{
    static inline void push (lua_State* L, T const& t)
    {
        UserdataValue <T>::push (L, t);
    }
    
    static inline T const& get (lua_State* L, int index)
    {
        return *Userdata::get <T> (L, index, true);
    }
};

template <typename T, typename Enable = void>
struct ClassOrEnum{};

template <typename T>
struct ClassOrEnum<T, typename std::enable_if<std::is_class<T>::value>::type>
{
    static inline void push (lua_State* L, T const& t)
    {
        StackHelper <T, TypeTraits::isContainer <T>::value>::push (L, t);
    }
    
    static inline T get (lua_State* L, int index)
    {
        return StackHelper <T, TypeTraits::isContainer <T>::value>::get (L, index);
    }
};

template <typename T>
struct ClassOrEnum<T, typename std::enable_if<std::is_enum<T>::value>::type>
{
    static inline void push (lua_State* L, T const& t)
    {
        lua_pushinteger (L, static_cast <lua_Integer> (t));
    }
    
    static inline T get (lua_State* L, int index)
    {
        return static_cast <T> (luaL_checkinteger (L, index));
    }
};

//==============================================================================

/**
 Lua stack conversions for class objects passed by value.
 */
template <typename T>
struct Stack
{
public:
    static inline void push (lua_State* L, T const& t)
    {
        ClassOrEnum<T>::push(L, t);
    }
    
    static inline T get (lua_State* L, int index)
    {
        return ClassOrEnum<T>::get(L, index);
    }
};

//------------------------------------------------------------------------------
/**
 Lua stack conversions for pointers and references to class objects.
 
 Lifetime is managed by C++. Lua code which remembers a reference to the
 value may result in undefined behavior if C++ destroys the object. The
 handling of the const and volatile qualifiers happens in UserdataPtr.
 */

// pointer
template <typename T>
struct Stack <T*>
{
    static inline void push (lua_State* L, T* const p)
    {
        UserdataPtr::push (L, p);
    }
    
    static inline T* const get (lua_State* L, int index)
    {
        return Userdata::get <T> (L, index, false);
    }
};

// Strips the const off the right side of *
template <typename T>
struct Stack <T* const>
{
    static inline void push (lua_State* L, T* const p)
    {
        UserdataPtr::push (L, p);
    }
    
    static inline T* const get (lua_State* L, int index)
    {
        return Userdata::get <T> (L, index, false);
    }
};

// pointer to const
template <typename T>
struct Stack <T const*>
{
    static inline void push (lua_State* L, T const* const p)
    {
        UserdataPtr::push (L, p);
    }
    
    static inline T const* const get (lua_State* L, int index)
    {
        return Userdata::get <T> (L, index, true);
    }
};

// Strips the const off the right side of *
template <typename T>
struct Stack <T const* const>
{
    static inline void push (lua_State* L, T const* const p)
    {
        UserdataPtr::push (L, p);
    }
    
    static inline T const* const get (lua_State* L, int index)
    {
        return Userdata::get <T> (L, index, true);
    }
};

// reference
template <typename T>
struct Stack <T&>
{
    static inline void push (lua_State* L, T& t)
    {
        UserdataPtr::push (L, &t);
    }
    
    static T& get (lua_State* L, int index)
    {
        T* const t = Userdata::get <T> (L, index, false);
        if (!t)
            luaL_error (L, "nil passed to reference");
        return *t;
    }
};

template <typename C, bool byContainer>
struct RefStackHelper
{
    typedef C return_type;  
    
    static inline void push (lua_State* L, C const& t)
    {
        UserdataSharedHelper <C,
        TypeTraits::isConst <typename ContainerTraits <C>::Type>::value>::push (L, t);
    }
    
    typedef typename TypeTraits::removeConst <
    typename ContainerTraits <C>::Type>::Type T;
    
    static return_type get (lua_State* L, int index)
    {
        return Userdata::get <T> (L, index, true);
    }
};

template <typename T>
struct RefStackHelper <T, false>
{
    typedef T const& return_type;  
    
    static inline void push (lua_State* L, T const& t)
    {
        UserdataPtr::push (L, &t);
    }
    
    static return_type get (lua_State* L, int index)
    {
        T const* const t = Userdata::get <T> (L, index, true);
        
        if (!t)
            luaL_error (L, "nil passed to reference");
        return *t;
    }
    
};

// reference to const
template <typename T>
struct Stack <T const&>
{
    typedef RefStackHelper <T, TypeTraits::isContainer <T>::value> helper_t;
    
    static inline void push (lua_State* L, T const& t)
    {
        helper_t::push (L, t);
    }
    
    static typename helper_t::return_type get (lua_State* L, int index)
    {
        return helper_t::get (L, index);
    }
};
