/**
 * Thanks to https://github.com/sth/callable.hpp
 * That's awesome.
 */

/** Count the number of types given to the template */
template<typename... Types>
struct tva_count;

template<>
struct tva_count<> {
    static const size_t value = 0;
};

template<typename Type, typename... Types>
struct tva_count<Type, Types...> {
    static const size_t value = tva_count<Types...>::value + 1;
};


/** Get the nth type given to the template */
template<size_t n, typename... Types>
struct tva_n;

template<size_t N, typename Type, typename... Types>
struct tva_n<N, Type, Types...> : tva_n<N-1, Types...> {
};

template<typename Type, typename... Types>
struct tva_n<0, Type, Types...> {
    typedef Type type;
};

/** Define traits for a function type */
template<typename Fun>
struct callable_traits_fn;

template<typename Ret, typename... Args>
struct callable_traits_fn<Ret(Args...)> {
    typedef Ret function_type(Args...);
    typedef Ret return_type;
    static const size_t argc;
    
    template<size_t N>
    using argument_type = typename tva_n<N, Args...>::type;
};

template<typename Ret, typename... Args>
const size_t callable_traits_fn<Ret(Args...)>::argc = tva_count<Args...>::value;


/** Define traits for a operator() member function pointer type */
template<typename MemFun>
struct callable_traits_memfn;

template<typename Class, typename Ret, typename... Args>
struct callable_traits_memfn<Ret(Class::*)(Args...)> : callable_traits_fn<Ret(Args...)> {
};

template<typename Class, typename Ret, typename... Args>
struct callable_traits_memfn<Ret(Class::*)(Args...) const> : callable_traits_fn<Ret(Args...)> {
};


// classes with operator()
template<typename Callable>
struct callable_traits_d : luaportal::callable_traits_memfn<decltype(&Callable::operator())> {
};

// functions
template<typename Ret, typename... Args>
struct callable_traits_d<Ret(Args...)> : luaportal::callable_traits_fn<Ret(Args...)> {
};

// function pointers
template<typename Ret, typename... Args>
struct callable_traits_d<Ret(*)(Args...)> : luaportal::callable_traits_fn<Ret(Args...)> {
};

// std::function specializations
template<typename Ret, typename... Args>
struct callable_traits_d<std::function<Ret(Args...)>> : luaportal::callable_traits_fn<Ret(Args...)> {
};


// Main template

template<typename Callable>
struct callable_traits : luaportal::callable_traits_d<typename std::remove_reference<Callable>::type> {
};

template<typename LambdaType, typename... Params>
struct RecursiveLambda{};

template<typename LambdaType>
struct RecursiveLambda<LambdaType>{
    template<typename... U>
    static void callVoidLambda(LambdaType func, lua_State *L, int start, U... u) {
        func(u...);
    }
    
    template<typename ReturnType, typename... U>
    static ReturnType callLambda(LambdaType func, lua_State *L, int start, U... u) {
        return func(u...);
    }
};

template<typename LambdaType, typename H,typename... Params>
struct RecursiveLambda<LambdaType, H, Params...>{
    template<typename... U>
    static void callVoidLambda(LambdaType func, lua_State *L,int start, U... u) {
        H h = Stack<H>::Get(L, sizeof...(u) + start);
        RecursiveLambda<LambdaType, Params...>::callVoidLambda(func, L, start, u..., h);
    }
    
    template<typename ReturnType, typename... U>
    static ReturnType callLambda(LambdaType func, lua_State *L,int start, U... u) {
        H h = Stack<H>::Get(L, sizeof...(u) + start);
        return RecursiveLambda<LambdaType, Params...>::template callLambda<ReturnType>(func, L, start, u..., h);
    }
};

template<typename T, typename MemLambda>
struct MemberLambda;

template<typename T, typename ReturnType, typename... Params>
struct MemberLambda<T, std::function<ReturnType(T*, Params...)> > {
    static int Call(lua_State *L)
    {
        T* t = Stack<T*>::Get(L, 1);
        typedef std::function<ReturnType(T*, Params...)> FunctionType;
        FunctionType func = *static_cast<FunctionType*>(lua_touserdata(L, lua_upvalueindex(1)));
        ReturnType ret = RecursiveLambda<FunctionType, Params...>::template callLambda<ReturnType>(func, L, 1, t);
        Stack<ReturnType>::Push(L, ret);
        return 1;
    }
};

template<typename T, typename... Params>
struct MemberLambda<T, std::function<void(T*, Params...)> > {
    static int Call(lua_State *L)
    {
        T* t = Stack<T*>::Get(L, 1);
        typedef std::function<void(T*, Params...)> FunctionType;
        FunctionType func = *static_cast<FunctionType*>(lua_touserdata(L, lua_upvalueindex(1)));
        RecursiveLambda<FunctionType, Params...>::callVoidLambda(func, L, 1, t);
        return 0;
    }
};


template<typename ALambda>
struct StaticLambda;

template<typename ReturnType, typename... Params>
struct StaticLambda<std::function<ReturnType(Params...)> > {
    static int Call(lua_State *L)
    {
        typedef std::function<ReturnType(Params...)> FunctionType;
        FunctionType func = *static_cast<FunctionType*>(lua_touserdata(L, lua_upvalueindex(1)));
        ReturnType ret = RecursiveLambda<FunctionType, Params...>::template callLambda<ReturnType>(func, L, 1);
        Stack<ReturnType>::Push(L, ret);
        return 1;
    }
};

template<typename... Params>
struct StaticLambda<std::function<void(Params...)> > {
    static int Call(lua_State *L)
    {
        typedef std::function<void(Params...)> FunctionType;
        FunctionType func = *static_cast<FunctionType*>(lua_touserdata(L, lua_upvalueindex(1)));
        RecursiveLambda<FunctionType, Params...>::callVoidLambda(func, L, 1);
        return 0;
    }
};