/**
 Since the throw specification is part of a function signature, the FuncTraits
 family of templates needs to be specialized for both types. The
 LUAPORTAL_THROWSPEC macro controls whether we use the 'throw()' form, or
 'noexcept'(if C++11 is available) to distinguish the functions.
 */
#if defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__clang__) || defined(__GNUC__) || \
(defined(_MSC_VER) &&(_MSC_VER >= 1700))
// Do not define LUAPORTAL_THROWSPEC since the Xcode and gcc  compilers do not
// distinguish the throw specification in the function signature.
#else
// Visual Studio 10 and earlier pay too much mind to useless throw() spec.
//
# define LUAPORTAL_THROWSPEC throw()
#endif

//==============================================================================
/**
 Traits for function pointers.
 
 There are three types of functions: global, non-const member, and const
 member. These templates determine the type of function, which class type it
 belongs to if it is a class member, the const-ness if it is a member
 function, and the type information for the return value and argument list.
 
 Expansions are provided for functions with up to 8 parameters. This can be
 manually extended, or expanded to an arbitrary amount using C++11 features.
 */
template<typename MemFn, typename D = MemFn>
struct FuncTraits
{
};

/* Ordinary function pointers. */

template<typename R, typename D, typename... P>
struct RecursiveCallStaticFunction;

template<typename R, typename D, typename... P>
struct RecursiveCallStaticFunction
{
    template<typename... U>
    static R Call(D fp, lua_State *L, U... u)
    {
        return fp(u...);
    }
};

template<typename R, typename D, typename H, typename... P>
struct RecursiveCallStaticFunction<R,D,H,P...>
{
    template<typename... U>
    static R Call(D fp, lua_State *L, U... u)
    {
        const int index = static_cast<int>(1 + sizeof...(u));
        H h = Stack<H>::Get(L, index);
        return RecursiveCallStaticFunction<R,D,P...>::Call(fp, L, u..., h);
    }
};

template<typename R, typename D, typename... Param>
struct FuncTraits<R(*)(Param...), D>
{
    static bool const isMemberFunction = false;
    typedef D DeclType;
    typedef R ReturnType;
    
    static R Call(D fp, lua_State *L)
    {
        return RecursiveCallStaticFunction<R, D, Param...>::Call(fp, L);
    }
};


/* Non-const member function pointers. */

template<typename T, typename R, typename D, typename... P>
struct RecursiveCallMemberFunction;

template<typename T, typename R, typename D, typename... P>
struct RecursiveCallMemberFunction
{
    template<typename... U>
    static R Call(T* obj, D fp, lua_State *L, U... u)
    {
        return(obj->*fp)(u...);
    }
    
    template<typename... U>
    static R CallConst(const T* obj, D fp, lua_State *L, U... u)
    {
        return(obj->*fp)(u...);
    }
};

template<typename T, typename R, typename D, typename H, typename... P>
struct RecursiveCallMemberFunction<T,R,D,H,P...>
{
    template<typename... U>
    static R Call(T* obj, D fp, lua_State *L, U... u)
    {
        const int index = static_cast<int>(2 + sizeof...(u));
        H h = Stack<H>::Get(L, index);
        return RecursiveCallMemberFunction<T,R,D,P...>::Call(obj, fp, L, u..., h);
    }
    
    template<typename... U>
    static R CallConst(const T* obj, D fp, lua_State *L, U... u)
    {
        const int index = static_cast<int>(2 + sizeof...(u));
        H h = Stack<H>::Get(L, index);
        return RecursiveCallMemberFunction<T,R,D,P...>::CallConst(obj, fp, L, u..., h);
    }
};

template<typename T, typename R, typename D, typename... Param>
struct FuncTraits<R(T::*)(Param...), D>
{
    static bool const isMemberFunction = true;
    static bool const IsConstMemberFunction = false;
    typedef D DeclType;
    typedef T ClassType;
    typedef R ReturnType;
    
    static R Call(T* obj, D fp, lua_State *L)
    {
        return RecursiveCallMemberFunction<T, R, D, Param...>::Call(obj, fp, L);
    }
};

/* Const member function pointers. */

template<typename T, typename R, typename D, typename... Param>
struct FuncTraits<R(T::*)(Param...) const, D>
{
    static bool const isMemberFunction = true;
    static bool const IsConstMemberFunction = true;
    typedef D DeclType;
    typedef T ClassType;
    typedef R ReturnType;
    
    static R Call(const T* obj, D fp, lua_State *L)
    {
        return RecursiveCallMemberFunction<T, R, D, Param...>::CallConst(obj, fp, L);
    }
};

#if defined(LUAPORTAL_THROWSPEC)

/* Ordinary function pointers. */

template<typename R, typename D, typename... Param>
struct FuncTraits<R(*)(Param...) LUAPORTAL_THROWSPEC, D>
{
    static bool const isMemberFunction = false;
    typedef D DeclType;
    typedef R ReturnType;
    
    static R Call(D fp, lua_State *L)
    {
        return RecursiveCallStaticFunction<R, D, Param...>::Call(fp, L);
    }
};

/* Non-const member function pointers with THROWSPEC. */

template<typename T, typename R, typename D, typename... Param>
struct FuncTraits<R(T::*)(Param...) LUAPORTAL_THROWSPEC, D>
{
    static bool const isMemberFunction = true;
    static bool const IsConstMemberFunction = false;
    typedef D DeclType;
    typedef T ClassType;
    typedef R ReturnType;
    
    static R Call(T* obj, D fp, lua_State *L)
    {
        return RecursiveCallMemberFunction<T, R, D, Param...>::Call(obj, fp, L);
    }
};

/* Const member function pointers with THROWSPEC. */

template<typename T, typename R, typename D, typename... Param>
struct FuncTraits<R(T::*)(Param...) const LUAPORTAL_THROWSPEC, D>
{
    static bool const isMemberFunction = true;
    static bool const IsConstMemberFunction = true;
    typedef D DeclType;
    typedef T ClassType;
    typedef R ReturnType;
    
    static R Call(const T* obj, D fp, lua_State *L)
    {
        return RecursiveCallMemberFunction<T, R, D, Param...>::CallConst(obj, fp, L);
    }
};

#endif
